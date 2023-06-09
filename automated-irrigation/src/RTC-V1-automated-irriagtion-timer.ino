#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

RTC_DS1307 rtc;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

int relayPin = 2;
int modeButtonPin = 3;
int plusButtonPin = 4;
int minusButtonPin = 5;
int okButtonPin = 6;

int mode = 0;  // 0 para ajuste de hora, 1 para inclusão de timer
int hour = 0;
int minute = 0;
int selectedTimer = 0;
int timerCount = 0;
int timer1Hour = 0;
int timer1Minute = 0;
int timer2Hour = 0;
int timer2Minute = 0;

void setup() {
  rtc.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);

  pinMode(relayPin, OUTPUT);
  pinMode(modeButtonPin, INPUT_PULLUP);
  pinMode(plusButtonPin, INPUT_PULLUP);
  pinMode(minusButtonPin, INPUT_PULLUP);
  pinMode(okButtonPin, INPUT_PULLUP);

  // Carregar os tempos da memória EEPROM
  timerCount = EEPROM.read(0);
  timer1Hour = EEPROM.read(1);
  timer1Minute = EEPROM.read(2);
  timer2Hour = EEPROM.read(3);
  timer2Minute = EEPROM.read(4);
}

void loop() {
  DateTime now = rtc.now();

  // Ativar o relé se a hora atual bater com algum dos tempos programados
  if (now.hour() == timer1Hour && now.minute() == timer1Minute) {
    digitalWrite(relayPin, HIGH);
  } else if (now.hour() == timer2Hour && now.minute() == timer2Minute) {
    digitalWrite(relayPin, HIGH);
  } else {
    digitalWrite(relayPin, LOW);
  }

  // Verificar os botões
  if (digitalRead(modeButtonPin) == LOW) {
    delay(50);  // debounce
    if (digitalRead(modeButtonPin) == LOW) {
      mode = 1 - mode;  // Trocar o modo entre ajuste de hora e inclusão de timer
      while (digitalRead(modeButtonPin) == LOW) {}  // Aguardar o botão ser solto
    }
  }

  if (mode == 0) {
    // Modo de ajuste de hora
    if (digitalRead(plusButtonPin) == LOW) {
      delay(50);  // debounce
      if (digitalRead(plusButtonPin) == LOW) {
        hour = (hour + 1) % 24;  // Incrementar a hora
        while (digitalRead(plusButtonPin) == LOW) {}  // Aguardar o botão ser solto
      }
    }

    if (digitalRead(minusButtonPin) == LOW) {
      delay(50);  // debounce
      if (digitalRead(minusButtonPin) == LOW) {
        hour = (hour + 23) % 24;  // Decrementar a hora
        while (digitalRead(minusButtonPin) == LOW) {}  // Aguardar o botão ser solto
      }
    }

    if (digitalRead(okButtonPin) == LOW) {
      delay(50);  // debounce
      if (digitalRead(okButtonPin) == LOW) {
        rtc.adjust(DateTime(now.year(), now.month(), now.day(), hour, minute, 0));  // Ajustar a hora do RTC
        while (digitalRead(okButtonPin) == LOW) {}  // Aguardar o botão ser solto
      }
    }
  } else {
    // Modo de inclusão de timer
    if (digitalRead(plusButtonPin) == LOW) {
      delay(50);  // debounce
      if (digitalRead(plusButtonPin) == LOW) {
        selectedTimer = 1 - selectedTimer;  // Alternar entre Timer 1 e Timer 2
        while (digitalRead(plusButtonPin) == LOW) {}  // Aguardar o botão ser solto
      }
    }

    if (digitalRead(minusButtonPin) == LOW) {
      delay(50);  // debounce
      if (digitalRead(minusButtonPin) == LOW) {
        if (selectedTimer == 0) {
          timer1Hour = (timer1Hour + 1) % 24;  // Incrementar a hora do Timer 1
        } else {
          timer2Hour = (timer2Hour + 1) % 24;  // Incrementar a hora do Timer 2
        }
        while (digitalRead(minusButtonPin) == LOW) {}  // Aguardar o botão ser solto
      }
    }

    if (digitalRead(okButtonPin) == LOW) {
      delay(50);  // debounce
      if (digitalRead(okButtonPin) == LOW) {
        if (selectedTimer == 0) {
          timer1Minute = (timer1Minute + 1) % 60;  // Incrementar os minutos do Timer 1
        } else {
          timer2Minute = (timer2Minute + 1) % 60;  // Incrementar os minutos do Timer 2
        }
        while (digitalRead(okButtonPin) == LOW) {}  // Aguardar o botão ser solto
      }
    }
  }

  // Salvar os tempos na memória EEPROM
  EEPROM.write(0, timerCount);
  EEPROM.write(1, timer1Hour);
  EEPROM.write(2, timer1Minute);
  EEPROM.write(3, timer2Hour);
  EEPROM.write(4, timer2Minute);

  // Atualizar o display OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Current Time:");
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print(now.hour());
  display.print(":");
  if (now.minute() < 10) {
    display.print("0");
  }
  display.println(now.minute());
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("Timer 1: ");
  display.print(timer1Hour);
  display.print(":");
  if (timer1Minute < 10) {
    display.print("0");
  }
  display.println(timer1Minute);
  display.setCursor(0, 54);
  display.print("Timer 2: ");
  display.print(timer2Hour);
  display.print(":");
  if (timer2Minute < 10) {
    display.print("0");
  }
  display.println(timer2Minute);
  display.display();
}
