# SONIAH Flashlight — Encoder Modification Plan

## 1. Hardware & pins
- Remove `positionOnepin` / `positionTwopin` (physical 3-pos switch gone)
- New mapping: `ENC_SW=1`, `ENC_CLK=2`, `ENC_DT=21`
- Add EncButton to `lib_deps` in `platformio.ini`

---

## 2. Data struct & global state
- Remove `wifiConnecting`, `wifiConnectStart` → move to separate WiFi manager
- Add `currentLightMode` (0 or 1), `wifiEnabled` (default false)
- Add `RTC_DATA_ATTR int wakeMode` to know short/long press after wakeup

---

## 3. WiFi management — extracted into `manageWifi()`
- On boot: WiFi fully OFF
- Turn on/off only via encoder gesture
- Removes the SoftAP/STA init from `setup()` into a dedicated function called on demand

---

## 4. Encoder gestures — `manageEncoder()`
- **Short click** → toggle light mode 1/2
- **Long press** → direct mode 2
- **Double click** → `enterDeepSleep(false)`
- **Rotate** → adjust brightness of current mode ±5%
- **Rotate right + hold** → WiFi ON
- **Rotate left + hold** → WiFi OFF

---

## 5. Deep sleep rework
- Single wakeup condition: GPIO1 HIGH (button press)
- No more `gpio_hold_en` dance for two pins
- On wakeup: read `wakeMode` from RTC — short click = mode1, long press = mode2
- `sleepByTimer` logic stays but simplified

---

## 6. `manageSwitcherPosition()` → replaced
- Now driven by `currentLightMode` software state instead of physical pin read
- `bindPositionLight()` and `adjustBrightness()` stay unchanged

---

## 7. Web interface `build()`
- Remove switcher position LEDs (no physical switch)
- Keep brightness sliders for both modes
- Add WiFi status indicator
- Remove `switchPosition1/2` DB keys (or keep for light color per mode)

---

## Discussion order
1. Deep sleep rework (most critical, affects everything)
2. Encoder setup + gestures
3. WiFi on-demand
4. Light mode switching
5. Web UI cleanup

---

## Notes
- WiFi starts fully OFF by default (energy saving)
- Wake from sleep: short click = mode 1, long press = mode 2
- EncButton library: https://github.com/GyverLibs/EncButton
- ESP32-C3 deep sleep wakeup only supported on GPIO 0–5 (RTC pins)
