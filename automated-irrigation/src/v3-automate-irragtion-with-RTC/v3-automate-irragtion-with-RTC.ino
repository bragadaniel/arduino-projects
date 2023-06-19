/***
 * author: Daniel Braga
 * refs:
 * progmem: https://arduino-esp8266.readthedocs.io/en/latest/PROGMEM.html
 * https://www.arduino.cc/reference/pt/language/variables/utilities/progmem/
 * rtc: https://github.com/adafruit/RTClib
 * am/pm:
 * https://forum.arduino.cc/t/how-to-get-ds1307-hour-in-am-pm-format-easily/22102
 */
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>

#include "RTClib.h"  // https://github.com/adafruit/RTClib

#define OLED_ADDRESS 0x3C  // OLED address
#define SCREEN_WIDTH 128   //--> OLED display width in pixels
#define SCREEN_HEIGHT 64   //--> OLED display height in pixels
#define OLED_RESET -1      // Reset pin (or -1 if sharing Arduino reset pin)
#define MODE_BUTTON_PIN 2
#define UP_BUTTON_PIN 3
#define DOWN_BUTTON_PIN 4
#define INTERVAL_TIME_DATE 1000  // interval ms

RTC_DS1307 rtc;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// cursor drawing for changes
const unsigned char topArrow[] PROGMEM = {0x20, 0x70, 0xf8};
const unsigned char bottomArrow[] PROGMEM = {0xf8, 0x70, 0x20};
const unsigned char sideArrow[] PROGMEM = {0x80, 0xc0, 0xe0, 0xc0, 0x80};

// Days of week pt-br
const char daysOfTheWeek0[] PROGMEM = "DOM";
const char daysOfTheWeek1[] PROGMEM = "SEG";
const char daysOfTheWeek2[] PROGMEM = "TER";
const char daysOfTheWeek3[] PROGMEM = "QUA";
const char daysOfTheWeek4[] PROGMEM = "QUI";
const char daysOfTheWeek5[] PROGMEM = "SEX";
const char daysOfTheWeek6[] PROGMEM = "SAB";

char daysOfTheWeek[7][12] = {
    daysOfTheWeek0, daysOfTheWeek1, daysOfTheWeek2, daysOfTheWeek3,
    daysOfTheWeek4, daysOfTheWeek5, daysOfTheWeek6,
};

// buttons debounce config
const int BUTTON_PINS[] PROGMEM = {
    MODE_BUTTON_PIN,
    DOWN_BUTTON_PIN,
    UP_BUTTON_PIN,
};
const int NUM_BUTTONS PROGMEM = sizeof(BUTTON_PINS) / sizeof(BUTTON_PINS[0]);
const int DEBOUNCE_DELAY = 50;
int buttonStates[NUM_BUTTONS];
int lastButtonStates[NUM_BUTTONS];
unsigned long lastDebounceTimes[NUM_BUTTONS];

// buttons
byte btnMode, btnUp, btnDown;
bool ButtonMode;
bool ButtonUP;
bool ButtonDown;

// Date config vars
int day, month, year, hour24, hour12, minute, second, dtw;
char hoursStr[3];
char minuteStr[3];
char secondStr[3];
int hr24;
char st[2];

// menu config
byte menuOptionCount = 0;
bool menuStatus = false;

// update Time and Date
unsigned long prevGetTimeDate = 0;  // store last time was updated

// debounce push button down

void setup() {
  Serial.begin(9600);
  // Push buttons start
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }
  // find rtc begin module
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;  //--> Don't proceed, loop forever
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // Set the time and date based on your computer time and date
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This manual set date: DateTime(YEAR, MONTH, DAY, HOUR, MINUTE, SECONDS)
    // rtc.adjust(DateTime(2023, 6, 17, 18, 20, 0));
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  //--> Don't proceed, loop forever
  }
  display.display();
  delay(1000);
}

void loop() {
  readyButton();

  // display menu and select options
  if (menuStatus == false) {
    initialDisplay();
  }

  // prototype debounce is temporally
  if (btnMode != lastButtonStates[0]) {
    lastDebounceTimes[0] = millis();
  }
  if ((millis() - lastDebounceTimes[0]) > DEBOUNCE_DELAY) {
    if (btnMode != buttonStates[0]) {
      buttonStates[0] = btnMode;
      if (buttonStates[0] == LOW && menuStatus == false) {
        menuStatus = true;
        menuOptionCount = 1;
      }
    }
  }

  if (menuStatus == true) {
    readyButton();

    switch (menuOptionCount) {
      case 1:
        menu(menuOptionCount);
        break;
      case 2:
        menu(menuOptionCount);
        break;
      case 3:
        menu(menuOptionCount);
        break;
      default:
        initialDisplay();
        break;
    }
    if (btnMode != lastButtonStates[0]) {
      lastDebounceTimes[0] = millis();
    }
    if ((millis() - lastDebounceTimes[0]) > DEBOUNCE_DELAY) {
      if (btnMode != buttonStates[0]) {
        buttonStates[0] = btnMode;
        if (buttonStates[0] == LOW && menuOptionCount == 1) {
          getDateTime();
        }

        if (buttonStates[0] == LOW && menuOptionCount == 2) {
          getDateTime();
        }
        if (buttonStates[0] == LOW && menuOptionCount == 3) {
          menuOptionCount = 0;
          menuStatus = false;
        }
      }
    }

    if (btnDown != lastButtonStates[1]) {
      lastDebounceTimes[1] = millis();
    }
    if ((millis() - lastDebounceTimes[1]) > DEBOUNCE_DELAY) {
      if (btnDown != buttonStates[1]) {
        buttonStates[1] = btnDown;
        if (buttonStates[1] == LOW) {
          menuOptionCount++;
          if (menuOptionCount > 3) menuOptionCount = 1;
        }
      }
    }

    if (btnUp != lastButtonStates[2]) {
      lastDebounceTimes[2] = millis();
    }
    if ((millis() - lastDebounceTimes[2]) > DEBOUNCE_DELAY) {
      if (btnUp != buttonStates[2]) {
        buttonStates[2] = btnUp;
        if (buttonStates[2] == LOW) {
          menuOptionCount--;
          if (menuOptionCount < 1) menuOptionCount = 3;
        }
      }
    }
  }

  // state debounce push buttons
  lastButtonStates[0] = btnMode;
  lastButtonStates[1] = btnDown;
  lastButtonStates[2] = btnUp;
}  // eof

void readyButton() {
  btnMode = digitalRead(MODE_BUTTON_PIN);
  btnUp = digitalRead(UP_BUTTON_PIN);
  btnDown = digitalRead(DOWN_BUTTON_PIN);
}

void getDateTime() {
  DateTime now = rtc.now();
  display.setCursor(0, 30);
  hour24 = (now.hour() + 1) % 24;
  sprintf(hoursStr, "%02d", now.hour());
  display.print(hoursStr);
  display.print(":");
  minute = (now.minute() + 59) % 60;
  sprintf(minuteStr, "%02d", now.minute());
  display.print(minuteStr);
  display.print(":");
  second = (now.second() + 59) % 60;
  sprintf(secondStr, "%02d", now.second());
  display.print(secondStr);
}

void menu(byte selectOption) {
  menuDisplay();
  switch (selectOption) {
    case 1:
      display.drawBitmap(0, 20, sideArrow, 3, 5, WHITE);
      break;
    case 2:
      display.drawBitmap(0, 30, sideArrow, 3, 5, WHITE);
      break;
    case 3:  // back option
      display.drawBitmap(0, 55, sideArrow, 3, 5, WHITE);
      break;
    default:
      selectOption = 0;
      break;
  }
  display.display();
}

void initialDisplay() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(11, 0);
  display.print("***- WELCOME -***");

  display.setCursor(7, 18);
  display.print("Press yellow button");

  display.setCursor(7, 28);
  display.print("for initialized!");

  display.display();
}

void menuDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(12, 0);
  display.print("***- MENU -***");

  display.setCursor(7, 20);
  display.print("Set Time and Date");

  display.setCursor(7, 30);
  display.print("Set Alarm");

  display.setCursor(7, 55);
  display.print("Back");
}