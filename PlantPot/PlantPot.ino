
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
#define TriggerPin D5
#define EchoPin D6
#define Relay D1

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long dataMillis = 0;
int wateringBatchMillis = 0;
int wateringBatchWait = 0;
int isWateringOn = 0;
int count = 0;
bool isInstanceWatering;
float cm;
float duration;
float specificHumidity;
float Humidity;

// Generic catch-all implementation.
template <typename T_ty> struct TypeInfo { static const char * name; };
template <typename T_ty> const char * TypeInfo<T_ty>::name = "unknown";

// Handy macro to make querying stuff easier.
#define TYPE_NAME(var) TypeInfo< typeof(var) >::name

// Handy macro to make defining stuff easier.
#define MAKE_TYPE_INFO(type)  template <> const char * TypeInfo<type>::name = #type;

// Type-specific implementations.
MAKE_TYPE_INFO( int )
MAKE_TYPE_INFO( float )
MAKE_TYPE_INFO( short )
MAKE_TYPE_INFO( bool )

void setup()
{
    pinMode(MoistureSensorPin, INPUT);
    pinMode(EchoPin, INPUT);
    pinMode(TriggerPin, OUTPUT);
    pinMode(Relay, OUTPUT);
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
    digitalWrite(Relay, HIGH);
}

void loop()
{
    if (millis() - dataMillis > 1500 && Firebase.ready())
    {
      int msValue = analogRead(MoistureSensorPin);
      Humidity = map(msValue, 0, 1023, 0, 100);

        dataMillis = millis();
        Firebase.RTDB.setInt(&fbdo, "/Status/Humidity", Humidity);

        digitalWrite(TriggerPin, LOW);
        delayMicroseconds(2);
        digitalWrite(TriggerPin, HIGH);
        delayMicroseconds(5);
        digitalWrite(TriggerPin, LOW);
        duration = pulseIn(EchoPin, HIGH);

        Serial.printf("real value %f \n", 8 - microsecondsToCentimeters(duration));

        cm = (8 - microsecondsToCentimeters(duration)) / 4 * 100;
        Firebase.RTDB.setInt(&fbdo, "/Status/Level", cm);

      Serial.println(cm);
      specificHumidity = atoi(Firebase.RTDB.getFloat(&fbdo, "/Setting/Humidity") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
      // wateringBatchMillis = atoi(Firebase.RTDB.getFloat(&fbdo, "/Setting/Humidity") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
      wateringBatchWait = atoi(Firebase.RTDB.getFloat(&fbdo, "/Setting/WateringBatchWait") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
      isWateringOn = atoi(Firebase.RTDB.getInt(&fbdo, "/Setting/IsEnableWatering") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
      isInstanceWatering = atoi(Firebase.RTDB.getInt(&fbdo, "/Setting/InstantWatering") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
     
    }
    if (isWateringOn == 1) {
      if (Humidity <= specificHumidity) {
        digitalWrite(Relay, LOW);
        Serial.println("Watering On");
        delay(wateringBatchWait * 1000);
        digitalWrite(Relay, HIGH);
        Serial.println("Watering Off");

        delay(10 * 1000);
      }
      if (isInstanceWatering == 1) {
        InstanceWatering();
      }
    }
    
}

void InstanceWatering() {
    digitalWrite(Relay, LOW);
    delay(wateringBatchWait * 1000);
    digitalWrite(Relay, HIGH);
    isInstanceWatering = 0;
    Firebase.RTDB.setInt(&fbdo, "/Setting/InstantWatering", 0);

    delay(10 * 1000);
}

float microsecondsToCentimeters(float microseconds)
{
  return microseconds / 29 / 2;
}
