#include <Wire.h>
#include "HUSKYLENS.h"

#define RELAY_PIN 3
#define COOLDOWN_DURATION 5000  // 5 seconds in milliseconds

HUSKYLENS huskylens;
unsigned long lastLockTime = 0;  // Tracks when the relay was last locked
bool isUnlocked = false;         // Tracks relay state

void setup() {
  Serial.begin(115200);
  Wire.begin();
  while (!huskylens.begin(Wire)) {
    Serial.println(F("HuskyLens I2C failed!"));
    Serial.println(F("1. Check Protocol Type in HUSKYLENS (General Settings>>Protocol Type>>I2C)"));
    Serial.println(F("2. Check connection."));
    delay(1000);
  }
  Serial.println(F("HuskyLens connected"));
  huskylens.writeAlgorithm(ALGORITHM_FACE_RECOGNITION);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Initialize relay to off (locked)
}

void loop() {
  unsigned long currentTime = millis();  // Get current time
  bool faceDetected = false;
  bool authorizedFacePresent = false;

  // Read HuskyLens data
  if (!huskylens.request()) {
    Serial.println(F("Fail to request data from HUSKYLENS, recheck the connection!"));
    digitalWrite(RELAY_PIN, LOW);  // Lock if no connection
    isUnlocked = false;
  } else if (!huskylens.isLearned()) {
    Serial.println(F("Nothing learned, press learn button on HUSKYLENS!"));
    digitalWrite(RELAY_PIN, LOW);  // Lock if nothing learned
    isUnlocked = false;
  } else if (!huskylens.available()) {
    Serial.println(F("No face detected!"));
    Serial.println("0");
    if (isUnlocked) {
      digitalWrite(RELAY_PIN, LOW);  // Lock when face disappears
      Serial.println(F("Locked"));
      lastLockTime = currentTime;    // Start cooldown
      isUnlocked = false;
    }
  } else {
    while (huskylens.available()) {
      HUSKYLENSResult result = huskylens.read();
      if (result.command == COMMAND_RETURN_BLOCK && result.ID > 0) {
        // Print coordinates
        Serial.print(result.ID);
        Serial.print(",");
        Serial.print(result.xCenter);
        Serial.print(",");
        Serial.print(result.yCenter);
        Serial.print(",");
        Serial.print(result.width);
        Serial.print(",");
        Serial.println(result.height);
        faceDetected = true;

        // Check for authorized face IDs (1 or 2)
        if (result.ID == 1 || result.ID == 2) {
          authorizedFacePresent = true;
        }
      }
    }
    // Handle relay based on authorized face presence
    if (authorizedFacePresent && !isUnlocked && (currentTime - lastLockTime >= COOLDOWN_DURATION)) {
      Serial.println(F("Authorized face detected! Unlocking..."));
      digitalWrite(RELAY_PIN, HIGH);  // Unlock
      isUnlocked = true;
    } else if (!authorizedFacePresent && isUnlocked) {
      Serial.println(F("Authorized face gone! Locking..."));
      digitalWrite(RELAY_PIN, LOW);   // Lock
      lastLockTime = currentTime;     // Start cooldown
      isUnlocked = false;
    }
    if (!faceDetected) {
      Serial.println("0");  // No face detected
      if (isUnlocked) {
        delay(1000); // Wait 1 second before locking
        digitalWrite(RELAY_PIN, LOW);  // Lock if no authorized face
        Serial.println(F("Locked"));
        lastLockTime = currentTime;
        isUnlocked = false;
      }
    }
  }
  delay(200);  // Balance responsiveness and load
}