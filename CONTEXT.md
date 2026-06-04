# SONIAH Smart Flashlight — Project Context

## Description
ESP32-C3 smart flashlight with WiFi web interface, OLED display, sleep timer and EN/UA localization.

---

## Hardware
- **MCU:** ESP32-C3
- **Display:** OLED SSD1306 128x64
- **Battery:** 2800mAh
- **Pins:**
  - VOLTMETER_PIN    = 3
  - POSITION_ONE_PIN = 1
  - POSITION_TWO_PIN = 2
  - RED_LIGHT_PIN    = 4
  - WHITE_LIGHT_PIN  = 7
  - BRIGHTNESS_PIN   = 0
  - SDA_PIN = 5, SCL_PIN = 6
- **PWM:** channel=0, freq=2000Hz, resolution=8bit

---

## Libraries
- GyverDBFile, SettingsGyver
- LittleFS
- Adafruit_SSD1306, Adafruit_GFX
- FluxGarage_RoboEyes72x40
- driver/gpio.h, esp_sleep.h

---

## DB Keys (enum kk)
```cpp
enum kk : size_t {
  wifiSsid, wifiPass,
  brightnessValuePosition1, brightnessValuePosition2,
  switchPosition1, switchPosition2,
  displayMode, themeColor, language,
  TimerSlider, apply
};
```

---

## Timing Constants (config.h)
```cpp
constexpr uint32_t BATTERY_UPDATE_MS = 5UL * 60 * 1000;  // 5 minutes
constexpr uint32_t WIFI_TIMEOUT_MS   = 5000;
constexpr uint32_t SLEEP_DEBOUNCE_MS = 5000;
constexpr uint32_t BOOT_RELOAD_MS    = 3000;
constexpr uint32_t EYE_ANIMATION_MS  = 3UL * 60 * 1000;  // 3 minutes
```

---

## Structs
```cpp
RTC_DATA_ATTR bool sleepByTimer = false;

struct Data {
  int      batteryChargePercent = 0;
  bool     wifiConnecting       = false;
  uint32_t wifiConnectStart     = 0;
  bool     timerActive          = false;
  uint32_t timerEndMillis       = 0;
  uint32_t timerDisplay         = 0;
};

struct Lang             { /* 2-element EN/UA arrays for all UI strings */ };
struct PowerManagement  { /* power/battery constants */ };
struct RoboEyesConfig   { EYE_WIDTH=24, EYE_HEIGHT=24, BORDER_RADIUS=8, SPACE_BETWEEN=4 };
```

---

## Features
- WiFi web interface (SettingsGyver/GyverDB)
- Two light modes with independent brightness and color
- OLED display — 4 modes:
  - 0: battery charge percent
  - 1: estimated time to discharge
  - 2: robot eyes animation (RoboEyes)
  - 3: battery bar graphic
- Auto-off timer with OLED countdown (MM:SS)
- Deep sleep on switch off (5s debounce)
- Deep sleep on timer expiry
- EN/UA localization
- 9 UI themes: Green/Red/Blue/Yellow/Mint/Orange/Pink/Aqua/Violet
- WiFi SoftAP: "soniahsf" / "soniahsf"
- Version: 1.25

---

## Solved Problems & Key Decisions

### 1. Deep sleep by timer — final solution
Problem: ESP32-C3 automatically enables pull-up during `esp_deep_sleep_start()`, preventing the pin from going LOW.

Solution — `gpio_hold_en` locks resistor state before deep sleep overwrites it:
```cpp
// in enterDeepSleep() when byTimer=true:
if (digitalRead(POSITION_ONE_PIN) == HIGH) {
  gpio_pullup_dis(GPIO_NUM_1);
  gpio_pulldown_en(GPIO_NUM_1);
  gpio_hold_en(GPIO_NUM_1);  // ← critical
  esp_deep_sleep_enable_gpio_wakeup(1ULL << POSITION_ONE_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
} else {
  gpio_pullup_dis(GPIO_NUM_2);
  gpio_pulldown_en(GPIO_NUM_2);
  gpio_hold_en(GPIO_NUM_2);  // ← critical
  esp_deep_sleep_enable_gpio_wakeup(1ULL << POSITION_TWO_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
}
esp_deep_sleep_start();
```

After wakeup in `setup()` — release the hold:
```cpp
gpio_hold_dis(GPIO_NUM_1);
gpio_hold_dis(GPIO_NUM_2);
```

After wakeup on LOW — go back to sleep waiting for HIGH (normal mode):
```cpp
if (sleepByTimer) {
  sleepByTimer = false;
  esp_deep_sleep_enable_gpio_wakeup(
    1ULL << POSITION_ONE_PIN | 1ULL << POSITION_TWO_PIN,
    ESP_GPIO_WAKEUP_GPIO_HIGH
  );
  esp_deep_sleep_start();
}
```

### 2. Timer OLED display — update lag
Problem: `timerDisplay` was updated via `update()` which depends on WiFi activity.

Solution — update directly in `loop()`:
```cpp
if (data.timerActive) {
  data.timerDisplay = manageTimer(data);  // ← here, not in update()
  displayTimerCountdown(display, data.timerDisplay);
}
```

### 3. Timer Stop button — timerEndMillis not reset
```cpp
// Stop button — must reset timerEndMillis
data.timerActive    = false;
data.timerDisplay   = 0;
data.timerEndMillis = 0;  // ← critical, otherwise enterDeepSleep fires
```

### 4. Timer OLED display — cursor offset
Cursor shifted right so text is not clipped on left edge:
```cpp
display.setCursor(28, 10);  // was 20
```

### 5. WiFi SoftAP
Password minimum 8 characters, otherwise AP is not created:
```cpp
WiFi.softAP("soniahsf", "soniahsf");  // ← 8 chars minimum
```

### 6. UI Theme
Applied only after restart — must be set in `setup()` after `db.begin()`:
```cpp
sett.config.theme = themes[(int)db[kk::themeColor]];
```

### 7. Hide "Powered by" — CSS
```cpp
b.HTML("", "<style>span[style*='margin-top: 18px']{display:none!important;}</style>");
```

### 8. Logo.avif — write to LittleFS
Check by file size to avoid rewriting every boot:
```cpp
File existing = LittleFS.open("/logo.avif", "r");
bool needWrite = !existing || existing.size() != logo_avif_len;
if (existing) existing.close();
```

### 9. displayChargeBatteryImage — bug fix
Original code had a duplicate `> 25` condition — the 1-segment case never triggered.
Fixed to `> 10`:
```cpp
if      (percent > 75) { seg[0]=1; seg[1]=1; seg[2]=1; seg[3]=1; }
else if (percent > 50) { seg[0]=1; seg[1]=1; seg[2]=1; }
else if (percent > 25) { seg[0]=1; seg[1]=1; }
else if (percent > 10) { seg[0]=1; }  // ← was duplicate > 25
```

### 10. build() / update() globals exception
SettingsGyver callbacks cannot receive custom parameters.
`build()` and `update()` access `data`, `db`, `lng` globals directly.
This is a documented intentional exception to the no-globals rule.

---

## ESP32-C3 Constraints

- `esp_sleep_enable_ext1_wakeup` — not supported
- `esp_deep_sleep_enable_ext0_wakeup` — not supported
- `gpio_wakeup_enable` + `esp_sleep_enable_gpio_wakeup` — Light-sleep only
- `esp_deep_sleep_enable_gpio_wakeup` — GPIO 0–5 only (RTC pins)
- Deep sleep auto-enables pull-up on pins — bypass with `gpio_hold_en`
- Wakeup supports only `ESP_GPIO_WAKEUP_GPIO_LOW` or `ESP_GPIO_WAKEUP_GPIO_HIGH`

---

## Project Structure
```
src/
  main.cpp    — globals, web callbacks build()/update(), setup(), loop()
  config.h    — all pins, constants, enums, structs
  battery.h   — batCharge(), estimationTimeHours()
  light.h     — bindPositionLight(), adjustBrightness(), manageSwitcherPosition()
  display.h   — all OLED rendering functions
  sleep.h     — enterDeepSleep(), checkButtonsForDeepSleep()
  timer.h     — manageTimer()
  images.h    — PROGMEM arrays: logo_avif[], sleep_image[], image_clock_bits[]
data/
  logo.avif
  favicon.svg
platformio.ini  — board_build.filesystem = littlefs
CONTEXT.md      — this file
CLAUDE.md       — clean code and style rules
plan.md         — encoder modification plan
```

---

