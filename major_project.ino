#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <DHT.h>                  //https://github.com/adafruit/DHT-sensor-library
#include <Firebase_ESP_Client.h>  //https://github.com/mobizt/Firebase-ESP-Client
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Variable to save current epoch time
unsigned long epochTime;

// Function that gets current epoch time
unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

#define API_KEY "AIzaSyAwREwEc71sAk0TV1TT0nL8-wLHDSLT4G4"
#define DATABASE_URL "https://green-house-monitoring-mbm-default-rtdb.asia-southeast1.firebasedatabase.app/"

#define WIFI_SSID "Error 404"
#define WIFI_PASSWORD "helloworld"

#define USER_EMAIL "lkrjangid@gmail.com"
#define USER_PASSWORD "lkrjangid@123"

std::string databasePath;

int soilPin = 16, // D0
    lightPin = 3; // RX
int relay_fan_1 = 12, // D6
    relay_fan_2 = 13, // D7
    relay_led = 14, // D5
    relay_pump = 15; // D8

DHT dht2(2, DHT11); // D4

bool signupOK = false;

void setup() {
  pinMode(soilPin, INPUT);
  pinMode(lightPin, INPUT);
  pinMode(relay_fan_1, OUTPUT);
  pinMode(relay_fan_2, OUTPUT);
  pinMode(relay_led, OUTPUT);
  pinMode(relay_pump, OUTPUT);
  dht2.begin();
  Serial.begin(9600);
//   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(250);
//     Serial.print(".");
//   }
//   Serial.println("");
//   Serial.println("WiFi Connected!");
//   Serial.println(WiFi.localIP());

//   config.api_key = API_KEY;
//   config.database_url = DATABASE_URL;

//   auth.user.email = USER_EMAIL;
//   auth.user.password = USER_PASSWORD;

//   if (Firebase.signUp(&config, &auth, "", "")) {
//     Serial.println("ok");
//     signupOK = true;
//   } else {
//     Serial.printf("%s\n", config.signer.signupError.message.c_str());
//   }
//   config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

// #if defined(ESP8266)
//     // In ESP8266 required for BearSSL rx/tx buffer for large data handle, increase Rx size as needed.
//   fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 2048 /* Tx buffer size in bytes from 512 - 16384 */);
// #endif
//   // Limit the size of response payload to be collected in FirebaseData
//   fbdo.setResponseSize(2048);
//   Firebase.begin(&config, &auth);

//   Serial.println("Firebase Config");
//   timeClient.begin();
}

void loop() {
  // epochTime = getTime();
  // databasePath = "ghm_data/" + std::to_string(epochTime);
  // int soilData = digitalRead(soilPin);  //Soil moisture threshold
  // if (soilData == 1) {
  //   digitalWrite(relay_pump, LOW);
  // } else if (soilData == 0) {
  //   digitalWrite(relay_pump, HIGH);
  // }

  int lightData = digitalRead(lightPin);  //Light intensity
  Serial.println("light");
  Serial.println(lightData);
  // if (lightData == 1) {
  //   digitalWrite(relay_led, LOW);
  // } else if (lightData == 0) {
  //   digitalWrite(relay_led, HIGH);
  // }

  // float t = dht2.readTemperature();
  // if (t > 35) {
  //   digitalWrite(relay_fan_1, LOW);
  // } else if (t < 35) {
  //   digitalWrite(relay_fan_1, HIGH);
  // }

  // float h = dht2.readHumidity();
  // if (h > 60) {
  //   digitalWrite(relay_fan_2, LOW);
  // } else if (h < 60) {
  //   digitalWrite(relay_fan_2, HIGH);
  // }

  // if (Firebase.ready()) {
  //   // to store old data
  //   firebaseRTDBFunc(databasePath, t, h, lightData, soilData);

  //   // to work in real time
  //   firebaseRTDBFunc("ghm_real_time_data", t, h, lightData, soilData);

    delay(1000);
  // } else {
  //   Serial.println("Error in connecting");
  // }
}


void firebaseRTDBFunc(std::string databasePath, int temperature, int humidity, float lightData, float soilData) {
  if (Firebase.RTDB.setString(&fbdo, databasePath + "/timestamp", std::to_string(epochTime))) {
    Serial.println("PASSED PATH: " + fbdo.dataPath());
  } else {
    Serial.println("FAILED: " + fbdo.errorReason());
  }
  if (Firebase.RTDB.setFloat(&fbdo, databasePath + "/temperature", temperature)) {
    Serial.println("PASSED PATH: " + fbdo.dataPath());
  } else {
    Serial.println("FAILED: " + fbdo.errorReason());
  }
  if (Firebase.RTDB.setFloat(&fbdo, databasePath + "/humidity", humidity)) {
    Serial.println("PASSED PATH: " + fbdo.dataPath());
  } else {
    Serial.println("FAILED: " + fbdo.errorReason());
  }
  if (Firebase.RTDB.setInt(&fbdo, databasePath + "/lightData", lightData)) {
    Serial.println("PASSED PATH: " + fbdo.dataPath());
  } else {
    Serial.println("FAILED: " + fbdo.errorReason());
  }
  if (Firebase.RTDB.setInt(&fbdo, databasePath + "/soilData", soilData)) {
    Serial.println("PASSED PATH: " + fbdo.dataPath());
  } else {
    Serial.println("FAILED: " + fbdo.errorReason());
  }
}

void sendFCMNotification(std::string title, std::string body) {
  Serial.print("Sending...");
  // Read more details about legacy HTTP API here https://firebase.google.com/docs/cloud-messaging/http-server-ref
  FCM_Legacy_HTTP_Message msg;

  msg.options.priority = "high";
  msg.payloads.notification.title = title;
  msg.payloads.notification.body = body;

  if (Firebase.FCM.send(&fbdo, &msg))  // send message to recipient
    Serial.printf("ok\n%s\n\n", Firebase.FCM.payload(&fbdo).c_str());
  else
    Serial.println(fbdo.errorReason());
  Serial.print("Sended");
}
