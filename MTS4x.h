// by MS, адаптировано для ESP8266/ESP32

#ifndef __MTS4X_H__
#define __MTS4X_H__

#include <Arduino.h>
#include <Wire.h>

// I2C адрес датчика
#define MTS4X_ADDRESS 0x41

// Температура и формула пересчёта
#define MTS4X_TEMP_LSB         0x00
#define MTS4X_TEMP_MSB         0x01
#define MTS4X_RAW_TO_CELSIUS(S_T) (((S_T) / 256.0f) + 25.0f)

// Регистр CRC температуры
#define MTS4X_CRC_TEMP         0x02
// Регистр статуса
#define MTS4X_STATUS           0x03
// Регистр команды измерения
#define MTS4X_TEMP_CMD         0x04
// Регистр конфигурации измерения
#define MTS4X_TEMP_CFG         0x05
// Режим тревог
#define MTS4X_ALERT_MODE       0x06
// Верхний порог (TH)
#define MTS4X_TH_LSB           0x07
#define MTS4X_TH_MSB           0x08
// Нижний порог (TL)
#define MTS4X_TL_LSB           0x09
#define MTS4X_TL_MSB           0x0A
// CRC scratch
#define MTS4X_CRC_SCRATCH      0x0B

// Пользовательские регистры
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
#define MTS4X_DEVICE_ID_LSB    0x18  // или Romcode1
#define MTS4X_DEVICE_ID_MSB    0x19  // или Romcode2

// Romcode
#define MTS4X_ROMCODE3         0x1A
#define MTS4X_ROMCODE4         0x1B
#define MTS4X_ROMCODE5         0x1C
#define MTS4X_ROMCODE6         0x1D
#define MTS4X_ROMCODE7         0x1E
#define MTS4X_CRC_ROMCODE      0x1F

// Режим измерения (используется в TEMP_CMD, биты 7:6)
typedef enum {
    MEASURE_CONTINUOUS = 0b00,  // непрерывный
    MEASURE_STOP       = 0b01,  // стоп
    MEASURE_SINGLE     = 0b11   // одиночный
} MeasurementMode;

// Messfrequenz (MPS) – частота измерения (TEMP_CFG биты 7:5)
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

// Mittelwertbildung (AVG) – усреднение (TEMP_CFG биты 4:3)
typedef enum {
    AVG_1   = 0b00 << 3,
    AVG_8   = 0b01 << 3,
    AVG_16  = 0b10 << 3,
    AVG_32  = 0b11 << 3
} TempCfgAVG;

// Алиасы под твой код
typedef TempCfgMPS MTS4x_MPS;
typedef TempCfgAVG MTS4x_AVG;

class MTS4X {
  public:
    MTS4X();

    // Инициализация I2C
    bool begin(int32_t sda, int32_t scl);
    // Инициализация + установка режима измерения
    bool begin(int32_t sda, int32_t scl, MeasurementMode mode);

    // Быстрый вызов одиночного измерения
    bool startSingleMessurement();

    // Режим измерения + подогрев
    bool setMode(MeasurementMode mode, bool heater);

    // Настройка частоты измерений, усреднения, sleep
    // sleep работает только в одиночном режиме
    bool setConfig(TempCfgMPS mps, TempCfgAVG avg, bool sleep);

    // Чтение температуры, при waitOnNewVal = true ждём новое значение
    float readTemperature(bool waitOnNewVal);

  private:
    bool inProgress();
};

#endif // __MTS4X_H__
