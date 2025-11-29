#include "MTS4x.h"

// ---------- КОНСТРУКТОР ----------

MTS4X::MTS4X(uint8_t address, TwoWire &wire)
  : _address(address),
    _wire(&wire),
    _i2cClock(400000UL),
    _lastError(MTS4X_OK),
    _cfgAvg(AVG_8),
    _cfgMps(MPS_1Hz),
    _cfgSleep(true),
    _lastMode(MEASURE_STOP)
{
}

// ---------- BEGIN ----------

bool MTS4X::begin(int32_t sda, int32_t scl) {
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
  if (!_wire->begin((int)sda, (int)scl)) {
    _lastError = MTS4X_ERR_I2C_BEGIN;
    return false;
  }
#else
  _wire->begin((int)sda, (int)scl);
#endif

  _wire->setClock(_i2cClock);
  _lastError = MTS4X_OK;
  return true;
}

bool MTS4X::begin(int32_t sda, int32_t scl, MeasurementMode mode) {
  if (!begin(sda, scl)) {
    return false;
  }
  return setMode(mode, false);
}

// ---------- MODE / CONFIG ----------

bool MTS4X::setMode(MeasurementMode mode, bool heater) {
  uint8_t command = 0;

  command |= (static_cast<uint8_t>(mode) & 0x03) << 6;

  if (heater) {
    command |= 0x0A;  // heater enable (low nibble)
  }

  _lastMode = mode;
  return writeRegister(MTS4X_TEMP_CMD, command);
}

bool MTS4X::startSingleMeasurement() {
  return setMode(MEASURE_SINGLE, false);
}

bool MTS4X::startSingleMessurement() {
  return startSingleMeasurement();
}

bool MTS4X::setConfig(TempCfgMPS mps, TempCfgAVG avg, bool sleep) {
  uint8_t command = 0;

  command |= static_cast<uint8_t>(mps);
  command |= static_cast<uint8_t>(avg);
  if (sleep) {
    command |= 0x01;
  }

  _cfgMps   = mps;
  _cfgAvg   = avg;
  _cfgSleep = sleep;

  return writeRegister(MTS4X_TEMP_CFG, command);
}

// ---------- ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ----------

uint16_t MTS4X::convTimeMsFromCfg() const {
  switch (_cfgAvg) {
    case AVG_1:   return 4;
    case AVG_8:   return 7;
    case AVG_16:  return 10;
    case AVG_32:
    default:      return 18;
  }
}

uint8_t MTS4X::crc8(const uint8_t *data, size_t len) {
  uint8_t crc = 0x00;
  while (len--) {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; ++i) {
      if (crc & 0x80) {
        crc = (uint8_t)((crc << 1) ^ 0x31);
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

// ---------- ТЕМПЕРАТУРА ----------

float MTS4X::readTemperature(bool waitOnNewVal) {
  float tC;
  if (!readTemperature(tC, waitOnNewVal)) {
    return -55.0f;
  }
  return tC;
}

bool MTS4X::readTemperature(float &tC, bool waitOnNewVal) {
  int16_t raw;
  if (!readTemperatureRaw(raw, waitOnNewVal)) {
    return false;
  }
  tC = MTS4X_RAW_TO_CELSIUS(raw);
  return true;
}

bool MTS4X::readTemperatureRaw(int16_t &raw, bool waitOnNewVal) {
  if (waitOnNewVal) {
    delay(convTimeMsFromCfg());
  }

  uint8_t buf[2];
  if (!readRegisters(MTS4X_TEMP_LSB, buf, 2)) {
    return false;
  }

  uint8_t lsb = buf[0];
  uint8_t msb = buf[1];

  raw = (int16_t)(((int16_t)msb << 8) | lsb);
  return true;
}

bool MTS4X::readTemperatureRawWithCrc(int16_t &raw, bool &crcOk,
                                      bool waitOnNewVal) {
  if (waitOnNewVal) {
    delay(convTimeMsFromCfg());
  }

  uint8_t buf[3];
  if (!readRegisters(MTS4X_TEMP_LSB, buf, 3)) {
    crcOk = false;
    return false;
  }

  uint8_t lsb = buf[0];
  uint8_t msb = buf[1];
  uint8_t crcReg = buf[2];

  raw = (int16_t)(((int16_t)msb << 8) | lsb);

  uint8_t crcCalc = crc8(buf, 2);
  crcOk = (crcCalc == crcReg);
  return true;
}

bool MTS4X::readTemperatureCrc(float &tC, bool &crcOk,
                               bool waitOnNewVal) {
  int16_t raw;
  if (!readTemperatureRawWithCrc(raw, crcOk, waitOnNewVal)) {
    return false;
  }
  tC = MTS4X_RAW_TO_CELSIUS(raw);
  return true;
}

bool MTS4X::singleShot(float &tC) {
  if (!startSingleMeasurement()) {
    return false;
  }
  return readTemperature(tC, true);
}

// ---------- STATUS / BUSY ----------

bool MTS4X::readStatus(uint8_t &status) {
  return readRegister(MTS4X_STATUS, status);
}

bool MTS4X::isBusy(bool &busy) {
  uint8_t status = 0;
  if (!readStatus(status)) {
    return false;
  }
  busy = (status & MTS4X_STATUS_BUSY) != 0;
  return true;
}

bool MTS4X::isBusy() {
  bool b = true;
  if (!isBusy(b)) {
    return true;
  }
  return b;
}

bool MTS4X::inProgress() {
  bool busy = true;
  if (!isBusy(busy)) {
    return true;
  }
  return busy;
}

// ---------- HEATER ----------

bool MTS4X::heaterOn() {
  uint8_t cmd = 0;
  if (!readRegister(MTS4X_TEMP_CMD, cmd)) return false;

  cmd &= 0xF0;
  cmd |= 0x0A;
  return writeRegister(MTS4X_TEMP_CMD, cmd);
}

bool MTS4X::heaterOff() {
  uint8_t cmd = 0;
  if (!readRegister(MTS4X_TEMP_CMD, cmd)) return false;

  cmd &= 0xF0;
  return writeRegister(MTS4X_TEMP_CMD, cmd);
}

bool MTS4X::isHeaterOn(bool &on) {
  uint8_t status = 0;
  if (!readStatus(status)) {
    return false;
  }
  on = (status & MTS4X_STATUS_HEATER_ON) != 0;
  return true;
}

// ---------- ALERT / ПОРОГИ ----------

bool MTS4X::readAlertRegister(uint8_t &regValue) {
  return readRegister(MTS4X_ALERT_MODE, regValue);
}

bool MTS4X::setAlertMode(bool enable, MTS4xAlertMode mode) {
  uint8_t v = 0;
  if (!readAlertRegister(v)) return false;

  v &= ~(uint8_t)(MTS4X_ALERT_EN_MASK | MTS4X_ALERT_IM_MASK);

  if (enable) {
    v |= MTS4X_ALERT_EN_MASK;
    if (mode == ALERT_MODE_HIGH_TH_LOW_ALARM) {
      v |= MTS4X_ALERT_IM_MASK;
    }
  }

  return writeRegister(MTS4X_ALERT_MODE, v);
}

bool MTS4X::getAlertMode(bool &enable, MTS4xAlertMode &mode) {
  uint8_t v = 0;
  if (!readAlertRegister(v)) return false;

  enable = (v & MTS4X_ALERT_EN_MASK) != 0;
  mode   = (v & MTS4X_ALERT_IM_MASK) ?
            ALERT_MODE_HIGH_TH_LOW_ALARM :
            ALERT_MODE_HIGH_TH_LOW_CLEAR;
  return true;
}

bool MTS4X::setHighLimit(float tHighC) {
  int16_t raw = MTS4X_CELSIUS_TO_RAW(tHighC);
  uint8_t lsb = (uint8_t)(raw & 0xFF);
  uint8_t msb = (uint8_t)((raw >> 8) & 0xFF);

  return writeRegister(MTS4X_TH_LSB, lsb) &&
         writeRegister(MTS4X_TH_MSB, msb);
}

bool MTS4X::setLowLimit(float tLowC) {
  int16_t raw = MTS4X_CELSIUS_TO_RAW(tLowC);
  uint8_t lsb = (uint8_t)(raw & 0xFF);
  uint8_t msb = (uint8_t)((raw >> 8) & 0xFF);

  return writeRegister(MTS4X_TL_LSB, lsb) &&
         writeRegister(MTS4X_TL_MSB, msb);
}

bool MTS4X::getHighLimit(float &tHighC) {
  uint8_t lsb = 0, msb = 0;
  if (!readRegister(MTS4X_TH_LSB, lsb) ||
      !readRegister(MTS4X_TH_MSB, msb)) {
    return false;
  }
  int16_t raw = (int16_t)(((int16_t)msb << 8) | lsb);
  tHighC = MTS4X_RAW_TO_CELSIUS(raw);
  return true;
}

bool MTS4X::getLowLimit(float &tLowC) {
  uint8_t lsb = 0, msb = 0;
  if (!readRegister(MTS4X_TL_LSB, lsb) ||
      !readRegister(MTS4X_TL_MSB, msb)) {
    return false;
  }
  int16_t raw = (int16_t)(((int16_t)msb << 8) | lsb);
  tLowC = MTS4X_RAW_TO_CELSIUS(raw);
  return true;
}

// ---------- ID / ROM ----------

bool MTS4X::readDeviceId(uint16_t &id) {
  uint8_t lsb = 0, msb = 0;
  if (!readRegister(MTS4X_DEVICE_ID_LSB, lsb) ||
      !readRegister(MTS4X_DEVICE_ID_MSB, msb)) {
    return false;
  }
  id = (uint16_t)(((uint16_t)msb << 8) | lsb);
  return true;
}

bool MTS4X::readRomCode(uint8_t rom[5]) {
  if (!rom) {
    _lastError = MTS4X_ERR_BAD_ARG;
    return false;
  }
  return readRegisters(MTS4X_ROMCODE3, rom, 5);
}

// ---------- USER REGISTERS ----------

bool MTS4X::readUserRegister(uint8_t index, uint8_t &value) {
  if (index > 9) {
    _lastError = MTS4X_ERR_BAD_ARG;
    return false;
  }
  uint8_t reg = (uint8_t)(MTS4X_USER_DEFINE_0 + index);
  return readRegister(reg, value);
}

bool MTS4X::writeUserRegister(uint8_t index, uint8_t value) {
  if (index > 9) {
    _lastError = MTS4X_ERR_BAD_ARG;
    return false;
  }
  uint8_t reg = (uint8_t)(MTS4X_USER_DEFINE_0 + index);
  return writeRegister(reg, value);
}

// ---------- SCRATCH + CRC ----------

bool MTS4X::readScratch(uint8_t scratch[8], bool &crcOk) {
  uint8_t buf[9];
  if (!readRegisters(MTS4X_STATUS, buf, 9)) {
    crcOk = false;
    return false;
  }
  uint8_t crcCalc = crc8(buf, 8);
  crcOk = (crcCalc == buf[8]);

  if (scratch) {
    for (uint8_t i = 0; i < 8; ++i) {
      scratch[i] = buf[i];
    }
  }
  return true;
}

bool MTS4X::readScratchExt(uint8_t scratchExt[10], bool &crcOk) {
  uint8_t buf[11];
  if (!readRegisters(MTS4X_USER_DEFINE_0, buf, 11)) {
    crcOk = false;
    return false;
  }
  uint8_t crcCalc = crc8(buf, 10);
  crcOk = (crcCalc == buf[10]);

  if (scratchExt) {
    for (uint8_t i = 0; i < 10; ++i) {
      scratchExt[i] = buf[i];
    }
  }
  return true;
}

// ---------- E2PROM / SOFT RESET ----------

bool MTS4X::waitEepromReady(uint32_t timeoutMs) {
  unsigned long start = millis();
  while (true) {
    uint8_t status = 0;
    if (!readStatus(status)) {
      _lastError = MTS4X_ERR_I2C_RX;
      return false;
    }
    if ((status & MTS4X_STATUS_EEBUSY) == 0) {
      _lastError = MTS4X_OK;
      return true;
    }
    if ((millis() - start) >= timeoutMs) {
      _lastError = MTS4X_ERR_TIMEOUT;
      return false;
    }
    delay(1);
  }
}

bool MTS4X::eepromCommand(uint8_t cmd, bool waitReady, uint32_t timeoutMs) {
  if (!writeRegister(MTS4X_E2PROM_CMD, cmd)) {
    return false;
  }
  if (!waitReady) {
    return true;
  }
  return waitEepromReady(timeoutMs);
}

bool MTS4X::eepromCopyPage(bool waitReady, uint32_t timeoutMs) {
  return eepromCommand(MTS4X_EECMD_COPY_PAGE, waitReady, timeoutMs);
}

bool MTS4X::eepromRecallPage(bool waitReady, uint32_t timeoutMs) {
  return eepromCommand(MTS4X_EECMD_LOAD_PAGE, waitReady, timeoutMs);
}

bool MTS4X::eepromRecallAll(bool waitReady, uint32_t timeoutMs) {
  return eepromCommand(MTS4X_EECMD_RECALL_EE, waitReady, timeoutMs);
}

bool MTS4X::eepromWritePageRaw(bool waitReady, uint32_t timeoutMs) {
  return eepromCommand(MTS4X_EECMD_WRITE_PAGE, waitReady, timeoutMs);
}

bool MTS4X::softReset(bool waitReady, uint32_t timeoutMs) {
  return eepromCommand(MTS4X_EECMD_SOFT_RESET, waitReady, timeoutMs);
}

// ---------- ПАРАЗИТНОЕ ПИТАНИЕ ----------

bool MTS4X::setParasiticPower(bool enable) {
  uint8_t val = 0;
  if (!readRegister(MTS4X_PPM_CFG, val)) return false;

  val &= 0xF0;
  if (enable) {
    val |= 0x0A;
  }
  return writeRegister(MTS4X_PPM_CFG, val);
}

// ---------- I2C CLOCK ----------

void MTS4X::setBusClock(uint32_t hz) {
  _i2cClock = hz;
  if (_wire) {
    _wire->setClock(_i2cClock);
  }
}

// ---------- НИЗКОУРОВНЕВЫЙ I2C ----------

bool MTS4X::writeRegister(uint8_t reg, uint8_t value) {
  _wire->beginTransmission(_address);
  _wire->write(reg);
  _wire->write(value);
  uint8_t rc = _wire->endTransmission();
  if (rc != 0) {
    _lastError = MTS4X_ERR_I2C_TX;
    return false;
  }
  _lastError = MTS4X_OK;
  return true;
}

bool MTS4X::readRegister(uint8_t reg, uint8_t &value) {
  _wire->beginTransmission(_address);
  _wire->write(reg);
  uint8_t rc = _wire->endTransmission();
  if (rc != 0) {
    _lastError = MTS4X_ERR_I2C_TX;
    return false;
  }

  uint8_t req = _wire->requestFrom((uint8_t)_address, (uint8_t)1);
  if (req != 1) {
    _lastError = MTS4X_ERR_I2C_RX;
    return false;
  }

  if (!_wire->available()) {
    _lastError = MTS4X_ERR_NO_DATA;
    return false;
  }

  value = _wire->read();
  _lastError = MTS4X_OK;
  return true;
}

bool MTS4X::readRegisters(uint8_t startReg, uint8_t *buf, size_t len) {
  if (!buf || len == 0) {
    _lastError = MTS4X_ERR_BAD_ARG;
    return false;
  }

  _wire->beginTransmission(_address);
  _wire->write(startReg);
  uint8_t rc = _wire->endTransmission();
  if (rc != 0) {
    _lastError = MTS4X_ERR_I2C_TX;
    return false;
  }

  uint8_t req = _wire->requestFrom((uint8_t)_address, (uint8_t)len);
  if (req != len) {
    _lastError = MTS4X_ERR_I2C_RX;
    return false;
  }

  for (size_t i = 0; i < len; ++i) {
    if (!_wire->available()) {
      _lastError = MTS4X_ERR_NO_DATA;
      return false;
    }
    buf[i] = _wire->read();
  }

  _lastError = MTS4X_OK;
  return true;
}
