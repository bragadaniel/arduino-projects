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
#include <ButtonDebounce.h>
#include <EEPROM.h>
// #include <SPI.h>
#include <Wire.h>

#include "RTClib.h"  // https://github.com/adafruit/RTClib

#define OLED_ADDRESS 0x3C  // OLED address
#define SCREEN_WIDTH 128   // OLED display width in pixels
#define SCREEN_HEIGHT 64   // OLED display height in pixels
#define OLED_RESET -1      // Reset pin (or -1 if sharing Arduino reset pin)
#define MODE_BUTTON_PIN 2
#define DOWN_BUTTON_PIN 3
#define UP_BUTTON_PIN 4
#define RELAY_ONE_PIN 5
#define RELAY_TWO_PIN 6
#define INTERVAL_TIME_DATE 1000  // interval ms

RTC_DS1307 rtc;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
ButtonDebounce btnDebounce;

// cursor drawing for changes
const unsigned char topArrow[] PROGMEM = {0x20, 0x70, 0xf8};
const unsigned char bottomArrow[] PROGMEM = {0xf8, 0x70, 0x20};
const unsigned char sideArrow[] PROGMEM = {0x80, 0xc0, 0xe0, 0xc0, 0x80};

// Days of week pt-br
// const char daysOfTheWeek0[] PROGMEM = "DOM";
// const char daysOfTheWeek1[] PROGMEM = "SEG";
// const char daysOfTheWeek2[] PROGMEM = "TER";
// const char daysOfTheWeek3[] PROGMEM = "QUA";
// const char daysOfTheWeek4[] PROGMEM = "QUI";
// const char daysOfTheWeek5[] PROGMEM = "SEX";
// const char daysOfTheWeek6[] PROGMEM = "SAB";

// const char daysOfTheWeek[7][12] = {
//     daysOfTheWeek0, daysOfTheWeek1, daysOfTheWeek2, daysOfTheWeek3,
//     daysOfTheWeek4, daysOfTheWeek5, daysOfTheWeek6,
// };
// Relay
const int RELAY_PINS[] PROGMEM = {RELAY_ONE_PIN, RELAY_TWO_PIN};
const int NUM_RELAY PROGMEM = sizeof(RELAY_PINS) / sizeof(RELAY_PINS[0]);

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
byte selectOption = 0;
bool mainMenu = false;
bool clockMenu = false;
bool setHour = false;
bool setMin = false;

// update Time and Date
unsigned long prevGetTimeDate = 0;  // store last time was updated

// blink mode
unsigned long previousMillis = 0;
const unsigned long blinkInterval = 700;
bool isBlinking = false;

void setup() {
  Serial.begin(9600);
  // Push buttons start
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }
  // Relay modules
  for (int i = 0; i < NUM_RELAY; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], HIGH);
  }

  // find rtc begin module
  if (!rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
    while (1)
      ;  //--> Don't proceed, loop forever
  }

  if (!rtc.isrunning()) {
    Serial.println(F("RTC is NOT running!"));
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
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;
    isBlinking = !isBlinking;
  }

  // display menu and select options
  if (mainMenu == false) {
    btnDebounce.runFnDebounce(btnMode, &lastButtonStates[0], &buttonStates[0],
                              DEBOUNCE_DELAY, &lastDebounceTimes[0],
                              modeFnDebounce);
    initialDisplay();
  }

  // menu clock or alarm
  while (mainMenu == true) {
    readyButton();
    switch (menuOptionCount) {
      case 1:  // clock
        menu(menuOptionCount);
        break;
      case 2:  // alarm
        menu(menuOptionCount);
        break;
      case 3:  // back option
        menu(menuOptionCount);
        break;
      default:
        initialDisplay();
        break;
    }
    // debounce buttons
    btnDebounce.runFnDebounce(btnMode, &lastButtonStates[0], &buttonStates[0],
                              DEBOUNCE_DELAY, &lastDebounceTimes[0],
                              modeMenuDebounce);

    btnDebounce.runFnDebounce(btnDown, &lastButtonStates[1], &buttonStates[1],
                              DEBOUNCE_DELAY, &lastDebounceTimes[1],
                              downFnDebounce);

    btnDebounce.runFnDebounce(btnUp, &lastButtonStates[2], &buttonStates[2],
                              DEBOUNCE_DELAY, &lastDebounceTimes[2],
                              upFnDebounce);
    display.display();
  }  // eof menu stat true

  // selected adjust clock
  while (clockMenu) {
    readyButton();
    switch (menuOptionCount) {
      case 1:  // set
        menuSetClock(menuOptionCount);
        break;
      case 2:  // save
        menuSetClock(menuOptionCount);
        break;
      case 3:  // back
        menuSetClock(menuOptionCount);
        break;
      default:
        break;
    }

    btnDebounce.runFnDebounce(btnMode, &lastButtonStates[0], &buttonStates[0],
                              DEBOUNCE_DELAY, &lastDebounceTimes[0],
                              modeAdjustClockDebounce);

    btnDebounce.runFnDebounce(btnDown, &lastButtonStates[1], &buttonStates[1],
                              DEBOUNCE_DELAY, &lastDebounceTimes[1],
                              downFnDebounce);

    btnDebounce.runFnDebounce(btnUp, &lastButtonStates[2], &buttonStates[2],
                              DEBOUNCE_DELAY, &lastDebounceTimes[2],
                              upFnDebounce);
    // set hour config
    while (setHour) {
      // TODO: blink arrow for option
      if (menuOptionCount == 1) {
        display.drawBitmap(39, 25, topArrow, 5, 3, WHITE);  // hour set cursor
      }
      if (menuOptionCount == 2) {
        display.drawBitmap(56, 25, topArrow, 5, 3, WHITE);  // min set cursor
      }
      display.display();
    }  // eof set hour config

    // set min config
    while (setMin) {
      // TODO: blink arrow for option
      display.drawBitmap(56, 25, topArrow, 5, 3, WHITE);  // min set cursor
      display.display();
    }  // eof set hour config

    display.display();
  }  // eof config clock

}  // eof

// role modeBtn for initial screen
void modeFnDebounce() {
  buttonStates[0] = btnMode;
  if (buttonStates[0] == LOW && mainMenu == false) {
    mainMenu = true;
    menuOptionCount = 1;
    display.drawBitmap(80, 56, bottomArrow, 3, 5, WHITE);
  }
}

// role modeBtn for menu screen
void modeMenuDebounce() {
  buttonStates[0] = btnMode;
  // to first option selected
  if (buttonStates[0] == LOW && menuOptionCount == 1) {
    clockMenu = true;
    mainMenu = false;
  }

  if (buttonStates[0] == LOW && menuOptionCount == 2) {
    // TODO: set alarm screen
    clockMenu = true;
    mainMenu = false;
  }
  if (buttonStates[0] == LOW && menuOptionCount == 3) {
    menuOptionCount = 0;
    mainMenu = false;
  }
}

// role modeBtn for clock screen
void modeAdjustClockDebounce() {
  buttonStates[0] = btnMode;
  // set hour
  if (buttonStates[0] == LOW && menuOptionCount == 1) {
    setHour = true;
  }
  // save
  if (buttonStates[0] == LOW && menuOptionCount == 2) {
  }
  // back
  if (buttonStates[0] == LOW && menuOptionCount == 3) {
    menuOptionCount = 1;
    clockMenu = false;
    mainMenu = true;
  }
}

void upFnDebounce() {
  buttonStates[2] = btnUp;
  if (buttonStates[2] == LOW) {
    menuOptionCount--;
    if (menuOptionCount < 1) menuOptionCount = 3;
  }
}

void downFnDebounce() {
  buttonStates[1] = btnDown;
  if (buttonStates[1] == LOW) {
    menuOptionCount++;
    if (menuOptionCount > 3) menuOptionCount = 1;
  }
}

void readyButton() {
  btnMode = digitalRead(MODE_BUTTON_PIN);
  btnUp = digitalRead(UP_BUTTON_PIN);
  btnDown = digitalRead(DOWN_BUTTON_PIN);
}

void getDateTime(bool isClear, int x, int y) {
  if (isClear) {
    clearDisplay();
  }
  DateTime now = rtc.now();
  display.setCursor(x, y);
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
  menuConfigDisplay();
  switch (selectOption) {
    case 1:  // clock adjust
      display.drawBitmap(0, 20, sideArrow, 3, 5, WHITE);
      break;
    case 2:  // alarm adjust
      display.drawBitmap(0, 30, sideArrow, 3, 5, WHITE);
      break;
    case 3:  // back option
      display.drawBitmap(0, 55, sideArrow, 3, 5, WHITE);
      break;
    default:
      selectOption = 0;
      break;
  }
}

void initialDisplay() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;
    isBlinking = !isBlinking;
  }
  clearDisplay();  // clear display

  display.setCursor(20, 0);
  display.print("** WELCOME **");

  display.setCursor(7, 16);
  if (!isBlinking) {
    display.print("Press yellow button");
  }
  display.setCursor(0, 26);
  display.print("Current time:");
  getDateTime(false, 0, 38);
  display.setCursor(0, 47);
  display.print("Next irrigation:");
  display.setCursor(0, 57);
  display.print("19:25:30");
  display.display();
}

void menuSetClock(byte selectOption) {
  adjustClockDisplay();
  switch (selectOption) {
    case 1:  // set
      display.drawBitmap(0, 35, sideArrow, 3, 5, WHITE);
      break;
    case 2:  // save
      display.drawBitmap(0, 45, sideArrow, 3, 5, WHITE);
      break;
    case 3:  // back option
      display.drawBitmap(0, 55, sideArrow, 3, 5, WHITE);
      break;
    default:
      selectOption = 0;
      break;
  }
}

void adjustClockDisplay() {
  clearDisplay();

  display.setCursor(14, 0);
  display.print("-- Adjust clock --");

  display.setCursor(35, 16);
  display.print("14:25:00");

  // display.drawBitmap(39, 25, topArrow, 5, 3, WHITE);  // hour set cursor
  // display.drawBitmap(56, 25, topArrow, 5, 3, WHITE); // min set cursor
  // display.setCursor(35, 16);

  display.setCursor(7, 35);
  display.print("Set");

  display.setCursor(7, 45);
  display.print("Save");

  display.setCursor(7, 55);
  display.print("Back");
}

void menuConfigDisplay() {
  clearDisplay();

  display.setCursor(18, 0);
  display.print("-- MENU --");

  display.setCursor(7, 20);
  display.print("Set Clock");

  display.setCursor(7, 30);
  display.print("Set Alarm");

  display.setCursor(7, 55);
  display.print("Back");
}

void clearDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  // display.display();
}