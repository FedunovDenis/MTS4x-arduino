/*
  MTS4x_MeteoStation.ino

  Wi-Fi метеостанция на базе датчика температуры MTS4P+T4 (MTS4x)
  с максимальной точностью, CRC-контролем и отправкой усреднённых
  данных на сервис narodmon.ru по TCP (порт 8283).

  Поддерживаемые платформы: ESP8266, ESP32.
*/

#include <Arduino.h>
#include <Wire.h>
#include "MTS4x.h"

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  ESP8266WebServer server(80);
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  WebServer server(80);
#else
  #error "This example is intended for ESP8266 / ESP32 only"
#endif

// --------------------------- Пользовательские настройки ----------------------

// Пины I2C (по умолчанию подходят для большинства плат)
#if defined(ESP8266)
  #define I2C_SDA_PIN D2
  #define I2C_SCL_PIN D1
#elif defined(ESP32)
  #define I2C_SDA_PIN 21
  #define I2C_SCL_PIN 22
#else
  #define I2C_SDA_PIN SDA
  #define I2C_SCL_PIN SCL
#endif

// Wi-Fi
const char *WIFI_SSID     = "YOUR_SSID";
const char *WIFI_PASSWORD = "YOUR_PASSWORD";

// Калибровка температуры: поправка в градусах Цельсия
// (например, если эталон показывает 23.10 °C, а датчик 22.95 °C,
//  то TEMP_OFFSET_C = 0.15f)
const float TEMP_OFFSET_C = 0.0f;

// Размер усреднения в одном цикле измерений для отображения на веб-странице
static const uint8_t UI_SAMPLES_PER_CYCLE = 8;

// Минимальный интервал отправки на NarodMon в минутах.
// Можно переопределить через -DNARODMON_INTERVAL_MINUTES=1/5/10 при компиляции.
#ifndef NARODMON_INTERVAL_MINUTES
  #define NARODMON_INTERVAL_MINUTES 10
#endif

// Период между циклами измерений (обновление UI), мс
static const unsigned long UI_UPDATE_INTERVAL_MS = 2000UL;

// ------------------------ Константы датчика / паспорт ------------------------

static const char *SENSOR_NAME                = "MTS4P+T4";
static const char *SENSOR_RANGE_C             = "-40…+85";
static const char *SENSOR_FULL_RANGE_C        = "-103…+153";
static const char *SENSOR_BEST_RANGE_C        = "-25…+25";
static const float SENSOR_BEST_ACCURACY_C     = 0.10f;   // ±0.1 °C
static const float SENSOR_TYPO_ACCURACY_C     = 0.25f;   // типично в -40…+85
static const float SENSOR_RESOLUTION_C        = 0.004f;  // 1 LSB = 0.004 °C

// ----------------------------- Глобальные переменные -------------------------

MTS4X mts;

// Последнее отображаемое значение температуры
float    g_lastTempC        = NAN;
bool     g_lastTempCrcAllOk = false;

// Счётчики CRC
uint32_t g_crcOkTotal       = 0;
uint32_t g_crcFailTotal     = 0;
uint16_t g_crcLastCycleOk   = 0;
uint16_t g_crcLastCycleFail = 0;

// NarodMon агрегатор
float    g_nmSumTemp        = 0.0f;
uint32_t g_nmCount          = 0;
uint32_t g_nmCrcSkipped     = 0;
float    g_nmLastAvg        = NAN;
bool     g_nmLastSendOk     = false;
unsigned long g_nmLastSendMs = 0;

// Внутренний таймер цикла измерений
unsigned long g_lastUiUpdateMs = 0;

// Расчёт интервала NarodMon в миллисекундах
static const unsigned long NARODMON_INTERVAL_MS =
    (unsigned long)NARODMON_INTERVAL_MINUTES * 60UL * 1000UL;

// --------------------------- Вспомогательные функции -------------------------

String getMacDashed() {
    // MAC вида "AA:BB:CC:DD:EE:FF" -> "AA-BB-CC-DD-EE-FF"
    String mac = WiFi.macAddress();
    mac.replace(':', '-');
    return mac;
}

bool sendToNarodMon(float avgTemp) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    WiFiClient client;
    if (!client.connect("narodmon.ru", 8283)) {
        return false;
    }

    String macDashed = getMacDashed();

    String payload;
    payload.reserve(64);
    payload  = "#";
    payload += macDashed;
    payload += "\n#T1#";
    payload += String(avgTemp, 2);
    payload += "\n##\n";

    client.print(payload);
    client.stop();
    return true;
}

void performMeasurementCycle() {
    uint16_t ok   = 0;
    uint16_t fail = 0;
    float    sum  = 0.0f;

    for (uint8_t i = 0; i < UI_SAMPLES_PER_CYCLE; ++i) {
        // Один single-shot
        if (!mts.startSingleMessurement()) {
            ++fail;
            ++g_crcFailTotal;
            delay(5);
            continue;
        }

        float tC = NAN;
        bool  crcOk = false;
        if (!mts.readTemperatureCrc(tC, crcOk, true)) {
            ++fail;
            ++g_crcFailTotal;
            delay(5);
            continue;
        }

        if (crcOk) {
            tC += TEMP_OFFSET_C;
            sum += tC;
            ++ok;
            ++g_crcOkTotal;

            // Для NarodMon копим только валидные (по CRC) значения
            g_nmSumTemp += tC;
            ++g_nmCount;
        } else {
            ++fail;
            ++g_crcFailTotal;
            ++g_nmCrcSkipped;
        }

        delay(10);
    }

    g_crcLastCycleOk   = ok;
    g_crcLastCycleFail = fail;

    if (ok > 0) {
        g_lastTempC        = sum / (float)ok;
        g_lastTempCrcAllOk = (fail == 0);
    }
}

void handleRoot() {
    String page;
    page.reserve(2048);

    page += F("<!DOCTYPE html><html lang='ru'><head><meta charset='UTF-8'>");
    page += F("<title>MTS4P+T4 — метеостанция</title>");
    page += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
    page += F("<style>");
    page += F("body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;"
              "background:#111;color:#eee;margin:0;padding:16px;}"
              "h1{font-size:1.4rem;margin:0 0 0.3rem 0;}"
              ".temp{font-size:3.2rem;font-weight:600;margin:0.4rem 0;}"
              ".ok{color:#4caf50;}.fail{color:#ff5252;}"
              ".card{background:#1e1e1e;border-radius:12px;padding:12px 14px;margin-bottom:12px;}"
              ".label{font-size:0.8rem;color:#aaa;margin-bottom:0.15rem;}"
              ".value{font-family:monospace;font-size:0.95rem;}"
              ".row{display:flex;flex-wrap:wrap;gap:8px;}"
              ".col{flex:1 1 220px;}"
              "table{width:100%;border-collapse:collapse;font-size:0.85rem;}"
              "th,td{border-bottom:1px solid #333;padding:4px 6px;text-align:left;}"
              "th{color:#bbb;font-weight:500;}"
              ".small{font-size:0.78rem;color:#888;margin-top:4px;}"
              "code{background:#252525;padding:2px 4px;border-radius:4px;font-size:0.8rem;}"
    );
    page += F("</style></head><body>");

    page += F("<h1>MTS4P+T4 — метеостанция</h1>");

    page += F("<div class='card'>");
    page += F("<div class='label'>Текущая температура воздуха</div><div class='temp'>");
    if (isnan(g_lastTempC)) {
        page += F("--.- °C");
    } else {
        page += String(g_lastTempC, 3);
        page += F(" °C");
    }
    page += F("</div>");

    page += F("<div class='small'>");

    if (g_crcLastCycleOk == 0 && g_crcLastCycleFail == 0) {
        page += F("CRC: пока нет измерений.");
    } else if (g_crcLastCycleFail == 0) {
        page += F("<span class='ok'>CRC последнего цикла: OK</span>");
    } else {
        page += F("<span class='fail'>CRC последнего цикла: есть ошибки</span>");
    }

    page += F("<br>Всего CRC OK: ");
    page += String(g_crcOkTotal);
    page += F(", CRC FAIL: ");
    page += String(g_crcFailTotal);
    page += F("</div>");

    page += F("</div>"); // card

    // NarodMon карточка
    page += F("<div class='card'><div class='label'>NarodMon.ru</div>");
    page += F("<div class='value'>");
    page += F("Интервал отправки: ");
    page += String((int)NARODMON_INTERVAL_MINUTES);
    page += F(" мин");

    page += F("<br>Текущая серия для усреднения: count=");
    page += String(g_nmCount);
    page += F(", пропущено по CRC=");
    page += String(g_nmCrcSkipped);

    page += F("<br>Последний отправленный средний: ");
    if (isnan(g_nmLastAvg)) {
        page += F("нет данных");
    } else {
        page += String(g_nmLastAvg, 3);
        page += F(" °C");
    }

    page += F("<br>Статус последней отправки: ");
    if (g_nmLastSendMs == 0) {
        page += F("ещё не отправлялось");
    } else {
        page += g_nmLastSendOk ? F("OK") : F("ошибка");
    }

    page += F("</div>");
    page += F("<div class='small'>Протокол: TCP &rarr; narodmon.ru:8283, формат пакета:"
              "<br><code>#MAC<br>#T1#value<br>##</code></div>");
    page += F("</div>");

    // Информация о датчике и режиме измерений
    page += F("<div class='row'>");

    page += F("<div class='card col'>");
    page += F("<div class='label'>Датчик</div>");
    page += F("<div class='value'>");
    page += SENSOR_NAME;
    page += F("<br>Рабочий диапазон (рекомендуемый): ");
    page += SENSOR_RANGE_C;
    page += F(" °C");
    page += F("<br>Внутренний диапазон чипа: ");
    page += SENSOR_FULL_RANGE_C;
    page += F(" °C");
    page += F("<br>Окно максимальной точности: ");
    page += SENSOR_BEST_RANGE_C;
    page += F(" °C, &plusmn;");
    page += String(SENSOR_BEST_ACCURACY_C, 2);
    page += F(" °C");
    page += F("<br>Типичная точность в диапазоне -40…+85 °C: &plusmn;");
    page += String(SENSOR_TYPO_ACCURACY_C, 2);
    page += F(" °C");
    page += F("<br>Разрешение АЦП: ");
    page += String(SENSOR_RESOLUTION_C, 3);
    page += F(" °C/LSB</div>");
    page += F("</div>");

    page += F("<div class='card col'>");
    page += F("<div class='label'>Режим измерений</div><div class='value'>");
    page += F("Temp_Cfg: MPS_1Hz, AVG_32, single-shot с программным усреднением.<br>");
    page += F("На каждый цикл UI: ");
    page += String(UI_SAMPLES_PER_CYCLE);
    page += F(" single-shot измерений с CRC и усреднением.<br>");
    page += F("Доп. поправка TEMP_OFFSET_C = ");
    page += String(TEMP_OFFSET_C, 3);
    page += F(" °C.");
    page += F("</div><div class='small'>"
              "Для точных измерений датчик должен быть в радиационном экране, "
              "вдали от стен и прямого солнца.</div>");
    page += F("</div>");

    // Информация о сети
    page += F("<div class='card col'>");
    page += F("<div class='label'>Сеть</div><div class='value'>");
    page += F("Wi-Fi SSID: ");
    page += WIFI_SSID;
    page += F("<br>IP: ");
    page += WiFi.localIP().toString();
    page += F("<br>MAC: ");
    page += WiFi.macAddress();
    page += F("</div>");
    page += F("<div class='small'>JSON API: "
              "<code>/json</code> (см. поля temperature_c, crc_ok, range_c и др.).</div>");
    page += F("</div>");

    page += F("</div>"); // row

    page += F("</body></html>");

    server.send(200, "text/html; charset=utf-8", page);
}

void handleJson() {
    String json;
    json.reserve(512);

    json += F("{");

    json += F("\"sensor\":\"");
    json += SENSOR_NAME;
    json += F("\",");

    json += F("\"temperature_c\":");
    if (isnan(g_lastTempC)) {
        json += F("null,");
    } else {
        json += String(g_lastTempC, 3);
        json += F(",");
    }

    bool crcOk = (g_crcLastCycleOk > 0 && g_crcLastCycleFail == 0);
    json += F("\"crc_ok\":");
    json += crcOk ? F("true,") : F("false,");

    json += F("\"crc_cycle_ok\":");
    json += String(g_crcLastCycleOk);
    json += F(",\"crc_cycle_fail\":");
    json += String(g_crcLastCycleFail);
    json += F(",\"crc_total_ok\":");
    json += String(g_crcOkTotal);
    json += F(",\"crc_total_fail\":");
    json += String(g_crcFailTotal);
    json += F(",");

    json += F("\"range_c\":\"");
    json += SENSOR_RANGE_C;
    json += F("\",\"full_range_c\":\"");
    json += SENSOR_FULL_RANGE_C;
    json += F("\",");

    json += F("\"best_accuracy_c\":");
    json += String(SENSOR_BEST_ACCURACY_C, 2);
    json += F(",\"best_accuracy_range_c\":\"");
    json += SENSOR_BEST_RANGE_C;
    json += F("\",");

    json += F("\"resolution_c\":");
    json += String(SENSOR_RESOLUTION_C, 3);
    json += F(",");

    json += F("\"avg_mode\":\"AVG_32 + ");
    json += String(UI_SAMPLES_PER_CYCLE);
    json += F("x single-shot\",");
    json += F("\"samples\":");
    json += String(UI_SAMPLES_PER_CYCLE);
    json += F(",");

    json += F("\"temp_offset_c\":");
    json += String(TEMP_OFFSET_C, 3);
    json += F(",");

    // Status register
    uint8_t st = 0;
    bool stOk = mts.readStatus(st);
    json += F("\"status_ok\":");
    json += stOk ? F("true,") : F("false,");
    json += F("\"status\":");
    if (stOk) {
        json += String((unsigned long)st);
    } else {
        json += F("null");
    }
    json += F(",");

    // NarodMon stats
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

    json += F("}");

    server.send(200, "application/json; charset=utf-8", json);
}

// -----------------------------------------------------------------------------
// Setup / loop
// -----------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println();
    Serial.println(F("[MTS4x] Meteostation starting..."));

    // I2C и датчик
    if (!mts.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
        Serial.print(F("[MTS4x] begin() failed, err="));
        Serial.println(mts.lastError());
    } else {
        Serial.println(F("[MTS4x] I2C init OK"));
    }

    // Конфигурация датчика: 1 Гц, AVG_32, Sleep_en=1 (single-shot)
    if (!mts.setConfig(MPS_1Hz, AVG_32, true)) {
        Serial.print(F("[MTS4x] setConfig() failed, err="));
        Serial.println(mts.lastError());
    } else {
        Serial.println(F("[MTS4x] setConfig OK: MPS_1Hz, AVG_32, sleep"));
    }

    // Просто убедимся, что TH/TL не в конфликте
    float th, tl;
    if (mts.getHighLimit(th) && mts.getLowLimit(tl)) {
        if (th <= tl) {
            Serial.println(F("[MTS4x] TH<=TL, сброс порогов в безопасные значения"));
            mts.setHighLimit(80.0f);
            mts.setLowLimit(-40.0f);
        }
    }

    // Wi-Fi
    WiFi.mode(WIFI_STA);
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
        Serial.println(F("[WiFi] Failed to connect, continuing in AP-less mode"));
    }

    // HTTP-маршруты
    server.on("/", handleRoot);
    server.on("/json", handleJson);
    server.begin();

    Serial.println(F("[HTTP] Server started on port 80"));

    g_lastUiUpdateMs = millis();
    g_nmLastSendMs   = 0;
}

void loop() {
    server.handleClient();

    unsigned long now = millis();

    // Периодический цикл измерений
    if ((now - g_lastUiUpdateMs) >= UI_UPDATE_INTERVAL_MS) {
        performMeasurementCycle();
        g_lastUiUpdateMs = now;
    }

    // Отправка усреднённых данных на NarodMon
    if ((now - g_nmLastSendMs) >= NARODMON_INTERVAL_MS &&
        g_nmCount > 0) {

        float avg = g_nmSumTemp / (float)g_nmCount;
        bool ok   = sendToNarodMon(avg);

        g_nmLastAvg    = avg;
        g_nmLastSendOk = ok;
        g_nmLastSendMs = now;

        // Сброс накопителей
        g_nmSumTemp    = 0.0f;
        g_nmCount      = 0;
        g_nmCrcSkipped = 0;

        Serial.print(F("[NarodMon] Send avg="));
        Serial.print(avg, 3);
        Serial.print(F(" °C, status="));
        Serial.println(ok ? F("OK") : F("FAIL"));
    }
}
