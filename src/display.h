#pragma once
#include "config.h"

/*відображення відсотків заряду батареї на OLED дисплеї*/
void displayChargeLevel(Adafruit_SSD1306& display, int percent) {
  static uint32_t tmrCharge = 0;
  if (millis() - tmrCharge < 1000) return;  // оновлюємо не частіше 1 разу/сек
  tmrCharge = millis();

  display.clearDisplay();
  if (percent == 100) {
      display.setCursor(30,10);
  } else if (percent >= 10) {
      display.setCursor(45,10);
  } else {
      display.setCursor(54,10);
  }
  display.println(percent);
  display.display();
}

/*графічне відображення заряду батарейки на OLED дисплеї*/
void displayChargeBatteryImage(Adafruit_SSD1306& display, int percent) {
  static uint32_t tmrCharge = 0;
  if (millis() - tmrCharge < 1000) return;  // оновлюємо не частіше 1 секунду]
  tmrCharge = millis();
  int seg[4] = {0,0,0,0};
  if      (percent > 75) { seg[0] = 1; seg[1] = 1; seg[2] = 1; seg[3] = 1; }
  else if (percent > 50) { seg[0] = 1; seg[1] = 1; seg[2] = 1; }
  else if (percent > 25) { seg[0] = 1; seg[1] = 1; }
  else if (percent > 10) { seg[0] = 1; }
  display.clearDisplay();
  display.drawRect(34, 5, 52, 32, 1);
  display.fillRect(37, 8, 10, 26, seg[3]);
  display.fillRect(49, 8, 10, 26, seg[2]);
  display.fillRect(61, 8, 10, 26, seg[1]);
  display.fillRect(73, 8, 10, 26, seg[0]);
  display.fillRect(30, 13, 5, 16, 1);
  display.display();  
}

/*відображення кількості часу, що залишився до розрядки батареї на OLED дисплеї*/
void displayEstimationTime(Adafruit_SSD1306& display, int hours) {
  static uint32_t tmr = 0;
  if (millis() - tmr < 1000) return;  // оновлюємо не частіше 1 разу/сек
  tmr = millis();
  display.clearDisplay();
  if (hours >= 10) {
      display.setCursor(30,10);
  } else {display.setCursor(45,10);
  }
  //display.setCursor(30,10);//перший координат - по горизонталі, другий - по вертикалі 
  display.println(String(hours));
  display.drawBitmap(65, 4, image_clock_bits, 30, 32, 1);
  display.display();
}

void displayRoboEyesAnimation(RoboEyes<Adafruit_SSD1306>& roboEyes) {
  roboEyes.update();
  static uint32_t tmr;
  if (millis() - tmr >= EYE_ANIMATION_MS) { 
        tmr = millis();
      switch (random(1, 5)) {
          case 1:
              roboEyes.setMood(DEFAULT); roboEyes.anim_laugh();    break;
          case 2:
              roboEyes.setMood(TIRED);   roboEyes.anim_confused(); break;
          case 3:
              roboEyes.setMood(ANGRY);   roboEyes.anim_confused(); break;
          case 4:
              roboEyes.setMood(HAPPY);   roboEyes.anim_laugh();    break;
      }
  }
}
/*відображення таймера на OLED дисплеї*/
void displayTimerCountdown(Adafruit_SSD1306& display, uint32_t remainingSeconds) {
  static uint32_t tmr = 0;
  if (millis() - tmr < 1000) return;
  tmr = millis();

  display.clearDisplay();
  display.setTextSize(2);  // ← зменшуємо розмір
  int m = remainingSeconds / 60;
  int s = remainingSeconds % 60;
  String timeStr = (m < 10 ? "0" : "") + String(m) + ":" +
                   (s < 10 ? "0" : "") + String(s);
  display.setCursor(28, 10);
  display.println(timeStr);
  display.display();
  display.setTextSize(3);  // ← повертаємо назад
}

// Renders sleep icon before entering deep sleep.
void displaySleepScreen(Adafruit_SSD1306& display) {
  display.clearDisplay();
  display.drawBitmap(38,  6, sleep_image, 30, 30, 1);
  display.drawCircle(67,  6, 2, 1);
  display.drawCircle(76, 17, 2, 1);
  display.drawCircle(77, 32, 2, 1);
  display.drawCircle(91, 11, 2, 1);
  display.display();
}

// Renders Wi Fi on icon. NOt implemented yet.
void displayWiFiIconOn(Adafruit_SSD1306& display) {
  display.clearDisplay();
  display.drawBitmap(38,  6, wi_fi_on, 30, 30, 1);
  display.drawCircle(67,  6, 2, 1);
  display.drawCircle(76, 17, 2, 1);
  display.drawCircle(77, 32, 2, 1);
  display.drawCircle(91, 11, 2, 1);
  display.display();
}

// Renders Wi Fi off icon. NOt implemented yet.
void displayWiFiIconOff(Adafruit_SSD1306& display) {
  display.clearDisplay();
  display.drawBitmap(38,  6, wi_fi_off, 30, 30, 1);
  display.drawCircle(67,  6, 2, 1);
  display.drawCircle(76, 17, 2, 1);
  display.drawCircle(77, 32, 2, 1);
  display.drawCircle(91, 11, 2, 1);
  display.display();
}