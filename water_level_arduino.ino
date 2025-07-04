#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <Firebase_ESP_Client.h>
#include <WiFiUdp.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Wi-Fi credentials

#define WIFI_SSID "OnePlus 11R 5G"
#define WIFI_PASSWORD "1234567890"

// Firebase credentials
#define API_KEY "AIzaSyBE-cnP72d3Wzf3987UD7wuMNWQG-YKwNI"
#define DATABASE_URL "https://realtimemotortank-default-rtdb.europe-west1.firebasedatabase.app/" // must end with '/'

bool signupOK = false;
unsigned long sendDataPrevMillis = 0;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000);

#define TRIG_PIN D5
#define ECHO_PIN D6

int lastSentHour = -1;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(D2, OUTPUT);
  digitalWrite(D2, LOW);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.status());
  timeClient.begin();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = "";
auth.user.password = "";
if (Firebase.signUp (&config, &auth, "", "") ) {
Serial.print ("sign up ok");
signupOK=true;
}else{
Serial.print (config.signer.signupError.message.c_str());
}
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Connected to Firebase");
}


float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.0343 / 2.0;
}

String getTimestamp() {
  timeClient.update();
  time_t raw = timeClient.getEpochTime();
  struct tm* ti = localtime(&raw);

  char buffer[30];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
          ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
          ti->tm_hour, ti->tm_min, ti->tm_sec);
  return String(buffer);
}

int getDayOfWeek() {
  timeClient.update();
  time_t raw = timeClient.getEpochTime();
  struct tm* ti = localtime(&raw);
  return ti->tm_wday;
}

int getCurrentHour() {
  timeClient.update();
  time_t raw = timeClient.getEpochTime();
  struct tm* ti = localtime(&raw);
  return ti->tm_hour;
}


void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 2000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    int currentHour = getCurrentHour();

    // ✅ Hourly sensor data upload
    if (currentHour != lastSentHour) {
      lastSentHour = currentHour;

      float distance = readDistanceCM();
      String timestamp = getTimestamp();
      int dayOfWeek = getDayOfWeek();

      FirebaseJson json;
      json.set("timestamp", timestamp);
      json.set("day", dayOfWeek);
      json.set("distance_cm", distance);

      if (Firebase.RTDB.pushJSON(&fbdo, "/sensor_logs", &json)) {
        Serial.println("✅ Hourly data uploaded:");
        Serial.println("Timestamp: " + timestamp);
        Serial.println("Day: " + String(dayOfWeek));
        Serial.println("Distance: " + String(distance) + " cm");
      } else {
        Serial.println("❌ Upload failed: " + fbdo.errorReason());
      }
    }
  }

  // ✅ Motor status check should happen all the time
  if (Firebase.ready() && signupOK) {
    if (Firebase.RTDB.getBool(&fbdo, "/motor_status")) {
      if (fbdo.dataType() == "boolean") {
        boolean motorState = fbdo.boolData();
        Serial.print("Successful READ from " + fbdo.dataPath() + ": " + motorState + " {" + fbdo.dataType() + "}\n");

        if (motorState) {
          digitalWrite(D2, HIGH);  // Turn motor ON
          Serial.println("Motor turned ON");
        } else {
          digitalWrite(D2, LOW);   // Turn motor OFF
          Serial.println("Motor turned OFF");
        }
      }
    } else {
      Serial.print("FAILED: " + fbdo.errorReason() + "\n");
    }
  }
}

