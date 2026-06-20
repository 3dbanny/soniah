#include <Arduino.h>

// ===== WEB INTERFACE =====
#include <GyverDBFile.h>
#include <LittleFS.h>
#include <SettingsGyver.h>

// ===== HARDWARE DRIVERS =====
#include <esp_sleep.h>
#include "driver/gpio.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes72x40.h>

// ===== PROJECT MODULES =====
#include "config.h"
#include "images.h"
#include "display.h"
// ===== GLOBAL OBJECTS =====
GyverDBFile db(&LittleFS, "/data.db");
SettingsGyver sett("SONIAH", &db);
Adafruit_SSD1306 display(ScreenWidth, ScreenHeight, &Wire, OledReset); 
RoboEyes<Adafruit_SSD1306> roboEyes(display);

// ===== GLOBAL STATE =====
Data data;
Lang lng;
RoboEyesConfig roboeyesconfig;
PowerManagement powerManagement;
RTC_DATA_ATTR bool sleepByTimer = false;

// ===== WEB UI HELPER =====
// Maps battery percent to widget color. UI concern — lives near build/update.
sets::Colors batteryWidgetColorChange(int value) {
  if (value < 30) {
      return sets::Colors::Red;
  } else if (value < 70) {
      return sets::Colors::Yellow;
  } else {
      return sets::Colors::Green;
  }
}

// ===== WEB UI BUILD =====
// Note: build/update are SettingsGyver callbacks — they cannot receive
// custom parameters, so they access global state (data, db, lng) directly.
// This is an intentional exception to the no-globals rule.
void build(sets::Builder& b) {
  int lang = (int)db[kk::language];
  b.HTML("", "<style>span[style*='margin-top: 18px']{display:none!important;}</style>");
  b.Image(H(img), "", "/logo.avif");
  b.LinearGauge(H(batCharge), lng.BATTERY[lang], 0, 100, "", data.batteryChargePercent,batteryWidgetColorChange(data.batteryChargePercent));
  
  
  if (b.beginGroup(lng.SWITCHER1[lang])) {
      b.Slider(kk::brightnessValuePosition1, lng.BRIGHTNESS[lang], 0, 100,1);
      if (b.beginRow()) {
        b.LED(H(led1), lng.POSITION1[lang],1, sets::Colors::Yellow,sets::Colors::Red);
        b.Switch(kk::switchPosition1, "");
        b.LED(H(led2), "",0, sets::Colors::Yellow,sets::Colors::Red);
        b.endRow();
      }
      b.endGroup();
  }

  if (b.beginGroup(lng.SWITCHER2[lang])) {
    b.Slider(kk::brightnessValuePosition2, lng.BRIGHTNESS[lang], 0, 100,1);
    if (b.beginRow()) {
      b.LED(H(led3), lng.POSITION2[lang],1, sets::Colors::Yellow,sets::Colors::Red);
      b.Switch(kk::switchPosition2, "");
      b.LED(H(led4), "",0, sets::Colors::Yellow,sets::Colors::Red);
      b.endRow();
    }
  b.endGroup(); 
  }
  if (b.beginGroup(lng.SCREEN[lang])) {
  b.Select(kk::displayMode, lng.DISPLAYMODE[lang], "Battery Charge;Time to discharge;Robot Eyes;Battery image");
  b.endGroup(); 
  }
  if (b.beginMenu(lng.TIMER[lang])) {
    if (b.beginGroup(lng.TIMER[lang])) {
    b.Time(H(timerDisplay), lng.REMINING[lang]);
    b.Slider(kk::TimerSlider, lng.SETTIME[lang], 0, 59, 1);

    if (b.beginButtons()) {
      if (b.Button(H(btnStart), lng.START[lang])) {
        data.timerActive = true;
        data.timerEndMillis = millis() + (int)db[kk::TimerSlider] * 60 * 1000UL;
      }
      if (b.Button(H(btnStop), lng.STOP[lang], sets::Colors::Red)) {
        data.timerActive = false;
        data.timerDisplay = 0;
        data.timerEndMillis = 0; 
      }
    b.endButtons();
    }
    b.endGroup();
    }
  b.endMenu();
  }
  if (b.beginMenu(lng.MAINSETTINGS[lang])) {
    if (b.beginGroup(lng.WIFICOLORSETTINGS[lang])) {
      b.Input(kk::wifiSsid, lng.SSID[lang]);
      b.Pass(kk::wifiPass, lng.PASSWORD[lang]);
      b.Label("IP", WiFi.localIP().toString());
      b.Select(kk::themeColor, lng.THEMECOLOR[lang], "Green;Red;Blue;Yellow;Mint;Orange;Pink;Aqua;Violet");  // ← додати
      if (b.Button(kk::apply, lng.SAVEBUTTON[lang])) {
        db.update();
        ESP.restart();
        
      }
    b.endGroup();
    }
    if (b.beginGroup(lng.LANGUAGE[lang])) {
      b.Select(kk::language, lng.LANGUAGE[lang], "English;Українська"); 
      if (b.build.id == kk::language) {
        lang = (int)db[kk::language];
        b.reload();
      }

    }
    b.endGroup();
  b.endMenu();  
  }
}

void update(sets::Updater& u) {
  u.update(H(batCharge), data.batteryChargePercent);
  u.updateColor(H(batCharge), batteryWidgetColorChange(data.batteryChargePercent));
  /*data.timerDisplay = manageTimer();*/
  u.update(H(timerDisplay), data.timerDisplay);
}

/*NOT NORMALIZED*/

/*=========================ФУНКЦІЇ===========================*/
/*перевірка і вхід у режим глибокого сну*/
void enterDeepSleep(bool byTimer) {
  digitalWrite(redLightPin, LOW);
  digitalWrite(whiteLightPin, LOW);
  ledcWrite(PWM_CHANNEL, 0);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);

  display.clearDisplay();
  display.drawBitmap(38, 6, sleep_image, 30, 30, 1);
  display.drawCircle(67, 6, 2, 1);
  display.drawCircle(76, 17, 2, 1);
  display.drawCircle(77, 32, 2, 1);
  display.drawCircle(91, 11, 2, 1);
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();

  if (byTimer) {
  if (digitalRead(positionOnepin) == HIGH) {
    gpio_pullup_dis(GPIO_NUM_1);
    gpio_pulldown_en(GPIO_NUM_1);
    gpio_hold_en(GPIO_NUM_1);
    esp_deep_sleep_enable_gpio_wakeup(
      1ULL << positionOnepin,
      ESP_GPIO_WAKEUP_GPIO_LOW
    );
  } else {
    gpio_pullup_dis(GPIO_NUM_2);
    gpio_pulldown_en(GPIO_NUM_2);
    gpio_hold_en(GPIO_NUM_2);
    esp_deep_sleep_enable_gpio_wakeup(
      1ULL << positionTwopin,
      ESP_GPIO_WAKEUP_GPIO_LOW
    );
  }
  } else {
    // звичайний режим - засинаємо по HIGH на обох пінах
    esp_deep_sleep_enable_gpio_wakeup(
      1ULL << positionOnepin | 1ULL << positionTwopin,
      ESP_GPIO_WAKEUP_GPIO_HIGH
    );
  }
  esp_deep_sleep_start();
}
void checkButtonsForDeepSleep() {
  static uint32_t offTimer = 0;
  const unsigned long DEBOUNCE_TIME = 5000;

  if (digitalRead(positionOnepin) == LOW && digitalRead(positionTwopin) == LOW) {
    if (offTimer == 0) offTimer = millis();
    if (millis() - offTimer >= DEBOUNCE_TIME) {
      enterDeepSleep(false);
    }
  } else {
    offTimer = 0;
  }
}
/*процедура таймеру*/
uint32_t manageTimer() {
  if (!data.timerActive) return 0;

  uint32_t now = millis();
  if (now >= data.timerEndMillis) {
    data.timerActive = false;
    return 0;
  }
  return (data.timerEndMillis - now) / 1000;
}
/*визначення заряду батареї у відсотках*/
int batCharge(uint8_t pin) {
    static float emaValue = 0;
    static bool initialized = false;
    
    // Усереднення 5 вимірів
    const int NUM_SAMPLES = 5;
    int sum = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += analogRead(pin);
        delayMicroseconds(100);  // мікросекунди для швидкості
    }
    int avgRawValue = sum / NUM_SAMPLES;
    
    // Ініціалізація EMA
    if (!initialized) {
        emaValue = avgRawValue;
        initialized = true;
    }
    
    // Експоненційне згладжування
    const float ALPHA = 0.15;  // 15% нове, 85% старе
    emaValue = ALPHA * avgRawValue + (1.0 - ALPHA) * emaValue;
    
    // Розрахунок напруги і відсотків
    float voltage = emaValue * powerManagement.ADC_VOLTAGE_MULTIPLIER;
    float percentFloat = ((voltage - powerManagement.BATTERY_MIN_VOLTAGE) / 
                          (powerManagement.BATTERY_MAX_VOLTAGE - powerManagement.BATTERY_MIN_VOLTAGE)) * 100.0;
    
    return constrain((int)percentFloat, 0, 100);
}

/*розрахунок кількості часу, що залишився до розрядки батареї*/
int estimationTimeHours(int chargePercent,int brightnessLevel) {
  int estimatedHours = 0;
  int totalConsumptionMah = brightnessLevel * powerManagement.MAX_LIGHT_CONSUMPTION_MAH / 100 + powerManagement.ESP32_CONSUMPTION_MAH + powerManagement.DIODE_DROP_MAH; // розрахунок споживання ліхтаря в мА в залежності від яскравості
  estimatedHours = (chargePercent * powerManagement.BATTERY_CAPACITY_MAH / 100 / totalConsumptionMah);  //приблизний розрахунок часу до розрядки батареї в годинах
  return estimatedHours;
}
/*керування вибором кольору світла*/
void bindPositionLight(int position) {
  if (position == 0) {
      digitalWrite(redLightPin, HIGH);
      digitalWrite(whiteLightPin, LOW);
  }
  if (position == 1) {
      digitalWrite(redLightPin, LOW);
      digitalWrite(whiteLightPin, HIGH);
  }
}
/*регулювання яскравості світла*/
void adjustBrightness(int brightnessValue) {
  int pwmValue = brightnessValue * powerManagement.BRIGHTNESS_MULTIPLIER / 100;
  ledcWrite(PWM_CHANNEL, pwmValue);  // ← замість analogWrite()
}


/*вибір рандомного числа*/
int getRandomNumber(int maxValue) {
  return random(1, maxValue + 1);
}
/*анімація очей робота*/

/*мапінг режимів перемикача та кольорів ліхтаря + регулювання яскравості*/
void manageSwitcherPosition() {
  static int lastBrightnessPosition1 = -1;
  static int lastBrightnessPosition2 = -1;
  static int lastSwitcherState = -1;

  bool currentState = digitalRead(positionTwopin);
  bool switcherChanged = (currentState != lastSwitcherState);
  lastSwitcherState = currentState;  

  if (currentState == LOW) {
      bindPositionLight(db[kk::switchPosition1]);
      if ((int)db[kk::brightnessValuePosition1] != lastBrightnessPosition1 || switcherChanged) {
          lastBrightnessPosition1 = (int)db[kk::brightnessValuePosition1];
          adjustBrightness(lastBrightnessPosition1);
      }
  } 
  if (currentState == HIGH) {
      bindPositionLight(db[kk::switchPosition2]);
      if ((int)db[kk::brightnessValuePosition2] != lastBrightnessPosition2 || switcherChanged) {
          lastBrightnessPosition2 = (int)db[kk::brightnessValuePosition2];
          adjustBrightness(lastBrightnessPosition2);
      }
  }
}

void setup() {
  Serial.begin(115200); // 115200 baud rate

  gpio_hold_dis(GPIO_NUM_1);
  gpio_hold_dis(GPIO_NUM_2);  
  
  pinMode(voltmeterPin, INPUT);
  pinMode(positionOnepin, INPUT_PULLDOWN);
  pinMode(positionTwopin, INPUT_PULLDOWN);
  pinMode(redLightPin, OUTPUT);
  pinMode(whiteLightPin, OUTPUT);
  
  if (sleepByTimer) {
  sleepByTimer = false;
  esp_deep_sleep_enable_gpio_wakeup(
    1ULL << positionOnepin | 1ULL << positionTwopin,
    ESP_GPIO_WAKEUP_GPIO_HIGH
  );
  esp_deep_sleep_start();
  }
  

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(brightnessPin, PWM_CHANNEL);
  //увімкнути/вимкнути файловий менеджер 
  sett.config.useFS = true;

  data.batteryChargePercent = batCharge(voltmeterPin);// перший раз отримуэмо значення заряду батареї. Наступний раз буде через 5 хвилин
  // ======== WIFI ========
  // STA
  WiFi.mode(WIFI_AP_STA);
  // ======== SETTINGS ========
  sett.begin(true,"soniah"); // базу даних підключаємо до підключення до точки
  sett.setVersion("1.25");
  sett.onBuild(build);
  sett.onUpdate(update);
  // ======== DATABASE ========
  #ifdef ESP32
    LittleFS.begin(true);
  #else
    LittleFS.begin();
  #endif

  db.begin();

  File existing = LittleFS.open("/logo.avif", "r");
  bool needWrite = !existing || existing.size() != logo_avif_len;
  if (existing) existing.close();

  if (needWrite) {
    File f = LittleFS.open("/logo.avif", "w");
    if (f) {
        f.write(logo_avif, logo_avif_len);
        f.close();
        Serial.println("Logo updated");
    } else {
        Serial.println("Logo write failed");
    }
  } else {
    Serial.println("Logo OK, skip write");
  }

  // ініціювання БД початковими даними
    db.init(kk::wifiSsid, "");
    db.init(kk::wifiPass, "");
    db.init(kk::brightnessValuePosition1, 100);
    db.init(kk::brightnessValuePosition2, 100);
    db.init(kk::switchPosition1, 0);
    db.init(kk::switchPosition2, 1);
    db.init(kk::displayMode, 2);
    db.init(kk::themeColor, 0);
    db.init(kk::language, 0); // 0 = English за замовчуванням
    db.init(kk::TimerSlider, 0);


  //налаштування теми
  const sets::Colors themes[] = {
    sets::Colors::Green, sets::Colors::Red, sets::Colors::Blue,
    sets::Colors::Yellow, sets::Colors::Mint, sets::Colors::Orange,
    sets::Colors::Pink, sets::Colors::Aqua, sets::Colors::Violet
  };
  sett.config.theme = themes[(int)db[kk::themeColor]];
  // ======= AP =======
  WiFi.softAP("soniahsf","soniahsf");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // ======= STA =======
  // один раз пробуємо підключитись до WiFi якщо є налаштування
if (db[kk::wifiSsid].length()) {
    Serial.print("WiFi → ");
    Serial.println(db[kk::wifiSsid]);
    WiFi.begin(db[kk::wifiSsid], db[kk::wifiPass]);
    data.wifiConnecting = true;
    data.wifiConnectStart = millis();
}

  /*=======================SETUP OLED========================================*/
  Wire.begin(SdaPin, SclPin);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.setRotation(2);
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  /*=======================SETUP robo eyes========================================*/
  roboEyes.begin(ScreenWidth, ScreenHeight, 100);
  // Define some automated eyes behaviour
  roboEyes.setAutoblinker(ON, 3, 2); // Start auto blinker animation cycle -> bool active, int interval, int variation -> turn on/off, set interval between each blink in full seconds, set range for random interval variation in full seconds
  // Define eye shapes, all values in pixels
  roboEyes.setWidth(roboeyesconfig.EYE_WIDTH, roboeyesconfig.EYE_WIDTH);
  roboEyes.setHeight(roboeyesconfig.EYE_HEIGHT, roboeyesconfig.EYE_HEIGHT);
  roboEyes.setBorderradius(roboeyesconfig.BORDER_RADIUS, roboeyesconfig.BORDER_RADIUS);
  roboEyes.setSpacebetween(roboeyesconfig.SPACE_BETWEEN);
  //roboEyes.setHFlicker(ON, 2); // horizontal flickering effect -> bool active, int intensity (1-5)
  roboEyes.setPosition(NE); // cardinal directions, can be N, NE, E, SE, S, SW, W, NW, DEFAULT (default = horizontally and vertically centered)
  //вітальна фраза на олед дисплеї
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(45,10);//перший координат - по горизонталі, другий - по вертикалі 
  display.println("Hi");
  display.display();
} 


void loop() {
    if (data.wifiConnecting) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WiFi: " + WiFi.localIP().toString());
            data.wifiConnecting = false;
        } else if (millis() - data.wifiConnectStart > 5000) {
            Serial.println("WiFi timeout");
            WiFi.disconnect(true);
            data.wifiConnecting = false;
            
        }
    }
  checkButtonsForDeepSleep();
  /*одноразове перезавантаження сторінки сайту*/
  static bool reloadDone = false;
  static uint32_t reloadTimer = 0;
  if (!reloadDone) {
    if (reloadTimer == 0) reloadTimer = millis();
    if (millis() - reloadTimer > 3000) {  // чекаємо 3 секунди
      sett.reload(true);
      reloadDone = true;
    }
  }
  /*======================battery charge manager===================*/
  static uint32_t tmrBattery;
  const unsigned long BATTERY_CHARGE_INTERVAL = 5 * 60 * 1000; // інтервал оновлення заряду батареї в мілісекундах (5 хвилин)
  if (millis() - tmrBattery >= BATTERY_CHARGE_INTERVAL) { // кожні 5 хвилин змінюємо анімацію
  data.batteryChargePercent = batCharge(voltmeterPin);
  //data.batteryChargePercent = random(0,101); //тестове значення заряду батареї
  tmrBattery = millis();
  }
  /*======================switcher position manager + BRIGHTNESS===================*/
  manageSwitcherPosition();

/*вибір режиму відображення на OLED дисплеї*/
  if (data.timerActive) {
    data.timerDisplay = manageTimer();
    displayTimerCountdown(display, data.timerDisplay);
} else {
  switch ((int)db[kk::displayMode]) {
    case 0: displayChargeLevel(display, data.batteryChargePercent); break;
    case 1: displayEstimationTime(display, estimationTimeHours(
              data.batteryChargePercent,
              digitalRead(positionTwopin) == LOW ?
                (int)db[kk::brightnessValuePosition1] :
                (int)db[kk::brightnessValuePosition2])); break;
    case 2: displayRoboEyesAnimation(roboEyes); break;
    case 3: displayChargeBatteryImage(display, data.batteryChargePercent); break;
  }
}

/*перевірка завершення таймера*/
if (!data.timerActive && data.timerDisplay == 0 && data.timerEndMillis > 0) {
  enterDeepSleep(true);
}

  sett.tick();
}


