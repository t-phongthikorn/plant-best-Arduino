
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
#include <math.h>
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

//Setting
float targetHumidity = 0.0;
float wateringBatchDurartion = 0.0; //second
float wateringBatchDelay = 10.0; //second
int canWatering = 0;
int isInstantWatering = 0;

//Status
float Humidity;
int isWatering = 0;
unsigned long pumpMillis; //milli
unsigned long pumpDelayMillis; //milli

//sensor
float cm;
float duration;
int wet = 275;
int dry = 610;


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

void Watering();
float ReadWaterLevel();
void setPumpEnable(bool isEnable);
float ReadHumidityLevel();
void StopPumpDelay();
void stopWatering();


void setup()
{
    pinMode(MoistureSensorPin, INPUT);
    pinMode(EchoPin, INPUT);
    pinMode(TriggerPin, OUTPUT);
    pinMode(Relay, OUTPUT);
    Serial.begin(115200);

    setPumpEnable(false);
    StopPumpDelay();

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
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
      Serial.print(msValue);
      Humidity = map(msValue, wet, dry, 100, 0);

        dataMillis = millis();
        Firebase.RTDB.setInt(&fbdo, "/Status/Humidity", Humidity);
        Firebase.RTDB.setInt(&fbdo, "/Status/IsWatering", isWatering);

        digitalWrite(TriggerPin, LOW);
        delayMicroseconds(2);
        digitalWrite(TriggerPin, HIGH);
        delayMicroseconds(5);
        digitalWrite(TriggerPin, LOW);
        duration = pulseIn(EchoPin, HIGH);

        Serial.printf("real value %f \n",  microsecondsToCentimeters(duration));

        cm = round((8 - microsecondsToCentimeters(duration)) / 4 * 100);
        Firebase.RTDB.setInt(&fbdo, "/Status/Level", cm);

      Serial.println(cm);
      targetHumidity = atoi(Firebase.RTDB.getFloat(&fbdo, "/Setting/Humidity") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
      wateringBatchDurartion = atoi(Firebase.RTDB.getFloat(&fbdo, "/Setting/WateringBatchDurartion") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
      canWatering = atoi(Firebase.RTDB.getInt(&fbdo, "/Setting/IsEnableWatering") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
      isInstantWatering = atoi(Firebase.RTDB.getInt(&fbdo, "/Setting/InstantWatering") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
      wet = atoi(Firebase.RTDB.getInt(&fbdo, "/Setting/Wet") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
      dry = atoi(Firebase.RTDB.getInt(&fbdo, "/Setting/Dry") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());

     
    }


    if (canWatering == 1) {
      if (isInstantWatering == 0) {
        Serial.println("Delaying" + String(millis() - pumpDelayMillis) + " >= " + String(wateringBatchDelay * 1000));
        if (millis() - pumpDelayMillis >= wateringBatchDelay * 1000) {
          if (Humidity <= targetHumidity) {
            if (isWatering == 0) {
              pumpMillis = millis();
              isWatering = 1;
            }
            Serial.println("Watering");
            Serial.println("Watering" + String(millis() - pumpMillis) + " <= " + String(wateringBatchDurartion * 1000));
            setPumpEnable(true);
            if (millis() - pumpMillis > wateringBatchDurartion * 1000) {
              setPumpEnable(false);
              pumpDelayMillis = millis();
              isWatering = 0;
              Serial.println("Stop Watering Wait for delay");
            }
          }
        } else {
          setPumpEnable(false);
        }
      } else {
        if (isWatering == 0) {
          pumpMillis = millis();
          isWatering = 1;
        }
        Serial.println("Watering");
        Serial.println("Watering" + String(millis() - pumpMillis) + " <= " + String(wateringBatchDurartion * 1000));
        setPumpEnable(true);
        if (millis() - pumpMillis > wateringBatchDurartion * 1000) {
          setPumpEnable(false);
          pumpDelayMillis = millis();
          isWatering = 0;
          isInstantWatering = 0;
          Firebase.RTDB.setInt(&fbdo, "/Setting/InstantWatering", 0);

          Serial.println("Stop Watering Wait for delay");
        }
      }
    } else {
      if (isInstantWatering == 0) {
        StopWatering();
        Serial.println("IsWatering == 0");
      } else {
        if (isWatering == 0) {
          pumpMillis = millis();
          isWatering = 1;
        }
        Serial.println("Watering");
        Serial.println("Watering" + String(millis() - pumpMillis) + " <= " + String(wateringBatchDurartion * 1000));
        setPumpEnable(true);
        if (millis() - pumpMillis > wateringBatchDurartion * 1000) {
          setPumpEnable(false);
          pumpDelayMillis = millis();
          isWatering = 0;
          isInstantWatering = 0;
          Firebase.RTDB.setInt(&fbdo, "/Setting/InstantWatering", 0);
          Serial.println("Stop Watering Wait for delay");
        }
      }
      
    }

    
}

void StopPumpDelay() {
  pumpDelayMillis = wateringBatchDelay * 1000;
}

void Watering() {
  isWatering = true;
  StopPumpDelay();
}

void StopWatering() {
  isWatering = false;
  setPumpEnable(false);
  pumpMillis = 0;
  pumpDelayMillis = 0;
}

void setPumpEnable(bool isEnable) {
  if (isEnable) {
    digitalWrite(Relay, LOW);
    Serial.println("Enable Pump");
  } else {
    digitalWrite(Relay, HIGH);
    Serial.println("Disable Pump");

  }
}


float microsecondsToCentimeters(float microseconds)
{
  return microseconds / 29 / 2;
}
