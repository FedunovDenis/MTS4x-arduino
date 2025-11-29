/*
  MTS4x_MeteoStation.ino

  Метеостанция на ESP8266/ESP32 с датчиком температуры MTS4P+T4 (линейка MTS4x).

  Возможности:
  - измерение температуры с максимально возможной точностью;
  - контроль CRC каждого измерения и счётчики ошибок;
  - веб-страница (/) с крупным выводом температуры и статусом;
  - JSON API (/json) для интеграций;
  - отправка усреднённой температуры на сервис narodmon.ru по TCP (порт 8283);
  - Wi-Fi с отключённым power-save и авто-реконнектом.
*/

#include <Arduino.h>
#include <Wire.h>
#include "MTS4x.h"

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  ESP8266WebServer server(80);
  #define I2C_SDA_PIN D2
  #define I2C_SCL_PIN D1
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  WebServer server(80);
  #define I2C_SDA_PIN 21
  #define I2C_SCL_PIN 22
#else
  #error "This example is intended for ESP8266 / ESP32 only"
#endif

// --------------------- Настройки пользователя ----------------------

// Wi-Fi
const char* WIFI_SSID     = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

// Поправка температуры (калибровка), °C.
// Например, датчик показывает 22.95 °C, эталон 23.10 °C => TEMP_OFFSET_C = 0.15f.
const float TEMP_OFFSET_C = 0.0f;

// Сколько single-shot измерений делаем за один цикл обновления UI
static const uint8_t  UI_SAMPLES_PER_CYCLE   = 8;

// Период обновления веб-страницы и измерений, мс
static const unsigned long UI_UPDATE_INTERVAL_MS = 2000UL;

// Интервал отправки на NarodMon, минут.
// Можно переопределить ключом компиляции -DNARODMON_INTERVAL_MINUTES=1/5/10 и т.п.
#ifndef NARODMON_INTERVAL_MINUTES
  #define NARODMON_INTERVAL_MINUTES 10
#endif

static const unsigned long NARODMON_INTERVAL_MS =
    (unsigned long)NARODMON_INTERVAL_MINUTES * 60UL * 1000UL;

// --------------------- Паспортные данные датчика -------------------

// Для отображения на веб-странице и в JSON (чисто справочная информация).
static const char* SENSOR_NAME             = "MTS4P+T4";
static const char* SENSOR_RANGE_C          = "-40..+85";
static const char* SENSOR_FULL_RANGE_C     = "-103..+153";
static const char* SENSOR_BEST_RANGE_C     = "-25..+25";
static const float SENSOR_BEST_ACCURACY_C  = 0.10f;   // ±0.1 °C
static const float SENSOR_TYP_ACCURACY_C   = 0.25f;   // типично в -40..+85 °C
static const float SENSOR_RESOLUTION_C     = 0.004f;  // 1 LSB = 0.004 °C

// ----------------------------- Глобальные переменные ----------------

MTS4X   mts;

// Последняя усреднённая температура по UI_SAMPLES_PER_CYCLE
float   g_lastTempC        = NAN;
bool    g_lastTempCrcOk    = false;

// Счётчики CRC за всё время работы
uint32_t g_crcOkTotal      = 0;
uint32_t g_crcFailTotal    = 0;

// Агрегатор для NarodMon
float    g_nmSumTemp       = 0.0f;
uint32_t g_nmCount         = 0;
uint32_t g_nmCrcSkipped    = 0;
float    g_nmLastAvg       = NAN;
bool     g_nmLastSendOk    = false;
unsigned long g_nmLastSendMs = 0;

// Wi-Fi состояние
int32_t       g_lastRssiDbm     = 0;
unsigned long g_lastWifiCheckMs = 0;

// Таймер обновления UI/измерений
unsigned long g_lastUiUpdateMs  = 0;

// ----------------------------- Вспомогательные функции --------------

static int wifiQuality(int32_t rssiDbm) {
  if (rssiDbm <= -100) return 0;
  if (rssiDbm >= -50)  return 100;
  return 2 * (rssiDbm + 100);
}

// MAC в виде AA-BB-CC-DD-EE-FF (требуется для narodmon.ru)
static String getMacDashed() {
  String mac = WiFi.macAddress();
  mac.replace(":", "-");
  return mac;
}

// Отправка одного усреднённого значения на NarodMon по TCP (порт 8283)
static bool sendToNarodMon(float avgTempC) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  WiFiClient client;
  if (!client.connect("narodmon.ru", 8283)) {
    return false;
  }

  String macDashed = getMacDashed();

  // Формат пакета:
  // #MAC
  // #T1#value
  // ##
  String payload;
  payload.reserve(64);
  payload  = "#";
  payload += macDashed;
  payload += "\n#T1#";
  payload += String(avgTempC, 2);
  payload += "\n##\n";

  client.print(payload);
  client.stop();
  return true;
}

// Один цикл измерений: делаем несколько single-shot, проверяем CRC и усредняем
static void performMeasurementCycle() {
  uint8_t okCount = 0;
  float   sum     = 0.0f;

  for (uint8_t i = 0; i < UI_SAMPLES_PER_CYCLE; ++i) {
    // Запуск одиночного измерения
    if (!mts.startSingleMessurement()) {
      ++g_crcFailTotal;
      delay(5);
      continue;
    }

    float tC    = NAN;
    bool  crcOk = false;
    if (!mts.readTemperatureCrc(tC, crcOk, true)) {
      ++g_crcFailTotal;
      delay(5);
      continue;
    }

    if (crcOk) {
      tC += TEMP_OFFSET_C;
      sum += tC;
      ++okCount;
      ++g_crcOkTotal;

      // Для NarodMon копим только значения с корректным CRC
      g_nmSumTemp += tC;
      ++g_nmCount;
    } else {
      ++g_crcFailTotal;
      ++g_nmCrcSkipped;
    }

    delay(5);
  }

  if (okCount > 0) {
    g_lastTempC     = sum / (float)okCount;
    g_lastTempCrcOk = true;
  } else {
    g_lastTempC     = NAN;
    g_lastTempCrcOk = false;
  }
}

// ----------------------------- HTTP / HTML ---------------------------

static void handleRoot() {
  String page;
  page.reserve(2048);

  page += F("<!DOCTYPE html><html lang='ru'><head><meta charset='UTF-8'>");
  page += F("<title>MTS4P+T4 — метеостанция</title>");
  page += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  page += F("<meta http-equiv='refresh' content='5'>");
  page += F("<style>"
            "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;"
            "background:#111;color:#eee;margin:0;padding:12px;}"
            ".card{background:#1e1e1e;border-radius:10px;padding:10px 12px;margin-bottom:10px;}"
            ".temp{font-size:2.6rem;font-weight:600;margin:0.4rem 0;}"
            ".ok{color:#4caf50;}.fail{color:#ff5252;}"
            ".label{font-size:0.85rem;color:#aaa;margin-bottom:0.2rem;}"
            ".small{font-size:0.78rem;color:#888;}"
            ".value{font-family:monospace;font-size:0.9rem;}"
            "</style></head><body>");

  page += F("<h1>MTS4P+T4 — метеостанция</h1>");

  // Температура и CRC
  page += F("<div class='card'>");
  page += F("<div class='label'>Текущая температура</div><div class='temp'>");
  if (isnan(g_lastTempC)) {
    page += F("--.- &deg;C");
  } else {
    page += String(g_lastTempC, 3);
    page += F(" &deg;C");
  }
  page += F("</div>");

  page += F("<div class='small'>CRC последнего цикла: ");
  page += g_lastTempCrcOk
          ? F("<span class='ok'>OK</span>")
          : F("<span class='fail'>есть ошибки</span>");
  page += F("<br>Всего CRC OK: ");
  page += String(g_crcOkTotal);
  page += F(", CRC FAIL: ");
  page += String(g_crcFailTotal);
  page += F("</div></div>");

  // Wi-Fi
  page += F("<div class='card'>");
  page += F("<div class='label'>Wi-Fi</div><div class='value'>");
  page += F("SSID: ");
  page += WIFI_SSID;
  page += F("<br>IP: ");
  page += WiFi.localIP().toString();
  page += F("<br>MAC: ");
  page += WiFi.macAddress();
  page += F("<br>RSSI: ");
  page += String(g_lastRssiDbm);
  page += F(" dBm (");
  page += String(wifiQuality(g_lastRssiDbm));
  page += F("%)</div></div>");

  // NarodMon
  page += F("<div class='card'>");
  page += F("<div class='label'>NarodMon.ru</div><div class='small'>");
  page += F("Интервал отправки: ");
  page += String((int)NARODMON_INTERVAL_MINUTES);
  page += F(" мин");
  page += F("<br>Скоплено для усреднения: ");
  page += String(g_nmCount);
  page += F(" (пропущено по CRC: ");
  page += String(g_nmCrcSkipped);
  page += F(")");
  if (!isnan(g_nmLastAvg)) {
    page += F("<br>Последний отправленный средний: ");
    page += String(g_nmLastAvg, 3);
    page += F(" &deg;C");
    page += F("<br>Статус последней отправки: ");
    page += g_nmLastSendOk ? F("OK") : F("ошибка");
  } else {
    page += F("<br>Пока ни одного пакета не отправлено.");
  }
  page += F("</div></div>");

  // Паспорт и режим измерений
  page += F("<div class='card'>");
  page += F("<div class='label'>Датчик</div><div class='small'>");
  page += F("Модуль: ");
  page += SENSOR_NAME;
  page += F("<br>Рекомендуемый диапазон: ");
  page += SENSOR_RANGE_C;
  page += F(" &deg;C");
  page += F("<br>Внутренний диапазон чипа: ");
  page += SENSOR_FULL_RANGE_C;
  page += F(" &deg;C");
  page += F("<br>Окно максимальной точности: ");
  page += SENSOR_BEST_RANGE_C;
  page += F(" &deg;C, &plusmn;");
  page += String(SENSOR_BEST_ACCURACY_C, 2);
  page += F(" &deg;C");
  page += F("<br>Типичная точность в -40..+85 &deg;C: &plusmn;");
  page += String(SENSOR_TYP_ACCURACY_C, 2);
  page += F(" &deg;C");
  page += F("<br>Разрешение: ");
  page += String(SENSOR_RESOLUTION_C, 3);
  page += F(" &deg;C/LSB");
  page += F("<br>Режим: single-shot, MPS_1Hz, AVG_32 + программное усреднение ");
  page += String(UI_SAMPLES_PER_CYCLE);
  page += F(" измерений.");
  page += F("<br>Поправка TEMP_OFFSET_C = ");
  page += String(TEMP_OFFSET_C, 3);
  page += F(" &deg;C");
  page += F("</div></div>");

  page += F("<div class='small'>JSON API: <code>/json</code></div>");

  page += F("</body></html>");
  server.send(200, "text/html; charset=utf-8", page);
}

static void handleJson() {
  String json;
  json.reserve(512);

  json += F("{");

  // Температура и CRC
  json += F("\"temperature_c\":");
  if (isnan(g_lastTempC)) {
    json += F("null,");
  } else {
    json += String(g_lastTempC, 3);
    json += F(",");
  }

  json += F("\"crc_cycle_ok\":");
  json += g_lastTempCrcOk ? F("true,") : F("false,");

  json += F("\"crc_total_ok\":");
  json += String(g_crcOkTotal);
  json += F(",\"crc_total_fail\":");
  json += String(g_crcFailTotal);
  json += F(",");

  // Паспорт
  json += F("\"sensor\":\"");
  json += SENSOR_NAME;
  json += F("\",\"range_c\":\"");
  json += SENSOR_RANGE_C;
  json += F("\",\"full_range_c\":\"");
  json += SENSOR_FULL_RANGE_C;
  json += F("\",\"best_accuracy_c\":");
  json += String(SENSOR_BEST_ACCURACY_C, 2);
  json += F(",\"best_accuracy_range_c\":\"");
  json += SENSOR_BEST_RANGE_C;
  json += F("\",\"resolution_c\":");
  json += String(SENSOR_RESOLUTION_C, 3);
  json += F(",");

  // Усреднение и поправка
  json += F("\"avg_mode\":\"AVG_32+");
  json += String(UI_SAMPLES_PER_CYCLE);
  json += F("x_single_shot\",");
  json += F("\"samples\":");
  json += String(UI_SAMPLES_PER_CYCLE);
  json += F(",\"temp_offset_c\":");
  json += String(TEMP_OFFSET_C, 3);
  json += F(",");

  // NarodMon состояние
  json += F("\"narodmon_interval_min\":");
  json += String((int)NARODMON_INTERVAL_MINUTES);
  json += F(",\"narodmon_count\":");
  json += String(g_nmCount);
  json += F(",\"narodmon_skipped_crc\":");
  json += String(g_nmCrcSkipped);
  json += F(",\"narodmon_last_avg_c\":");
  if (isnan(g_nmLastAvg)) {
    json += F("null");
  } else {
    json += String(g_nmLastAvg, 3);
  }
  json += F(",\"narodmon_last_send_ok\":");
  if (g_nmLastSendMs == 0) {
    json += F("null");
  } else {
    json += g_nmLastSendOk ? F("true") : F("false");
  }
  json += F(",");

  // Wi-Fi
  json += F("\"wifi_ssid\":\"");
  json += WIFI_SSID;
  json += F("\",\"wifi_rssi_dbm\":");
  json += String(g_lastRssiDbm);
  json += F(",\"wifi_quality_percent\":");
  json += String(wifiQuality(g_lastRssiDbm));

  json += F("}");
  server.send(200, "application/json; charset=utf-8", json);
}

// ----------------------------- Wi-Fi и main loop --------------------

static void connectWifi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
#if defined(ESP8266) || defined(ESP32)
  WiFi.setSleep(false);        // выключаем power-save, чтобы не рвалось соединение
  WiFi.setAutoReconnect(true); // автоматический реконнект
#endif
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print(F("[WiFi] Connecting to "));
  Serial.print(WIFI_SSID);
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 60) {
    delay(500);
    Serial.print('.');
    ++attempts;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("[WiFi] Connected, IP = "));
    Serial.println(WiFi.localIP());
    Serial.print(F("[WiFi] MAC = "));
    Serial.println(WiFi.macAddress());
  } else {
    Serial.println(F("[WiFi] Failed to connect, work without network"));
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("[MTS4x] Meteostation starting..."));

  // I2C + датчик
  if (!mts.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
    Serial.print(F("[MTS4x] begin() failed, err="));
    Serial.println(mts.lastError());
  } else {
    Serial.println(F("[MTS4x] I2C init OK"));
  }

  // Конфигурация: 1 Гц, AVG_32, Sleep_en=1 (single-shot)
  if (!mts.setConfig(MPS_1Hz, AVG_32, true)) {
    Serial.print(F("[MTS4x] setConfig() failed, err="));
    Serial.println(mts.lastError());
  } else {
    Serial.println(F("[MTS4x] setConfig OK: MPS_1Hz, AVG_32, sleep"));
  }

  // Запускаем Wi-Fi и HTTP-сервер
  connectWifi();
  server.on("/", handleRoot);
  server.on("/json", handleJson);
  server.begin();
  Serial.println(F("[HTTP] Server started on port 80"));

  g_lastUiUpdateMs  = millis();
  g_nmLastSendMs    = 0;
  g_lastWifiCheckMs = millis();
}

void loop() {
  server.handleClient();

  unsigned long now = millis();

  // Периодический цикл измерений/обновления UI
  if ((now - g_lastUiUpdateMs) >= UI_UPDATE_INTERVAL_MS) {
    performMeasurementCycle();
    g_lastUiUpdateMs = now;
  }

  // Отправка на NarodMon
  if ((now - g_nmLastSendMs) >= NARODMON_INTERVAL_MS && g_nmCount > 0) {
    float avg = g_nmSumTemp / (float)g_nmCount;
    bool  ok  = sendToNarodMon(avg);

    g_nmLastAvg    = avg;
    g_nmLastSendOk = ok;
    g_nmLastSendMs = now;

    // Сбрасываем накопители
    g_nmSumTemp    = 0.0f;
    g_nmCount      = 0;
    g_nmCrcSkipped = 0;

    Serial.print(F("[NarodMon] Send avg="));
    Serial.print(avg, 3);
    Serial.print(F(" °C, status="));
    Serial.println(ok ? F("OK") : F("FAIL"));
  }

  // Периодически обновляем RSSI
  if (now - g_lastWifiCheckMs >= 3000UL) {
    if (WiFi.status() == WL_CONNECTED) {
      g_lastRssiDbm = WiFi.RSSI();
    }
    g_lastWifiCheckMs = now;
  }
}
