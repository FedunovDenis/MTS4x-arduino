// MTS4x Arduino library (max-feature version)
// Поддержка ESP8266 / ESP32 / AVR, без esp32-hal.h
// Реализовано:
//  - режимы измерения (single / continuous / stop)
//  - конфигурация MPS/AVG/Sleep
//  - чтение температуры (raw / °C) + CRC
//  - статус (busy, alarm, heater, E2PROM busy, TL>=TH)
//  - пороги TH/TL в градусах
//  - alert mode (2 режима) + enable/disable
//  - heater on/off
//  - ID + ROM code
//  - 10 пользовательских регистров User_define[0..9]
//  - E2PROM команды: copy page, recall page, recall all, write page, soft reset
//  - чтение scratch и scratch_ext с CRC
//  - включение паразитного питания (PPM_Cfg)
//  - TwoWire* (Wire / Wire1) + настраиваемая частота шины

#ifndef __MTS4X_H__
#define __MTS4X_H__

#include <Arduino.h>
#include <Wire.h>

// ---------- БАЗОВЫЕ РЕГИСТРЫ И КОНВЕРСИЯ ----------

#define MTS4X_ADDRESS          0x41
#define MTS4X_DEFAULT_ADDRESS  MTS4X_ADDRESS

// Температура
#define MTS4X_TEMP_LSB         0x00
#define MTS4X_TEMP_MSB         0x01

// raw (16-bit signed) <-> °C, по даташиту (LSB = 1/256 °C, offset 25 °C)
#define MTS4X_RAW_TO_CELSIUS(S_T)  ( ((S_T) / 256.0f) + 25.0f )
#define MTS4X_CELSIUS_TO_RAW(T_C)  ( (int16_t) (((T_C) - 25.0f) * 256.0f) )

// Остальные регистры
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

// User-define (10 байт)
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
#define MTS4X_ROMCODE3         0x1A
#define MTS4X_ROMCODE4         0x1B
#define MTS4X_ROMCODE5         0x1C
#define MTS4X_ROMCODE6         0x1D
#define MTS4X_ROMCODE7         0x1E
#define MTS4X_CRC_ROMCODE      0x1F

// Паразитный режим (PPM_Cfg)
#define MTS4X_PPM_CFG          0x63  // бит3..0 = 0xA => parasitic power enable

// ---------- БИТЫ СТАТУСА / ALERT / E2PROM ----------

// Status (0x03)
#define MTS4X_STATUS_ALERT_HIGH  0x80  // T > TH
#define MTS4X_STATUS_ALERT_LOW   0x40  // T < TL
#define MTS4X_STATUS_BUSY        0x20  // конверсия в процессе
#define MTS4X_STATUS_EEBUSY      0x10  // E2PROM R/W в процессе
#define MTS4X_STATUS_HEATER_ON   0x08  // heater активен
#define MTS4X_STATUS_TL_GE_TH    0x04  // TL >= TH (ошибка конфигурации)

// Alert_Mode (0x06)
#define MTS4X_ALERT_EN_MASK      0x80
#define MTS4X_ALERT_IM_MASK      0x40

// E2PROM команды (0x17)
#define MTS4X_EECMD_WRITE_PAGE   0x08  // записать scratch -> E2PROM (page)
#define MTS4X_EECMD_LOAD_PAGE    0xB6  // загрузить E2PROM -> scratch (page)
#define MTS4X_EECMD_COPY_PAGE    0x48  // copy page
#define MTS4X_EECMD_RECALL_EE    0xB8  // загрузить все 32 байта E2PROM
#define MTS4X_EECMD_SOFT_RESET   0x6A  // soft reset + recall

// ---------- ENUM'ы И ТИПЫ ----------

// Режим измерения (Temp_Cmd[7:6])
typedef enum {
    MEASURE_CONTINUOUS = 0b00,  // непрерывный режим
    MEASURE_STOP       = 0b01,  // стоп
    MEASURE_SINGLE     = 0b11   // одиночное измерение
} MeasurementMode;

// Частота измерений (Temp_Cfg[7:5]) – MPS
typedef enum {
    MPS_8Hz       = 0b000 << 5,
    MPS_4Hz       = 0b001 << 5,
    MPS_2Hz       = 0b010 << 5,
    MPS_1Hz       = 0b011 << 5,
    MPS_0_5Hz     = 0b100 << 5,
    MPS_0_25Hz    = 0b101 << 5,
    MPS_0_125Hz   = 0b110 << 5,
    MPS_0_0625Hz  = 0b111 << 5
} TempCfgMPS;

// Усреднение (Temp_Cfg[4:3]) – AVG
typedef enum {
    AVG_1   = 0b00 << 3,
    AVG_8   = 0b01 << 3,
    AVG_16  = 0b10 << 3,
    AVG_32  = 0b11 << 3
} TempCfgAVG;

// Алиасы под привычные типы
typedef TempCfgMPS MTS4x_MPS;
typedef TempCfgAVG MTS4x_AVG;

// Режим аларма (Alert_Mode bit6)
typedef enum {
    ALERT_MODE_HIGH_TH_LOW_CLEAR = 0,  // >TH тревога, <TL сброс тревоги
    ALERT_MODE_HIGH_TH_LOW_ALARM = 1   // >TH тревога, <TL тоже тревога
} MTS4xAlertMode;

// Коды ошибок (lastError)
typedef enum {
    MTS4X_OK            = 0,
    MTS4X_ERR_I2C_BEGIN = -1,
    MTS4X_ERR_I2C_TX    = -2,
    MTS4X_ERR_I2C_RX    = -3,
    MTS4X_ERR_NO_DATA   = -4,
    MTS4X_ERR_BAD_ARG   = -5,
    MTS4X_ERR_TIMEOUT   = -6
} MTS4xError;

// ---------- КЛАСС ----------

class MTS4X {
  public:
    explicit MTS4X(uint8_t address = MTS4X_DEFAULT_ADDRESS,
                   TwoWire &wire = Wire);

    // Инициализация I2C на заданных SDA/SCL
    bool begin(int32_t sda, int32_t scl);
    // Инициализация + установка режима
    bool begin(int32_t sda, int32_t scl, MeasurementMode mode);

    // Режим измерения + heater
    bool setMode(MeasurementMode mode, bool heater);

    // Быстрый старт одиночного измерения (два имени на выбор)
    bool startSingleMessurement();
    bool startSingleMeasurement();

    // Конфигурация MPS/AVG/Sleep
    bool setConfig(TempCfgMPS mps, TempCfgAVG avg, bool sleep);

    // ---------- ТЕМПЕРАТУРА ----------

    float readTemperature(bool waitOnNewVal = true);
    float readTemperatureC(bool waitOnNewVal = true) { return readTemperature(waitOnNewVal); }

    bool readTemperature(float &tC, bool waitOnNewVal = true);
    bool readTemperatureRaw(int16_t &raw, bool waitOnNewVal = true);
    bool readTemperatureRawWithCrc(int16_t &raw, bool &crcOk,
                                   bool waitOnNewVal = true);
    bool readTemperatureCrc(float &tC, bool &crcOk,
                            bool waitOnNewVal = true);

    bool singleShot(float &tC);

    // ---------- СТАТУС / BUSY ----------

    bool readStatus(uint8_t &status);
    bool isBusy(bool &busy);
    bool isBusy();

    // ---------- HEATER ----------

    bool heaterOn();
    bool heaterOff();
    bool isHeaterOn(bool &on);

    // ---------- ALERT / ПОРОГИ ----------

    bool setAlertMode(bool enable, MTS4xAlertMode mode);
    bool getAlertMode(bool &enable, MTS4xAlertMode &mode);
    bool readAlertRegister(uint8_t &regValue);

    bool setHighLimit(float tHighC);
    bool setLowLimit(float tLowC);
    bool getHighLimit(float &tHighC);
    bool getLowLimit(float &tLowC);

    // ---------- ID / ROM ----------

    bool readDeviceId(uint16_t &id);
    bool readRomCode(uint8_t rom[5]); // ROM3..ROM7

    // ---------- USER REGISTERS (0..9) ----------

    bool readUserRegister(uint8_t index, uint8_t &value);
    bool writeUserRegister(uint8_t index, uint8_t value);

    // ---------- SCRATCH + CRC ----------

    bool readScratch(uint8_t scratch[8], bool &crcOk);
    bool readScratchExt(uint8_t scratchExt[10], bool &crcOk);

    // ---------- E2PROM / SOFT RESET ----------

    bool eepromCopyPage(bool waitReady = true, uint32_t timeoutMs = 50);
    bool eepromRecallPage(bool waitReady = true, uint32_t timeoutMs = 50);
    bool eepromRecallAll(bool waitReady = true, uint32_t timeoutMs = 50);
    bool eepromWritePageRaw(bool waitReady = true, uint32_t timeoutMs = 50);
    bool softReset(bool waitReady = true, uint32_t timeoutMs = 50);
    bool waitEepromReady(uint32_t timeoutMs = 50);

    // ---------- ПАРАЗИТНОЕ ПИТАНИЕ (PPM_Cfg) ----------

    bool setParasiticPower(bool enable);

    // ---------- I2C CLOCK / ERROR ----------

    void setBusClock(uint32_t hz);
    uint32_t busClock() const { return _i2cClock; }

    int8_t lastError() const { return _lastError; }

  private:
    uint8_t         _address;
    TwoWire        *_wire;
    uint32_t        _i2cClock;
    int8_t          _lastError;
    TempCfgAVG      _cfgAvg;
    TempCfgMPS      _cfgMps;
    bool            _cfgSleep;
    MeasurementMode _lastMode;

    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegister(uint8_t reg, uint8_t &value);
    bool readRegisters(uint8_t startReg, uint8_t *buf, size_t len);

    bool inProgress();

    uint16_t convTimeMsFromCfg() const;

    static uint8_t crc8(const uint8_t *data, size_t len);

    bool eepromCommand(uint8_t cmd, bool waitReady, uint32_t timeoutMs);
};

#endif // __MTS4X_H__
