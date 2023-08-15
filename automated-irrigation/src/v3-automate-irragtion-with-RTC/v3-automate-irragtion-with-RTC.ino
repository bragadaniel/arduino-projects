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

// menu config
byte menuOptionCount = 0;
byte menuEditOptionCount = 0;
byte selectOption = 0;
bool mainMenu = false;
bool alarmEditMenu = false;
bool setHours = false;
bool setMinutes = false;
bool save = false;

// buttons
byte btnMode, btnUp, btnDown;

// clock settings
int hours = 0, minutes = 0;

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
}  // eof setup

void loop() {
  readyButtons();
  // display menu and select options
  if (mainMenu == false) {
    btnDebounce.runFnDebounce(btnMode, &lastButtonStates[0], &buttonStates[0],
                              DEBOUNCE_DELAY, &lastDebounceTimes[0],
                              modeFnDebounce);
    initialDisplay();
  }
  while (mainMenu == true) {
    readyButtons();
    switch (menuOptionCount) {
      case 1:  // Alarm 1
        menuAlarmConfig(menuOptionCount);
        break;
      case 2:  // Alarm 2
        menuAlarmConfig(menuOptionCount);
        break;
      case 3:  // back option
        menuAlarmConfig(menuOptionCount);
        break;
      default:
        initialDisplay();
        break;
    }
    // debounce action buttons
    btnDebounce.runFnDebounce(btnMode, &lastButtonStates[0], &buttonStates[0],
                              DEBOUNCE_DELAY, &lastDebounceTimes[0],
                              modeFnAlarmDebounce);

    btnDebounce.runFnDebounce(btnDown, &lastButtonStates[1], &buttonStates[1],
                              DEBOUNCE_DELAY, &lastDebounceTimes[1],
                              downFnDebounce);

    btnDebounce.runFnDebounce(btnUp, &lastButtonStates[2], &buttonStates[2],
                              DEBOUNCE_DELAY, &lastDebounceTimes[2],
                              upFnDebounce);

    display.display();
  }

  while (alarmEditMenu == true) {
    readyButtons();
    switch (menuEditOptionCount) {
      case 1:  // set hour
        menuAlarmEdit(menuEditOptionCount);
        break;
      case 2:  // set min
        menuAlarmEdit(menuEditOptionCount);
        break;
      case 3:  // save option
        menuAlarmEdit(menuEditOptionCount);
        break;
      case 4:  // back option
        menuAlarmEdit(menuEditOptionCount);
        break;
      default:
        break;
    }
    // debounce action buttons
    btnDebounce.runFnDebounce(btnMode, &lastButtonStates[0], &buttonStates[0],
                              DEBOUNCE_DELAY, &lastDebounceTimes[0],
                              modeEditFnAlarmDebounce);

    btnDebounce.runFnDebounce(btnUp, &lastButtonStates[2], &buttonStates[2],
                              DEBOUNCE_DELAY, &lastDebounceTimes[2],
                              upEditAlarmFnDebounce);

    btnDebounce.runFnDebounce(btnDown, &lastButtonStates[1], &buttonStates[1],
                              DEBOUNCE_DELAY, &lastDebounceTimes[1],
                              downEditAlarmFnDebounce);

    display.display();
  }
}  // eof

void initialDisplay() {
  clearDisplay();  // clear display

  display.setCursor(20, 0);
  display.print("** WELCOME **");

  display.setCursor(7, 16);
  display.print("Press yellow button");
  display.setCursor(0, 26);
  display.print("Current time:");
  getTime();
  display.setCursor(0, 47);
  display.print("Next irrigation:");
  display.setCursor(0, 57);
  display.print("19:25:30");
  display.display();
}

void menuAlarmEdit(byte selectOption) {
  alarmEditDisplay();
  switch (selectOption) {
    case 1:  // adjust hours
      display.drawBitmap(38, 25, topArrow, 5, 3, WHITE);
      break;
    case 2:  // adjust minutes
      display.drawBitmap(56, 25, topArrow, 5, 3, WHITE);
      break;
    case 3:  // save option
      display.drawBitmap(0, 45, sideArrow, 3, 5, WHITE);
      break;
    case 4:  // back option
      display.drawBitmap(0, 55, sideArrow, 3, 5, WHITE);
      break;
    default:
      selectOption = 0;
      break;
  }
}

void menuAlarmConfig(byte selectOption) {
  alarmsViewDisplay();
  switch (selectOption) {
    case 1:  // Alarm 1 config
      display.drawBitmap(0, 22, sideArrow, 3, 5, WHITE);
      break;
    case 2:  // Alarm 2 config
      display.drawBitmap(0, 32, sideArrow, 3, 5, WHITE);
      break;
    case 3:  // back option
      display.drawBitmap(0, 55, sideArrow, 3, 5, WHITE);
      break;
    default:
      selectOption = 0;
      break;
  }
}

void alarmsViewDisplay() {
  clearDisplay();
  char alarmHourStr1[3];
  char alarmMinStr1[3];
  char alarmHourStr2[3];
  char alarmMinStr2[3];
  byte alarmHour1 = EEPROM.read(0);
  byte alarmMin1 = EEPROM.read(1);
  byte alarmHour2 = EEPROM.read(2);
  byte alarmMin2 = EEPROM.read(3);
  sprintf(alarmHourStr1, "%02d", alarmHour1);
  sprintf(alarmMinStr1, "%02d", alarmMin1);
  sprintf(alarmHourStr2, "%02d", alarmHour2);
  sprintf(alarmMinStr2, "%02d", alarmMin2);

  display.setCursor(20, 0);
  display.print("** ALARMS **");

  display.setCursor(7, 20);
  display.print("#1 Alarm: ");
  display.print(alarmHourStr1);
  display.print(":");
  display.print(alarmMinStr1);

  display.setCursor(7, 30);
  display.print("#2 Alarm: ");
  display.print(alarmHourStr2);
  display.print(":");
  display.print(alarmMinStr2);

  display.setCursor(7, 55);
  display.print("Back");
}

void alarmEditDisplay() {
  char strHours[3];
  char strMinutes[3];
  clearDisplay();
  display.setCursor(10, 0);
  display.print("Edit Alarm #");
  display.print(menuOptionCount);

  display.setCursor(35, 16);
  sprintf(strHours, "%02d", hours);
  display.print(strHours);
  display.print(":");
  sprintf(strMinutes, "%02d", minutes);
  display.print(strMinutes);
  display.print(":00");

  display.setCursor(7, 45);
  display.print("Save");

  display.setCursor(7, 55);
  display.print("Back");
}

// get time
void getTime() {
  char hoursStr[3];
  char minuteStr[3];
  char secondStr[3];

  DateTime now = rtc.now();
  display.setCursor(0, 38);
  sprintf(hoursStr, "%02d", now.hour());
  display.print(hoursStr);
  display.print(":");
  sprintf(minuteStr, "%02d", now.minute());
  display.print(minuteStr);
  display.print(":");
  sprintf(secondStr, "%02d", now.second());
  display.print(secondStr);
}

// ### ACTIONS BUTTONS ###
// role modeBtn for initial screen
void modeFnDebounce() {
  buttonStates[0] = btnMode;
  if (buttonStates[0] == LOW && mainMenu == false) {
    mainMenu = true;
    menuOptionCount = 1;
  }
}

void modeFnAlarmDebounce() {
  buttonStates[0] = btnMode;
  if (buttonStates[0] == LOW && menuOptionCount == 1) {  // Alarm 1 edit
    mainMenu = false;
    menuEditOptionCount = 1;
    alarmEditMenu = true;
  }

  if (buttonStates[0] == LOW && menuOptionCount == 2) {  // Alarm 2 edit
    mainMenu = false;
    menuEditOptionCount = 1;
    alarmEditMenu = true;
  }

  if (buttonStates[0] == LOW && menuOptionCount == 3) {  // back initial screen
    mainMenu = false;
    alarmEditMenu = false;
    menuEditOptionCount = 0;
  }
}

void modeEditFnAlarmDebounce() {
  buttonStates[0] = btnMode;

  if (buttonStates[0] == LOW && menuEditOptionCount == 1) {  // hour edit
    setMinutes = false;
    if (!setHours) {
      setHours = true;
    } else {
      setHours = false;
    }
  }

  if (buttonStates[0] == LOW && menuEditOptionCount == 2) {  // Minutes edit
    setHours = false;
    if (!setMinutes) {
      setMinutes = true;
    } else {
      setMinutes = false;
    }
  }

  if (buttonStates[0] == LOW && menuEditOptionCount == 3) {  // save in memory
    savingDisplay();

    if (menuOptionCount == 1) {  // save alarm1
      EEPROM.write(0, (byte)(hours));
      EEPROM.write(1, (byte)(minutes));
    }
    if (menuOptionCount == 2) {  // save alarm2
      EEPROM.write(2, (byte)(hours));
      EEPROM.write(3, (byte)(minutes));
    }
    mainMenu = true;
    alarmEditMenu = false;
    setHours = false;
    setMinutes = false;
    menuOptionCount = 1;
    menuEditOptionCount = 0;
  }

  if (buttonStates[0] == LOW && menuEditOptionCount == 4 &&
      !setHours) {  // back edit screen
    mainMenu = true;
    alarmEditMenu = false;
    setHours = false;
    setMinutes = false;
    menuOptionCount = 1;
    menuEditOptionCount = 0;
  }
}

// up and down roles
void upFnDebounce() {
  buttonStates[2] = btnUp;
  if (buttonStates[2] == LOW) {
    menuOptionCount--;
    if (menuOptionCount < 1) menuOptionCount = 3;
  }
}

void upEditAlarmFnDebounce() {
  buttonStates[2] = btnUp;
  if (buttonStates[2] == LOW && (!setHours && !setMinutes)) {
    menuEditOptionCount--;
    if (menuEditOptionCount < 1) menuEditOptionCount = 4;
  }
  if (buttonStates[2] == LOW && setHours) {  // decrement hours
    hours = (hours - 1 + 24) % 24;
  }
  if (buttonStates[2] == LOW && setMinutes) {  // decrement minutes
    minutes = (minutes + 59) % 60;
  }
}

void downEditAlarmFnDebounce() {
  buttonStates[1] = btnDown;
  if (buttonStates[1] == LOW && (!setHours && !setMinutes)) {
    menuEditOptionCount++;
    if (menuEditOptionCount > 4) menuEditOptionCount = 1;
  }

  if (buttonStates[1] == LOW && setHours) {  // increment hours
    hours = (hours + 1) % 24;
  }
  if (buttonStates[1] == LOW && setMinutes) {  // increment minutes
    minutes = (minutes - 59 + 60) % 60;
  }
}

void downFnDebounce() {
  buttonStates[1] = btnDown;
  if (buttonStates[1] == LOW) {
    menuOptionCount++;
    if (menuOptionCount > 3) menuOptionCount = 1;
  }
}

// ready buttons
void readyButtons() {
  btnMode = digitalRead(MODE_BUTTON_PIN);
  btnUp = digitalRead(UP_BUTTON_PIN);
  btnDown = digitalRead(DOWN_BUTTON_PIN);
}

void savingDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(35, 20);
  display.print("Saving...");
  display.display();
  delay(900);
}
// clear display
void clearDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  // display.display();
}