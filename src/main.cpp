#include <Arduino.h>
#include <esp_sleep.h>
/*налаштування веб інтерфейсу*/
#include <GyverDBFile.h>
#include <LittleFS.h>
GyverDBFile db(&LittleFS, "/data.db");
#include <SettingsGyver.h>
SettingsGyver sett("SONIAH", &db);
/*налаштування ключів БД*/
enum kk : size_t {
  wifiSsid,
  wifiPass,
  brightnessValue,
  switchPosition1,
  switchPosition2,
  displayMode,
  apply
};
/*глобальні змінні без потреби у постійному зберігання в енергонезалежній пам'яті*/
struct Data {
  int batteryChargePercent = 0;
};
Data data;
/*структура конфігурації розміру очей робота*/
struct RoboEyesConfig {
  const uint8_t EYE_WIDTH = 24;
  const uint8_t EYE_HEIGHT = 24;
  const uint8_t BORDER_RADIUS = 8;
  const uint8_t SPACE_BETWEEN = 4;
};

  RoboEyesConfig roboeyesconfig;
/*структура для управління живленням*/
struct PowerManagement {
  const float ADC_VOLTAGE_MULTIPLIER = 3.3 * 2.0 / 4095.0;
  const float BATTERY_MAX_VOLTAGE = 4.1; // максимальна напруга батареї
  const float BATTERY_MIN_VOLTAGE = 3.2; // мінімальна напруга батар
  const int DIODE_DROP_MAH = 50; // падіння напруги на діодах в мА
  const int BATTERY_CAPACITY_MAH = 2800; // ємність батареї в мАг
  const int ESP32_CONSUMPTION_MAH = 80; // середнє споживання ESP32 в мА
  const int MAX_LIGHT_CONSUMPTION_MAH = 200; // максимальне споживання ліхтаря в мА
  const int BRIGHTNESS_MULTIPLIER = 255;
};
  PowerManagement powerManagement;

/*структура для локалізації - ця частина ще в розробці*/ 
struct Lang {
  // Вказуємо розмір [2], оскільки у нас 2 мови
  const char* WIFI[2] = {"WiFi", "WiFi"};
  const char* SSID[2] = {"ssid", "назва мережі"};
  const char* PASSWORD[2] = {"password", "пароль"};
};

Lang lng;

const int PWM_CHANNEL = 0;
const int PWM_FREQ = 2000;        // 2 kHz - максимум для LDO6AJSA
const int PWM_RESOLUTION = 8; 

/*бібліотеки для роботи олед дисплея*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes72x40.h>

#define SdaPin 5
#define SclPin 6

#define ScreenWidth 128
#define ScreenHeight 64
#define OledReset    -1
Adafruit_SSD1306 display(ScreenWidth, ScreenHeight, &Wire, OledReset); 

RoboEyes<Adafruit_SSD1306> roboEyes(display);

/*=============бінд пінів=================*/
#define voltmeterPin 3 // пін для вольтметра
//піни для визначення позиції перемикача ліхтарика
#define positionOnepin 1 //додати номер піна 
#define positionTwopin 2 //додати номер піна 
//піни для керування кольором світла
#define blueLightPin 4 //додати номер піна
#define whiteLightPin 7 //додати номер піна
//пін для керування яскравістю світла
#define brightnessPin 0 //додати номер піна
/*=========================ФУНКЦІЇ===========================*/
/*перевірка і вхід у режим глибокого сну з debounce*/
/*перевірка і вхід у режим глибокого сну*/
void checkAndEnterDeepSleep() {
    static uint32_t offTimer = 0;
    const unsigned long DEBOUNCE_TIME = 500;  // 500 мс затримка
    
    // Якщо обидва піни LOW - перемикач в OFF
    if (digitalRead(positionOnepin) == LOW && digitalRead(positionTwopin) == LOW) {
        if (offTimer == 0) {
            offTimer = millis();
        }
        
        // Якщо OFF стан тримається 500 мс - йдемо спати
        if (millis() - offTimer >= DEBOUNCE_TIME) {
            Serial.println("=== ENTERING DEEP SLEEP ===");
            Serial.print("positionOnepin: ");
            Serial.println(digitalRead(positionOnepin));
            Serial.print("positionTwopin: ");
            Serial.println(digitalRead(positionTwopin));
            
            // Вимикаємо світлодіоди і PWM
            digitalWrite(blueLightPin, LOW);
            digitalWrite(whiteLightPin, LOW);
            ledcWrite(PWM_CHANNEL, 0);
            
            // Вимикаємо WiFi
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            delay(100);
            
            // Повідомлення на дисплей
            display.clearDisplay();
            display.setCursor(30, 20);
            display.setTextSize(2);
            display.println("Sleep");
            display.display();
            delay(1000);
            
            display.clearDisplay();
            display.display();
            
            // Налаштування wake-up по HIGH рівню на будь-якому з пінів
            esp_deep_sleep_enable_gpio_wakeup(1ULL << positionOnepin | 1ULL << positionTwopin, ESP_GPIO_WAKEUP_GPIO_HIGH);
            
            Serial.println("Wake-up configured for GPIO1 or GPIO2");
            Serial.println("Going to sleep NOW...");
            Serial.flush();
            delay(100);
            
            // DEEP SLEEP
            esp_deep_sleep_start();
        }
    } else {
        // Скидаємо таймер якщо хоч один пін HIGH
        offTimer = 0;
    }
}
/*визначення заряду батареї у відсотках*/
int batCharge(uint8_t pin) {
  int rawValue = analogRead(pin);  // Зчитуємо значення від 0 до 4095
  
  float voltage = rawValue * powerManagement.ADC_VOLTAGE_MULTIPLIER;  // перетворюємо у напругу

  float percentFloat = ((voltage - powerManagement.BATTERY_MIN_VOLTAGE) / (powerManagement.BATTERY_MAX_VOLTAGE - powerManagement.BATTERY_MIN_VOLTAGE)) * 100.0;
  int percent = (int)percentFloat;
  if (percent > 100) {
      percent = 100;
  } else if (percent < 0) {
      percent = 0;
    }  // Перетворюємо у відсотки
  return percent;
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
      digitalWrite(blueLightPin, HIGH);
      digitalWrite(whiteLightPin, LOW);
  }
  if (position == 1) {
      digitalWrite(blueLightPin, LOW);
      digitalWrite(whiteLightPin, HIGH);
  }
}
/*регулювання яскравості світла*/
void adjustBrightness(int brightnessValue) {
  int pwmValue = brightnessValue * powerManagement.BRIGHTNESS_MULTIPLIER / 100;
  ledcWrite(PWM_CHANNEL, pwmValue);  // ← замість analogWrite()
}
/*відображення кольору віджета батареї*/
sets::Colors batteryWidgetColorChange(int value) {
  if (value < 30) {
      return sets::Colors::Red;
  } else if (value < 70) {
      return sets::Colors::Yellow;
  } else {
      return sets::Colors::Green;
  }
}
/*відображення відсотків заряду батареї на OLED дисплеї*/
void displayChargeLevel(int info) {
  static uint32_t tmrCharge = 0;
  if (millis() - tmrCharge < 1000) return;  // оновлюємо не частіше 1 разу/сек
  tmrCharge = millis();

  display.clearDisplay();
  if (info == 100) {
      display.setCursor(30,10);
  } else if (info >= 10) {
      display.setCursor(45,10);
  } else {
      display.setCursor(54,10);
  }
  display.println(info);
  display.display();
}
/*відображення кількості часу, що залишився до розрядки батареї на OLED дисплеї*/
void displayEstimationTime(int estimatedHours) {
  static uint32_t tmrEstimation = 0;
  if (millis() - tmrEstimation < 1000) return;  // оновлюємо не частіше 1 разу/сек
  tmrEstimation = millis();
    
  display.clearDisplay();
  if (estimatedHours >= 10) {
      display.setCursor(30,10);
  } else {display.setCursor(45,10);
  }
  //display.setCursor(30,10);//перший координат - по горизонталі, другий - по вертикалі 
  display.println(String(estimatedHours) + "H");
  display.display();
}
/*вибір рандомного числа*/
int getRandomNumber(int maxValue) {
  return random(1, maxValue + 1);
}
/*анімація очей робота*/
void displayRoboEyesAnimation() {
  roboEyes.update();
  static uint32_t tmrAnimation;
  const unsigned long EYE_ANIMATION_INTERVAL = 3 * 60 * 1000; // інтервал зміни анімації в мілісекундах (3 хвилини)
  if (millis() - tmrAnimation >= EYE_ANIMATION_INTERVAL) { // кожні 3 хвилини змінюємо анімацію
      int randNumFace = getRandomNumber(4);
      tmrAnimation = millis();
      switch (randNumFace) {
          case 1:
              roboEyes.setMood(DEFAULT);
              roboEyes.anim_laugh();
              break;
          case 2:
              roboEyes.setMood(TIRED);
              roboEyes.anim_confused();
              break;
          case 3:
              roboEyes.setMood(ANGRY);
              roboEyes.anim_confused();
              break;
          case 4:
              roboEyes.setMood(HAPPY);
              roboEyes.anim_laugh();
              break;
      }
  }
}
/*мапінг режимів перемикача та кольорів ліхтаря*/
void manageSwitcherPosition() {
  if (digitalRead(positionTwopin) == LOW) {
      bindPositionLight(db[kk::switchPosition1]);
  }
  if (digitalRead(positionTwopin) == HIGH) {
      bindPositionLight(db[kk::switchPosition2]);
  } 
}
/*----------------------------------------------------------------------------*/
/*=======================================*/



/*створення блоків веб  інтерфейсу*/
void build(sets::Builder& b) {
  b.Image(H(img), "", "/logo.png");
  b.LinearGauge(H(batCharge), "Battery", 0, 100, "", data.batteryChargePercent,batteryWidgetColorChange(data.batteryChargePercent));
  if (b.beginGroup("WiFi")) {
      b.Input(kk::wifiSsid, "SSID");
      b.Pass(kk::wifiPass, "Password");
      if (b.Button(kk::apply, "Save & Restart")) {
      db.update();  // зберігаємо БД не очікуючи таймауту
      ESP.restart();
      }
      b.endGroup(); 
  }
  if (b.beginGroup("Flashlight Settings")) {
      b.Slider(kk::brightnessValue, "Brightness Slider", 0, 100,1);
      if (b.beginRow()) {
        b.LED(H(led1), "Position 1",1, sets::Colors::Yellow,sets::Colors::Blue);
        b.Switch(kk::switchPosition1, "");
        b.LED(H(led2), "",0, sets::Colors::Yellow,sets::Colors::Blue);
        b.endRow();
      }

      if (b.beginRow()) {
        b.LED(H(led3), "Position 2",1, sets::Colors::Yellow,sets::Colors::Blue);
        b.Switch(kk::switchPosition2, "");
        b.LED(H(led4), "",0, sets::Colors::Yellow,sets::Colors::Blue);
        b.endRow();
      }

      b.Select(kk::displayMode, "Display Mode", "Battery Charge;Time to discharge;Robot Eyes");
      b.endGroup();
  }
}

void update(sets::Updater u) {
  u.update(H(batCharge), data.batteryChargePercent);
  u.updateColor(H(batCharge), batteryWidgetColorChange(data.batteryChargePercent));
}

void setup() {
  Serial.begin(115200); // 115200 baud rate
  pinMode(voltmeterPin, INPUT);
  pinMode(positionOnepin, INPUT_PULLDOWN);
  pinMode(positionTwopin, INPUT_PULLDOWN);
  pinMode(blueLightPin, OUTPUT);
  pinMode(whiteLightPin, OUTPUT);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(brightnessPin, PWM_CHANNEL);

  data.batteryChargePercent = batCharge(voltmeterPin);// перший раз отримуэмо значення заряду батареї. Наступний раз буде через 5 хвилин
  // ======== WIFI ========
  // STA
  WiFi.mode(WIFI_AP_STA);
  // ======== SETTINGS ========
  sett.begin(true,"soniah"); // базу даних підключаємо до підключення до точки
  sett.onBuild(build);
  sett.onUpdate(update);
  // ======== DATABASE ========
  #ifdef ESP32
    LittleFS.begin(true);
  #else
    LittleFS.begin();
  #endif

  db.begin();

  // ініціювання БД початковими даними
    db.init(kk::wifiSsid, "");
    db.init(kk::wifiPass, "");
    db.init(kk::brightnessValue, 100);
    db.init(kk::switchPosition1, 0);
    db.init(kk::switchPosition2, 1);
    db.init(kk::displayMode, 2);

  // ======= AP =======
  WiFi.softAP("SONIAH🌻");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // ======= STA =======
  // если логин задан - подключаемся
  if (db[kk::wifiSsid].length()) {
    WiFi.begin(db[kk::wifiSsid], db[kk::wifiPass]);
    Serial.print("Connect STA");
    int tries = 20;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print('.');
      if (!--tries) break;
    }
      Serial.println();
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      
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
  delay(1000); // Затримка в 1 секунду
} 


void loop() {
  checkAndEnterDeepSleep();
  /*======================battery charge manager===================*/
  static uint32_t tmrBattery;
  const unsigned long BATTERY_CHARGE_INTERVAL = 5 * 60 * 1000; // інтервал оновлення заряду батареї в мілісекундах (5 хвилин)
  if (millis() - tmrBattery >= BATTERY_CHARGE_INTERVAL) { // кожні 5 хвилин змінюємо анімацію
  data.batteryChargePercent = batCharge(voltmeterPin);
  //data.batteryChargePercent = random(0,101); //тестове значення заряду батареї
  tmrBattery = millis();
  }
  /*======================switcher position manager===================*/
  manageSwitcherPosition();
  /*========================BRIGHTNESS=================================*/
  int lastBrightnessValue = 0;
  if (db[kk::brightnessValue] != lastBrightnessValue) {
    lastBrightnessValue = db[kk::brightnessValue];
    adjustBrightness(db[kk::brightnessValue]);
  }
  /*========================robot eyes=================================*/
  switch ((int)db[kk::displayMode]){
    /*======================== display percent of battery charge=================================*/
    case 0:
      displayChargeLevel(data.batteryChargePercent);
      break;
    /*========================display quantity hours to battery discharge=================================*/
    case 1:
      displayEstimationTime(estimationTimeHours(data.batteryChargePercent,db[kk::brightnessValue]));
      break;
    case 2:
      displayRoboEyesAnimation();
      break;
  }
  sett.tick();
}


