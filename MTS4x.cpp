#include <Arduino.h>
#include <Wire.h>
#include "MTS4x.h"

MTS4X::MTS4X() {
}

// Инициализация только шины I2C
bool MTS4X::begin(int32_t sda, int32_t scl) {
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
  if (!Wire.begin((int)sda, (int)scl)) {
    return false;
  }
#else
  // ESP8266 (и прочие) – begin() ничего не возвращает
  Wire.begin((int)sda, (int)scl);
#endif
  return true;
}

// Инициализация I2C + установка режима измерения
bool MTS4X::begin(int32_t sda, int32_t scl, MeasurementMode mode) {
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
  if (!Wire.begin((int)sda, (int)scl)) {
    return false;
  }
#else
  Wire.begin((int)sda, (int)scl);
#endif

  // Быстрая шина (до 400 кГц)
  Wire.setClock(400000);

  setMode(mode, false);
  return true;
}

// Старт одиночного измерения
bool MTS4X::startSingleMessurement() {
  return setMode(MEASURE_SINGLE, false);
}

// Установка режима измерения и нагревателя
bool MTS4X::setMode(MeasurementMode mode, bool heater) {
  uint8_t command = 0;

  // Биты 7:6 – режим измерения
  command |= (static_cast<uint8_t>(mode) & 0x03) << 6;
  // Биты 5:4 – зарезервированы (0)

  if (heater) {
    // Биты 3:0 – включение нагревателя (0b1010 по оригинальному коду)
    command |= 0x0A;
  }

  Wire.beginTransmission(MTS4X_ADDRESS);
  Wire.write(MTS4X_TEMP_CMD);
  Wire.write(command);
  return Wire.endTransmission() == 0;
}

// Настройка частоты измерения, усреднения и sleep
bool MTS4X::setConfig(TempCfgMPS mps, TempCfgAVG avg, bool sleep) {
  uint8_t command = 0;

  // MPS уже сдвинут в биты 7:5
  command |= static_cast<uint8_t>(mps);
  // AVG уже сдвинут в биты 4:3
  command |= static_cast<uint8_t>(avg);
  // Bit 0 – sleep
  if (sleep) {
    command |= 0x01;
  }

  Wire.beginTransmission(MTS4X_ADDRESS);
  Wire.write(MTS4X_TEMP_CFG);
  Wire.write(command);
  return Wire.endTransmission() == 0;
}

// Чтение температуры
float MTS4X::readTemperature(bool waitOnNewVal) {
  if (waitOnNewVal) {
    // В оригинале был опрос статуса, часто глючный,
    // поэтому оставляем фиксированную небольшую задержку.
    delay(20);  // при 8 Гц более чем достаточно
  }

  // Указываем адрес регистра температуры
  Wire.beginTransmission(MTS4X_ADDRESS);
  Wire.write(MTS4X_TEMP_LSB);
  Wire.endTransmission();

  // Читаем два байта (LSB + MSB)
  if (Wire.requestFrom(MTS4X_ADDRESS, static_cast<uint8_t>(2)) != 2) {
    return -55.0f;  // ошибка чтения
  }

  if (Wire.available() < 2) {
    return -55.0f;
  }

  uint8_t lsb = Wire.read();
  uint8_t msb = Wire.read();

  int16_t rawTemp = (static_cast<int16_t>(msb) << 8) | lsb;

  return MTS4X_RAW_TO_CELSIUS(rawTemp);
}

// Проверка, идёт ли ещё измерение (по статусу)
bool MTS4X::inProgress() {
  Wire.beginTransmission(MTS4X_ADDRESS);
  Wire.write(MTS4X_STATUS);
  Wire.endTransmission();

  if (Wire.requestFrom(MTS4X_ADDRESS, static_cast<uint8_t>(1)) != 1) {
    Serial.println(F("MTS4X: status read fail"));
    return true;  // считаем, что всё ещё "занят"
  }

  if (!Wire.available()) {
    return true;
  }

  uint8_t status = Wire.read();
  // Бит 5 (0x20) используется как признак "busy"
  return (status & 0x20) != 0;
}
