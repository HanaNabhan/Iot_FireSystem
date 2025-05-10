#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// HC-12 Serial pins
#define RXD2 16
#define TXD2 17

// LEDs
#define KITCHEN_ALARM_BLUE_LED     13
#define LIVING_ALARM_RED_LED       14
#define RECEPTION_ALARM_GREEN_LED  27

// Buzzer
#define BUZZER_PIN                 4

// Wi-Fi credentials
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// IFTTT URLs
const char* iftttUrlKitchen   = "https://maker.ifttt.com/trigger/fire_alert_kitchen /with/key/";
const char* iftttUrlLiving    = "https://maker.ifttt.com/trigger/fire_alert_livingROOM/with/key/";
const char* iftttUrlReception = "https://maker.ifttt.com/trigger/fire_alert_reception /with/key/";

// ThingSpeak
const char* apiKey = "PCX94AD9IZL6BQ13";
const char* server = "http://api.thingspeak.com/update";

// Timing and threshold
const float threshold = 40.0;
const unsigned long alertCooldown = 60000;  // 1 minute
const unsigned long updateInterval = 20000;

unsigned long lastAlertKitchen = 0;
unsigned long lastAlertLiving = 0;
unsigned long lastAlertReception = 0;
unsigned long lastThingSpeakUpdate = 0;
unsigned long thresholdStartTime = 0;

bool buzzerOn = false;
bool overThreshold = false;

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  pinMode(KITCHEN_ALARM_BLUE_LED, OUTPUT);
  pinMode(LIVING_ALARM_RED_LED, OUTPUT);
  pinMode(RECEPTION_ALARM_GREEN_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(KITCHEN_ALARM_BLUE_LED, LOW);
  digitalWrite(LIVING_ALARM_RED_LED, LOW);
  digitalWrite(RECEPTION_ALARM_GREEN_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Fire Alarm Sys");
  delay(2000);
  lcd.clear();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
}

void loop() {
  if (Serial2.available()) {
    String msg = Serial2.readStringUntil('\n');
    msg.trim();
    if (msg.length() > 0) {
      parseAndAct(msg);
    }
  }
}

void parseAndAct(String data) {
  Serial.println("Received: " + data);

  float t1 = extractTemperature(data, "T1:");
  float t2 = extractTemperature(data, "T2:");
  float t3 = extractTemperature(data, "T3:");

  bool t1Over = t1 >= threshold;
  bool t2Over = t2 >= threshold;
  bool t3Over = t3 >= threshold;

  digitalWrite(KITCHEN_ALARM_BLUE_LED, t1Over ? HIGH : LOW);
  digitalWrite(LIVING_ALARM_RED_LED, t2Over ? HIGH : LOW);
  digitalWrite(RECEPTION_ALARM_GREEN_LED, t3Over ? HIGH : LOW);

  lcd.clear();
  if (t1Over || t2Over || t3Over) {
    lcd.setCursor(0, 0);
    lcd.print("Fire in:");
    if (t1Over) lcd.print(" KIT");
    if (t2Over) lcd.print(" LIV");
    if (t3Over) lcd.print(" REC");
    lcd.setCursor(0, 1);
    lcd.print(t1, 1); lcd.print(" ");
    lcd.print(t2, 1); lcd.print(" ");
    lcd.print(t3, 1);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("All Temps OK");
    lcd.setCursor(0, 1);
    lcd.print(t1, 1); lcd.print(" ");
    lcd.print(t2, 1); lcd.print(" ");
    lcd.print(t3, 1);
  }

  // Buzzer Logic
  if (t1Over || t2Over || t3Over) {
    if (!overThreshold) {
      thresholdStartTime = millis();
      overThreshold = true;
    } else if (!buzzerOn && millis() - thresholdStartTime > 5000) {
      digitalWrite(BUZZER_PIN, HIGH);
      buzzerOn = true;
    }

    // IFTTT Alerts
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      if (t1Over && millis() - lastAlertKitchen >= alertCooldown) {
        http.begin(iftttUrlKitchen);
        int httpCode = http.GET();
        Serial.println("Kitchen Alert Code: " + String(httpCode));
        http.end();
        lastAlertKitchen = millis();
      }

      if (t2Over && millis() - lastAlertLiving >= alertCooldown) {
        http.begin(iftttUrlLiving);
        int httpCode = http.GET();
        Serial.println("Living Alert Code: " + String(httpCode));
        http.end();
        lastAlertLiving = millis();
      }

      if (t3Over && millis() - lastAlertReception >= alertCooldown) {
        http.begin(iftttUrlReception);
        int httpCode = http.GET();
        Serial.println("Reception Alert Code: " + String(httpCode));
        http.end();
        lastAlertReception = millis();
      }
    }

  } else {
    digitalWrite(BUZZER_PIN, LOW);
    overThreshold = false;
    buzzerOn = false;
  }

  // ThingSpeak Update
  if (WiFi.status() == WL_CONNECTED && millis() - lastThingSpeakUpdate > updateInterval) {
    HTTPClient http;
    String url = String(server) + "?api_key=" + apiKey +
                 "&field1=" + String(t1) +
                 "&field2=" + String(t2) +
                 "&field3=" + String(t3);
    http.begin(url);
    int httpCode = http.GET();
    Serial.println("ThingSpeak Code: " + String(httpCode));
    http.end();
    lastThingSpeakUpdate = millis();
  }

  // Debug
  Serial.println("T1: " + String(t1) + "C | LED: " + (t1Over ? "ON" : "OFF"));
  Serial.println("T2: " + String(t2) + "C | LED: " + (t2Over ? "ON" : "OFF"));
  Serial.println("T3: " + String(t3) + "C | LED: " + (t3Over ? "ON" : "OFF"));
}

float extractTemperature(String data, String label) {
  int index = data.indexOf(label);
  if (index == -1) return 0;
  int start = index + label.length();
  int end = data.indexOf("C", start);
  if (end == -1) return 0;
  String valueStr = data.substring(start, end);
  return valueStr.toFloat();
}
