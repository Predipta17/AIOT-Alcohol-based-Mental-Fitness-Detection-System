#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

// Sensor Pins
#define MQ3_PIN 34     // MQ3 Alcohol Sensor
#define GSR_PIN 35     // GSR Sensor
#define REPORTING_PERIOD_MS 5000

PulseOximeter pox;
uint32_t tsLastReport = 0;

// Beat callback
void onBeatDetected() {
  Serial.println("♥ Beat detected!");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Initializing...");

  // Start I2C with ESP32 default I2C pins
  Wire.begin(21, 22);

  // MAX30100 Init
  if (!pox.begin()) {
    Serial.println("❌ MAX30100 not found!");
  } else {
    Serial.println("✅ MAX30100 initialized.");
    pox.setIRLedCurrent(MAX30100_LED_CURR_11MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
  }

  pinMode(MQ3_PIN, INPUT);
  pinMode(GSR_PIN, INPUT);
}

void loop() {
  pox.update();  // Read pulse oximeter

  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    tsLastReport = millis();

    float bpm = pox.getHeartRate();
    float spO2 = pox.getSpO2();

    int alcoholRaw = analogRead(MQ3_PIN);
    float alcoholVoltage = alcoholRaw * (3.3 / 4095.0);
    float alcoholLevel = alcoholVoltage / 3.3;

    int gsrRaw = analogRead(GSR_PIN);
    float gsrValue = gsrRaw / 4095.0;

    Serial.println("----- Sensor Readings -----");
    Serial.print("Heart Rate (BPM): ");
    Serial.println(bpm);
    Serial.print("SpO2 (%): ");
    Serial.println(spO2);
    Serial.print("Alcohol Level (Normalized): ");
    Serial.println(alcoholLevel, 6);
    Serial.print("GSR (Normalized): ");
    Serial.println(gsrValue, 6);
    Serial.println("----------------------------\n");
  }
}
