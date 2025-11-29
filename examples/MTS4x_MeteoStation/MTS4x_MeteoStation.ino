// examples/MTS4x_MeteoStation/MTS4x_MeteoStation.ino
//
// Вариант "метеостанция с максимальной точностью":
//  - MTS4x в режиме single-shot с AVG_32
//  - на каждое обновление усредняем несколько измерений (SAMPLES)
//  - проверяем CRC для каждого измерения
//  - отдельный TEMP_OFFSET_C для калибровки по эталону

#include <Arduino.h>
#include <Wire.h>
#include "MTS4x.h"

// ---------------- ПЛАТФОРМА И ВЕБ-СЕРВЕР ----------------

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  ESP8266WebServer server(80);
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  WebServer server(80);
#else
  #error "This example is intended for ESP8266 or ESP32"
#endif

// ---------------- ПИНЫ I2C ----------------

#if defined(ESP8266)
  #define I2C_SDA_PIN D2   // GPIO4
  #define I2C_SCL_PIN D1   // GPIO5
#elif defined(ESP32)
  #define I2C_SDA_PIN 21
  #define I2C_SCL_PIN 22
#endif

// ---------------- НАСТРОЙКИ WIFI ----------------

const char* WIFI_SSID     = "YOUR_SSID";      // <<< впиши сюда свою сеть
const char* WIFI_PASSWORD = "YOUR_PASSWORD";  // <<< и пароль

// ---------------- ДАТЧИК MTS4x ----------------
// Предполагаем MTS4P+T4:
//  - рабочий диапазон: -103…+153 °C
//  - паспортная точность: ±0.1 °C в диапазоне -25…+25 °C
//  - разрешение: 0.004 °C (16 бит)

MTS4X mts;  // адрес 0x41 по умолчанию

// Конфигурация "максимальная точность":
const MTS4x_MPS MTS_RATE  = MPS_1Hz;   // 1 измерение/с (для periodic режима)
const MTS4x_AVG MTS_AVG   = AVG_32;    // максимальное усреднение ×32
const bool      MTS_SLEEP = true;      // sleep в single-режиме

// Дополнительное усреднение на стороне контроллера:
const uint8_t   MTS_SAMPLES = 8;       // количество измерений на одну "точку"

// Калибровочный сдвиг (после сравнения с эталоном):
// измеренная_темп + TEMP_OFFSET_C
const float TEMP_OFFSET_C = 0.00f;     // подстрой по месту

float currentTempC = NAN;          // последняя измеренная температура (с offset и усреднением)
bool  currentCrcOk = false;        // результат совокупной проверки CRC
unsigned long lastMeasureMs = 0;
const unsigned long MEASURE_PERIOD_MS = 2000; // раз в 2 сек

// ---------------- WIFI ----------------

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print(F("Connecting to WiFi "));
  Serial.print(WIFI_SSID);
  Serial.print(F(" ... "));

  uint8_t retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 60) { // ~30 секунд
    delay(500);
    Serial.print('.');
    retries++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("WiFi connected, IP: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("WiFi connect FAILED (IP=0.0.0.0). "
                     "Проверь SSID/PASS."));
  }
}

// ---------------- ИЗМЕРЕНИЕ ТЕМПЕРАТУРЫ ----------------
//
// Берём MTS_SAMPLES измерений с CRC, усредняем,
// CRC считаем OK только если все выборки прошли проверку.
//

void updateTemperatureIfNeeded() {
  unsigned long now = millis();
  if (now - lastMeasureMs < MEASURE_PERIOD_MS &&
      !isnan(currentTempC)) {
    return;
  }

  lastMeasureMs = now;

  float sum = 0.0f;
  bool  allCrcOk = true;
  uint8_t goodCount = 0;

  for (uint8_t i = 0; i < MTS_SAMPLES; ++i) {
    float tC = NAN;
    bool crcOk = false;

    if (!mts.readTemperatureCrc(tC, crcOk, true)) {
      Serial.print(F("readTemperatureCrc() FAILED at sample "));
      Serial.print(i);
      Serial.print(F(", err="));
      Serial.println(mts.lastError());
      allCrcOk = false;
      continue;  // пробуем следующую выборку
    }

    if (!crcOk) {
      allCrcOk = false;
    }

    sum += tC;
    goodCount++;

    // небольшой интервал между выборками (формально не обязателен)
    delay(5);
  }

  if (goodCount == 0) {
    // ничего не удалось измерить
    currentTempC = NAN;
    currentCrcOk = false;
    Serial.println(F("No valid samples, temperature = NaN"));
    return;
  }

  // Среднее по успешным выборкам
  float avg = sum / goodCount;

  // Применяем калибровочный сдвиг
  avg += TEMP_OFFSET_C;

  currentTempC = avg;
  currentCrcOk = allCrcOk;

  Serial.print(F("Temp(avg of "));
  Serial.print(goodCount);
  Serial.print(F(") = "));
  Serial.print(currentTempC, 3);
  Serial.print(F(" °C  CRC="));
  Serial.println(currentCrcOk ? F("ALL_OK") : F("SOME_FAIL"));
}

// ---------------- ОБРАБОТЧИК / (HTML) ----------------

void handleRoot() {
  updateTemperatureIfNeeded();

  String html;
  html.reserve(2300);
  html += F("<!DOCTYPE html><html><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<meta http-equiv='refresh' content='5'>"); // автообновление каждые 5 сек
  html += F("<title>MTS4x Meteo Station</title>");
  html += F("<style>"
            "body{font-family:sans-serif;background:#111;color:#eee;"
            "display:flex;flex-direction:column;align-items:center;justify-content:center;"
            "min-height:100vh;margin:0;}"
            ".card{background:#222;padding:20px 30px;border-radius:12px;box-shadow:0 0 15px #0008;"
            "text-align:center;max-width:440px;}"
            ".temp{font-size:3rem;margin:10px 0;}"
            ".small{font-size:0.9rem;color:#aaa;}"
            ".badge{display:inline-block;padding:3px 8px;border-radius:8px;font-size:0.75rem;"
            "margin-top:4px;}"
            ".ok{background:#2e7d32;color:#fff;}"
            ".fail{background:#c62828;color:#fff;}"
            ".spec{margin-top:12px;text-align:left;font-size:0.9rem;line-height:1.4;}"
            ".spec h2{font-size:1rem;margin:0 0 4px 0;color:#ddd;}"
            ".spec ul{margin:4px 0 0 18px;padding:0;}"
            ".spec li{margin:2px 0;}"
            "a{color:#6cf;}"
            "</style></head><body>");

  html += F("<div class='card'>");
  html += F("<h1>MTS4x — метеостанция</h1>");
  html += F("<div class='temp'>");

  if (isnan(currentTempC)) {
    html += F("--.- &deg;C");
  } else {
    html += String(currentTempC, 2);
    html += F(" &deg;C");
  }
  html += F("</div>");

  // CRC badge
  html += F("<div>");
  if (!isnan(currentTempC)) {
    if (currentCrcOk) {
      html += F("<span class='badge ok'>CRC OK (все выборки)</span>");
    } else {
      html += F("<span class='badge fail'>CRC частично не прошёл</span>");
    }
  } else {
    html += F("<span class='badge fail'>NO DATA</span>");
  }
  html += F("</div>");

  // IP + JSON
  html += F("<div class='small'>Обновление каждые 5 секунд<br>");
  html += F("IP: ");
  html += WiFi.localIP().toString();
  html += F("<br>JSON: <a href='/json'>/json</a></div>");

  // Характеристики датчика: диапазон / точность / режим
  html += F("<div class='spec'>");
  html += F("<h2>Характеристики и режим измерения</h2>");
  html += F("<ul>");
  html += F("<li>Датчик: MTS4P+T4 (серия MTS4)</li>");
  html += F("<li>Рабочий диапазон: −103…+153 &deg;C</li>");
  html += F("<li>Паспортная точность: &plusmn;0.1 &deg;C в диапазоне −25…+25 &deg;C</li>");
  html += F("<li>Разрешение АЦП: 0.004 &deg;C (16 бит)</li>");
  html += F("<li>Режим библиотеки: single-shot, AVG_32 (максимальное усреднение на чипе)</li>");
  html += F("<li>Дополнительно: усреднение ");
  html += String(MTS_SAMPLES);
  html += F(" последовательных измерений + калибровочный сдвиг ");
  html += String(TEMP_OFFSET_C, 3);
  html += F(" &deg;C</li>");
  html += F("</ul>");
  html += F("</div>");

  html += F("</div></body></html>");

  server.send(200, "text/html; charset=utf-8", html);
}

// ---------------- ОБРАБОТЧИК /json ----------------

void handleJson() {
  updateTemperatureIfNeeded();

  String json;
  json.reserve(256);
  json += F("{\"temperature_c\":");
  if (isnan(currentTempC)) {
    json += F("null");
  } else {
    json += String(currentTempC, 3);
  }
  json += F(",\"crc_ok\":");
  json += currentCrcOk ? F("true") : F("false");

  json += F(",\"sensor\":\"MTS4P+T4\"");
  json += F(",\"range_c\":[-103,153]");
  json += F(",\"best_accuracy_c\":0.1");
  json += F(",\"best_accuracy_range_c\":[-25,25]");
  json += F(",\"resolution_c\":0.004");

  json += F(",\"avg_mode\":\"AVG_32\"");
  json += F(",\"samples\":");
  json += MTS_SAMPLES;
  json += F(",\"temp_offset_c\":");
  json += String(TEMP_OFFSET_C, 3);

  uint8_t st = 0;
  if (mts.readStatus(st)) {
    json += F(",\"status\":");
    json += st;
  }

  json += F("}");
  server.send(200, "application/json", json);
}

// ---------------- SETUP ----------------

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("MTS4x Meteo Station (max precision) starting..."));

  // Инициализация датчика
  if (!mts.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
    Serial.print(F("MTS4x begin FAILED, err="));
    Serial.println(mts.lastError());
    while (1) delay(1000);
  }

  mts.setBusClock(400000); // 400 кГц

  if (!mts.setConfig(MTS_RATE, MTS_AVG, MTS_SLEEP)) {
    Serial.print(F("setConfig() FAILED, err="));
    Serial.println(mts.lastError());
  }

  // Останавливаем continuous, работаем по single-shot чтениям
  mts.setMode(MEASURE_STOP, false);

  connectWiFi();

  server.on("/", handleRoot);
  server.on("/json", handleJson);
  server.onNotFound([]() {
    server.send(404, "text/plain; charset=utf-8", "404 Not found");
  });

  server.begin();
  Serial.println(F("HTTP server started."));
}

// ---------------- LOOP ----------------

void loop() {
  server.handleClient();
}
