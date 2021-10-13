#include <Wire.h> 
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h> 
#define SENSOR_PIN A0
#define VALVE_PIN 10
#define DRY_THRESHOLD 77

Adafruit_SSD1306 display = Adafruit_SSD1306();

const long int TIME_WATERING = 50;
int soilMoisture = 0;

void setup() {
  Wire.begin();
  pinMode(VALVE_PIN, OUTPUT);
  // valve off
  digitalWrite(VALVE_PIN, HIGH);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  display.setTextColor(WHITE);
  display.clearDisplay(); 
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Rega \ndo Mano");
  display.display();
  delay(2000);
  Serial.begin(9600);
}

void getWatering(){
  display.clearDisplay();
  display.setCursor(4, 2);
  display.println(F("Regando..."));
  display.display();
  // valve on
  digitalWrite(VALVE_PIN, LOW);
  // waiting time
  delay(TIME_WATERING*1000);
  digitalWrite(VALVE_PIN, HIGH);
}

void wateringWarning(){
  display.clearDisplay();
  display.setCursor(5, 0);
  display.println(F("Solo"));
  display.println(F("Encharcado!"));
  display.display();
  delay(3000);
} 

void loop() {
  for(int i=0; i < 5; i++) {
    display.clearDisplay();
    display.setCursor(20, 0);
    display.println("Umidade:");
    soilMoisture = analogRead(SENSOR_PIN);
    soilMoisture = map(soilMoisture, 1023, 0, 0, 100);
    display.setCursor(50, 18);
    display.print(soilMoisture);
    display.println("%");
    display.display();
    delay(1000);
  }
  
  if(soilMoisture < DRY_THRESHOLD) {
     getWatering();
  }
  else {
    wateringWarning();
  }
}
