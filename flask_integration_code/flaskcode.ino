#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "MAX30100_PulseOximeter.h"

// ====== Settings ======
#define REPORTING_PERIOD_MS 5000  // 5 seconds
#define MQ3_PIN 34                // MQ3 alcohol sensor analog pin
#define GSR_PIN 35                // GSR sensor analog pin

PulseOximeter pox;
uint32_t tsLastReport = 0;

// ====== WiFi Credentials ======
const char* ssid = "vivo Y27";
const char* password = "Saketh20";

// ====== Flask Server URL ======
const char* serverUrl = "http://192.168.253.236:5000/predict";

// ====== Beat Detected Callback ======
void onBeatDetected() {
  Serial.println("♥ Beat!");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Initializing sensors...");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected. IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize MAX30100 sensor
  Wire.begin(21, 22); // SDA, SCL pins
  if (!pox.begin()) {
    Serial.println("❌ MAX30100 not found. Check connections!");
  } else {
    Serial.println("✅ MAX30100 initialized.");
    pox.setIRLedCurrent(MAX30100_LED_CURR_11MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
  }

  // MQ3 and GSR sensor setup
  pinMode(MQ3_PIN, INPUT);
  pinMode(GSR_PIN, INPUT);
}

void loop() {
  pox.update();  // Update MAX30100 continuously

  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    tsLastReport = millis();

    // Collect MAX30100 readings
    float spO2 = pox.getSpO2();
    float bpm = pox.getHeartRate();

    // MQ3 - Alcohol sensor
    int alcoholRaw = analogRead(MQ3_PIN);
    float voltage = alcoholRaw * (3.3 / 4095.0);
    float alcoholLevelV = voltage / 3.3;  // Normalize to 0-1

    // GSR sensor
    int gsrRaw = analogRead(GSR_PIN);
    float gsrNormalized = gsrRaw / 4095.0;

    // Print data
    Serial.println("------ Sensor Data ------");
    Serial.print("Heart Rate (BPM): "); Serial.println(bpm);
    Serial.print("SpO2 (%): "); Serial.println(spO2);
    Serial.print("Alcohol Level (Normalized): "); Serial.println(alcoholLevelV, 6);
    Serial.print("GSR Raw: "); Serial.print(gsrRaw);
    Serial.print(" | GSR Normalized: "); Serial.println(gsrNormalized, 6);

    // Send to Flask
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");

      // Prepare JSON
      StaticJsonDocument<256> doc;
      doc["Heart_Rate_BPM"] = bpm;
      doc["SpO2_Percent"] = spO2;
      doc["Alcohol_Level_V"] = alcoholLevelV;
      doc["GSR_Value"] = gsrNormalized;

      String jsonPayload;
      serializeJson(doc, jsonPayload);

      int responseCode = http.POST(jsonPayload);
      if (responseCode > 0) {
        Serial.print("✅ HTTP Response: ");
        Serial.println(responseCode);
        String response = http.getString();
        Serial.println("Response: " + response);
      } else {
        Serial.print("❌ Failed to send. Error: ");
        Serial.println(responseCode);
      }

      http.end();
    } else {
      Serial.println("❌ WiFi not connected");
    }
  }
}

