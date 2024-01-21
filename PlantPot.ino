
#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>)
#include <WiFiS3.h>
#endif

#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#define WIFI_SSID "Redmi Note 8 Pro"
#define WIFI_PASSWORD "321123456"

#define API_KEY "AIzaSyDM0iZyZKfvO3S9g1miCdPrAGWJKX2kZ-E"

#define USER_EMAIL "nongaathong@gmail.com"
#define USER_PASSWORD "Sasiraksa12345"

#define DATABASE_URL "smart-plant-pot-65987-default-rtdb.asia-southeast1.firebasedatabase.app" 
#define DATABASE_SECRET "PneOjq0nn3pSMJMVXAlNaMKv3SWbmABhGBMAwtmh"

#define MoistureSensorPin A0

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long dataMillis = 0;
int count = 0;


void setup()
{
    pinMode(MoistureSensorPin, INPUT);
    Serial.begin(115200);

    Serial.print("Connecting to Wi-Fi");
    unsigned long ms = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);

    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    config.api_key = API_KEY;

    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    config.database_url = DATABASE_URL;

    Firebase.reconnectNetwork(true);
    fbdo.setBSSLBufferSize(4096, 1024);

    fbdo.setResponseSize(4096);

    String base_path = "/UsersData/";

    config.token_status_callback = tokenStatusCallback;
    Firebase.begin(&config, &auth);
    String var = "$userId";
    String val = "($userId === auth.uid && auth.token.premium_account === true && auth.token.admin === true)";
    Firebase.RTDB.setReadWriteRules(&fbdo, base_path, var, val, val, DATABASE_SECRET);

}

void loop()
{
    if (millis() - dataMillis > 1500 && Firebase.ready())
    {
      int msValue = analogRead(MoistureSensorPin);
      int msPercent = map(msValue, 0, 1023, 0, 100);
      Serial.println(msPercent);

        dataMillis = millis();
        Firebase.RTDB.setInt(&fbdo, "/Status/Humidity", msPercent);
    }
}