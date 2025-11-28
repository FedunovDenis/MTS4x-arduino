# MTS4x Arduino Library

Arduino-библиотека для цифровых датчиков температуры **MTS4x**  
(адаптирована под **ESP8266** и **ESP32**, без `esp32-hal.h` и прочих завязок на конкретную платформу).

## Возможности

- Работа по I2C с датчиком MTS4x (адрес по умолчанию `0x41`)
- Чтение температуры в °C с помощью простой функции `readTemperature()`
- Настройка:
  - частоты измерений (MPS: 8 Hz … 0.0625 Hz)
  - усреднения (AVG: 1, 8, 16, 32 измерений)
  - режима сна (sleep) в одиночном измерении
- Совместимость:
  - ESP8266 (NodeMCU, WeMos D1 mini и др.)
  - ESP32
  - Теоретически любые платы, где есть `Wire`

## Подключение (ESP8266: NodeMCU / WeMos D1 mini)

Рекомендуемые пины I2C:

- `SDA` → **D2** (GPIO4)
- `SCL` → **D1** (GPIO5)
- Питание: `VCC` датчика → 3.3V, `GND` → G

## Пример (Serial)

Пример находится в `examples/MTS4x_Simple/MTS4x_Simple.ino`:

```cpp
#include <Arduino.h>
#include <Wire.h>
#include "MTS4x.h"

MTS4X Temp;

// Те же типы, что в библиотеке:
// typedef TempCfgMPS MTS4x_MPS;
// typedef TempCfgAVG MTS4x_AVG;

const MTS4x_MPS MTS_RATE  = MPS_8Hz;
const MTS4x_AVG MTS_AVG   = AVG_32;
const bool      MTS_BLOCK = true;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("MTS4x Simple example"));

  // Для ESP8266: SDA = D2 (GPIO4), SCL = D1 (GPIO5)
  if (!Temp.begin(D2, D1)) {
    Serial.println(F("MTS4x init FAILED! Check wiring."));
    while (1) delay(1000);
  }

  Temp.setConfig(MTS_RATE, MTS_AVG, true);
  Serial.println(F("MTS4x init OK"));
}

void loop() {
  Temp.startSingleMessurement();
  float t = Temp.readTemperature(MTS_BLOCK);

  Serial.print(F("Temp: "));
  Serial.print(t, 2);
  Serial.println(F(" °C"));

  delay(1000);
}
