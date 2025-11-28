#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "MTS4x.h"

// I2C pins for ESP8266 (NodeMCU / WeMos D1 mini)
const int I2C_SDA_PIN = D2;  // GPIO4
const int I2C_SCL_PIN = D1;  // GPIO5

// WiFi config
const char* WIFI_SSID     = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

MTS4X Temp;

const MTS4x_MPS MTS_RATE  = MPS_8Hz;
const MTS4x_AVG MTS_AVG   = AVG_32;
const bool      MTS_BLOCK = true;

ESP8266WebServer server(80);

float currentTempC = NAN;
unsigned long lastMeasureMs = 0;
const unsigned long MEASURE_PERIOD_MS = 2000;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print(F("Connecting to WiFi "));
  Serial.print(WIFI_SSID);
  Serial.print(F("... "));

  uint8_t retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 60) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("WiFi connected, IP: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("WiFi connect FAILED."));
  }
}

float readTemperatureC() {
  Temp.startSingleMessurement();
  return Temp.readTemperature(MTS_BLOCK);
}

void updateTemperatureIfNeeded() {
  unsigned long now = millis();
  if (now - lastMeasureMs >= MEASURE_PERIOD_MS || isnan(currentTempC)) {
    lastMeasureMs = now;
    currentTempC = readTemperatureC();
    Serial.print(F("Temp: "));
    Serial.print(currentTempC);
    Serial.println(F(" °C"));
  }
}

void handleRoot() {
  updateTemperatureIfNeeded();

  String html;
  html.reserve(1024);
  html += F("<!DOCTYPE html><html><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<meta http-equiv='refresh' content='5'>");
  html += F("<title>MTS4x Meteo</title>");
  html += F("<style>"
            "body{font-family:sans-serif;background:#111;color:#eee;"
            "display:flex;flex-direction:column;align-items:center;justify-content:center;"
            "min-height:100vh;margin:0;}"
            ".card{background:#222;padding:20px 30px;border-radius:12px;box-shadow:0 0 15px #0008;"
            "text-align:center;}"
            ".temp{font-size:3rem;margin:10px 0;}"
            ".small{font-size:0.9rem;color:#aaa;}"
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
  html += F("<div class='small'>Обновление каждые 5 секунд<br>");
  html += F("IP: ");
  html += WiFi.localIP().toString();
  html += F("<br>JSON: <a href='/json'>/json</a></div>");
  html += F("</div></body></html>");

  server.send(200, "text/html; charset=utf-8", html);
}

void handleJson() {
  updateTemperatureIfNeeded();

  String json = F("{\"temperature_c\":");
  if (isnan(currentTempC)) {
    json += F("null");
  } else {
    json += String(currentTempC, 2);
  }
  json += F("}");

  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("MTS4x Meteo ESP8266"));

  if (!Temp.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
    Serial.println(F("MTS4x init FAILED. Check wiring."));
  } else {
    Temp.setConfig(MTS_RATE, MTS_AVG, true);
    Serial.println(F("MTS4x init OK."));
  }

  connectWiFi();

  server.on("/", handleRoot);
  server.on("/json", handleJson);
  server.onNotFound([]() {
    server.send(404, "text/plain; charset=utf-8", "404 Not found");
  });
  server.begin();
  Serial.println(F("HTTP server started."));
}

void loop() {
  server.handleClient();
  updateTemperatureIfNeeded();
}
