#include <SoftwareSerial.h>

SoftwareSerial hc12(10, 11); 

float filteredTemps[3] = {0, 0, 0}; 

void setup() {
  Serial.begin(9600);
  hc12.begin(9600);
}

void loop() {
  int analogPins[6] = {A0, A1, A2, A3, A4, A5};

  float roomTemps[3] = {0, 0, 0};  
  for (int i = 0; i < 3; i++) {
    float t1 = readTemperature(analogPins[i * 2]);
    float t2 = readTemperature(analogPins[i * 2 + 1]);
    float avg = (t1 + t2) / 2.0;
    filteredTemps[i] = 0.3 * filteredTemps[i] + 0.7 * avg;
    roomTemps[i] = filteredTemps[i];
  }

  Serial.print("Kitchen: ");
  Serial.print(roomTemps[0]);
  Serial.print(" °C, Living: ");
  Serial.print(roomTemps[1]);
  Serial.print(" °C, Reception: ");
  Serial.print(roomTemps[2]);
  Serial.println(" °C");

hc12.print("T1:");
hc12.print(roomTemps[0]);
hc12.print("C T2:");
hc12.print(roomTemps[1]);
hc12.print("C T3:");
hc12.print(roomTemps[2]);
hc12.println("C");


  delay(500);
}

float readTemperature(int pin) {
  int adcValue = analogRead(pin);
  float voltage = (adcValue * 5) / 1023.0;
  float temperatureC = voltage * 100.0 + 0.5;
  return temperatureC;
}
