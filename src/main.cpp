#include <Arduino.h>
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
/*структура з глобальними змінними*/
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
  const float BATTERY_MAX_VOLTAGE = 4.2; // максимальна напруга батареї
  const float BATTERY_MIN_VOLTAGE = 3.0; // мінімальна напруга батар
  const int DIODE_DROP_MAH = 100; // падіння напруги на діодах в мА
  const int BATTERY_CAPACITY_MAH = 2800; // ємність батареї в мАг
  const int ESP32_CONSUMPTION_MAH = 80; // середнє споживання ESP32 в мА
  const int MAX_LIGHT_CONSUMPTION_MAH = 200; // максимальне споживання ліхтаря в мА
  const int BRIGHTNESS_MULTIPLIER = 255;
};
  PowerManagement powerManagement;

/*структура для локалізації - ця частина навіть ще не почалася на виконання*/ 
struct Lang {
  // Вказуємо розмір [2], оскільки у нас 2 мови
  const char* WIFI[2] = {"WiFi", "WiFi"};
  const char* SSID[2] = {"ssid", "назва мережі"};
  const char* PASSWORD[2] = {"password", "пароль"};
};
// Створюємо об'єкт структури, щоб до нього звертатися
Lang lng;
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
#define voltmeterPin 7 // пін для вольтметра
//піни для визначення позиції перемикача ліхтарика
#define positionOnepin 1 //додати номер піна 
#define positionTwopin 2 //додати номер піна 
//піни для керування кольором світла
#define redLightPin 4 //додати номер піна
#define whiteLightPin 3 //додати номер піна
//пін для керування яскравістю світла
#define brightnessPin 0 //додати номер піна
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
        b.LED(H(led1), "Position 1",1, sets::Colors::Red,sets::Colors::Yellow);
        b.Switch(kk::switchPosition1, "");
        b.LED(H(led2), "",0, sets::Colors::Red,sets::Colors::Yellow);
        b.endRow();
      }

      if (b.beginRow()) {
        b.LED(H(led3), "Position 2",1, sets::Colors::Red,sets::Colors::Yellow);
        b.Switch(kk::switchPosition2, "");
        b.LED(H(led4), "",0, sets::Colors::Red,sets::Colors::Yellow);
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
/*=========================ФУНКЦІЇ===========================*/
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
  int pwmValue = brightnessValue * powerManagement.BRIGHTNESS_MULTIPLIER/100; // множник для переведення відсотків у значення від 0 до 255
  analogWrite(brightnessPin, pwmValue);
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
  if (digitalRead(positionOnepin) == digitalRead(positionTwopin)) {
      bindPositionLight(db[kk::switchPosition2]);
  } else if (digitalRead(positionOnepin) != digitalRead(positionTwopin)) {
      bindPositionLight(db[kk::switchPosition1]);
  } 
}
/*----------------------------------------------------------------------------*/
void setup() {
  Serial.begin(115200); // 115200 baud rate
  pinMode(voltmeterPin, INPUT);
  pinMode(positionOnepin, INPUT);
  pinMode(positionTwopin, INPUT);
  pinMode(redLightPin, OUTPUT);
  pinMode(whiteLightPin, OUTPUT);

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
  WiFi.softAP("SONIAH_" + String(random((100))));
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // ======= STA =======
  // если логин задан - подключаемся
  if (db[kk::wifiSsid].length()) {
    WiFi.begin(db[kk::wifiSsid], db[kk::wifiPass]);
    Serial.print("Connect STA");
    int tries = 5;
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
  static uint32_t tmrBattery;
  const unsigned long BATTERY_CHARGE_INTERVAL = 5 * 60 * 1000; // інтервал зміни анімації в мілісекундах (5 хвилин)
  if (millis() - tmrBattery >= BATTERY_CHARGE_INTERVAL) { // кожні 5 хвилин змінюємо анімацію
  data.batteryChargePercent = batCharge(voltmeterPin);
  tmrBattery = millis();
  }
  /*======================switcher position manager===================*/
  manageSwitcherPosition();
  /*========================BRIGHTNESS=================================*/
  adjustBrightness(db[kk::brightnessValue]);

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


