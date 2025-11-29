// MTS4x Arduino driver
// Author: Denis (FedunovDenis)
// Version: 2.0.1

#ifndef __MTS4X_H__
#define __MTS4X_H__

#include <Arduino.h>
#include <Wire.h>

// I2C address (fixed for most MTS4/MTS4P+T4 modules)
#define MTS4X_ADDRESS 0x41

// Temperature registers and conversion
#define MTS4X_TEMP_LSB         0x00
#define MTS4X_TEMP_MSB         0x01
// raw (signed 16-bit) -> °C
#define MTS4X_RAW_TO_CELSIUS(S_T) (((S_T) / 256.0f) + 25.0f)

// Scratch / configuration / ID registers
#define MTS4X_CRC_TEMP         0x02
#define MTS4X_STATUS           0x03
#define MTS4X_TEMP_CMD         0x04
#define MTS4X_TEMP_CFG         0x05
#define MTS4X_ALERT_MODE       0x06
#define MTS4X_TH_LSB           0x07
#define MTS4X_TH_MSB           0x08
#define MTS4X_TL_LSB           0x09
#define MTS4X_TL_MSB           0x0A
#define MTS4X_CRC_SCRATCH      0x0B

// User registers (mapped to EEPROM user area)
#define MTS4X_USER_DEFINE_0    0x0C
#define MTS4X_USER_DEFINE_1    0x0D
#define MTS4X_USER_DEFINE_2    0x0E
#define MTS4X_USER_DEFINE_3    0x0F
#define MTS4X_USER_DEFINE_4    0x10
#define MTS4X_USER_DEFINE_5    0x11
#define MTS4X_USER_DEFINE_6    0x12
#define MTS4X_USER_DEFINE_7    0x13
#define MTS4X_USER_DEFINE_8    0x14
#define MTS4X_USER_DEFINE_9    0x15

#define MTS4X_CRC_SCRATCH_EXT  0x16
#define MTS4X_E2PROM_CMD       0x17
#define MTS4X_DEVICE_ID_LSB    0x18
#define MTS4X_DEVICE_ID_MSB    0x19
#define MTS4X_ROMCODE_3        0x1A
#define MTS4X_ROMCODE_4        0x1B
#define MTS4X_ROMCODE_5        0x1C
#define MTS4X_ROMCODE_6        0x1D
#define MTS4X_ROMCODE_7        0x1E
#define MTS4X_CRC_ROMCODE      0x1F

// Parasitic power configuration register
#define MTS4X_PPM_CFG          0x63

// Status bits (Status register 0x03)
#define MTS4X_STATUS_ALERT_HIGH 0x80
#define MTS4X_STATUS_ALERT_LOW  0x40
#define MTS4X_STATUS_BUSY       0x20
#define MTS4X_STATUS_EE_BUSY    0x10
#define MTS4X_STATUS_HEATER_ON  0x08
#define MTS4X_STATUS_TH_TL_ERR  0x04

// Simple error codes for lastError()
#define MTS4X_ERR_OK         0
#define MTS4X_ERR_WIRE      -1
#define MTS4X_ERR_TIMEOUT   -2
#define MTS4X_ERR_PARAM     -3
#define MTS4X_ERR_CRC       -4

// Measurement mode (Temp_Cmd[7:6])
typedef enum {
    MEASURE_CONTINUOUS          = 0, // 00: continuous
    MEASURE_STOP                = 1, // 01: stop (power-down if Sleep_en=1)
    MEASURE_CONTINUOUS_READBACK = 2, // 10: continuous, readback as 00
    MEASURE_SINGLE              = 3  // 11: single-shot
} MeasurementMode;

// MPS (measurements per second) Temp_Cfg[7:5]
typedef enum {
    MPS_8Hz       = 0b000 << 5,  // 8  conv/s
    MPS_4Hz       = 0b001 << 5,  // 4  conv/s
    MPS_2Hz       = 0b010 << 5,  // 2  conv/s
    MPS_1Hz       = 0b011 << 5,  // 1  conv/s
    MPS_0_5Hz     = 0b100 << 5,  // 1 conv / 2 s
    MPS_0_25Hz    = 0b101 << 5,  // 1 conv / 4 s
    MPS_0_125Hz   = 0b110 << 5,  // 1 conv / 8 s
    MPS_0_0625Hz  = 0b111 << 5   // 1 conv / 16 s
} TempCfgMPS;

// AVG (averaging count) Temp_Cfg[4:3]
typedef enum {
    AVG_1   = 0b00 << 3,  // 1 sample, 2.2 ms
    AVG_8   = 0b01 << 3,  // 8 samples, 5.2 ms
    AVG_16  = 0b10 << 3,  // 16 samples, 8.5 ms
    AVG_32  = 0b11 << 3   // 32 samples, 15.3 ms
} TempCfgAVG;

// Alert mode (Alert_Mode[6])
typedef enum {
    ALERT_MODE_HIGH_TH_LOW_CLEAR = 0, // high > TH alarm, low < TL clears
    ALERT_MODE_HIGH_TH_LOW_ALARM = 1  // alarm outside TL..TH
} MTS4xAlertMode;

class MTS4X {
  public:
    explicit MTS4X(uint8_t address = MTS4X_ADDRESS, TwoWire &wire = Wire);

    // Initialization
    bool begin(int32_t sda, int32_t scl);
    bool begin(int32_t sda, int32_t scl, MeasurementMode mode);
    void setUseCrc(bool enable);

    void     setBusClock(uint32_t hz);
    uint32_t busClock() const;

    int8_t lastError() const;

    // Measurement mode and configuration
    bool setMode(MeasurementMode mode, bool heater);
    bool startSingleMessurement(); // convenience for MEASURE_SINGLE

    bool setConfig(TempCfgMPS mps, TempCfgAVG avg, bool sleep);

    // Temperature reading (convenience)
    float readTemperature(bool waitOnNewVal = true);
    float readTemperatureC(bool waitOnNewVal = true);

    bool readTemperature(float &tC, bool waitOnNewVal = true);
    bool readTemperatureRaw(int16_t &raw, bool waitOnNewVal = true);
    bool readTemperatureRawWithCrc(int16_t &raw, bool &crcOk,
                                   bool waitOnNewVal = true);
    bool readTemperatureCrc(float &tC, bool &crcOk,
                            bool waitOnNewVal = true);

    // Trigger one conversion and read it back
    bool singleShot(float &tC);

    // Status / busy / heater
    bool readStatus(uint8_t &status);
    bool isBusy(bool &busy);
    bool isBusy();

    bool heaterOn();
    bool heaterOff();
    bool isHeaterOn(bool &on);

    // Alert configuration and limits
    bool setAlertMode(bool enable, MTS4xAlertMode mode);
    bool getAlertMode(bool &enable, MTS4xAlertMode &mode);
    bool readAlertRegister(uint8_t &regValue);

    bool setHighLimit(float tHighC);
    bool setLowLimit(float tLowC);
    bool getHighLimit(float &tHighC);
    bool getLowLimit(float &tLowC);

    // ID and ROM code
    bool readDeviceId(uint16_t &id);
    bool readRomCode(uint8_t rom[5]);

    // User registers
    bool readUserRegister(uint8_t index, uint8_t &value);
    bool writeUserRegister(uint8_t index, uint8_t value);

    // Scratch and extended scratch (with CRC)
    bool readScratch(uint8_t scratch[8], bool &crcOk);
    bool readScratchExt(uint8_t scratchExt[10], bool &crcOk);

    // EEPROM and reset
    bool eepromCopyPage(bool waitReady = true, uint32_t timeoutMs = 50);
    bool eepromRecallPage(bool waitReady = true, uint32_t timeoutMs = 50);
    bool eepromRecallAll(bool waitReady = true, uint32_t timeoutMs = 50);
    bool eepromWritePageRaw(bool waitReady = true, uint32_t timeoutMs = 50);
    bool softReset(bool waitReady = true, uint32_t timeoutMs = 50);
    bool waitEepromReady(uint32_t timeoutMs = 50);

    // Parasitic power configuration (PPM_Cfg at 0x63)
    bool setParasiticPower(bool enable);

  private:
    TwoWire   *_wire;
    uint8_t    _addr;
    int8_t     _lastError;
    uint32_t   _busClock;
    bool       _useCrc;

    bool writeRegister(uint8_t reg, uint8_t value);
    bool writeRegisterRaw(uint8_t reg, const uint8_t *data, size_t len);
    bool readRegister(uint8_t reg, uint8_t &value);
    bool readRegisterRaw(uint8_t startReg, uint8_t *data, size_t len);

    bool inProgress();

    void setError(int8_t err);
};

#endif // __MTS4X_H__
