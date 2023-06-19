

/*
 * This is the first version of the final sensor that is going to
 * measure quality of water bodies.
 * Author: Ameer H.
 */

#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include "time.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
// Configuration for firebase
#include <FirebaseESP32.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#define FIREBASE_HOST "https://esp32-test-project-a01-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "mgqNwC71uXQ9xizdRhveS78tFxizsZXcnIfWT31a"
#define WIFI_SSID "LABS"
#define WIFI_PASSWORD "07081041"

//#define FIREBASE_USE_PSRAM

//#define WIFI_SSID "Muzzamil Mirza"
//#define WIFI_PASSWORD "123456789"

// Define Firebase Data Objects
FirebaseData fbd0;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long count = 0;

String path = "/Nodes";
String node_id = "Node1";

#define SENSOR A0               // pH meter analog input to Arduino
#define OFFSET 0.0              // offset for pH for deviation compensation.
#define SAMPLING_INTERVAL 20
#define ARRAY_LENGTH 40         // times of collection
#define PRINT_INTERVAL 800

const int phPin = 34;
float ph_val = 7.1;
float ph_act;
const int pwr_one = 0;

const int turb_sensor_pin = 35;
float turb_sensor_voltage;
int samples = 100;
float ntu;
int turb_sensor_raw;

int PH_ARRAY[ARRAY_LENGTH];
int PH_ARRAY_INDEX=0;



// Any data to be saved into RTC memory is kept here.
RTC_DATA_ATTR int bootCount = 0;
float sleep_time = 0.1;
// RTC_DATA_ATTR unsigned long long mul_factor = 60*60*1000*1000;
// 1 equals to 1 hour
RTC_DATA_ATTR unsigned long long total_sleep = 0.05 * 3600000000;


Adafruit_BME280 bme;            // FOR I2C TEMP etc.

unsigned long delay_time;

float temp;
float humi;
float pressure;

/*
 * Variables for DATETIME
 */
struct tm timeinfo;
const char* ntp_server = "pool.ntp.org";
const long gmt_offset = 5 * 3600;
const int daylight_offset = 0;
char datentime[20];
char datesntimes[20];
int wifi_retries = 5;


  
/*
 * Following functions and associated code will be used to
 * connect the device to a Firebase Realtime database
 */
void streamCallback(StreamData data) {
 if (data.dataType() == "boolean") {
   if (data.boolData()) {
     Serial.println("Set " + node_id + " to High");
   }
   else {
     Serial.println("Set " + node_id + " to Low");
   }
   // digitalWrite()
 }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println();
    Serial.println("Stream timeout, resuming stream... ");
    Serial.println();
  }
}

/*
 * Method to print the reason by which ESP32 has been taken away from sleep
 */

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO");break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL");break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by TOUCHPAD"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP Program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;    
  }
}
/*
 * Method to Initiate a WiFi Connection
 * Not currently in use because we donot have a wifi connection handy.
 */

void initWiFi() {
  // WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
    wifi_retries -= 1;
    if (wifi_retries <= 0) {
      break;
    }
  }
  // print the IP Address of the ESP32
  Serial.println(WiFi.localIP());
}


void setup() {
  // put your setup code here, to run once:
  pinMode(pwr_one, OUTPUT);   // power to all the circuitry.
  pinMode(turb_sensor_pin, INPUT);
  
  Serial.begin(115200);
  delay(1000);

  ++bootCount;
  while(!Serial);
  Serial.println("Boot Number: " + String(bootCount));
  print_wakeup_reason();
  
  Serial.println("WiFi Connection TESTING");

  initWiFi();
  
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

  Serial.println("Setup Done!");
  Serial.println(WIFI_SSID);

  // IPAddress IP = WiFi.softAPIP();
  IPAddress IP = WiFi.localIP();
  Serial.print("AP IP Address: ");
  Serial.println(IP);

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = FIREBASE_AUTH;
  config.database_url = FIREBASE_HOST;
  
  config.signer.test_mode = true;
  config.max_token_generation_retry = 5;
  

  // Setting up FIREBASE
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  
  Firebase.reconnectWiFi(true);

  if (!Firebase.beginStream(fbd0, path + "/" + node_id)) {
    Serial.println("Could not begin Stream");
    Serial.println("REASON: " + fbd0.errorReason());
  }


  // Finished setting up Firebase
  unsigned status;
  status = bme.begin(0x76);     // status = bme.begin(0x76) --> specifying an I2C address for checking status.
  if (!status){
    Serial.println("Could not find a valid Sensor connected to our System.. Please check connection etc.");
    Serial.println();
    Serial.println("SensorID was: 0x");
    Serial.println(bme.sensorID(),16);
    while(1) delay(10);
  }
  // Configure to use the time library
  configTime(gmt_offset, daylight_offset, ntp_server);
  
  Serial.println("--TESTING HAS BEGUN--");
  Serial.println("--Currently testing BME280 and pH--");
  Serial.print("Current time: ");
  printLocalTime();
  Serial.println();
}

void loop() {
  digitalWrite(pwr_one, HIGH);
  Serial.println("Powering on devices!");
  delay(500);
  temp = bme.readTemperature();
  delay(100);
  humi = bme.readHumidity();
  delay(100);
  pressure = bme.readPressure() / 100.0F;
  delay(100);

  // have to include Turbidity sensor
  turb_sensor_voltage = 0;
  // averaging over 100 samples
  for(int i=0; i<samples; i++) {
    turb_sensor_voltage += ((float)analogRead(turb_sensor_pin)/4096) * 3.3;
    turb_sensor_raw += analogRead(turb_sensor_pin);
    delay(10);
  }

  turb_sensor_voltage = turb_sensor_voltage/samples;
  turb_sensor_raw = turb_sensor_raw/samples;
  ntu = -1120.4 * (turb_sensor_voltage*turb_sensor_voltage) + 5742.3 * turb_sensor_voltage - 4532.9;

  static unsigned long SAMPLING_TIME = millis();
  static unsigned long PRINT_TIME = millis();
  static float VOLTAGE;


            
  if(millis() - SAMPLING_TIME > SAMPLING_INTERVAL) {
    PH_ARRAY[PH_ARRAY_INDEX++] = analogRead(phPin);
    if(PH_ARRAY_INDEX==ARRAY_LENGTH)PH_ARRAY_INDEX=0;
    VOLTAGE = (AVERAGE_ARRAY(PH_ARRAY, ARRAY_LENGTH) * 3.3)/4096;
    SAMPLING_TIME = millis();
  }
  if(millis() - PRINT_TIME > PRINT_INTERVAL) {
    Serial.print("Voltage OUTPUT: ");
    Serial.println(VOLTAGE, 2);
    float ph_val = (float) 1.711 * VOLTAGE;
    float ph_act = (float) VOLTAGE;
    Serial.print("PH: ");
    //Serial.println(ph_val);
  }

  for(int i=0; i<samples; i++) {
    ph_val += ((float)analogRead(phPin)/4096) * 3.3;
    ph_act += analogRead(phPin);
    delay(10);
  }
  
  ph_val = ph_val/samples;
  ph_act = ph_act/samples;

  // now get the time
  strftime(datesntimes, 20, "%F %T", &timeinfo);
  Serial.println("Sending the following Date and Time to Firebase..");
  delay(100);

  
  if (Firebase.setFloat(fbd0, path + "/" + node_id + "/" + datesntimes + "/Temperature", temp)) {
    if (fbd0.dataTypeEnum() == fb_esp_rtdb_data_type_float) {
      Serial.println(fbd0.to<float>());
    }
  }
  else {
    Serial.println("ERROR!");
    Serial.println(fbd0.errorReason());
  }
  delay(100);
  if (Firebase.setFloat(fbd0, path + "/" + node_id + "/" + datesntimes +  "/Humidity", humi)) {
    if (fbd0.dataTypeEnum() == fb_esp_rtdb_data_type_float) {
      Serial.println(fbd0.to<float>());
    }
  }
  else {
    Serial.println("ERROR!");
    Serial.println(fbd0.errorReason());
  }
  delay(100);
  if (Firebase.setFloat(fbd0, path + "/" + node_id + "/" + datesntimes +  "/Pressure", pressure)) {
    if (fbd0.dataTypeEnum() == fb_esp_rtdb_data_type_float) {
      Serial.println(fbd0.to<float>());
    }
  }
  else {
    Serial.println("ERROR!");
    Serial.println(fbd0.errorReason());
  }
  delay(100);
  if (Firebase.setFloat(fbd0, path + "/" + node_id + "/" + datesntimes + "/PH", ph_act)) {
    if (fbd0.dataTypeEnum() == fb_esp_rtdb_data_type_float) {
      Serial.println(fbd0.to<float>());
    }
  }
  else {
    Serial.println("ERROR!");
    Serial.println(fbd0.errorReason());
  }
  delay(100);
  if (Firebase.setFloat(fbd0, path + "/" + node_id + "/" + datesntimes + "/Turbidity", turb_sensor_raw)) {
    if (fbd0.dataTypeEnum() == fb_esp_rtdb_data_type_float) {
      Serial.println(fbd0.to<float>());
    }
  }
  else {
    Serial.println("ERROR!");
    Serial.println(fbd0.errorReason());
  }

  Serial.println("Powering OFF devices!");
  delay(100);
  digitalWrite(pwr_one, LOW);

  esp_sleep_enable_timer_wakeup(total_sleep);

  
  Serial.println("ESP set to sleep for every 20 second/s");
  Serial.println("Going to sleep NOW!");
  Serial.println(temp);
  Serial.println(humi);
  Serial.println(pressure);
  digitalWrite(pwr_one, LOW);
  delay(1000);
  
  esp_deep_sleep_start();

}

double AVERAGE_ARRAY(int*ARR, int NUMBER)
{
    int i;
    int max, min;
    double AVG;
    long AMOUNT=0;
    if(NUMBER<=0){
        Serial.println("ERROR!/n");
        return 0;
    }
    if(NUMBER < 5){
        for(i=0; i<NUMBER;i++){
            AMOUNT += ARR[i];
        }
        AVG = AMOUNT/NUMBER;
        return AVG;
    }else{
        if(ARR[0]<ARR[1]){
            min = ARR[0]; max = ARR[1];
        }
        else{
            min = ARR[1]; max = ARR[0];
        };
        for(i=2;i<NUMBER;i++){
            if(ARR[i]<min){
                AMOUNT += min;
                min = ARR[i];
            }else {
                if(ARR[i] > max){
                    AMOUNT += max;
                    max = AMOUNT+=ARR[i];
                }else{
                    AMOUNT += ARR[i];
                }
            }
        }
        AVG = (double)AMOUNT/(NUMBER-2);
    }
    return AVG;
}

void printLocalTime() {
  // struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  strftime(datentime, 20, "%F %T", &timeinfo);
  Serial.println(datentime);
  Serial.println();
}
