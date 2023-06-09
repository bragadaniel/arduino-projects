#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define LCD_ADDRESS 0x27
#define LCD_COL 16
#define LCD_ROW 2
#define RELAY_PIN 2
#define MODE_BUTTON_PIN 3
#define PLUS_BUTTON_PIN 4
#define MINUS_BUTTON_PIN 5
#define OK_BUTTON_PIN 6
#define RESET_BUTTON_PIN 7
#define LED_PIN 13
#define OK_CANCEL_TEXT "OK? or CANCEL?"
#define OPTIONS_SELECT_TEXT "1:Clock 2:Alarm"
#define MAIN_TITLE "Select Mode"
#define CLOCK_SETTINGS_TEXT " Set Clock"
#define ALARM_SETTINGS_TEXT " Set Alarm"
#define MAIN_TITLE_ALARM " Alarm"
#define MAIN_TITLE_CLOCK " Clock"
#define ALARM_CONFIG "HH:MM:ss"
#define CLOCK_CONFIG "HH:MM:ss"

// Testing typing in board
typedef void (*Func)();

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COL, LCD_ROW);

int mode = 0;
int submode = 0;
int slot = 0;
int selectedTimer = 0;
int timerHour[2] = {0, 0};
int timerMinute[2] = {0, 0};

int buttonState = HIGH;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// chars
byte clockChar[] = {B00000, B01110, B10101, B10101,
                    B10111, B10001, B10001, B01110};
byte alarmChar[] = {B00000, B00100, B01110, B01110,
                    B01110, B11111, B00100, B00000};

void setup() {
  Wire.begin();

  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PLUS_BUTTON_PIN, INPUT_PULLUP);
  pinMode(MINUS_BUTTON_PIN, INPUT_PULLUP);
  pinMode(OK_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  lcd.begin(LCD_COL, LCD_ROW);
  lcd.backlight();

  initialScreen();

  Serial.begin(9600);
}

void loop() {
  if (digitalRead(MODE_BUTTON_PIN) == LOW) {
    mainMenu();
  }

  if (mode == 1 && digitalRead(OK_BUTTON_PIN) == LOW) {
    clockSettings();
    while (submode == 1 && mode == 1) {
      if (digitalRead(RESET_BUTTON_PIN) == LOW) {
        submode = -1;
        clockModeScreen();
      }
      if (digitalRead(OK_BUTTON_PIN) == LOW) {
        savignScreen();
      }
    }
  } else if (mode == 1 && submode == -2 &&
             digitalRead(RESET_BUTTON_PIN) == LOW) {
    initialScreen();
  }

  if (mode == 2 && digitalRead(OK_BUTTON_PIN) == LOW) {
    alarmSettings();
    while (submode == 1 && mode == 2) {
      if (digitalRead(RESET_BUTTON_PIN) == LOW) {
        submode = -1;
        alarmModeScreen();
      }
      if (digitalRead(OK_BUTTON_PIN) == LOW) {
        savignScreen();
      }
    }
  } else if (mode == 2 && submode == -2 &&
             digitalRead(RESET_BUTTON_PIN) == LOW) {
    initialScreen();
  }

}  // eof loop

void initialScreen() {
  mode = 0;
  submode = 0;
  lcd.clear();
  lcd.createChar(0, alarmChar);
  lcd.setCursor(0, 0);
  lcd.write(byte(0));

  lcd.createChar(1, clockChar);
  lcd.setCursor(15, 0);
  lcd.write(byte(1));

  lcd.setCursor(2, 0);
  lcd.print(MAIN_TITLE);
  lcd.setCursor(0, 1);
  lcd.print(OPTIONS_SELECT_TEXT);
}

void clockModeScreen() {
  submode = -2;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(1);
  lcd.print(MAIN_TITLE_CLOCK);
  lcd.setCursor(0, 1);
  lcd.print(OK_CANCEL_TEXT);
}

void clockSettings() {
  submode = 1;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(1);
  lcd.print(CLOCK_SETTINGS_TEXT);
  lcd.setCursor(2, 1);
  lcd.print(CLOCK_CONFIG);
}

void alarmModeScreen() {
  submode = -2;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(0);
  lcd.print(MAIN_TITLE_ALARM);
  lcd.setCursor(0, 1);
  lcd.print(OK_CANCEL_TEXT);
}

void alarmSettings() {
  submode = 1;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(0);
  lcd.print(ALARM_SETTINGS_TEXT);
  lcd.setCursor(2, 1);
  lcd.print(ALARM_CONFIG);
}

void mainMenu() {
  mode++;
  switch (mode) {
    case 0:
      initialScreen();
      break;
    case 1:
      clockModeScreen();
      break;
    case 2:
      alarmModeScreen();
      break;
    default:
      mode = 0;
      initialScreen();
      break;
  }
}

void savignScreen() {
  lcd.setCursor(0, 1);
  lcd.print("Saving.");
  delay(200);
  lcd.print(".");
  delay(200);
  lcd.print(".");
  delay(200);
  lcd.print(".");
  delay(200);
  lcd.print(".");
  delay(200);
  lcd.print(".");
  delay(200);
  lcd.print(".");
  delay(200);
  lcd.print(".");
  delay(200);
  lcd.print(".");
}