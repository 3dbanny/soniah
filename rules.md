# SONIAH Flashlight — Claude Rules

## Clean Code Rules

### 1. Functions must not access global variables directly
Functions receive what they need via **parameters** and return results. No hidden dependencies.
```cpp
// ❌ Bad
int batCharge() { return analogRead(voltmeterPin); }

// ✅ Good
int batCharge(uint8_t pin, const PowerManagement& pm) { ... }
```

### 2. One function — one responsibility
If a function name needs "and" to describe it, split it.
```cpp
// ❌ Bad
void updateDisplayAndCheckTimer() { ... }

// ✅ Good
void updateDisplay();
void checkTimer();
```

### 3. No magic numbers — use named constants
```cpp
// ❌ Bad
if (millis() - t > 5000) { ... }

// ✅ Good
constexpr uint32_t DEBOUNCE_MS = 5000;
if (millis() - t > DEBOUNCE_MS) { ... }
```

### 4. Functions return results, don't write to globals
```cpp
// ❌ Bad
void manageTimer() { data.timerDisplay = ...; }

// ✅ Good
uint32_t manageTimer(Data& data) { return ...; }
```

### 5. Headers declare, source files define
Each `.h` file contains only declarations and constants — no implementation
(except small `inline` helpers).

### 6. Related things live together
Group by **domain**, not by type:
```
display.h     — all OLED logic
battery.h     — voltage reading, estimation
light.h       — PWM, color binding
sleep.h       — deep sleep logic
encoder.h     — encoder + gestures
wifi_mgr.h    — WiFi on/off management
```

### 7. Structs own their data, functions operate on them
```cpp
// ✅ Pass by reference, don't touch globals
void updateBattery(Data& data, uint8_t pin);
```

### 8. No side effects in display functions
Display functions only **render** — they never modify state.

### 9. setup() and loop() stay thin
They only **orchestrate** — call managers, never contain logic themselves.

### 10. Separate decision from action
Functions that check a condition should return a result, not call the next step.
```cpp
// ❌ Bad — decision and action coupled
void checkButtonsForDeepSleep(Display& d) {
  if (bothLow && timeout) enterDeepSleep(false, d);
}

// ✅ Good — decision returns bool, action lives in loop()
bool shouldSleep() { return bothLow && timeout; }
// in loop():
if (shouldSleep()) enterDeepSleep(false, display);
```

### 11. Intentional exceptions must be documented
When a rule must be broken (e.g. library callback constraints), explain why with a comment.
```cpp
// Note: build/update are SettingsGyver callbacks — they cannot receive
// custom parameters, so they access global state directly.
// This is an intentional exception to the no-globals rule.
void build(sets::Builder& b) { ... }
```

---

## Code Style Rules

### S1. File guard
Every header starts with `#pragma once`.

### S2. Include order
```cpp
#include <Arduino.h>          // 1. Arduino core
#include <SomeLibrary.h>      // 2. Third-party libraries (angle brackets)
#include "config.h"           // 3. Project headers (quotes)
```

### S3. Constants — `constexpr` over `#define`
Use `#define` only for pins and screen dimensions (untyped integer literals).
Use `constexpr` for everything else.
```cpp
#define VOLTMETER_PIN  3          // ✅ pin — #define ok
constexpr uint32_t TIMEOUT = 5000; // ✅ typed constant — constexpr
```

### S4. Naming conventions
| Kind | Style | Example |
|---|---|---|
| Constants / pins | `SCREAMING_SNAKE_CASE` | `SLEEP_DEBOUNCE_MS` |
| Functions | `camelCase` | `batCharge()` |
| Variables | `camelCase` | `offTimer` |
| Structs / classes | `PascalCase` | `PowerManagement` |
| Enum values | `camelCase` | `brightnessValuePosition1` |
| Local constexpr | `SCREAMING_SNAKE_CASE` | `constexpr int NUM_SAMPLES = 5` |

### S5. Casts
Use `static_cast<>` — never C-style casts.
```cpp
// ❌ Bad
float x = (float)sum / n;

// ✅ Good
float x = static_cast<float>(sum) / n;
```

### S6. Struct parameters — always const reference
```cpp
void adjustBrightness(int percent, const PowerManagement& pm);
```

### S7. Early return for guard clauses
```cpp
void displayChargeLevel(Adafruit_SSD1306& display, int percent) {
  static uint32_t tmr = 0;
  if (millis() - tmr < 1000) return;  // ← guard first
  tmr = millis();
  // ... actual logic
}
```

### S8. Static local variables for persistent state
```cpp
void someFunction() {
  static uint32_t tmr = 0;  // persists between calls
  static bool initialized = false;
}
```

### S9. Aligned assignments in related groups
```cpp
// ✅ Align = signs in struct members and related variable groups
struct Data {
  int      batteryChargePercent = 0;
  bool     wifiConnecting       = false;
  uint32_t timerEndMillis       = 0;
};
```

### S10. Section separators in main.cpp
```cpp
// ===== SECTION NAME =====
```

### S11. Function comments
One or two lines above the function, describing what it does and any key constraints.
```cpp
// Returns battery charge percent (0–100).
// Applies 5-sample averaging + EMA smoothing.
int batCharge(uint8_t pin, const PowerManagement& pm) { ... }
```

### S12. Inline notation
Use `// ←` for critical inline notes. Use `→` in doc comments for flow.
```cpp
gpio_hold_en(GPIO_NUM_1);  // ← critical: locks state before deep sleep
// byTimer=true → wakes when switch pin goes LOW
```

### S13. F() macro for Serial strings
```cpp
Serial.println(F("SSD1306 allocation failed"));  // saves RAM
```

### S14. Brace style — K&R
Opening brace on the same line, always use braces even for single-line bodies.
```cpp
if (condition) {
  doSomething();
}
```

### S15. Universal project structure
main.cpp
.h
.h
.h
