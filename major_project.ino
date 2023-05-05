#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <DHT.h>                  //https://github.com/adafruit/DHT-sensor-library
#include <Firebase_ESP_Client.h>  //https://github.com/mobizt/Firebase-ESP-Client
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Servo.h>  // servo library


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

#define WIFI_SSID "Infinix HOT 7"
#define WIFI_PASSWORD "lkrjangid99"

#define USER_EMAIL "lkrjangid@gmail.com"
#define USER_PASSWORD "lkrjangid@123"

std::string databasePath;

int soilPin = 16,      // D0
  lightPin = 5;        // D1
int relay_fan_1 = 12,  // D6
  relay_led = 14,      // D5
  relay_pump = 13,     // D7
  humidity_refrance_value = 60,
    temprature_refrance_value = 25,
    is_operate_manually = 0,
    is_turn_on_LED = 0,
    is_turn_on_servo = 0,
    is_turn_on_fan_1 = 0,
    is_turn_on_pump = 0;

DHT dht2(0, DHT11);  // D4
Servo s1;

bool signupOK = false;

void setup() {
  s1.attach(4);  // servo attach D2 pin of arduino
  pinMode(soilPin, INPUT);
  pinMode(lightPin, INPUT);
  pinMode(relay_fan_1, OUTPUT);
  pinMode(relay_led, OUTPUT);
  pinMode(relay_pump, OUTPUT);
  dht2.begin();
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Connected!");
  Serial.println(WiFi.localIP());

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

#if defined(ESP8266)
    // In ESP8266 required for BearSSL rx/tx buffer for large data handle, increase Rx size as needed.
  fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 2048 /* Tx buffer size in bytes from 512 - 16384 */);
#endif
  // Limit the size of response payload to be collected in FirebaseData
  fbdo.setResponseSize(2048);
  Firebase.begin(&config, &auth);

  Serial.println("Firebase Config");
  timeClient.begin();
}

void loop() {
  epochTime = getTime();
  databasePath = "ghm_data/" + std::to_string(epochTime);
  getFirebaseData();
  int soilData = digitalRead(soilPin);    //Soil moisture threshold
  int lightData = digitalRead(lightPin);  //Light intensity
  float t = dht2.readTemperature();
  float h = dht2.readHumidity();

  if (is_operate_manually == 1) {
    manualOperate();
  } else {
    autoOperate(soilData, lightData, t, h);
  }

  if (Firebase.ready()) {
    // to store old data
    firebaseRTDBFunc(databasePath, t, h, lightData, soilData);
    // to work in real time
    firebaseRTDBFunc("ghm_real_time_data", t, h, lightData, soilData);
    delay(5000);
  } else {
    Serial.println("Error in connecting");
  }
}

void getFirebaseData() {
  Firebase.RTDB.getInt(&fbdo, "ghm_real_time_data/humidity_refrance_value");
  humidity_refrance_value = fbdo.to<int>();
  Firebase.RTDB.getInt(&fbdo, "ghm_real_time_data/temprature_refrance_value");
  temprature_refrance_value = fbdo.to<int>();
  Firebase.RTDB.getInt(&fbdo, "ghm_real_time_data/is_operate_manually");
  is_operate_manually = fbdo.to<int>();
  Firebase.RTDB.getInt(&fbdo, "ghm_real_time_data/is_turn_on_pump");
  is_turn_on_pump = fbdo.to<int>();
  Firebase.RTDB.getInt(&fbdo, "ghm_real_time_data/is_turn_on_fan_1");
  is_turn_on_fan_1 = fbdo.to<int>();
  Firebase.RTDB.getInt(&fbdo, "ghm_real_time_data/is_turn_on_servo");
  is_turn_on_servo = fbdo.to<int>();
  Firebase.RTDB.getInt(&fbdo, "ghm_real_time_data/is_turn_on_LED");
  is_turn_on_LED = fbdo.to<int>();
}

void manualOperate() {
  if (is_turn_on_pump == 1) {
    digitalWrite(relay_pump, HIGH);
  } else {
    digitalWrite(relay_pump, LOW);
  }
  if (is_turn_on_fan_1 == 1) {
    digitalWrite(relay_fan_1, HIGH);
  } else {
    digitalWrite(relay_fan_1, LOW);
  }
  if (is_turn_on_servo == 1) {
    s1.write(90);
  } else {
    s1.write(0);
  }
  if (is_turn_on_LED == 1) {
    digitalWrite(relay_led, HIGH);
  } else {
    digitalWrite(relay_led, LOW);
  }
}

void autoOperate(int soilData, int lightData, float t, float h) {
  if (soilData == 1) {
    digitalWrite(relay_pump, HIGH);
    sendFCMNotification("Warning water lavel is low", "It's time to irrigation");
  } else {
    digitalWrite(relay_pump, LOW);
  }

  if (lightData == 1) {
    digitalWrite(relay_led, LOW);
    sendFCMNotification("Warning", "It's time turn on the light");
  } else {
    digitalWrite(relay_led, HIGH);
  }

  if (t > temprature_refrance_value) {
    digitalWrite(relay_fan_1, LOW);
    digitalWrite(relay_led, LOW);
    sendFCMNotification("Warning high temperature", "It's time turn on the fan");
  } else {
    digitalWrite(relay_fan_1, HIGH);
  }

  if (h > humidity_refrance_value) {
    sendFCMNotification("Warning high humidity", "It's time to open the window");
    s1.write(90);
  } else {
    s1.write(0);
  }
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
