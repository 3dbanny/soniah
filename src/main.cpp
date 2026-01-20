#include <Arduino.h>
/*налаштування веб інтерфейсу*/
#include <GyverDBFile.h>
#include <LittleFS.h>
GyverDBFile db(&LittleFS, "/data.db");
#include <SettingsGyver.h>
SettingsGyver sett("SONIAH", &db);
/*налаштування ключів БД*/
enum kk : size_t {
    wifi_ssid,
    wifi_pass,
    brightnessValue,
    switchPosition1,
    switchPosition2,
    displayMode,
    apply};
/*бібліотеки для роботи олед дисплея*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes72x40.h>

#define SDA_PIN 5
#define SCL_PIN 6

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); 

RoboEyes<Adafruit_SSD1306> roboEyes(display);

/*=============бінд пінів=================*/
#define voltmeterPin 5 // пін для вольтметра
/*-------------------------------------------------------------------------------------*/
//піни для визначення позиції перемикача ліхтарика
#define positionOnepin 1 //додати номер піна 
#define positionTwopin 2 //додати номер піна 
//піни для керування кольором світла
#define redLightPin 4 //додати номер піна
#define whiteLightPin 3 //додати номер піна
//пін для керування яскравістю світла
#define brightnessPin 0 //додати номер піна
/*=====================змінні===========================================================*/
String localIP;
int percent = 0; // змінна для відсотків заряду батареї
/*створення блоків веб  інтерфейсу*/
void build(sets::Builder& b) {
  if (b.beginGroup("WiFi")) {
      b.Input(kk::wifi_ssid, "SSID");
      b.Pass(kk::wifi_pass, "Password");
      b.LabelNum(776,"Battery",percent);
      if (b.Button(kk::apply, "Save & Restart")) {
      db.update();  // зберігаємо БД не очікуючи таймауту
      ESP.restart();
      }
      b.endGroup(); 
  }
  if (b.beginGroup("Flashlight Settings")) {
      //b.Spinner(kk::brightnessValue, "Brightness", 0, 255,1);
      b.Slider(kk::brightnessValue, "Brightness Slider", 0, 100,1);
      b.Select(kk::switchPosition1, "Switch Position 1", "Red;White");
      b.Select(kk::switchPosition2, "Switch Position 2", "Red;White");
      b.Select(kk::displayMode, "Display Mode", "Percent;Time to discharge;Robot Eyes");
      b.endGroup();
  }
}
void update(sets::Updater u) {
  u.update(776, percent);

}
/*=========================ФУНКЦІЇ===========================*/
/*визначення заряду батареї у відсотках*/
int batCharge(uint8_t pin) {
    int rawValue = analogRead(pin);  // Зчитуємо значення від 0 до 4095
    float voltage = (rawValue * 1.48 / 1000);  // перетворюємо у напругу
    float percentFloat = ((voltage - 3.2) * 100);
    int percent = (int)percentFloat;
    if (percent > 100) {
        percent = 100;
    } else if (percent < 0) {
        percent = 0;
    }  // Перетворюємо у відсотки
    return percent;
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
    analogWrite(brightnessPin, brightnessValue);
}
/*розрахунок кількості часу, що залишився до розрядки батареї*/
int estimationTimeHours(int chargePercent,int brightnessLevel) {
    int estimatedHours = 0;
    estimatedHours = (chargePercent * 2800 / 100 / (brightnessLevel+150));  //приблизний розрахунок часу до розрядки батареї в годинах
    return estimatedHours;
}
/*відображення відсотків заряду батареї на OLED дисплеї*/
void displayChargeLevelOled(int info) {
    display.clearDisplay();
    if (info == 100) {display.setCursor(30,10);}
    else if (info >= 10) {display.setCursor(45,10);}
    else {display.setCursor(54,10);}
    display.println(info);
    display.display();
}
/*відображення кількості часу, що залишився до розрядки батареї на OLED дисплеї*/
void displayestimationTime(int estimatedHours) {
    display.clearDisplay();
    if (estimatedHours >= 10) {display.setCursor(30,10);}
    else {display.setCursor(45,10);}
    //display.setCursor(30,10);//перший координат - по горизонталі, другий - по вертикалі 
    display.println(String(estimatedHours) + "H");
    display.display();
}
/*вибір рандомного числа*/
int getRandomNumber(int maxValue) {
  return random(1, maxValue + 1);
}
/*анімація очей робота*/
void robotSeeYou() {
    roboEyes.update();
  static uint32_t tmr2;
  if (millis() - tmr2 >= 180000) { // кожні 3 хвилини змінюємо анімацію
    int randNumFace = getRandomNumber(4);
    tmr2 = millis();
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
  }
  else if (digitalRead(positionOnepin) != digitalRead(positionTwopin)) {
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

  percent = batCharge(voltmeterPin);// перший раз отримуэмо значення заряду батареї. Наступний раз буде через 5 хвилин
  // ======== WIFI ========

  // STA
  WiFi.mode(WIFI_AP_STA);
// ======== SETTINGS ========
  sett.begin(); // базу даних підключаємо до підключення до точки
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
    db.init(kk::wifi_ssid, "");
    db.init(kk::wifi_pass, "");
    db.init(kk::brightnessValue, 128);
    db.init(kk::switchPosition1, 0);
    db.init(kk::switchPosition2, 1);
    db.init(kk::displayMode, 2);

  

// ======= AP =======
  WiFi.softAP("SONIAH");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // ======= STA =======
  // если логин задан - подключаемся
  if (db[kk::wifi_ssid].length()) {
    WiFi.begin(db[kk::wifi_ssid], db[kk::wifi_pass]);
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
      //localIP = WiFi.localIP().toString();
  }



  /*=======================SETUP OLED========================================*/
  Wire.begin(SDA_PIN, SCL_PIN);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.setRotation(2);
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  /*=======================SETUP robo eyes========================================*/
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  // Define some automated eyes behaviour
  roboEyes.setAutoblinker(ON, 3, 2); // Start auto blinker animation cycle -> bool active, int interval, int variation -> turn on/off, set interval between each blink in full seconds, set range for random interval variation in full seconds
  // Define eye shapes, all values in pixels
  roboEyes.setWidth(24, 24); // byte leftEye, byte rightEye
  roboEyes.setHeight(24, 24); // byte leftEye, byte rightEye
  roboEyes.setBorderradius(8, 8); // byte leftEye, byte rightEye
  roboEyes.setSpacebetween(4); // int space -> can also be neg
  //roboEyes.setHFlicker(ON, 2); // horizontal flickering effect -> bool active, int intensity (1-5)
  roboEyes.setPosition(NE); // cardinal directions, can be N, NE, E, SE, S, SW, W, NW, DEFAULT (default = horizontally and vertically centered)

  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(45,10);//перший координат - по горизонталі, другий - по вертикалі 
  display.println("Hi");
  display.display();
  delay(1000); // Затримка в 1 секунду
} 


void loop() {
  Serial.begin(115200);
  
  /*======================switcher position manager===================*/
  manageSwitcherPosition();
  /*========================BRIGHTNESS=================================*/
  adjustBrightness(db[kk::brightnessValue*25]);

  /*========================robot eyes=================================*/
  switch ((int)db[kk::displayMode]){
    /*======================== display percent of battery charge=================================*/
    case 0:
      display.clearDisplay();
      display.setCursor(30,10);
      display.println(100);
      display.display();
      break;
    /*========================display quantity hours to battery discharge=================================*/
    case 1:
      display.clearDisplay();
      display.setCursor(30,10);
      display.println("10H");
      display.display();
      break;
    case 2:
      robotSeeYou();
      break;
  }
  /*depricated*/
  // if (db[kk::displayMode] == 2) {
  //   robotSeeYou();
  // };
  // if (db[kk::displayMode] == 1) {
  //   display.clearDisplay();
  //   display.setCursor(30,10);
  //   display.println("10H");
  //   display.display();
  //   /*static uint32_t tmr2;
  //   if (millis() - tmr2 >= 300000) { // кожні 5 хвилин оновлюэмо значення кількості часу до розрядки батареї та оновлюємо відображення на олед дисплеї
  //     displayestimationTime(estimationTimeHours(batCharge(voltmeterPin),db[kk::brightnessValue*25]));
  //     tmr2 = millis();  
  //   }; */
  // };
   
  // /*======================== display percent of battery charge=================================*/
  // if (db[kk::displayMode] == 0) {
  //   display.clearDisplay();
  //   display.setCursor(30,10);
  //   display.println(100);
  //   display.display();
  //   /*static uint32_t tmr3;
  //   if (millis() - tmr3 >= 300000) { // кожні 5 хвилин оновлюємо значення заряду батареї та оновлюємо відображення на олед дисплеї
  //     displayChargeLevelOled(batCharge(voltmeterPin));
  //     tmr3 = millis();  
  //   }; */
  // };
  int temperature = temperatureRead();
  sett.tick();
}


