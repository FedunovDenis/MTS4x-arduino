#include "MTS4x.h"
#include <string.h>
#include <math.h>

// Local CRC8 (polynomial x^8 + x^5 + x^4 + 1, 0x31, init 0x00)
static uint8_t mts4x_crc8(const uint8_t *data, size_t len) {
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

// -----------------------------------------------------------------------------
// MTS4X class implementation
// -----------------------------------------------------------------------------

MTS4X::MTS4X(uint8_t address, TwoWire &wire)
: _wire(&wire),
  _addr(address),
  _lastError(MTS4X_ERR_OK),
  _busClock(400000UL) {
}

void MTS4X::setError(int8_t err) {
    _lastError = err;
}

int8_t MTS4X::lastError() const {
    return _lastError;
}

uint32_t MTS4X::busClock() const {
    return _busClock;
}

void MTS4X::setBusClock(uint32_t hz) {
    _busClock = hz;
    if (_wire) {
        _wire->setClock(hz);
    }
}

bool MTS4X::begin(int32_t sda, int32_t scl) {
#if defined(ESP8266) || defined(ESP32)
    if (!_wire->begin(sda, scl)) {
        setError(MTS4X_ERR_WIRE);
        return false;
    }
#else
    (void)sda;
    (void)scl;
    _wire->begin();
#endif
    _wire->setClock(_busClock);
    setError(MTS4X_ERR_OK);
    return true;
}

bool MTS4X::begin(int32_t sda, int32_t scl, MeasurementMode mode) {
    if (!begin(sda, scl)) {
        return false;
    }
    // Default config: 1 Hz, AVG_8, sleep after commands
    if (!setConfig(MPS_1Hz, AVG_8, true)) {
        return false;
    }
    if (!setMode(mode, false)) {
        return false;
    }
    return true;
}

// -----------------------------------------------------------------------------
// Low level I2C helpers
// -----------------------------------------------------------------------------

bool MTS4X::writeRegister(uint8_t reg, uint8_t value) {
    if (!_wire) {
        setError(MTS4X_ERR_WIRE);
        return false;
    }
    _wire->beginTransmission(_addr);
    _wire->write(reg);
    _wire->write(value);
    uint8_t res = _wire->endTransmission();
    if (res != 0) {
        setError(MTS4X_ERR_WIRE);
        return false;
    }
    setError(MTS4X_ERR_OK);
    return true;
}

bool MTS4X::writeRegisterRaw(uint8_t reg, const uint8_t *data, size_t len) {
    if (!_wire) {
        setError(MTS4X_ERR_WIRE);
        return false;
    }
    _wire->beginTransmission(_addr);
    _wire->write(reg);
    for (size_t i = 0; i < len; ++i) {
        _wire->write(data[i]);
    }
    uint8_t res = _wire->endTransmission();
    if (res != 0) {
        setError(MTS4X_ERR_WIRE);
        return false;
    }
    setError(MTS4X_ERR_OK);
    return true;
}

bool MTS4X::readRegister(uint8_t reg, uint8_t &value) {
    if (!_wire) {
        setError(MTS4X_ERR_WIRE);
        return false;
    }
    _wire->beginTransmission(_addr);
    _wire->write(reg);
    if (_wire->endTransmission(false) != 0) {
        setError(MTS4X_ERR_WIRE);
        return false;
    }
    if (_wire->requestFrom((int)_addr, (int)1) != 1) {
        setError(MTS4X_ERR_WIRE);
        return false;
    }
    value = (uint8_t)_wire->read();
    setError(MTS4X_ERR_OK);
    return true;
}

bool MTS4X::readRegisterRaw(uint8_t startReg, uint8_t *data, size_t len) {
    if (!_wire || !data || !len) {
        setError(MTS4X_ERR_PARAM);
        return false;
    }
    _wire->beginTransmission(_addr);
    _wire->write(startReg);
    if (_wire->endTransmission(false) != 0) {
        setError(MTS4X_ERR_WIRE);
        return false;
    }

    size_t toRead = len;
    size_t offset = 0;
    while (toRead > 0) {
        uint8_t chunk = (toRead > 32) ? 32 : (uint8_t)toRead; // Wire buffer safe
        uint8_t got   = _wire->requestFrom((int)_addr, (int)chunk);
        if (got != chunk) {
            setError(MTS4X_ERR_WIRE);
            return false;
        }
        for (uint8_t i = 0; i < chunk; ++i) {
            data[offset + i] = (uint8_t)_wire->read();
        }
        offset += chunk;
        toRead -= chunk;
    }
    setError(MTS4X_ERR_OK);
    return true;
}

// -----------------------------------------------------------------------------
// Status / busy helpers
// -----------------------------------------------------------------------------

bool MTS4X::readStatus(uint8_t &status) {
    return readRegister(MTS4X_STATUS, status);
}

bool MTS4X::isBusy(bool &busy) {
    uint8_t st = 0;
    if (!readStatus(st)) {
        return false;
    }
    busy = (st & MTS4X_STATUS_BUSY) != 0;
    return true;
}

bool MTS4X::isBusy() {
    bool b = false;
    if (!isBusy(b)) return false;
    return b;
}

bool MTS4X::inProgress() {
    bool b = false;
    if (!isBusy(b)) {
        return false;
    }
    return b;
}

// -----------------------------------------------------------------------------
// Measurement mode / configuration
// -----------------------------------------------------------------------------

bool MTS4X::setMode(MeasurementMode mode, bool heater) {
    uint8_t cmd = 0;

    switch (mode) {
        case MEASURE_CONTINUOUS:
            cmd = (0b00 << 6);
            break;
        case MEASURE_STOP:
            cmd = (0b01 << 6);
            break;
        case MEASURE_CONTINUOUS_READBACK:
            cmd = (0b10 << 6);
            break;
        case MEASURE_SINGLE:
        default:
            cmd = (0b11 << 6);
            break;
    }

    if (heater) {
        // Heater enable code 0b1010 (bits 3..0)
        cmd |= 0x0A;
    }

    return writeRegister(MTS4X_TEMP_CMD, cmd);
}

bool MTS4X::startSingleMessurement() {
    // Single conversion, heater off
    return setMode(MEASURE_SINGLE, false);
}

bool MTS4X::setConfig(TempCfgMPS mps, TempCfgAVG avg, bool sleep) {
    // Temp_Cfg[7:5] = MPS, [4:3] = AVG, [0] = Sleep_en
    uint8_t cfg = 0;
    cfg |= ((uint8_t)mps & 0xE0);
    cfg |= ((uint8_t)avg & 0x18);
    if (sleep) cfg |= 0x01; // Sleep_en = 1
    return writeRegister(MTS4X_TEMP_CFG, cfg);
}

// -----------------------------------------------------------------------------
// Temperature reading helpers
// -----------------------------------------------------------------------------

bool MTS4X::readTemperatureRawWithCrc(int16_t &raw, bool &crcOk,
                                      bool waitOnNewVal) {
    raw   = 0;
    crcOk = false;

    if (waitOnNewVal) {
        // Wait until conversion is complete (Status bit5 == 0)
        unsigned long start = millis();
        while (true) {
            uint8_t st = 0;
            if (!readStatus(st)) {
                return false;
            }
            if ((st & MTS4X_STATUS_BUSY) == 0) {
                break;
            }
            if (millis() - start > 50UL) {
                setError(MTS4X_ERR_TIMEOUT);
                return false;
            }
            delay(1);
        }
    }

    uint8_t buf[3];
    if (!readRegisterRaw(MTS4X_TEMP_LSB, buf, 3)) {
        return false;
    }

    uint8_t calc = mts4x_crc8(buf, 2);
    crcOk = (calc == buf[2]);

    raw = (int16_t)(((uint16_t)buf[1] << 8) | (uint16_t)buf[0]);
    setError(crcOk ? MTS4X_ERR_OK : MTS4X_ERR_CRC);
    return true;
}

bool MTS4X::readTemperatureRaw(int16_t &raw, bool waitOnNewVal) {
    bool dummyCrc = false;
    return readTemperatureRawWithCrc(raw, dummyCrc, waitOnNewVal);
}

bool MTS4X::readTemperatureCrc(float &tC, bool &crcOk,
                               bool waitOnNewVal) {
    int16_t raw = 0;
    if (!readTemperatureRawWithCrc(raw, crcOk, waitOnNewVal)) {
        tC = NAN;
        return false;
    }
    tC = MTS4X_RAW_TO_CELSIUS(raw);
    return true;
}

bool MTS4X::readTemperature(float &tC, bool waitOnNewVal) {
    bool crcOk = false;
    return readTemperatureCrc(tC, crcOk, waitOnNewVal);
}

float MTS4X::readTemperature(bool waitOnNewVal) {
    float t = NAN;
    (void)readTemperature(t, waitOnNewVal);
    return t;
}

float MTS4X::readTemperatureC(bool waitOnNewVal) {
    return readTemperature(waitOnNewVal);
}

bool MTS4X::singleShot(float &tC) {
    if (!startSingleMessurement()) {
        tC = NAN;
        return false;
    }
    bool crcOk = false;
    if (!readTemperatureCrc(tC, crcOk, true)) {
        return false;
    }
    if (!crcOk) {
        setError(MTS4X_ERR_CRC);
        return false;
    }
    return true;
}

// -----------------------------------------------------------------------------
// Heater control
// -----------------------------------------------------------------------------

bool MTS4X::heaterOn() {
    // Keep current mode, just enable heater bits
    uint8_t cmd = 0;
    if (!readRegister(MTS4X_TEMP_CMD, cmd)) {
        return false;
    }
    cmd &= 0xF0;       // clear heater bits
    cmd |= 0x0A;       // heater on
    return writeRegister(MTS4X_TEMP_CMD, cmd);
}

bool MTS4X::heaterOff() {
    uint8_t cmd = 0;
    if (!readRegister(MTS4X_TEMP_CMD, cmd)) {
        return false;
    }
    cmd &= 0xF0;       // clear heater bits -> heater off
    return writeRegister(MTS4X_TEMP_CMD, cmd);
}

bool MTS4X::isHeaterOn(bool &on) {
    uint8_t st = 0;
    if (!readStatus(st)) {
        return false;
    }
    on = (st & MTS4X_STATUS_HEATER_ON) != 0;
    return true;
}

// -----------------------------------------------------------------------------
// Alert configuration and limits
// -----------------------------------------------------------------------------

bool MTS4X::setAlertMode(bool enable, MTS4xAlertMode mode) {
    uint8_t reg = 0;
    // bit7 = Alert_en (0=enable, 1=disable)
    // bit6 = mode
    if (!enable) {
        reg |= 0x80; // disable alerts
    }
    if (mode == ALERT_MODE_HIGH_TH_LOW_ALARM) {
        reg |= 0x40;
    }
    return writeRegister(MTS4X_ALERT_MODE, reg);
}

bool MTS4X::getAlertMode(bool &enable, MTS4xAlertMode &mode) {
    uint8_t reg = 0;
    if (!readRegister(MTS4X_ALERT_MODE, reg)) {
        return false;
    }
    enable = ((reg & 0x80) == 0); // 0 = enabled, 1 = disabled
    mode   = (reg & 0x40) ? ALERT_MODE_HIGH_TH_LOW_ALARM
                          : ALERT_MODE_HIGH_TH_LOW_CLEAR;
    return true;
}

bool MTS4X::readAlertRegister(uint8_t &regValue) {
    return readRegister(MTS4X_ALERT_MODE, regValue);
}

// Helpers for limit conversion
static int16_t mts4x_c_to_raw(float tC) {
    float rawF = (tC - 25.0f) * 256.0f;
    if (rawF > 32767.0f) rawF = 32767.0f;
    if (rawF < -32768.0f) rawF = -32768.0f;
    return (int16_t)lroundf(rawF);
}

static float mts4x_raw_to_c(int16_t raw) {
    return MTS4X_RAW_TO_CELSIUS(raw);
}

bool MTS4X::setHighLimit(float tHighC) {
    int16_t raw = mts4x_c_to_raw(tHighC);
    uint8_t buf[2];
    buf[0] = (uint8_t)(raw & 0xFF);
    buf[1] = (uint8_t)((raw >> 8) & 0xFF);
    return writeRegisterRaw(MTS4X_TH_LSB, buf, 2);
}

bool MTS4X::setLowLimit(float tLowC) {
    int16_t raw = mts4x_c_to_raw(tLowC);
    uint8_t buf[2];
    buf[0] = (uint8_t)(raw & 0xFF);
    buf[1] = (uint8_t)((raw >> 8) & 0xFF);
    return writeRegisterRaw(MTS4X_TL_LSB, buf, 2);
}

bool MTS4X::getHighLimit(float &tHighC) {
    uint8_t buf[2];
    if (!readRegisterRaw(MTS4X_TH_LSB, buf, 2)) {
        return false;
    }
    int16_t raw = (int16_t)(((uint16_t)buf[1] << 8) | (uint16_t)buf[0]);
    tHighC = mts4x_raw_to_c(raw);
    return true;
}

bool MTS4X::getLowLimit(float &tLowC) {
    uint8_t buf[2];
    if (!readRegisterRaw(MTS4X_TL_LSB, buf, 2)) {
        return false;
    }
    int16_t raw = (int16_t)(((uint16_t)buf[1] << 8) | (uint16_t)buf[0]);
    tLowC = mts4x_raw_to_c(raw);
    return true;
}

// -----------------------------------------------------------------------------
// ID and ROM code
// -----------------------------------------------------------------------------

bool MTS4X::readDeviceId(uint16_t &id) {
    uint8_t buf[2];
    if (!readRegisterRaw(MTS4X_DEVICE_ID_LSB, buf, 2)) {
        return false;
    }
    id = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    return true;
}

bool MTS4X::readRomCode(uint8_t rom[5]) {
    if (!rom) {
        setError(MTS4X_ERR_PARAM);
        return false;
    }
    return readRegisterRaw(MTS4X_DEVICE_ID_LSB, rom, 5);
}

// -----------------------------------------------------------------------------
// User registers
// -----------------------------------------------------------------------------

bool MTS4X::readUserRegister(uint8_t index, uint8_t &value) {
    if (index > 9) {
        setError(MTS4X_ERR_PARAM);
        return false;
    }
    uint8_t reg = (uint8_t)(MTS4X_USER_DEFINE_0 + index);
    return readRegister(reg, value);
}

bool MTS4X::writeUserRegister(uint8_t index, uint8_t value) {
    if (index > 9) {
        setError(MTS4X_ERR_PARAM);
        return false;
    }
    uint8_t reg = (uint8_t)(MTS4X_USER_DEFINE_0 + index);
    return writeRegister(reg, value);
}

// -----------------------------------------------------------------------------
// Scratch / extended scratch with CRC
// -----------------------------------------------------------------------------

bool MTS4X::readScratch(uint8_t scratch[8], bool &crcOk) {
    if (!scratch) {
        setError(MTS4X_ERR_PARAM);
        return false;
    }
    uint8_t buf[9];
    if (!readRegisterRaw(MTS4X_STATUS, buf, 9)) {
        return false;
    }
    memcpy(scratch, buf, 8);
    uint8_t calc = mts4x_crc8(buf, 8);
    crcOk = (calc == buf[8]);
    setError(crcOk ? MTS4X_ERR_OK : MTS4X_ERR_CRC);
    return true;
}

bool MTS4X::readScratchExt(uint8_t scratchExt[10], bool &crcOk) {
    if (!scratchExt) {
        setError(MTS4X_ERR_PARAM);
        return false;
    }
    uint8_t buf[11];
    if (!readRegisterRaw(MTS4X_USER_DEFINE_0, buf, 11)) {
        return false;
    }
    memcpy(scratchExt, buf, 10);
    uint8_t calc = mts4x_crc8(buf, 10);
    crcOk = (calc == buf[10]);
    setError(crcOk ? MTS4X_ERR_OK : MTS4X_ERR_CRC);
    return true;
}

// -----------------------------------------------------------------------------
// EEPROM operations and reset
// -----------------------------------------------------------------------------

bool MTS4X::waitEepromReady(uint32_t timeoutMs) {
    unsigned long start = millis();
    while (true) {
        uint8_t st = 0;
        if (!readStatus(st)) {
            return false;
        }
        if ((st & MTS4X_STATUS_EE_BUSY) == 0) {
            return true;
        }
        if (millis() - start > timeoutMs) {
            setError(MTS4X_ERR_TIMEOUT);
            return false;
        }
        delay(1);
    }
}

bool MTS4X::eepromCopyPage(bool waitReady, uint32_t timeoutMs) {
    // 0x08: copy scratch -> EEPROM
    if (!writeRegister(MTS4X_E2PROM_CMD, 0x08)) {
        return false;
    }
    if (waitReady) {
        return waitEepromReady(timeoutMs);
    }
    return true;
}

bool MTS4X::eepromRecallPage(bool waitReady, uint32_t timeoutMs) {
    // 0xB6: recall EEPROM -> scratch
    if (!writeRegister(MTS4X_E2PROM_CMD, 0xB6)) {
        return false;
    }
    if (waitReady) {
        return waitEepromReady(timeoutMs);
    }
    return true;
}

bool MTS4X::eepromRecallAll(bool waitReady, uint32_t timeoutMs) {
    // For this device, recall-all is identical to a soft reset + recall
    return softReset(waitReady, timeoutMs);
}

bool MTS4X::eepromWritePageRaw(bool waitReady, uint32_t timeoutMs) {
    // Same command as copy page (scratch already holds data)
    return eepromCopyPage(waitReady, timeoutMs);
}

bool MTS4X::softReset(bool waitReady, uint32_t timeoutMs) {
    // 0x6A: soft reset + recall EEPROM to scratch
    if (!writeRegister(MTS4X_E2PROM_CMD, 0x6A)) {
        return false;
    }
    if (waitReady) {
        return waitEepromReady(timeoutMs);
    }
    return true;
}

// -----------------------------------------------------------------------------
// Parasitic power configuration
// -----------------------------------------------------------------------------

bool MTS4X::setParasiticPower(bool enable) {
    // PPM_Cfg (0x63): bits 3:0 = 0b1010 to enable, others disable
    uint8_t val = enable ? 0x0A : 0x00;
    return writeRegister(MTS4X_PPM_CFG, val);
}
