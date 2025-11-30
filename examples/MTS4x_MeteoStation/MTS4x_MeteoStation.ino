/*
  MTS4x_MeteoStation_v2.ino
  
  Метеостанция на ESP8266/ESP32 с датчиком MTS4P+T4.
  Версия с улучшенным Wi-Fi менеджером (Reconnection logic + Modem sleep disable).
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

const char* WIFI_SSID     = "YOUR_SSID";      // <-- Впишите имя сети
const char* WIFI_PASSWORD = "YOUR_PASSWORD";  // <-- Впишите пароль

// Калибровка температуры
const float TEMP_OFFSET_C = 0.0f;

// Настройки усреднения UI
static const uint8_t  UI_SAMPLES_PER_CYCLE   = 8;
static const unsigned long UI_UPDATE_INTERVAL_MS = 2000UL;

// Настройки NarodMon (интервал в минутах)
#ifndef NARODMON_INTERVAL_MINUTES
  #define NARODMON_INTERVAL_MINUTES 5
#endif

static const unsigned long NARODMON_INTERVAL_MS =
    (unsigned long)NARODMON_INTERVAL_MINUTES * 60UL * 1000UL;

// --------------------- Паспортные данные датчика -------------------
static const char* SENSOR_NAME             = "MTS4P+T4";
static const char* SENSOR_RANGE_C          = "-40..+85";
static const char* SENSOR_FULL_RANGE_C     = "-103..+153";
static const char* SENSOR_BEST_RANGE_C     = "-25..+25";
static const float SENSOR_BEST_ACCURACY_C  = 0.10f;
static const float SENSOR_TYP_ACCURACY_C   = 0.25f;
static const float SENSOR_RESOLUTION_C     = 0.004f;

// ----------------------------- Глобальные переменные ----------------
MTS4X   mts;

// Данные измерений
float   g_lastTempC        = NAN;
bool    g_lastTempCrcOk    = false;
uint32_t g_crcOkTotal      = 0;
uint32_t g_crcFailTotal    = 0;

// Агрегатор NarodMon
float    g_nmSumTemp       = 0.0f;
uint32_t g_nmCount         = 0;
uint32_t g_nmCrcSkipped    = 0;
float    g_nmLastAvg       = NAN;
bool     g_nmLastSendOk    = false;
unsigned long g_nmLastSendMs = 0;

// Wi-Fi состояние
int32_t       g_lastRssiDbm     = -100;
unsigned long g_lastWifiCheckMs = 0;
unsigned long g_lastUiUpdateMs  = 0;

// Таймер для переподключения Wi-Fi
unsigned long g_wifiReconnectTimer = 0;

// ----------------------------- Вспомогательные функции --------------

static int wifiQuality(int32_t rssiDbm) {
  if (rssiDbm <= -100) return 0;
  if (rssiDbm >= -50)  return 100;
  return 2 * (rssiDbm + 100);
}

static String getMacDashed() {
  String mac = WiFi.macAddress();
  mac.replace(":", "-");
  return mac;
}

static bool sendToNarodMon(float avgTempC) {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClient client;
  // Таймаут соединения 5 сек
  client.setTimeout(5000);
  
  if (!client.connect("narodmon.ru", 8283)) {
    return false;
  }

  String payload;
  payload.reserve(64);
  payload  = "#";
  payload += getMacDashed();
  payload += "\n#T1#";
  payload += String(avgTempC, 2);
  payload += "\n##\n";

  client.print(payload);
  
  // Ждем ответа (необязательно, но полезно для диагностики)
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 2000) {
      client.stop();
      return true; // Считаем, что отправили (UDP-style подход), или false если строг
    }
  }
  client.stop();
  return true;
}

static void performMeasurementCycle() {
  uint8_t okCount = 0;
  float   sum     = 0.0f;

  for (uint8_t i = 0; i < UI_SAMPLES_PER_CYCLE; ++i) {
    if (!mts.startSingleMessurement()) {
      ++g_crcFailTotal;
      delay(5);
      continue;
    }

    float tC    = NAN;
    bool  crcOk = false;
    // Чтение с проверкой CRC
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
      
      // Копим для NarodMon
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
  page += F("<title>MTS4P+T4 Station</title>");
  page += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  page += F("<meta http-equiv='refresh' content='5'>");
  page += F("<style>"
            "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;"
            "background:#111;color:#eee;margin:0;padding:12px;}"
            ".card{background:#1e1e1e;border-radius:10px;padding:10px 12px;margin-bottom:10px;}"
            ".temp{font-size:2.6rem;font-weight:600;margin:0.4rem 0;}"
            ".ok{color:#4caf50;}.fail{color:#ff5252;}.warn{color:#ff9800;}"
            ".label{font-size:0.85rem;color:#aaa;margin-bottom:0.2rem;}"
            ".small{font-size:0.78rem;color:#888;}"
            ".value{font-family:monospace;font-size:0.9rem;}"
            "</style></head><body>");

  page += F("<h1>MTS4P+T4 Station</h1>");

  // Карточка: Температура
  page += F("<div class='card'>");
  page += F("<div class='label'>Текущая температура</div><div class='temp'>");
  if (isnan(g_lastTempC)) {
    page += F("--.- &deg;C");
  } else {
    page += String(g_lastTempC, 3);
    page += F(" &deg;C");
  }
  page += F("</div>");
  page += F("<div class='small'>CRC: ");
  page += g_lastTempCrcOk ? F("<span class='ok'>OK</span>") : F("<span class='fail'>ERR</span>");
  page += F(" | Всего OK/Fail: ");
  page += String(g_crcOkTotal) + " / " + String(g_crcFailTotal);
  page += F("</div></div>");

  // Карточка: Wi-Fi
  page += F("<div class='card'>");
  page += F("<div class='label'>Wi-Fi Status</div><div class='value'>");
  page += F("SSID: ");
  page += (WiFi.status() == WL_CONNECTED) ? WIFI_SSID : "Disconnected";
  page += F("<br>IP: ");
  page += WiFi.localIP().toString();
  page += F("<br>RSSI: ");
  page += String(g_lastRssiDbm);
  page += F(" dBm (");
  page += String(wifiQuality(g_lastRssiDbm));
  page += F("%)");
  if (g_lastRssiDbm < -85 && g_lastRssiDbm > -100) {
     page += F(" <span class='warn'>Weak!</span>");
  }
  page += F("</div></div>");

  // Карточка: NarodMon
  page += F("<div class='card'>");
  page += F("<div class='label'>NarodMon.ru</div><div class='small'>");
  page += F("Queue: ");
  page += String(g_nmCount);
  page += F(" samples<br>Last Avg: ");
  if (!isnan(g_nmLastAvg)) {
    page += String(g_nmLastAvg, 3);
    page += F(" &deg;C (");
    page += g_nmLastSendOk ? F("<span class='ok'>Sent</span>") : F("<span class='fail'>Fail</span>");
    page += F(")");
  } else {
    page += F("Wait...");
  }
  page += F("</div></div>");

  page += F("<div class='small'>JSON API: <a href='/json' style='color:#fff'>/json</a></div>");
  page += F("</body></html>");
  
  server.send(200, "text/html; charset=utf-8", page);
}

static void handleJson() {
  String json;
  json.reserve(512);

  json += F("{");
  json += F("\"temperature_c\":");
  if (isnan(g_lastTempC)) json += F("null,");
  else { json += String(g_lastTempC, 3); json += F(","); }

  json += F("\"crc_ok\":");
  json += g_lastTempCrcOk ? F("true,") : F("false,");
  
  json += F("\"narodmon_last_send_ok\":");
  if (g_nmLastSendMs == 0) json += F("null,");
  else json += g_nmLastSendOk ? F("true,") : F("false,");

  json += F("\"wifi_rssi\":");
  json += String(g_lastRssiDbm);
  
  json += F("}");
  server.send(200, "application/json", json);
}

// ----------------------------- Wi-Fi Logic (IMPROVED) --------------------

static void connectWifi() {
  // Чистая инициализация
  WiFi.persistent(false); 
  WiFi.disconnect(true);  
  delay(200);

  WiFi.mode(WIFI_STA);
  
#if defined(ESP8266)
  WiFi.hostname("MTS4x-Station");
  // КРИТИЧНО для ESP8266: отключаем Modem Sleep, чтобы не терять пакеты
  WiFi.setSleep(false);           
#elif defined(ESP32)
  WiFi.setHostname("MTS4x-Station");
  WiFi.setSleep(false);
#endif

  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print(F("[WiFi] Connecting to "));
  Serial.print(WIFI_SSID);

  uint8_t attempts = 0;
  // Ждем до 15 сек при первом старте
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print('.');
    ++attempts;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("[WiFi] Connected! IP: "));
    Serial.println(WiFi.localIP());
    g_lastRssiDbm = WiFi.RSSI();
  } else {
    Serial.println(F("[WiFi] Connect failed. Will retry in loop."));
    g_lastRssiDbm = -100;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("\n[System] Booting MTS4x Station v2..."));

  // I2C
  if (!mts.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
    Serial.println(F("[MTS4x] Error: I2C/Sensor init failed!"));
  } else {
    Serial.println(F("[MTS4x] Sensor found."));
    // Конфигурация: 1 Гц, AVG 32, Sleep включен
    mts.setConfig(MPS_1Hz, AVG_32, true);
  }

  connectWifi();

  server.on("/", handleRoot);
  server.on("/json", handleJson);
  server.begin();
  
  g_lastUiUpdateMs  = millis();
  g_wifiReconnectTimer = millis();
}

void loop() {
  // 1. Обработка веб-сервера (только если есть сеть)
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }

  unsigned long now = millis();

  // 2. Менеджер подключения Wi-Fi (Watchdog)
  // Проверяем статус раз в 30 секунд. Если отвалился - пробуем снова.
  if (now - g_wifiReconnectTimer >= 30000UL) {
    g_wifiReconnectTimer = now;
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(F("[WiFi] Connection lost. Reconnecting..."));
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      g_lastRssiDbm = -100;
    } else {
      // Если подключены - обновляем RSSI
      g_lastRssiDbm = WiFi.RSSI();
      if (g_lastRssiDbm < -90) {
        Serial.print(F("[WiFi] Weak signal: "));
        Serial.println(g_lastRssiDbm);
      }
    }
  }

  // 3. Цикл измерений (раз в 2 сек)
  if ((now - g_lastUiUpdateMs) >= UI_UPDATE_INTERVAL_MS) {
    performMeasurementCycle();
    g_lastUiUpdateMs = now;
  }

  // 4. Отправка на NarodMon
  if ((now - g_nmLastSendMs) >= NARODMON_INTERVAL_MS && g_nmCount > 0) {
    float avg = g_nmSumTemp / (float)g_nmCount;
    
    Serial.print(F("[NarodMon] Sending avg="));
    Serial.print(avg, 2);
    
    bool ok = sendToNarodMon(avg);
    
    Serial.println(ok ? F(" -> OK") : F(" -> FAIL"));

    g_nmLastAvg    = avg;
    g_nmLastSendOk = ok;
    g_nmLastSendMs = now;
    
    // Сброс
    g_nmSumTemp    = 0.0f;
    g_nmCount      = 0;
    g_nmCrcSkipped = 0;
  }
}