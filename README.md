<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="utf-8" />
  <title>MTS4x Arduino Library — README</title>
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <style>
    :root {
      --bg: #0f172a;
      --bg-alt: #020617;
      --card: #020617;
      --card-alt: #020617;
      --border: #1e293b;
      --accent: #38bdf8;
      --accent-soft: rgba(56,189,248,0.15);
      --accent-strong: #0ea5e9;
      --text-main: #e5e7eb;
      --text-soft: #9ca3af;
      --text-muted: #6b7280;
      --text-strong: #fefce8;
      --code-bg: #020617;
      --code-border: #1e293b;
      --danger-bg: #450a0a;
      --danger-text: #fecaca;
      --table-header: #020617;
    }

    * {
      box-sizing: border-box;
    }

    body {
      margin: 0;
      padding: 0;
      font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      background: radial-gradient(circle at top left, #0b1120, #020617 55%);
      color: var(--text-main);
      line-height: 1.6;
    }

    a {
      color: var(--accent);
      text-decoration: none;
    }

    a:hover {
      text-decoration: underline;
      color: var(--accent-strong);
    }

    .page {
      max-width: 1100px;
      margin: 0 auto;
      padding: 32px 16px 48px;
    }

    header.hero {
      margin-bottom: 24px;
      padding: 24px 24px 20px;
      border-radius: 18px;
      border: 1px solid rgba(148,163,184,0.3);
      background: radial-gradient(circle at top left, #0b1120, #020617 65%);
      box-shadow: 0 24px 60px rgba(15,23,42,0.9);
      display: flex;
      gap: 24px;
      flex-wrap: wrap;
    }

    .hero-main {
      flex: 2 1 260px;
    }

    .hero-side {
      flex: 1 1 220px;
      display: flex;
      flex-direction: column;
      gap: 8px;
    }

    h1 {
      margin: 0 0 8px;
      font-size: 2.2rem;
      letter-spacing: 0.03em;
      color: #e5e7eb;
    }

    .hero-subtitle {
      margin: 0 0 16px;
      color: var(--text-soft);
      font-size: 0.98rem;
    }

    .hero-badges {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
      margin-bottom: 8px;
    }

    .badge {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      border-radius: 999px;
      padding: 4px 10px;
      font-size: 0.78rem;
      letter-spacing: 0.04em;
      text-transform: uppercase;
      border: 1px solid rgba(148,163,184,0.35);
      background: rgba(15,23,42,0.7);
      color: var(--text-soft);
      backdrop-filter: blur(10px);
    }

    .badge strong {
      font-weight: 600;
      color: #e5e7eb;
    }

    .badge.version {
      border-color: rgba(74,222,128,0.5);
      background: rgba(22,163,74,0.12);
      color: #bbf7d0;
    }

    .badge.sensors {
      border-color: rgba(56,189,248,0.5);
      background: rgba(15,118,110,0.4);
      color: #e0f2fe;
    }

    .badge.platforms {
      border-color: rgba(248,250,252,0.2);
    }

    .hero-links {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
      margin-top: 8px;
    }

    .btn {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      border-radius: 999px;
      padding: 7px 14px;
      font-size: 0.85rem;
      border: 1px solid rgba(148,163,184,0.4);
      background: rgba(15,23,42,0.9);
      color: var(--text-main);
      text-decoration: none;
      cursor: pointer;
    }

    .btn.primary {
      border-color: rgba(56,189,248,0.7);
      background: radial-gradient(circle at top left, #0284c7, #0f172a 65%);
      color: #eff6ff;
      font-weight: 500;
      box-shadow: 0 12px 35px rgba(56,189,248,0.35);
    }

    .btn span.icon {
      font-size: 1rem;
    }

    .hero-side-box {
      border-radius: 14px;
      border: 1px solid rgba(148,163,184,0.35);
      background: rgba(15,23,42,0.85);
      padding: 10px 12px;
      font-size: 0.78rem;
      color: var(--text-soft);
    }

    .hero-side-box-title {
      font-weight: 600;
      margin-bottom: 6px;
      color: #e5e7eb;
      font-size: 0.8rem;
      letter-spacing: 0.07em;
      text-transform: uppercase;
    }

    .hero-side-box dl {
      margin: 0;
      display: grid;
      grid-template-columns: auto 1fr;
      column-gap: 10px;
      row-gap: 4px;
    }

    .hero-side-box dt {
      color: var(--text-muted);
    }

    .hero-side-box dd {
      margin: 0;
      color: var(--text-main);
    }

    .grid {
      display: grid;
      grid-template-columns: minmax(0, 3fr) minmax(0, 2fr);
      gap: 18px;
    }

    @media (max-width: 900px) {
      .grid {
        grid-template-columns: minmax(0, 1fr);
      }
    }

    h2 {
      margin-top: 24px;
      margin-bottom: 8px;
      font-size: 1.5rem;
      border-bottom: 1px solid rgba(148,163,184,0.3);
      padding-bottom: 4px;
    }

    h3 {
      margin-top: 20px;
      margin-bottom: 6px;
      font-size: 1.2rem;
      color: #e5e7eb;
    }

    h4 {
      margin-top: 16px;
      margin-bottom: 4px;
      font-size: 1rem;
      color: #e5e7eb;
    }

    p {
      margin-top: 6px;
      margin-bottom: 6px;
      color: var(--text-main);
    }

    ul, ol {
      margin-top: 4px;
      margin-bottom: 8px;
      padding-left: 20px;
    }

    li {
      margin-bottom: 3px;
    }

    .muted {
      color: var(--text-soft);
    }

    .note {
      margin-top: 8px;
      margin-bottom: 8px;
      padding: 8px 10px;
      border-radius: 8px;
      border: 1px dashed rgba(148,163,184,0.6);
      background: rgba(15,23,42,0.7);
      font-size: 0.9rem;
    }

    .note.small {
      font-size: 0.82rem;
    }

    .callout-danger {
      background: var(--danger-bg);
      color: var(--danger-text);
      border-color: rgba(248,113,113,0.7);
    }

    table {
      width: 100%;
      border-collapse: collapse;
      margin: 8px 0 10px;
      font-size: 0.9rem;
    }

    th, td {
      padding: 6px 8px;
      border: 1px solid rgba(30,41,59,0.9);
    }

    th {
      background: var(--table-header);
      color: var(--text-soft);
      font-weight: 600;
      text-align: left;
    }

    tr:nth-child(even) td {
      background: rgba(15,23,42,0.7);
    }

    code {
      font-family: "JetBrains Mono", ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace;
      font-size: 0.85rem;
      background: rgba(15,23,42,0.8);
      padding: 1px 4px;
      border-radius: 4px;
      border: 1px solid rgba(30,64,175,0.5);
    }

    pre {
      margin: 8px 0 10px;
      padding: 10px 12px;
      background: var(--code-bg);
      border-radius: 10px;
      border: 1px solid var(--code-border);
      overflow-x: auto;
      font-size: 0.86rem;
    }

    pre code {
      background: transparent;
      border: none;
      padding: 0;
      white-space: pre;
      display: block;
    }

    .toc {
      margin: 10px 0 18px;
      padding: 12px 14px;
      border-radius: 12px;
      border: 1px solid rgba(148,163,184,0.4);
      background: rgba(15,23,42,0.8);
      font-size: 0.9rem;
    }

    .toc-title {
      font-size: 0.85rem;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.08em;
      color: var(--text-soft);
      margin-bottom: 4px;
    }

    .toc ul {
      margin: 2px 0;
      padding-left: 18px;
    }

    .toc li {
      margin-bottom: 2px;
    }

    .section {
      margin-top: 20px;
      margin-bottom: 8px;
      padding: 12px 14px;
      border-radius: 16px;
      border: 1px solid rgba(30,64,175,0.4);
      background: radial-gradient(circle at top left, rgba(15,23,42,0.95), rgba(2,6,23,0.98));
    }

    .section.light {
      border-color: rgba(148,163,184,0.4);
    }

    .section-header {
      display: flex;
      align-items: baseline;
      justify-content: space-between;
      gap: 8px;
      margin-bottom: 6px;
    }

    .section-header h2,
    .section-header h3 {
      border-bottom: none;
      padding-bottom: 0;
      margin-bottom: 0;
    }

    footer {
      margin-top: 30px;
      padding-top: 12px;
      border-top: 1px solid rgba(148,163,184,0.35);
      font-size: 0.82rem;
      color: var(--text-soft);
    }
  </style>
</head>
<body>
  <div class="page">
    <header class="hero">
      <div class="hero-main">
        <h1>MTS4x Arduino Library</h1>
        <p class="hero-subtitle">
          Полнофункциональная библиотека для высокоточных цифровых датчиков температуры
          семейства <strong>MTS4</strong> (MTS4 / MTS4Z / MTS4P / MTS4B, включая модуль
          <strong>MTS4P+T4</strong>) с поддержкой ESP8266, ESP32 и классических Arduino-плат.
        </p>
        <div class="hero-badges">
          <div class="badge version">
            <span>Версия</span>
            <strong>2.0.0</strong>
          </div>
          <div class="badge sensors">
            <span>Датчики</span>
            <strong>MTS4 / Z / P / B</strong>
          </div>
          <div class="badge platforms">
            <span>Платы</span>
            <strong>ESP8266 / ESP32 / AVR</strong>
          </div>
        </div>
        <div class="hero-links">
          <a class="btn primary" href="https://github.com/FedunovDenis/MTS4x-arduino" target="_blank" rel="noopener noreferrer">
            <span class="icon">⭐</span>
            <span>Открыть репозиторий на GitHub</span>
          </a>
          <a class="btn" href="#sections-examples">
            <span class="icon">📦</span>
            <span>Смотреть примеры</span>
          </a>
          <a class="btn" href="#sections-connection">
            <span class="icon">🔌</span>
            <span>Схема подключения</span>
          </a>
        </div>
      </div>
      <div class="hero-side">
        <div class="hero-side-box">
          <div class="hero-side-box-title">Кратко о возможностях</div>
          <dl>
            <dt>Диапазон</dt>
            <dd>≈ −103…+153&nbsp;°C</dd>
            <dt>Разрешение</dt>
            <dd>16&nbsp;бит (0.004&nbsp;°C/LSB)</dd>
            <dt>Точность</dt>
            <dd>до ±0.1&nbsp;°C (в окне высокой точности)</dd>
            <dt>Фишки</dt>
            <dd>CRC, TH/TL, Alert, heater, user-регистры, EEPROM</dd>
          </dl>
        </div>
        <div class="hero-side-box">
          <div class="hero-side-box-title">Информация</div>
          <dl>
            <dt>Репозиторий</dt>
            <dd><a href="https://github.com/FedunovDenis/MTS4x-arduino" target="_blank" rel="noopener noreferrer">github.com/FedunovDenis/MTS4x-arduino</a></dd>
            <dt>Папка библиотеки</dt>
            <dd><code>MTS4x/</code></dd>
            <dt>Лицензия</dt>
            <dd>MIT (рекомендуется)</dd>
          </dl>
        </div>
      </div>
    </header>

    <section class="toc" id="toc">
      <div class="toc-title">Оглавление</div>
      <ul>
        <li><a href="#features">Особенности</a></li>
        <li><a href="#supported-sensors">Поддерживаемые датчики</a></li>
        <li><a href="#platforms">Поддерживаемые платформы</a></li>
        <li><a href="#install">Установка</a></li>
        <li><a href="#sections-connection">Подключение (таблицы + схемы)</a></li>
        <li><a href="#quickstart">Быстрый старт</a></li>
        <li><a href="#sections-examples">Примеры из библиотеки</a></li>
        <li><a href="#recipes">Рецепты использования</a></li>
        <li><a href="#api-overview">API класса MTS4X</a></li>
        <li><a href="#max-precision">Режим максимальной точности</a></li>
        <li><a href="#practical">Практические рекомендации</a></li>
        <li><a href="#layout-emc">Рекомендации по разводке и EMC</a></li>
        <li><a href="#troubleshooting">Типичные ошибки и диагностика</a></li>
        <li><a href="#eeprom">Работа с E²PROM</a></li>
        <li><a href="#alerts">Пороги и аварийные сигналы</a></li>
        <li><a href="#parasitic">Паразитное питание</a></li>
        <li><a href="#changelog">Changelog</a></li>
        <li><a href="#license">License</a></li>
        <li><a href="#english-summary">English summary</a></li>
      </ul>
    </section>

    <section id="features" class="section light">
      <div class="section-header">
        <h2>✨ Особенности</h2>
      </div>

      <h3>Диапазон и разрешение</h3>
      <ul>
        <li>Внутренняя модель и формат регистров ~ <strong>−103…+153 °C</strong>.</li>
        <li>16-битный результат, шаг <strong>0.004 °C</strong>.</li>
      </ul>

      <h3>Точность</h3>
      <ul>
        <li>Паспортная точность до <strong>±0.1 °C</strong> в «окне высокой точности»:
          <ul>
            <li>MTS4P — типично −25…+25&nbsp;°C;</li>
            <li>MTS4Z — типично 0…+50&nbsp;°C.</li>
          </ul>
        </li>
        <li>Вне окна точность обычно ±0.5…1.0&nbsp;°C (смотрите даташит конкретной модели).</li>
        <li>Выбор аппаратного усреднения: <code>AVG_1 / AVG_8 / AVG_16 / AVG_32</code>.</li>
        <li>Дополнительное цифровое усреднение в микроконтроллере (пример метеостанции).</li>
      </ul>

      <h3>Режимы измерения</h3>
      <ul>
        <li><strong>Single-shot</strong> (одиночное измерение) — режим для максимальной точности и экономии энергии.</li>
        <li><strong>Continuous</strong> (непрерывный) — периодические измерения с частотой <code>MPS_xHz</code>.</li>
        <li><strong>Stop</strong> — остановка измерений, работа только по запросу.</li>
        <li>Настройка частоты <code>MPS</code> и флага <code>sleep</code> через <code>setConfig()</code>.</li>
      </ul>

      <h3>CRC и целостность данных</h3>
      <ul>
        <li>CRC8:
          <ul>
            <li>для температуры (<code>Temp_Lsb/Msb + Crc_temp</code>);</li>
            <li>для области <code>scratch</code> (0x03–0x0A + <code>Crc_Scratch</code>);</li>
            <li>для <code>scratch_ext</code> (0x0C–0x15 + <code>Crc_Scratch_Ext</code>).</li>
          </ul>
        </li>
        <li>Высокоуровневые функции: <code>readTemperatureCrc()</code>, <code>readTemperatureRawWithCrc()</code>, <code>readScratch()</code>, <code>readScratchExt()</code>.</li>
      </ul>

      <h3>Пороги, алармы, heater, EEPROM</h3>
      <ul>
        <li>Программируемые пороги <strong>TH / TL</strong> (в °C), пересчёт в raw внутри библиотеки.</li>
        <li><code>Alert_Mode</code>:
          <ul>
            <li><code>HIGH_TH_LOW_CLEAR</code> — тревога выше TH, сброс ниже TL;</li>
            <li><code>HIGH_TH_LOW_ALARM</code> — любое значение вне диапазона TL–TH считается аварией.</li>
          </ul>
        </li>
        <li>Статусный байт <code>Status</code> с флагами:
          HIGH/LOW alarm, BUSY, EE_BUSY, HEATER, TL ≥ TH и др.</li>
        <li>Встроенный <strong>heater</strong> для борьбы с конденсатом и самотеста.</li>
        <li>До 10 <strong>user-регистров</strong> + команды E²PROM:
          <code>copy</code>, <code>recall</code>, <code>write</code>, <code>soft reset</code>.</li>
      </ul>
    </section>

    <section id="supported-sensors" class="section light">
      <div class="section-header">
        <h2>🧊 Поддерживаемые датчики</h2>
      </div>
      <p>Библиотека ориентирована на всю линейку <strong>MTS4</strong>:</p>
      <ul>
        <li><strong>MTS4</strong> — базовый вариант;</li>
        <li><strong>MTS4Z</strong> — максимальная точность в 0…+50&nbsp;°C;</li>
        <li><strong>MTS4P</strong> — максимальная точность в −25…+25&nbsp;°C;</li>
        <li><strong>MTS4B</strong> — вариант с ±0.5&nbsp;°C в широком диапазоне.</li>
      </ul>
      <p>Особое внимание — модулю <strong>MTS4P+T4</strong>, который часто используется как готовая плата датчика.</p>
    </section>

    <section id="platforms" class="section light">
      <div class="section-header">
        <h2>🧩 Поддерживаемые платформы</h2>
      </div>
      <ul>
        <li><strong>ESP8266</strong> — NodeMCU, Wemos D1 mini и аналоги;</li>
        <li><strong>ESP32</strong> — DevKitC, NodeMCU-32S и т.п.;</li>
        <li><strong>Arduino AVR</strong> — Uno, Nano, Mega, Pro Mini;</li>
        <li>Любая плата с поддержкой <code>TwoWire</code> (I²C).</li>
      </ul>
      <p>Можно использовать любую шину I²C:</p>
      <pre><code class="language-cpp">#include &lt;Wire.h&gt;
#include "MTS4x.h"

MTS4X sensor1(0x41, Wire);   // основная шина I2C
// MTS4X sensor2(0x42, Wire1); // вторая шина, если есть Wire1</code></pre>
    </section>

    <section id="install" class="section light">
      <div class="section-header">
        <h2>📥 Установка</h2>
      </div>

      <h3 id="arduino-ide">Arduino IDE</h3>
      <ol>
        <li>Склонируйте репозиторий:
          <pre><code class="language-bash">git clone https://github.com/FedunovDenis/MTS4x-arduino.git</code></pre>
        </li>
        <li>Скопируйте папку <code>MTS4x/</code> в:
          <pre><code>Documents/Arduino/libraries/MTS4x/</code></pre>
        </li>
        <li>Убедитесь, что структура такая:
          <pre><code>MTS4x/
  library.properties
  MTS4x.h
  MTS4x.cpp
  examples/
    MTS4x_FullDemo/
      MTS4x_FullDemo.ino
    MTS4x_MeteoStation/
      MTS4x_MeteoStation.ino</code></pre>
        </li>
        <li>Перезапустите Arduino IDE — примеры появятся в
          <strong>Файл → Примеры → MTS4x</strong>.
        </li>
      </ol>

      <h3 id="platformio">PlatformIO</h3>
      <pre><code class="language-ini">[env:esp8266]
platform      = espressif8266
board         = d1_mini
framework     = arduino

lib_extra_dirs = /path/to/MTS4x-arduino
lib_deps       = MTS4x</code></pre>
      <p>Либо положите <code>MTS4x/</code> в каталог <code>lib/</code> вашего проекта.</p>
    </section>

    <section id="sections-connection" class="section">
      <div class="section-header">
        <h2>🔌 Подключение (таблицы + схемы)</h2>
      </div>

      <h3 id="i2c-common">Общие рекомендации по I²C</h3>
      <ul>
        <li>Питание датчика: <strong>3.3 V или 5 V</strong> — смотрите документацию к модулю.</li>
        <li>Общий <strong>GND</strong> между датчиком и контроллером обязателен.</li>
        <li>Линии SDA/SCL:
          <ul>
            <li>желательно pull-up резисторы к VCC (типично 4.7&nbsp;kΩ);</li>
            <li>на большинстве модулей MTS4P+T4 они уже установлены.</li>
          </ul>
        </li>
        <li>Длина шины:
          <ul>
            <li>до ~0.5&nbsp;м — обычно без проблем на 100–400&nbsp;кГц;</li>
            <li>до ~1–2&nbsp;м — лучше 100&nbsp;кГц и желательно экранированный/витой провод.</li>
          </ul>
        </li>
        <li>Адрес по умолчанию чаще всего <strong>0x41</strong>.</li>
        <li>Рекомендовано ставить керамический конденсатор <strong>0.1&nbsp;µF</strong> максимально близко к VCC/GND датчика.</li>
      </ul>

      <div class="grid">
        <div>
          <h3 id="esp8266-nodemcu--wemos-d1-mini">ESP8266 (NodeMCU / Wemos D1 mini)</h3>
          <table>
            <thead>
              <tr>
                <th>MTS4P+T4</th>
                <th>ESP8266 (D1 mini)</th>
              </tr>
            </thead>
            <tbody>
              <tr>
                <td>VCC</td>
                <td>3V3</td>
              </tr>
              <tr>
                <td>GND</td>
                <td>G</td>
              </tr>
              <tr>
                <td>SDA</td>
                <td>D2 (GPIO4)</td>
              </tr>
              <tr>
                <td>SCL</td>
                <td>D1 (GPIO5)</td>
              </tr>
            </tbody>
          </table>
          <pre><code class="language-cpp">#define I2C_SDA_PIN D2
#define I2C_SCL_PIN D1</code></pre>
        </div>
        <div>
          <h3 id="esp32">ESP32</h3>
          <table>
            <thead>
              <tr>
                <th>MTS4P+T4</th>
                <th>ESP32</th>
              </tr>
            </thead>
            <tbody>
              <tr>
                <td>VCC</td>
                <td>3V3</td>
              </tr>
              <tr>
                <td>GND</td>
                <td>GND</td>
              </tr>
              <tr>
                <td>SDA</td>
                <td>GPIO21</td>
              </tr>
              <tr>
                <td>SCL</td>
                <td>GPIO22</td>
              </tr>
            </tbody>
          </table>
          <pre><code class="language-cpp">#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22</code></pre>
        </div>
      </div>

      <h3 id="arduino-uno--nano">Arduino Uno / Nano</h3>
      <table>
        <thead>
          <tr>
            <th>MTS4P+T4</th>
            <th>Arduino Uno/Nano</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td>VCC</td>
            <td>5V или 3.3V* (*если модуль допускает)</td>
          </tr>
          <tr>
            <td>GND</td>
            <td>GND</td>
          </tr>
          <tr>
            <td>SDA</td>
            <td>A4 / SDA</td>
          </tr>
          <tr>
            <td>SCL</td>
            <td>A5 / SCL</td>
          </tr>
        </tbody>
      </table>
      <pre><code class="language-cpp">#define I2C_SDA_PIN SDA
#define I2C_SCL_PIN SCL</code></pre>

      <h3>ASCII-схемы подключения (быстрый взгляд)</h3>

      <h4>Схема №1 — ESP8266 D1 mini + MTS4P+T4 (3.3&nbsp;В)</h4>
      <pre><code>        +---------------------+            +--------------------+
        |   ESP8266 D1 mini   |            |     MTS4P+T4      |
        |                     |            |   (sensor board)  |
        |   [3V3] ----------- +------------+ VCC               |
        |   [GND] ----------- +------------+ GND               |
        |   [D2 / GPIO4] ---- +------------+ SDA               |
        |   [D1 / GPIO5] ---- +------------+ SCL               |
        +---------------------+            +--------------------+

  * Питание 3.3 В общее.
  * При необходимости добавить рядом с датчиком 0.1 µF между VCC и GND.</code></pre>

      <h4>Схема №2 — ESP32 DevKit + MTS4P+T4</h4>
      <pre><code>        +---------------------+            +--------------------+
        |     ESP32 DevKit    |            |     MTS4P+T4      |
        |                     |            |   (sensor board)  |
        |   [3V3] ----------- +------------+ VCC               |
        |   [GND] ----------- +------------+ GND               |
        |   [GPIO21] -------- +------------+ SDA               |
        |   [GPIO22] -------- +------------+ SCL               |
        +---------------------+            +--------------------+</code></pre>

      <h4>Схема №3 — Arduino Uno + MTS4P+T4 (5&nbsp;В)</h4>
      <pre><code>        +---------------------+            +--------------------+
        |     Arduino Uno     |            |     MTS4P+T4      |
        |                     |            |   (sensor board)  |
        |   [5V] ------------ +------------+ VCC               |
        |   [GND] ----------- +------------+ GND               |
        |   [A4 / SDA] ------ +------------+ SDA               |
        |   [A5 / SCL] ------ +------------+ SCL               |
        +---------------------+            +--------------------+

  Важно:
    * Убедитесь, что модуль допускает 5 В по питанию и логике I²C.
    * Если модуль строго 3.3 В — используйте понижающий регулятор питания
      и при необходимости уровневый преобразователь для SDA/SCL.</code></pre>

      <h3 id="multiple-sensors">Несколько датчиков MTS4 на одной шине I²C</h3>
      <div class="note callout-danger">
        ⚠️ Большинство готовых модулей MTS4P+T4 имеют фиксированный адрес 0x41. Если адрес одинаковый у всех — просто повесить их параллельно нельзя.
      </div>
      <p>Варианты подключить несколько датчиков:</p>
      <ul>
        <li>Использовать модули/чипы с <strong>разными адресами</strong> (если это поддерживается аппаратно).</li>
        <li>Использовать <strong>I²C-мультиплексор</strong> (например, TCA9548A), по одному датчику на канал.</li>
        <li>Использовать разные шины I²C (<code>Wire</code>, <code>Wire1</code> и т.п.).</li>
      </ul>
      <p>Пример кода для двух датчиков с разными адресами — в разделе
        <a href="#recipe-two-sensors">«Два датчика с разными адресами»</a>.
      </p>
    </section>

    <section id="quickstart" class="section light">
      <div class="section-header">
        <h2>🚀 Быстрый старт</h2>
      </div>
      <p>Минимальный пример: один датчик, single-shot, температура + CRC.</p>
      <pre><code class="language-cpp">#include &lt;Wire.h&gt;
#include "MTS4x.h"

#if defined(ESP8266)
  #define I2C_SDA_PIN D2
  #define I2C_SCL_PIN D1
#elif defined(ESP32)
  #define I2C_SDA_PIN 21
  #define I2C_SCL_PIN 22
#else
  #define I2C_SDA_PIN SDA
  #define I2C_SCL_PIN SCL
#endif

MTS4X mts;

void setup() {
  Serial.begin(115200);
  delay(200);

  if (!mts.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
    Serial.print(F("MTS4x init failed, err="));
    Serial.println(mts.lastError());
    while (1) delay(1000);
  }

  // Частота и усреднение: 1 Гц, AVG_8, single-shot
  mts.setConfig(MPS_1Hz, AVG_8, true);
  mts.setMode(MEASURE_STOP, false);
}

void loop() {
  float t;
  bool crcOk;

  if (mts.readTemperatureCrc(t, crcOk, true)) {
    Serial.print(F("T = "));
    Serial.print(t, 3);
    Serial.print(F(" °C  CRC="));
    Serial.println(crcOk ? F("OK") : F("FAIL"));
  } else {
    Serial.print(F("readTemperatureCrc() error, err="));
    Serial.println(mts.lastError());
  }

  delay(1000);
}</code></pre>
    </section>

    <section id="sections-examples" class="section light">
      <div class="section-header">
        <h2>📂 Примеры из библиотеки</h2>
      </div>

      <h3 id="example-fulldemo">1. <code>MTS4x_FullDemo</code> — полный Serial-demo</h3>
      <p><strong>Путь:</strong> <code>examples/MTS4x_FullDemo/MTS4x_FullDemo.ino</code></p>
      <p>Демонстрирует:</p>
      <ul>
        <li>чтение температуры (raw и °C) с CRC;</li>
        <li>чтение и расшифровку <code>Status</code>;</li>
        <li>пороги <code>TH/TL</code> и <code>Alert_Mode</code>;</li>
        <li>включение/выключение heater (подогревателя);</li>
        <li>работу с user-регистрами;</li>
        <li>команды EEPROM (<code>copy</code>, <code>recall</code>, <code>soft reset</code>);</li>
        <li>чтение <code>scratch</code> и <code>scratch_ext</code> с CRC;</li>
        <li>чтение Device ID и ROM-кода.</li>
      </ul>
      <p><strong>Как использовать:</strong></p>
      <ol>
        <li>Откройте пример:
          <br /><strong>Файл → Примеры → MTS4x → MTS4x_FullDemo</strong>.
        </li>
        <li>Выберите подходящую плату и порт.</li>
        <li>Скомпилируйте, залейте и откройте Serial Monitor (115200).</li>
        <li>Команды:
          <ul>
            <li><code>?</code> — список команд;</li>
            <li><code>i</code> — ID и ROM-код;</li>
            <li><code>a</code> — TH/TL и AlertMode;</li>
            <li><code>t</code> — heater on/off;</li>
            <li><code>s</code> — <code>scratch</code>;</li>
            <li><code>x</code> — <code>scratch_ext</code>;</li>
            <li><code>u</code> — пример user-регистров;</li>
            <li><code>e</code> — <code>eepromCopyPage()</code>;</li>
            <li><code>r</code> — <code>softReset() + recall</code>.</li>
          </ul>
        </li>
      </ol>

      <h3 id="example-meteostation">2. <code>MTS4x_MeteoStation</code> — WiFi-метеостанция с максимальной точностью</h3>
      <p><strong>Путь:</strong> <code>examples/MTS4x_MeteoStation/MTS4x_MeteoStation.ino</code></p>
      <p>Функционал:</p>
      <ul>
        <li>ESP8266 / ESP32 подключается к вашей Wi-Fi сети;</li>
        <li>поднимается HTTP-сервер:
          <ul>
            <li><code>/</code> — веб-страница:
              <ul>
                <li>крупный вывод температуры;</li>
                <li>индикатор CRC по нескольким выборкам;</li>
                <li>IP устройства;</li>
                <li>паспортные характеристики датчика (диапазон, точность, разрешение);</li>
                <li>описание режима усреднения (<code>AVG_32</code> + N выборок + <code>TEMP_OFFSET_C</code>).</li>
              </ul>
            </li>
            <li><code>/json</code> — JSON-API:
              <ul>
                <li><code>temperature_c</code>, <code>crc_ok</code>;</li>
                <li><code>sensor</code>, <code>range_c</code>, <code>best_accuracy_c</code>,
                  <code>best_accuracy_range_c</code>, <code>resolution_c</code>;</li>
                <li><code>avg_mode</code>, <code>samples</code>, <code>temp_offset_c</code>;</li>
                <li><code>status</code> (если удалось прочитать статус).</li>
              </ul>
            </li>
          </ul>
        </li>
      </ul>
      <p><strong>Запуск:</strong></p>
      <ol>
        <li>Задайте свои данные Wi-Fi:
          <pre><code class="language-cpp">const char* WIFI_SSID     = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";</code></pre>
        </li>
        <li>Скомпилируйте и залейте скетч на ESP8266/ESP32.</li>
        <li>Откройте Serial Monitor — там будет IP.</li>
        <li>Зайдите в браузере:
          <ul>
            <li><code>http://IP/</code> — веб-интерфейс;</li>
            <li><code>http://IP/json</code> — JSON-ответ.</li>
          </ul>
        </li>
        <li>При необходимости подстройте <code>TEMP_OFFSET_C</code> для калибровки.</li>
      </ol>
    </section>

    <section id="recipes" class="section light">
      <div class="section-header">
        <h2>🔧 Рецепты использования (code snippets)</h2>
      </div>

      <h3 id="recipe-minimal">1. Минималка: только температура + CRC</h3>
      <p>См. <a href="#quickstart">«Быстрый старт»</a> — это именно этот сценарий.</p>

      <h3 id="recipe-thresholds">2. Пороги TH/TL и реле по аварии</h3>
      <pre><code class="language-cpp">#include &lt;Wire.h&gt;
#include "MTS4x.h"

MTS4X mts;
const int RELAY_PIN = 5; // GPIO5 (например D1 на ESP8266)

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(115200);
  delay(200);

  mts.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  mts.setConfig(MPS_1Hz, AVG_8, true);
  mts.setMode(MEASURE_STOP, false);

  // Пороги: ниже 5 °C и выше 40 °C считаем аварией
  mts.setHighLimit(40.0f);
  mts.setLowLimit(5.0f);
  mts.setAlertMode(true, ALERT_MODE_HIGH_TH_LOW_ALARM);
}

void loop() {
  float t;
  bool crcOk;
  uint8_t st = 0;

  mts.readTemperatureCrc(t, crcOk, true);
  mts.readStatus(st);

  bool alarm = (st &amp; (MTS4X_STATUS_ALERT_HIGH | MTS4X_STATUS_ALERT_LOW));

  digitalWrite(RELAY_PIN, alarm ? HIGH : LOW);

  Serial.print(F("T="));
  Serial.print(t, 3);
  Serial.print(F(" °C  alarm="));
  Serial.println(alarm ? F("ON") : F("OFF"));

  delay(1000);
}</code></pre>

      <h3 id="recipe-offset-user">3. Калибровка <code>TEMP_OFFSET_C</code> и хранение в user-регистре</h3>
      <pre><code class="language-cpp">// Сохранить калибровку в user-регистр и EEPROM
void saveOffsetToUser(float offsetC) {
  int8_t val = round(offsetC * 10.0f);   // 0.1 °C/шаг
  mts.writeUserRegister(0, (uint8_t)val);
  mts.eepromCopyPage(true, 50);          // сохраняем в EEPROM
}

// Прочитать калибровку при старте
float loadOffsetFromUser() {
  uint8_t raw;
  if (!mts.readUserRegister(0, raw)) return 0.0f;
  int8_t val = (int8_t)raw;
  return (float)val / 10.0f;
}</code></pre>
      <pre><code class="language-cpp">float g_offset = 0;

void setup() {
  ...
  if (!mts.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
    ...
  }
  g_offset = loadOffsetFromUser();
  Serial.print(F("Loaded offset: "));
  Serial.println(g_offset);
}

void loop() {
  float t;
  bool crcOk;
  mts.readTemperatureCrc(t, crcOk, true);
  t += g_offset;
  ...
}</code></pre>

      <h3 id="recipe-eeprom">4. Сохранение конфигурации в EEPROM</h3>
      <pre><code class="language-cpp">void configureAndStore() {
  // Настройки порогов, alert, user-регистров и т.п.
  mts.setHighLimit(40.0f);
  mts.setLowLimit(5.0f);
  mts.setAlertMode(true, ALERT_MODE_HIGH_TH_LOW_CLEAR);
  mts.writeUserRegister(0, 0x12);
  mts.writeUserRegister(1, 0x34);

  // Сохранить всё, что сейчас в scratch, в EEPROM:
  if (!mts.eepromCopyPage(true, 50)) {
    Serial.print(F("EEPROM copy failed, err="));
    Serial.println(mts.lastError());
  }
}

void restoreFromEepromOnStart() {
  if (!mts.eepromRecallPage(true, 50)) {
    Serial.print(F("EEPROM recall failed, err="));
    Serial.println(mts.lastError());
  }
}</code></pre>

      <h3 id="recipe-two-sensors">5. Два датчика с разными адресами</h3>
      <pre><code class="language-cpp">#include &lt;Wire.h&gt;
#include "MTS4x.h"

MTS4X sensorIndoor(0x41);
MTS4X sensorOutdoor(0x42);

void setup() {
  Serial.begin(115200);
  delay(200);

  sensorIndoor.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  sensorOutdoor.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  sensorIndoor.setConfig(MPS_1Hz, AVG_8, true);
  sensorOutdoor.setConfig(MPS_1Hz, AVG_8, true);

  sensorIndoor.setMode(MEASURE_STOP, false);
  sensorOutdoor.setMode(MEASURE_STOP, false);
}

void loop() {
  float tIn, tOut;
  bool crcIn, crcOut;

  sensorIndoor.readTemperatureCrc(tIn, crcIn, true);
  sensorOutdoor.readTemperatureCrc(tOut, crcOut, true);

  Serial.print(F("Indoor: "));
  Serial.print(tIn, 3);
  Serial.print(F(" °C  CRC="));
  Serial.println(crcIn ? F("OK") : F("FAIL"));

  Serial.print(F("Outdoor: "));
  Serial.print(tOut, 3);
  Serial.print(F(" °C  CRC="));
  Serial.println(crcOut ? F("OK") : F("FAIL"));

  Serial.println(F("----"));
  delay(1000);
}</code></pre>
      <p>С I²C-мультиплексором (TCA9548A) перед чтением каждого сенсора нужно выбрать соответствующий канал (отдельным I²C-запросом к мультиплексору).</p>

      <h3 id="recipe-continuous">6. Непрерывный режим (continuous) с заданной частотой</h3>
      <pre><code class="language-cpp">void setup() {
  ...
  mts.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // Допустим: MPS_8Hz + AVG_8
  mts.setConfig(MPS_8Hz, AVG_8, false);
  mts.setMode(MEASURE_CONTINUOUS, false);  // heater = false
}

void loop() {
  float t;
  bool crcOk;

  if (mts.readTemperatureCrc(t, crcOk, false)) {
    Serial.print(F("T="));
    Serial.print(t, 3);
    Serial.print(F(" °C, CRC="));
    Serial.println(crcOk ? F("OK") : F("FAIL"));
  } else {
    Serial.print(F("Error: "));
    Serial.println(mts.lastError());
  }

  delay(200);
}</code></pre>

      <h3 id="recipe-heater">7. Heater: самотест и прогрев от конденсата</h3>
      <pre><code class="language-cpp">void heaterSelfTest() {
  float tBefore, tAfter;
  bool crcOk;

  // Снять начальную температуру
  mts.readTemperatureCrc(tBefore, crcOk, true);

  // Включить heater
  mts.heaterOn();

  // Подождать, пока прогреется (например, 10 секунд)
  delay(10000);

  // Снять температуру ещё раз
  mts.readTemperatureCrc(tAfter, crcOk, true);

  // Выключить heater
  mts.heaterOff();

  Serial.print(F("Heater self-test: "));
  Serial.print(tBefore, 2);
  Serial.print(F(" -> "));
  Serial.print(tAfter, 2);
  Serial.println(F(" °C"));

  if (tAfter <= tBefore) {
    Serial.println(F("WARNING: temperature did not rise, check heater or environment."));
  }
}</code></pre>
    </section>

    <section id="api-overview" class="section light">
      <div class="section-header">
        <h2>🧠 API класса <code>MTS4X</code> (обзор)</h2>
      </div>
      <p>Краткий обзор основных методов (точные сигнатуры — в <code>MTS4x.h</code>).</p>

      <h3>Конструктор и инициализация</h3>
      <pre><code class="language-cpp">MTS4X(uint8_t address = 0x41, TwoWire &amp;wire = Wire);

bool begin(int32_t sda, int32_t scl);
bool begin(int32_t sda, int32_t scl, MeasurementMode mode);

void setBusClock(uint32_t hz);
uint32_t busClock() const;

int8_t lastError() const; // 0 = OK, &lt;0 = ошибка</code></pre>

      <h3>Режимы измерения и конфигурация</h3>
      <pre><code class="language-cpp">bool setMode(MeasurementMode mode, bool heater);
bool startSingleMessurement(); // алиас для single-shot старта

bool setConfig(TempCfgMPS mps, TempCfgAVG avg, bool sleep);</code></pre>

      <h3>Чтение температуры</h3>
      <pre><code class="language-cpp">float readTemperature(bool waitOnNewVal = true);
float readTemperatureC(bool waitOnNewVal = true);

bool readTemperature(float &amp;tC, bool waitOnNewVal = true);
bool readTemperatureRaw(int16_t &amp;raw, bool waitOnNewVal = true);
bool readTemperatureRawWithCrc(int16_t &amp;raw, bool &amp;crcOk,
                               bool waitOnNewVal = true);
bool readTemperatureCrc(float &amp;tC, bool &amp;crcOk,
                        bool waitOnNewVal = true);

bool singleShot(float &amp;tC); // удобная обёртка</code></pre>

      <h3>Status и heater</h3>
      <pre><code class="language-cpp">bool readStatus(uint8_t &amp;status);
bool isBusy(bool &amp;busy);
bool isBusy();

bool heaterOn();
bool heaterOff();
bool isHeaterOn(bool &amp;on);</code></pre>

      <h3>Пороги и алармы</h3>
      <pre><code class="language-cpp">bool setAlertMode(bool enable, MTS4xAlertMode mode);
bool getAlertMode(bool &amp;enable, MTS4xAlertMode &amp;mode);
bool readAlertRegister(uint8_t &amp;regValue);

bool setHighLimit(float tHighC);
bool setLowLimit(float tLowC);
bool getHighLimit(float &amp;tHighC);
bool getLowLimit(float &amp;tLowC);</code></pre>

      <h3>ID и ROM</h3>
      <pre><code class="language-cpp">bool readDeviceId(uint16_t &amp;id);
bool readRomCode(uint8_t rom[5]);</code></pre>

      <h3>User-регистры</h3>
      <pre><code class="language-cpp">bool readUserRegister(uint8_t index, uint8_t &amp;value);
bool writeUserRegister(uint8_t index, uint8_t value);</code></pre>

      <h3>Scratch и расширенный scratch (с CRC)</h3>
      <pre><code class="language-cpp">bool readScratch(uint8_t scratch[8], bool &amp;crcOk);
bool readScratchExt(uint8_t scratchExt[10], bool &amp;crcOk);</code></pre>

      <h3>EEPROM и сброс</h3>
      <pre><code class="language-cpp">bool eepromCopyPage(bool waitReady = true, uint32_t timeoutMs = 50);
bool eepromRecallPage(bool waitReady = true, uint32_t timeoutMs = 50);
bool eepromRecallAll(bool waitReady = true, uint32_t timeoutMs = 50);
bool eepromWritePageRaw(bool waitReady = true, uint32_t timeoutMs = 50);
bool softReset(bool waitReady = true, uint32_t timeoutMs = 50);
bool waitEepromReady(uint32_t timeoutMs = 50);</code></pre>

      <h3>Паразитное питание</h3>
      <pre><code class="language-cpp">bool setParasiticPower(bool enable);</code></pre>
    </section>

    <section id="max-precision" class="section light">
      <div class="section-header">
        <h2>🎯 Режим максимальной точности</h2>
      </div>
      <p>Для метеостанций и калибраторов можно максимально выжать точность датчика.</p>

      <h3>Настройки на чипе</h3>
      <ul>
        <li>Режим: <strong>single-shot</strong> — <code>MEASURE_STOP</code> + вызов <code>readTemperatureCrc()</code>.</li>
        <li>Максимальное аппаратное усреднение: <code>AVG_32</code>.</li>
        <li>Шина I²C: 100–400&nbsp;кГц (на точность напрямую почти не влияет).</li>
      </ul>

      <h3>Усреднение на микроконтроллере</h3>
      <ol>
        <li>Делаем <code>N</code> измерений (например, <code>N = 8…16</code>).</li>
        <li>Каждое:
          <ul>
            <li>читаем через <code>readTemperatureCrc()</code>;</li>
            <li>если CRC не OK — выборку можно игнорировать.</li>
          </ul>
        </li>
        <li>Считаем среднее по валидным выборкам.</li>
        <li>Флаг <code>crc_ok</code> можно трактовать как <code>true</code>, если все измерения прошли CRC.</li>
      </ol>

      <h3>Калибровка <code>TEMP_OFFSET_C</code></h3>
      <ol>
        <li>Ставим датчик рядом с эталонным термометром.</li>
        <li>Ждём 15–30 минут, пока всё стабилизируется.</li>
        <li>Измеряем:
          <ul>
            <li><code>T_real</code> — эталон;</li>
            <li><code>T_sensor</code> — MTS4x.</li>
          </ul>
        </li>
        <li>Вычисляем:
          <pre><code>TEMP_OFFSET_C = T_real - T_sensor</code></pre>
        </li>
        <li>Записываем <code>TEMP_OFFSET_C</code> в код или user-регистр.</li>
        <li>При желании — повторяем на нескольких температурах и выбираем среднее.</li>
      </ol>

      <h3>Монтаж для реальной метеостанции</h3>
      <ul>
        <li><strong>Радиационный экран:</strong> белые «тарелки» или метеошахта, чтобы убрать прямое солнце.</li>
        <li><strong>Обдув:</strong> небольшой вентилятор, чтобы датчик видел температуру воздуха, а не нагретого корпуса.</li>
        <li><strong>Удаление от тепловых источников:</strong> не ставим вплотную к ESP, DC-DC, силовым ключам.</li>
        <li><strong>Расположение на улице:</strong>
          <ul>
            <li>северная сторона здания;</li>
            <li>примерно 2&nbsp;м над землёй;</li>
            <li>подальше от стен, окон, кондиционеров и выхлопов.</li>
          </ul>
        </li>
      </ul>
    </section>

    <section id="practical" class="section light">
      <div class="section-header">
        <h2>🛠 Практические рекомендации</h2>
      </div>
      <ul>
        <li><strong>Частота I²C:</strong>
          <ul>
            <li>короткие проводники — можно 400&nbsp;кГц;</li>
            <li>длинные/шумные линии — лучше 100&nbsp;кГц.</li>
          </ul>
        </li>
        <li><strong>Питание:</strong>
          <ul>
            <li>у датчика малый ток, но шум по питанию от ESP / Wi-Fi может давать дрейф;</li>
            <li>ставьте 0.1&nbsp;µF рядом с датчиком, при длинных проводах — дополнительно 4.7–10&nbsp;µF.</li>
          </ul>
        </li>
        <li><strong>CRC:</strong>
          <ul>
            <li>для надёжных систем всегда используйте <code>readTemperatureCrc()</code>;</li>
            <li>ошибки CRC — повод проверить проводку и помехи.</li>
          </ul>
        </li>
        <li><strong>Таймауты EEPROM:</strong>
          <ul>
            <li>не ставьте <code>timeoutMs</code> &lt; 10–20&nbsp;мс без особой необходимости;</li>
            <li>значение по умолчанию (50&nbsp;мс) — безопасное.</li>
          </ul>
        </li>
        <li><strong>Автономные устройства:</strong>
          <ul>
            <li>single-shot + deep-sleep контроллера;</li>
            <li>редкие измерения, например раз в 10–60&nbsp;секунд.</li>
          </ul>
        </li>
      </ul>
    </section>

    <section id="layout-emc" class="section light">
      <div class="section-header">
        <h2>📐 Рекомендации по разводке и EMC (для «железа»)</h2>
      </div>
      <ul>
        <li><strong>Разводка на плате:</strong>
          <ul>
            <li>держите SDA и SCL короче и по возможности параллельно друг другу;</li>
            <li>избегайте трассировки рядом с силовыми дорожками (MOTOR, PWM, реле и т.п.);</li>
            <li>ложите под датчик «чистую» землю без токов силовых цепей.</li>
          </ul>
        </li>
        <li><strong>Кабели к внешнему датчику:</strong>
          <ul>
            <li>для длины &gt; 0.5&nbsp;м используйте витую пару (SDA+GND, SCL+GND);</li>
            <li>для особо шумной среды — экранированный кабель, экран сажаем на GND только со стороны контроллера;</li>
            <li>при проблемах с фронтами сигнала можно добавить последовательно 33–100&nbsp;Ω в разрыв SDA и SCL.</li>
          </ul>
        </li>
        <li><strong>Защита от статики/грязи:</strong>
          <ul>
            <li>если датчик вынесен наружу (длинный кабель) — имеет смысл TVS-диод по линии питания;</li>
            <li>не допускайте попадания воды/конденсата непосредственно на плату датчика (особенно на выводы).</li>
          </ul>
        </li>
        <li><strong>Размещение относительно ESP:</strong>
          <ul>
            <li>ESP8266/ESP32 греются сами по себе — не крепите датчик непосредственно на них;</li>
            <li>вынос 5–20&nbsp;см по кабелю уже заметно улучшает ситуацию.</li>
          </ul>
        </li>
      </ul>
    </section>

    <section id="troubleshooting" class="section light">
      <div class="section-header">
        <h2>🐞 Типичные ошибки и диагностика</h2>
      </div>
      <ol>
        <li><strong>«Ничего не читается» (NaN, ошибки I²C)</strong>
          <ul>
            <li>Проверьте адрес (0x41 по умолчанию, но бывает иначе).</li>
            <li>Проверьте наличие pull-up резисторов.</li>
            <li>Проверьте SDA/SCL/GND — нет ли перепутанных линий или обрыва.</li>
          </ul>
        </li>
        <li><strong>Скетч зависает при старте</strong>
          <ul>
            <li>I²C-конфликт (пины заняты другим модулем).</li>
            <li>Короткое между SDA/SCL и GND/VCC.</li>
          </ul>
        </li>
        <li><strong>CRC постоянно FAIL</strong>
          <ul>
            <li>слишком длинный или шумный кабель;</li>
            <li>плохой контакт, окисленные клеммы;</li>
            <li>слишком высокая частота I²C — попробуйте 100&nbsp;кГц.</li>
          </ul>
        </li>
        <li><strong>Температура «гуляет» при неподвижном датчике</strong>
          <ul>
            <li>самонагрев от платы контроллера;</li>
            <li>сквозняки/локальный обдув;</li>
            <li>слишком мало усреднения (поднять <code>AVG</code> или количество выборок).</li>
          </ul>
        </li>
        <li><strong>Резкий скачок при включении heater</strong>
          <ul>
            <li>это нормально — heater греет сам датчик;</li>
            <li>эти измерения не стоит воспринимать как температуру воздуха.</li>
          </ul>
        </li>
        <li><strong>После softReset «слетают» настройки</strong>
          <ul>
            <li>после настройки TH/TL/Alert/user-регистров нужно вызвать <code>eepromCopyPage()</code>;</li>
            <li>при старте — <code>eepromRecallPage()</code> или <code>softReset()</code> с последующим чтением.</li>
          </ul>
        </li>
      </ol>
    </section>

    <section id="eeprom" class="section light">
      <div class="section-header">
        <h2>💾 Работа с E²PROM</h2>
      </div>
      <p>Внутренняя E²PROM датчика хранит:</p>
      <ul>
        <li>пороги <code>TH/TL</code>;</li>
        <li>режим <code>Alert_Mode</code>;</li>
        <li>user-регистры;</li>
        <li>прочие конфигурационные данные.</li>
      </ul>
      <p>Типичный сценарий:</p>
      <ol>
        <li>В отладке подбираете пороги и настройки.</li>
        <li>Вызываете <code>eepromCopyPage(true, 50)</code> для сохранения.</li>
        <li>При старте устройства:
          <ul>
            <li><code>eepromRecallPage(true, 50)</code> или</li>
            <li><code>softReset(true, 50)</code>.</li>
          </ul>
        </li>
      </ol>
    </section>

    <section id="alerts" class="section light">
      <div class="section-header">
        <h2>🚨 Пороги и аварийные сигналы (Alert и Status)</h2>
      </div>
      <p>См. кодовый рецепт <a href="#recipe-thresholds">«Пороги TH/TL и реле по аварии»</a>.</p>
      <p>Применение:</p>
      <ul>
        <li><strong>Охранные/аварийные схемы:</strong>
          <code>Alert_Mode = HIGH_TH_LOW_ALARM</code> — всё вне диапазона TL–TH считается аварией.</li>
        <li><strong>Климат/комфорт:</strong>
          <code>Alert_Mode = HIGH_TH_LOW_CLEAR</code> — тревога только при превышении TH, сброс ниже TL.</li>
      </ul>
      <p>Пример чтения статуса:</p>
      <pre><code class="language-cpp">uint8_t st;
mts.readStatus(st);

if (st &amp; MTS4X_STATUS_ALERT_HIGH) {
  // Температура &gt; TH
}
if (st &amp; MTS4X_STATUS_ALERT_LOW) {
  // Температура &lt; TL
}</code></pre>
    </section>

    <section id="parasitic" class="section light">
      <div class="section-header">
        <h2>🔋 Паразитное питание</h2>
      </div>
      <p>Если аппаратная схема использует паразитное питание (без отдельного VCC), чип поддерживает специальный режим:</p>
      <pre><code class="language-cpp">mts.setParasiticPower(true);</code></pre>
      <p>Для обычной I²C-схемы с отдельными линиями VCC и GND этот режим обычно не нужен.</p>
    </section>

    <section id="changelog" class="section light">
      <div class="section-header">
        <h2>📜 Changelog</h2>
      </div>
      <h3>v2.0.0</h3>
      <ul>
        <li>Переписан драйвер под «чистый» Arduino-стиль (без жёсткой привязки к esp32-hal).</li>
        <li>Добавлена полная поддержка:
          <ul>
            <li><code>Status</code>, <code>Alert_Mode</code>, <code>TH/TL</code>;</li>
            <li>команд EEPROM (<code>copy</code>, <code>recall</code>, <code>write</code>, <code>soft reset</code>);</li>
            <li>областей <code>scratch</code> и <code>scratch_ext</code> с CRC;</li>
            <li>user-регистров, heater и паразитного питания;</li>
            <li>настройки частоты I²C-шины.</li>
          </ul>
        </li>
        <li>Добавлены примеры:
          <ul>
            <li><code>MTS4x_FullDemo</code> — демонстрация всех возможностей;</li>
            <li><code>MTS4x_MeteoStation</code> — Wi-Fi метеостанция с JSON-API и режимом максимальной точности.</li>
          </ul>
        </li>
      </ul>
    </section>

    <section id="license" class="section light">
      <div class="section-header">
        <h2>📄 License</h2>
      </div>
      <p>Рекомендуется использовать <strong>MIT License</strong> (можно адаптировать под ваши требования).</p>
      <pre><code class="language-text">MIT License
Copyright (c) 2025 Denis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

[...]</code></pre>
      <p class="muted">Полный текст MIT-лицензии добавьте в репозиторий при необходимости.</p>
    </section>

    <section id="english-summary" class="section light">
      <div class="section-header">
        <h2>🇬🇧 English summary (short)</h2>
      </div>
      <p><strong>MTS4x</strong> is a full-feature Arduino driver for the <strong>MTS4</strong> family of high-precision CMOS digital temperature sensors (MTS4 / MTS4Z / MTS4P / MTS4B, including the <strong>MTS4P+T4</strong> module).</p>
      <ul>
        <li>~−103…+153&nbsp;°C internal range, 16-bit output, <strong>0.004&nbsp;°C</strong> resolution.</li>
        <li>Single-shot and continuous measurement modes, configurable MPS &amp; AVG (up to <code>AVG_32</code>).</li>
        <li>CRC8 verification for temperature, scratch and extended scratch registers.</li>
        <li>High/low temperature limits (<code>TH</code> / <code>TL</code>) and <code>Alert_Mode</code> support.</li>
        <li>Status register with alarm flags, busy flags, heater state, etc.</li>
        <li>On-chip heater control (anti-condensation, self-test).</li>
        <li>User registers and EEPROM commands (copy/recall/write/soft reset).</li>
        <li>Optional parasitic power configuration.</li>
        <li>Works on <strong>ESP8266</strong>, <strong>ESP32</strong> and classic Arduino boards.</li>
      </ul>
      <p><strong>Examples:</strong></p>
      <ul>
        <li><code>MTS4x_FullDemo</code> — serial demo showing temperature/CRC, alerts, heater, user registers, EEPROM and scratch access.</li>
        <li><code>MTS4x_MeteoStation</code> — WiFi “meteostation” for ESP8266/ESP32 with:
          <ul>
            <li>a web UI (large temperature display, CRC indicator, sensor specs),</li>
            <li>a <code>/json</code> endpoint exposing:
              <code>temperature_c</code>, <code>crc_ok</code>, <code>sensor</code>, <code>range_c</code>,
              <code>best_accuracy_c</code>, <code>best_accuracy_range_c</code>, <code>resolution_c</code>,
              <code>avg_mode</code>, <code>samples</code>, <code>temp_offset_c</code>, <code>status</code>.</li>
          </ul>
        </li>
      </ul>
      <p>See the <strong>examples</strong> directory and the code snippets in this README for practical usage patterns.</p>
    </section>

    <footer>
      HTML-версия <strong>README.md</strong> для библиотеки <strong>MTS4x</strong>.  
      Можно сохранить этот текст как <code>README.md</code> (GitHub корректно отобразит HTML внутри)
      или как <code>README.html</code> для отдельной публикации.
    </footer>
  </div>
</body>
</html>
