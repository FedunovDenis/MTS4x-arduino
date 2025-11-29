#include <Arduino.h>
#include <Wire.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <WiFiUdp.h>
  ESP8266WebServer server(80);
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  #include <WiFiUdp.h>
  WebServer server(80);
#else
  #error "This example is for ESP8266/ESP32 only"
#endif

#include "MTS4x.h"

// -------------------- WIFI --------------------
const char* WIFI_SSID     = "YOUR_SSID";      // <<< сюда свою сеть
const char* WIFI_PASSWORD = "YOUR_PASSWORD";  // <<< сюда пароль

// -------------------- I2C PINS ----------------
#if defined(ESP8266)
  const int I2C_SDA_PIN = D2;
  const int I2C_SCL_PIN = D1;
#elif defined(ESP32)
  const int I2C_SDA_PIN = 21;
  const int I2C_SCL_PIN = 22;
#else
  const int I2C_SDA_PIN = SDA;
  const int I2C_SCL_PIN = SCL;
#endif

// -------------------- MTS4x CONFIG ------------
MTS4X mts;

// "максимальная точность" для метеостанции
static const TempCfgMPS MTS_RATE    = MPS_1Hz;   // частота конверсий
static const TempCfgAVG MTS_AVG     = AVG_32;    // максимальное усреднение в чипе
static const uint8_t    NUM_SAMPLES = 8;         // доп. усреднение в МК

// программная калибровка, °C (подгоняется по эталонному термометру)
static const float TEMP_OFFSET_C = 0.0f;

// -------------------- NARODMON (TCP/UDP 8283) ----------------
#define NARODMON_ENABLE      1      // 0 — выключить, 1 — включить
#define NARODMON_USE_UDP     0      // 0 — TCP, 1 — UDP

const char*   NARODMON_HOST    = "narodmon.ru";
const uint16_t NARODMON_PORT   = 8283;

// Минимальный интервал отправки на NarodMon (по правилам не чаще 1 раза в минуту)
// Выбери один из вариантов: 1, 5 или 10 минут
const unsigned long NARODMON_INTERVAL_1_MIN_MS  = 1UL  * 60UL * 1000UL;
const unsigned long NARODMON_INTERVAL_5_MIN_MS  = 5UL  * 60UL * 1000UL;
const unsigned long NARODMON_INTERVAL_10_MIN_MS = 10UL * 60UL * 1000UL;

// <<< ВЫБОР ТЕКУЩЕГО ИНТЕРВАЛА >>>
//const unsigned long NARODMON_MIN_INTERVAL_MS = NARODMON_INTERVAL_1_MIN_MS;
 const unsigned long NARODMON_MIN_INTERVAL_MS = NARODMON_INTERVAL_5_MIN_MS;
// const unsigned long NARODMON_MIN_INTERVAL_MS = NARODMON_INTERVAL_10_MIN_MS;

// Название станции и сенсора (опционально)
const char* NARODMON_STATION_NAME = "Meteo";        // #MAC#Meteo
const char* NARODMON_SENSOR_NAME  = "Outdoor";      // #T1#19.73#Outdoor

// NarodMon ID = MAC платы в формате AA-BB-CC-DD-EE-FF (автоматически)
String   g_narodmonMac;
unsigned long g_lastNarodmonSendMs = 0;
WiFiUDP narodmonUdp;               // для UDP-режима

// -------------------- STATE -------------------
float    g_lastTempC     = NAN;    // усреднённая + OFFSET температура за последний цикл измерения
bool     g_lastCrcAllOk  = false;  // true, если все выборки в последнем цикле с CRC OK
uint32_t g_crcOkTotal    = 0;
uint32_t g_crcFailTotal  = 0;

unsigned long g_lastMeasureMs = 0;
const unsigned long MEASURE_INTERVAL_MS = 2000UL; // цикл измерений раз в 2 с

// Аккумулятор для усреднения за интервал NarodMon
double   g_nmSum      = 0.0;      // сумма температур за интервал
uint32_t g_nmCount    = 0;        // количество учтённых значений
uint32_t g_nmCrcBad   = 0;        // сколько раз не учитывали из-за CRC (для отладки)

// --------------------------------------------------------
// NarodMon MAC = MAC WiFi (AA:BB:CC:DD:EE:FF -> AA-BB-CC-DD-EE-FF)
// --------------------------------------------------------
void initNarodmonMac()
{
#if defined(ESP8266) || defined(ESP32)
  String mac = WiFi.macAddress();
  for (uint16_t i = 0; i < mac.length(); ++i) {
    if (mac[i] == ':') mac[i] = '-';
  }
  mac.toUpperCase();
  g_narodmonMac = mac;
#else
  g_narodmonMac = "00-00-00-00-00-00";
#endif
}

// --------------------------------------------------------
// WIFI CONNECT
// --------------------------------------------------------
void connectWiFi()
{
#if defined(ESP8266)
  WiFi.mode(WIFI_STA);
#elif defined(ESP32)
  WiFi.mode(WIFI_MODE_STA);
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print(F("Connecting to WiFi "));
  Serial.print(WIFI_SSID);
  Serial.println(F(" ..."));

  uint8_t retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 60) { // ~30 сек
    delay(500);
    Serial.print('.');
    retries++;
  }
  Serial.println();

  // MAC доступен и до соединения, но заодно и тут, когда всё уже стартовало
  initNarodmonMac();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("WiFi connected, IP: "));
    Serial.println(WiFi.localIP());
    Serial.print(F("WiFi MAC: "));
    Serial.println(WiFi.macAddress());
  } else {
    Serial.println(F("WiFi connect FAILED, работаем только локально."));
  }

  Serial.print(F("NarodMon ID (MAC): "));
  Serial.println(g_narodmonMac);
}

// --------------------------------------------------------
// ОДИН ЦИКЛ ИЗМЕРЕНИЙ С УСРЕДНЕНИЕМ
// --------------------------------------------------------
void doMeasurement()
{
  float   sum          = 0.0f;
  uint8_t goodSamples  = 0;
  uint8_t crcOkSamples = 0;

  for (uint8_t i = 0; i < NUM_SAMPLES; ++i) {
    float t;
    bool  crcOk;

    // ждём новое значение, с CRC
    bool ok = mts.readTemperatureCrc(t, crcOk, true);
    if (ok && crcOk) {
      sum += t;
      goodSamples++;
      crcOkSamples++;
      g_crcOkTotal++;
    } else {
      g_crcFailTotal++;
    }

    delay(50); // пауза между выборками
  }

  if (goodSamples > 0) {
    float avg = sum / goodSamples;
    avg += TEMP_OFFSET_C;

    g_lastTempC    = avg;
    g_lastCrcAllOk = (crcOkSamples == NUM_SAMPLES && goodSamples == NUM_SAMPLES);
  } else {
    g_lastTempC    = NAN;
    g_lastCrcAllOk = false;
  }

  // --- накопитель для NarodMon: усреднение за интервал ---
  if (!isnan(g_lastTempC)) {
    if (g_lastCrcAllOk) {
      g_nmSum   += g_lastTempC;
      g_nmCount += 1;
    } else {
      g_nmCrcBad += 1;
    }
  }

  Serial.print(F("Temp(avg): "));
  if (isnan(g_lastTempC)) {
    Serial.print(F("NaN"));
  } else {
    Serial.print(g_lastTempC, 3);
    Serial.print(F(" °C"));
  }
  Serial.print(F("  CRC last cycle: "));
  Serial.println(g_lastCrcAllOk ? F("ALL OK") : F("HAS ERRORS"));
}

// --------------------------------------------------------
// ПЕРИОДИЧЕСКОЕ ОБНОВЛЕНИЕ ТЕМПЕРАТУРЫ
// --------------------------------------------------------
void updateTemperatureIfNeeded()
{
  unsigned long now = millis();
  if (now - g_lastMeasureMs >= MEASURE_INTERVAL_MS || isnan(g_lastTempC)) {
    g_lastMeasureMs = now;
    doMeasurement();
  }
}

// --------------------------------------------------------
// NarodMon: формирование пакета TCP/UDP на порт 8283
// Формат:
// #MAC[#NAME]\n
// #T1#value[#name]\n
// ##\n
// --------------------------------------------------------
String buildNarodmonPacket(float tempToSendC)
{
  String pkt;
  pkt.reserve(256);

  // 1) MAC и имя станции
  pkt += "#";
  pkt += g_narodmonMac;
  if (NARODMON_STATION_NAME && NARODMON_STATION_NAME[0] != '\0') {
    pkt += "#";
    pkt += NARODMON_STATION_NAME;
  }
  pkt += "\n";

  // 2) Само измерение температуры
  //   пример: #T1#19.73 или #T1#19.73#Улица
  pkt += "#T1#";
  pkt += String(tempToSendC, 2);  // 2 знака после запятой
  if (NARODMON_SENSOR_NAME && NARODMON_SENSOR_NAME[0] != '\0') {
    pkt += "#";
    pkt += NARODMON_SENSOR_NAME;
  }
  pkt += "\n";

  // 3) конец пакета
  pkt += "##\n";

  return pkt;
}

// --------------------------------------------------------
// NarodMon: отправка пакета (TCP или UDP) со средним за интервал
// --------------------------------------------------------
void sendToNarodmonIfNeeded()
{
#if NARODMON_ENABLE
  if (WiFi.status() != WL_CONNECTED) return;

  unsigned long now = millis();
  if (now - g_lastNarodmonSendMs < NARODMON_MIN_INTERVAL_MS) return;

  // готовим среднее за интервал
  float tempToSend;

  if (g_nmCount > 0) {
    tempToSend = (float)(g_nmSum / (double)g_nmCount);
  } else {
    // на всякий случай fallback — последнее измерение
    if (isnan(g_lastTempC)) return;  // вообще нечего отправлять
    tempToSend = g_lastTempC;
  }

  g_lastNarodmonSendMs = now;

  String pkt = buildNarodmonPacket(tempToSend);
  Serial.println(F("[NarodMon] Avg interval value: "));
  Serial.print(F("  count="));
  Serial.print(g_nmCount);
  Serial.print(F(", avg="));
  Serial.println(tempToSend, 3);
  Serial.println(F("[NarodMon] Send packet:"));
  Serial.println(pkt);

#if NARODMON_USE_UDP
  // ------------ UDP ------------
  if (!narodmonUdp.begin(0)) {
    Serial.println(F("[NarodMon] UDP begin() failed"));
    return;
  }
  if (narodmonUdp.beginPacket(NARODMON_HOST, NARODMON_PORT) != 1) {
    Serial.println(F("[NarodMon] UDP beginPacket() failed"));
    return;
  }
  narodmonUdp.write((const uint8_t*)pkt.c_str(), pkt.length());
  narodmonUdp.endPacket();
  Serial.println(F("[NarodMon] UDP packet sent"));
#else
  // ------------ TCP ------------
  WiFiClient client;
  Serial.println(F("[NarodMon] Connecting TCP..."));
  if (!client.connect(NARODMON_HOST, NARODMON_PORT)) {
    Serial.println(F("[NarodMon] TCP connect failed"));
    return;
  }

  // Важно: протокол требует только '\n' (0x0A), без \r\n
  client.print(pkt);

  // можно (необязательно) немного подождать и прочитать ответ
  unsigned long t0 = millis();
  while (client.connected() && millis() - t0 < 3000) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      line.trim();
      if (line.length() > 0) {
        Serial.print(F("[NarodMon] Response: "));
        Serial.println(line);
        break;
      }
    }
  }

  client.stop();
  Serial.println(F("[NarodMon] TCP packet sent"));
#endif // NARODMON_USE_UDP

  // после отправки — обнуляем накопитель интервала
  g_nmSum    = 0.0;
  g_nmCount  = 0;
  g_nmCrcBad = 0;

#else
  // NarodMon отключён
#endif
}

// --------------------------------------------------------
// HTTP: /  (страница)
// --------------------------------------------------------
void handleRoot()
{
  updateTemperatureIfNeeded();

  String html;
  html.reserve(2300);

  html += F("<!DOCTYPE html><html><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<meta http-equiv='refresh' content='5'>");
  html += F("<title>MTS4P+T4 Meteo + NarodMon</title>");
  html += F("<style>"
            "body{font-family:-apple-system,BlinkMacSystemFont,Segoe UI,Roboto,Arial,sans-serif;"
            "background:#111;color:#eee;display:flex;flex-direction:column;"
            "align-items:center;justify-content:center;min-height:100vh;margin:0;}"
            ".card{background:#222;padding:20px 24px;border-radius:14px;"
            "box-shadow:0 0 18px #000a;max-width:460px;width:100%;}"
            "h1{font-size:1.4rem;margin:0 0 10px 0;}"
            ".temp{font-size:3.2rem;margin:10px 0;font-weight:600;}"
            ".ok{color:#6fcf97;}.bad{color:#eb5757;}"
            ".small{font-size:0.9rem;color:#ccc;line-height:1.4;}"
            "code{background:#000;padding:1px 4px;border-radius:4px;font-size:0.85em;}"
            "a{color:#6cf;}"
            "</style></head><body>");

  html += F("<div class='card'>");
  html += F("<h1>MTS4P+T4 — метеостанция</h1>");

  html += F("<div class='temp'>");
  if (isnan(g_lastTempC)) {
    html += F("--.- &deg;C");
  } else {
    html += String(g_lastTempC, 3);
    html += F(" &deg;C");
  }
  html += F("</div>");

  // CRC-индикатор
  html += F("<div class='small'>CRC последнего цикла: ");
  if (g_lastCrcAllOk) {
    html += F("<span class='ok'>OK (все выборки)</span>");
  } else {
    html += F("<span class='bad'>есть ошибки</span>");
  }
  html += F("<br>Всего CRC OK: ");
  html += String(g_crcOkTotal);
  html += F(", CRC FAIL: ");
  html += String(g_crcFailTotal);
  html += F("<br>Усреднение для NarodMon: count=");
  html += String(g_nmCount);
  html += F(", пропущено по CRC=");
  html += String(g_nmCrcBad);
  html += F("</div><hr style='border:0;border-top:1px solid #333;margin:10px 0;'>");

  // Паспортные характеристики
  html += F("<div class='small'>");
  html += F("<b>Датчик:</b> MTS4P+T4 (семейство MTS4)<br>");
  html += F("<b>Диапазон чипа:</b> −103…+153&nbsp;&deg;C<br>");
  html += F("<b>Окно максимальной точности:</b> −25…+25&nbsp;&deg;C (≈&plusmn;0.1&nbsp;&deg;C)<br>");
  html += F("<b>Разрешение:</b> 0.004&nbsp;&deg;C (16&nbsp;бит)<br>");
  html += F("<b>Усреднение:</b> MPS_1Hz, AVG_32, ");
  html += String(NUM_SAMPLES);
  html += F(" выборок в МК<br>");
  html += F("<b>TEMP_OFFSET_C:</b> ");
  html += String(TEMP_OFFSET_C, 3);
  html += F("&nbsp;&deg;C<br>");
  html += F("</div>");

  html += F("<hr style='border:0;border-top:1px solid #333;margin:10px 0;'>");

  // Сеть + NarodMon
  html += F("<div class='small'>");
  html += F("<b>Wi-Fi IP:</b> ");
  if (WiFi.status() == WL_CONNECTED) {
    html += WiFi.localIP().toString();
  } else {
    html += F("нет подключения");
  }
  html += F("<br><b>JSON API:</b> <a href='/json'>/json</a><br>");

#if NARODMON_ENABLE
  html += F("<b>NarodMon:</b> включено, ");
  html += F("сервер <code>narodmon.ru:");
  html += String(NARODMON_PORT);
  html += F("</code>, протокол <code>");
  html += (NARODMON_USE_UDP ? F("UDP") : F("TCP"));
  html += F("</code><br>ID (MAC): <code>");
  html += g_narodmonMac;
  html += F("</code><br>");
  html += F("Интервал усреднения и отправки: ");
  html += String(NARODMON_MIN_INTERVAL_MS / 60000UL);
  html += F(" мин<br>");
  html += F("Формат пакета:<br><code>#MAC[#NAME]<br>#T1#value[#name]<br>##</code><br>");
#else
  html += F("<b>NarodMon:</b> выключено (см. NARODMON_ENABLE в скетче)<br>");
#endif

  html += F("</div>");

  html += F("</div></body></html>");

  server.send(200, "text/html; charset=utf-8", html);
}

// --------------------------------------------------------
// HTTP: /json
// --------------------------------------------------------
void handleJson()
{
  updateTemperatureIfNeeded();

  String json;
  json.reserve(600);
  json += F("{");

  // температура
  json += F("\"temperature_c\":");
  if (isnan(g_lastTempC)) {
    json += F("null");
  } else {
    json += String(g_lastTempC, 3);
  }

  json += F(",\"crc_ok\":");
  json += (g_lastCrcAllOk ? F("true") : F("false"));

  json += F(",\"samples\":");
  json += String(NUM_SAMPLES);

  json += F(",\"avg_mode\":\"AVG_32\"");
  json += F(",\"temp_offset_c\":");
  json += String(TEMP_OFFSET_C, 3);

  json += F(",\"crc_ok_total\":");
  json += String(g_crcOkTotal);
  json += F(",\"crc_fail_total\":");
  json += String(g_crcFailTotal);

  // накопитель NarodMon
  json += F(",\"narodmon_avg_acc\":{");
  json += F("\"sum\":");
  json += String(g_nmSum, 6);
  json += F(",\"count\":");
  json += String(g_nmCount);
  json += F(",\"crc_bad\":");
  json += String(g_nmCrcBad);
  json += F("}");

  // паспорт датчика
  json += F(",\"sensor\":\"MTS4P+T4\"");
  json += F(",\"range_c\":[-103,153]");
  json += F(",\"best_accuracy_c\":0.1");
  json += F(",\"best_accuracy_range_c\":[-25,25]");
  json += F(",\"resolution_c\":0.004");

#if NARODMON_ENABLE
  json += F(",\"narodmon\":{");
  json += F("\"enabled\":true,");
  json += F("\"mac\":\"");
  json += g_narodmonMac;
  json += F("\",\"host\":\"");
  json += NARODMON_HOST;
  json += F("\",\"port\":");
  json += String(NARODMON_PORT);
  json += F(",\"protocol\":\"");
  json += (NARODMON_USE_UDP ? F("UDP") : F("TCP"));
  json += F("\",\"min_interval_s\":");
  json += String(NARODMON_MIN_INTERVAL_MS / 1000UL);
  json += F("}");
#else
  json += F(",\"narodmon\":{\"enabled\":false}");
#endif

  json += F("}");

  server.send(200, "application/json; charset=utf-8", json);
}

// --------------------------------------------------------
// SETUP
// --------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println(F("MTS4x Meteo + NarodMon (TCP/UDP 8283, interval avg) start"));

  // I2C + датчик
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  if (!mts.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
    Serial.print(F("MTS4x init FAILED, err="));
    Serial.println(mts.lastError());
  } else {
    Serial.println(F("MTS4x init OK"));
  }

  // STOP + AVG_32, single-shot
  mts.setConfig(MTS_RATE, MTS_AVG, true);          // sleep=true
  mts.setMode(MEASURE_STOP, false);                // стоп, heater off

  // Wi-Fi
  connectWiFi();

  // HTTP маршруты
  server.on("/", handleRoot);
  server.on("/json", handleJson);
  server.onNotFound([]() {
    server.send(404, "text/plain; charset=utf-8", "404 Not found");
  });

  server.begin();
  Serial.println(F("HTTP server started"));
}

// --------------------------------------------------------
// LOOP
// --------------------------------------------------------
void loop()
{
  server.handleClient();
  updateTemperatureIfNeeded();
  sendToNarodmonIfNeeded();
}
