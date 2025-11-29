#include <Arduino.h>
#include <Wire.h>
#include "MTS4x.h"

// ======================= НАСТРОЙКА ПИНОВ I2C =========================

#if defined(ESP8266)
  // NodeMCU / WeMos D1 mini
  #define I2C_SDA_PIN D2   // GPIO4
  #define I2C_SCL_PIN D1   // GPIO5
#elif defined(ESP32)
  // Стандартные пины ESP32 (можешь поменять под свою плату)
  #define I2C_SDA_PIN 21
  #define I2C_SCL_PIN 22
#else
  // Для AVR и прочих
  #define I2C_SDA_PIN SDA
  #define I2C_SCL_PIN SCL
#endif

// ======================= ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ =======================

MTS4X mts;  // адрес 0x41 по умолчанию

const MTS4x_MPS MTS_RATE  = MPS_1Hz;   // 1 измерение/с
const MTS4x_AVG MTS_AVG   = AVG_8;     // усреднение ×8
const bool      MTS_SLEEP = true;      // sleep в single-режиме

unsigned long lastMeasureMs = 0;
const unsigned long MEASURE_PERIOD_MS = 1000;

// Пороговые температуры (пример)
const float HIGH_LIMIT_C = 40.0f;
const float LOW_LIMIT_C  = 5.0f;

// ======================= ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ====================

void printHelp() {
  Serial.println();
  Serial.println(F("===== MTS4x Full Demo (ESP8266/ESP32) ====="));
  Serial.println(F("Команды по Serial:"));
  Serial.println(F("  ?   - показать это меню"));
  Serial.println(F("  i   - прочитать Device ID и ROM-код"));
  Serial.println(F("  a   - прочитать Alert_Mode и пороги TH/TL"));
  Serial.println(F("  t   - включить/выключить heater"));
  Serial.println(F("  s   - прочитать scratch (0x03..0x0A) + CRC"));
  Serial.println(F("  x   - прочитать scratch_ext (0x0C..0x15) + CRC"));
  Serial.println(F("  u   - записать и прочитать user-регистры 0..1"));
  Serial.println(F("  e   - E2PROM copy page (scratch -> E2PROM)"));
  Serial.println(F("  r   - soft reset + E2PROM recall"));
  Serial.println(F("--------------------------------------------"));
  Serial.println(F("Температура измеряется автоматически раз в 1 сек"));
  Serial.println();
}

void printStatus() {
  uint8_t st = 0;
  if (!mts.readStatus(st)) {
    Serial.print(F("Status read error, lastError="));
    Serial.println(mts.lastError());
    return;
  }

  Serial.print(F("STATUS=0x"));
  Serial.print(st, HEX);
  Serial.print(F("  ["));
  if (st & MTS4X_STATUS_ALERT_HIGH) Serial.print(F(" HIGH"));
  if (st & MTS4X_STATUS_ALERT_LOW)  Serial.print(F(" LOW"));
  if (st & MTS4X_STATUS_BUSY)       Serial.print(F(" BUSY"));
  if (st & MTS4X_STATUS_EEBUSY)     Serial.print(F(" EE_BUSY"));
  if (st & MTS4X_STATUS_HEATER_ON)  Serial.print(F(" HEATER"));
  if (st & MTS4X_STATUS_TL_GE_TH)   Serial.print(F(" TL>=TH"));
  Serial.println(F(" ]"));
}

void setupThresholdsAndAlert() {
  Serial.println(F("Настройка порогов TH/TL и Alert_Mode..."));

  if (!mts.setHighLimit(HIGH_LIMIT_C)) {
    Serial.println(F("setHighLimit() FAILED"));
  }
  if (!mts.setLowLimit(LOW_LIMIT_C)) {
    Serial.println(F("setLowLimit() FAILED"));
  }

  float tH = 0, tL = 0;
  if (mts.getHighLimit(tH)) {
    Serial.print(F("TH = "));
    Serial.print(tH, 3);
    Serial.println(F(" °C"));
  }
  if (mts.getLowLimit(tL)) {
    Serial.print(F("TL = "));
    Serial.print(tL, 3);
    Serial.println(F(" °C"));
  }

  // Включаем тревогу в режиме: >TH — тревога, <TL — сброс
  if (!mts.setAlertMode(true, ALERT_MODE_HIGH_TH_LOW_CLEAR)) {
    Serial.println(F("setAlertMode() FAILED"));
  } else {
    bool en = false;
    MTS4xAlertMode mode;
    if (mts.getAlertMode(en, mode)) {
      Serial.print(F("Alert enabled="));
      Serial.print(en ? F("YES") : F("NO"));
      Serial.print(F(", mode="));
      Serial.println(mode == ALERT_MODE_HIGH_TH_LOW_CLEAR ?
                     F("HIGH_TH_LOW_CLEAR") :
                     F("HIGH_TH_LOW_ALARM"));
    }
  }
}

void printIdAndRom() {
  uint16_t id = 0;
  if (mts.readDeviceId(id)) {
    Serial.print(F("Device ID = 0x"));
    Serial.println(id, HEX);
  } else {
    Serial.print(F("readDeviceId() FAILED, err="));
    Serial.println(mts.lastError());
  }

  uint8_t rom[5];
  if (mts.readRomCode(rom)) {
    Serial.print(F("ROM code: "));
    for (uint8_t i = 0; i < 5; ++i) {
      if (i) Serial.print(' ');
      Serial.print(F("0x"));
      Serial.print(rom[i], HEX);
    }
    Serial.println();
  } else {
    Serial.print(F("readRomCode() FAILED, err="));
    Serial.println(mts.lastError());
  }
}

void demoScratch() {
  uint8_t scratch[8];
  bool crcOk = false;
  if (!mts.readScratch(scratch, crcOk)) {
    Serial.print(F("readScratch() FAILED, err="));
    Serial.println(mts.lastError());
    return;
  }
  Serial.print(F("SCRATCH (0x03..0x0A), CRC "));
  Serial.println(crcOk ? F("OK") : F("FAIL"));

  for (uint8_t i = 0; i < 8; ++i) {
    Serial.print(F("  ["));
    Serial.print(0x03 + i, HEX);
    Serial.print(F("] = 0x"));
    Serial.println(scratch[i], HEX);
  }
}

void demoScratchExt() {
  uint8_t ext[10];
  bool crcOk = false;
  if (!mts.readScratchExt(ext, crcOk)) {
    Serial.print(F("readScratchExt() FAILED, err="));
    Serial.println(mts.lastError());
    return;
  }
  Serial.print(F("SCRATCH_EXT (0x0C..0x15), CRC "));
  Serial.println(crcOk ? F("OK") : F("FAIL"));

  for (uint8_t i = 0; i < 10; ++i) {
    Serial.print(F("  ["));
    Serial.print(0x0C + i, HEX);
    Serial.print(F("] = 0x"));
    Serial.println(ext[i], HEX);
  }
}

void demoUserRegisters() {
  Serial.println(F("User-регистры 0..1: запись и чтение"));

  if (!mts.writeUserRegister(0, 0x12) ||
      !mts.writeUserRegister(1, 0x34)) {
    Serial.print(F("writeUserRegister() FAILED, err="));
    Serial.println(mts.lastError());
    return;
  }

  // Можно сохранить в E2PROM current page, чтобы не выпадало после reset
  if (!mts.eepromCopyPage(true, 50)) {
    Serial.print(F("eepromCopyPage() FAILED, err="));
    Serial.println(mts.lastError());
  }

  uint8_t v0 = 0, v1 = 0;
  if (!mts.readUserRegister(0, v0) ||
      !mts.readUserRegister(1, v1)) {
    Serial.print(F("readUserRegister() FAILED, err="));
    Serial.println(mts.lastError());
    return;
  }

  Serial.print(F("User[0] = 0x"));
  Serial.println(v0, HEX);
  Serial.print(F("User[1] = 0x"));
  Serial.println(v1, HEX);
}

void toggleHeater() {
  bool on = false;
  if (!mts.isHeaterOn(on)) {
    Serial.print(F("isHeaterOn() FAILED, err="));
    Serial.println(mts.lastError());
    return;
  }
  if (on) {
    Serial.println(F("Heater OFF"));
    mts.heaterOff();
  } else {
    Serial.println(F("Heater ON"));
    mts.heaterOn();
  }
}

// ======================= SETUP ==========================

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("MTS4x Full Demo starting..."));

  if (!mts.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
    Serial.print(F("MTS4x begin FAILED, err="));
    Serial.println(mts.lastError());
    while (1) delay(1000);
  }

  // Частота шины (можно поднять до 2 МГц, если железо позволяет)
  mts.setBusClock(400000);  // 400 кГц

  if (!mts.setConfig(MTS_RATE, MTS_AVG, MTS_SLEEP)) {
    Serial.print(F("setConfig() FAILED, err="));
    Serial.println(mts.lastError());
  }

  // Останавливаем непрерывный режим, будем делать single-shot
  mts.setMode(MEASURE_STOP, false);

  printIdAndRom();
  setupThresholdsAndAlert();

  printHelp();
}

// ======================= LOOP ===========================

void loop() {
  // 1. Периодическое измерение с CRC
  unsigned long now = millis();
  if (now - lastMeasureMs >= MEASURE_PERIOD_MS) {
    lastMeasureMs = now;

    float tC = NAN;
    bool crcOk = false;

    if (mts.readTemperatureCrc(tC, crcOk, true)) {
      Serial.print(F("T = "));
      Serial.print(tC, 3);
      Serial.print(F(" °C  (CRC:"));
      Serial.print(crcOk ? F("OK") : F("FAIL"));
      Serial.print(F(")  "));
      printStatus();
    } else {
      Serial.print(F("readTemperatureCrc() FAILED, err="));
      Serial.println(mts.lastError());
    }
  }

  // 2. Обработка команд из Serial
  if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case '?':
        printHelp();
        break;

      case 'i':
        printIdAndRom();
        break;

      case 'a':
        setupThresholdsAndAlert();  // перечитаем пороги и режим
        break;

      case 't':
        toggleHeater();
        break;

      case 's':
        demoScratch();
        break;

      case 'x':
        demoScratchExt();
        break;

      case 'u':
        demoUserRegisters();
        break;

      case 'e':
        Serial.println(F("E2PROM copy page (scratch -> E2PROM)..."));
        if (!mts.eepromCopyPage(true, 50)) {
          Serial.print(F("FAILED, err="));
          Serial.println(mts.lastError());
        } else {
          Serial.println(F("OK"));
        }
        break;

      case 'r':
        Serial.println(F("Soft reset + recall E2PROM..."));
        if (!mts.softReset(true, 50)) {
          Serial.print(F("FAILED, err="));
          Serial.println(mts.lastError());
        } else {
          Serial.println(F("OK"));
        }
        break;

      default:
        // игнорируем прочее
        break;
    }
  }
}
