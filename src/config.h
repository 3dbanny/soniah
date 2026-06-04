#pragma once
#include <Arduino.h>

// ===== PINS =====

#define brightnessPin  0 
#define positionOnepin 1 
#define positionTwopin 2 
#define voltmeterPin   3
#define redLightPin    4 
#define SdaPin         5
#define SclPin         6
#define whiteLightPin  7 

// ===== OLED =====

#define ScreenWidth  128
#define ScreenHeight 64
#define OledReset    -1

// ===== PWM =====

const int PWM_CHANNEL    = 0;
const int PWM_FREQ       = 2000;        // 2 kHz - максимум для LDO6AJSA
const int PWM_RESOLUTION = 8; 

// ===== DB KEYS =====

enum kk : size_t {
  wifiSsid,
  wifiPass,
  brightnessValuePosition1,
  brightnessValuePosition2,
  switchPosition1,
  switchPosition2,
  displayMode,
  themeColor,
  language,
  TimerSlider,
  apply
};

// ===== RUNTIME STATE =====
struct Data {
  int batteryChargePercent  = 0;
  bool wifiConnecting       = false;
  uint32_t wifiConnectStart = 0;
  bool timerActive          = false;
  uint32_t timerEndMillis   = 0;
  uint32_t timerDisplay     = 0;
};

// ===== POWER MANAGEMENT =====
struct PowerManagement {
  const float ADC_VOLTAGE_MULTIPLIER  = 3.3 * 1.8 / 4095.0;
  const float BATTERY_MAX_VOLTAGE     = 4.1; // максимальна напруга батареї
  const float BATTERY_MIN_VOLTAGE     = 3.2; // мінімальна напруга батар
  const int DIODE_DROP_MAH            = 50; // падіння напруги на діодах в мА
  const int BATTERY_CAPACITY_MAH      = 2800; // ємність батареї в мАг
  const int ESP32_CONSUMPTION_MAH     = 80; // середнє споживання ESP32 в мА
  const int MAX_LIGHT_CONSUMPTION_MAH = 200; // максимальне споживання ліхтаря в мА
  const int BRIGHTNESS_MULTIPLIER     = 255;
};

// ===== ROBO EYES =====
struct RoboEyesConfig {
  const uint8_t EYE_WIDTH     = 24;
  const uint8_t EYE_HEIGHT    = 24;
  const uint8_t BORDER_RADIUS = 8;
  const uint8_t SPACE_BETWEEN = 4;
};
// ===== TIMING =====
constexpr uint32_t BATTERY_UPDATE_MS  = 5UL * 60 * 1000;  // 5 minutes
constexpr uint32_t WIFI_TIMEOUT_MS    = 5000;
constexpr uint32_t SLEEP_DEBOUNCE_MS  = 5000;
constexpr uint32_t BOOT_RELOAD_MS     = 3000;
constexpr uint32_t EYE_ANIMATION_MS   = 3UL * 60 * 1000;  // 3 minutes

// ===== LOCALIZATION =====
struct Lang {
  const char* BATTERY[2]           = {"Battery charge",        "Заряд батареї"};
  const char* LIGHTSETTINGS[2]     = {"Flashlight",            "Ліхтарик"};
  const char* BRIGHTNESS[2]        = {"Brightness slider",     "Яскравість"};
  const char* SWITCHER1[2]         = {"Switcher 1",            "Перемикач 1"};
  const char* POSITION1[2]         = {"Position 1",            "Позиція перемикача 1"};
  const char* SWITCHER2[2]         = {"Switcher 2",            "Перемикач 2"};
  const char* POSITION2[2]         = {"Position 2",            "Позиція перемикача 2"};
  const char* SCREEN[2]            = {"Flashlight screen",     "Екран ліхтарика"};
  const char* DISPLAYMODE[2]       = {"Display mode",          "Інформація на екрані"};
  const char* TIMER[2]             = {"Timer",                 "Таймер"};
  const char* REMINING[2]          = {"Remaining time",        "Час, що залишився"};
  const char* SETTIME[2]           = {"Set time(min)",         "Встановити час(хв)"};
  const char* START[2]             = {"Start",                 "Старт"};
  const char* STOP[2]              = {"Stop",                  "Стоп"};
  const char* MAINSETTINGS[2]      = {"Main settings",         "Основні налаштування"};
  const char* WIFICOLORSETTINGS[2] = {"WIFI & theme settings", "Налаштування WIFI та теми"};
  const char* WIFI[2]              = {"WiFi",                  "WiFi"};
  const char* SSID[2]              = {"SSID",                  "Назва мережі"};
  const char* PASSWORD[2]          = {"Password",              "Пароль"};
  const char* THEMECOLOR[2]        = {"Theme color",           "Колір теми"};
  const char* LANGUAGE[2]          = {"Language",              "Мова"};
  const char* SAVEBUTTON[2]        = {"Save & restart",        "Зберегти та перезавантажити"};
};