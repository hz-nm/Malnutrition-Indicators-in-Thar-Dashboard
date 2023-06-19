/*
 * First test for connecting the ESP to AWS IoT Core
 * Written by: Ameer H.
 */
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "time.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define AWS_IOT_PUBLISH_TOPIC "testnode/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "testnode/sub"

#define SENSOR A0
#define OFFSET 0.0
#define SAMPLING_INTERVAL 20
#define ARRAY_LENGTH 40
#define PRINT_INTERVAL 800

const int ph_pin = 34;
float ph_val = 7.1;
float ph_act;
const int pwr_one = 0;

const int turb_sensor_pin = 35;
float turb_sensor_voltage;
int samples = 100;
float ntu;
int turb_sensor_raw;

int PH_ARRAY[ARRAY_LENGTH];
int PH_ARRAY_INDEX = 0;

/*
 * Data to be saved in RTC Memory which is not erased
 * on restart
 */
RTC_DATA_ATTR int bootCount = 0;
float sleep_time = 0.5; // 1 equals to 1 hour
RTC_DATA_ATTR unsigned long long total_sleep = sleep_time * 3600000000;

/*
 * Configuration for BME Sensor
 */
Adafruit_BME280 bme;
unsigned long delay_time;

float humidity;
float temperature;
float pressure;

/*
 * Configuration for extracting date and time
 */
struct tm timeinfo;
const char* ntp_server = "pool.ntp.org";
const long gmt_offset = 5 * 3600;
const int daylight_offset = 0;
char datentime[20];

// retries after which device will go into sleep mode.
int wifi_retries = 5;


WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

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

void connectAWS() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    /*wifi_retries -= 1;
    if (wifi_retries <= 0) {
      Serial.println("Cannot connect to WiFi..");
      break;
    }*/
  }
  Serial.println("");
  Serial.println(WiFi.localIP());

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // connect to the mqtt broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);
  
  client.setKeepAlive(90);

  // create a message handler
  client.setCallback(messageHandler);

  Serial.println("Connecting to AWS IoT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("AWS IoT Connected!");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(THINGNAME)) {
      Serial.println("connected");
      // Subscribe
      client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publishMessage() {
  StaticJsonDocument<200> doc;
  doc["Temperature"] = temperature;
  doc["Humidity"] = humidity;
  doc["Pressure"] = pressure;
  /*
   * TODO
   * 1 - Add Node ID  --> Partition Key --> "id" x
   * 2 - Add Datentime -> Sort Key      --> "TakenAt" x
   * 3 - Add PH x
   * 4 - Add Turbidity x
   * 5 - Integrate to Node
   */
  doc["id"] = "1";    // Node ID
  doc["TakenAt"] = datentime;
  doc["PH"] = ph_act;
  doc["Turbidity"] = turb_sensor_raw;
  char jsonBuffer[2048];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  delay(2000);
}

void messageHandler(char* topic, byte* payload, unsigned int length) {
  Serial.print("Incoming: ");
  Serial.println(topic);

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}

void setup() {
  pinMode(pwr_one, OUTPUT); // power to cut off the circuitry
  pinMode(turb_sensor_pin, INPUT); // turbidity sensor input
  
  Serial.begin(115200);
  delay(1000);

  ++bootCount;
  while(!Serial);
  Serial.println("Boot Number: " + String(bootCount));
  print_wakeup_reason();

  Serial.println("Connecting to WiFi / AWS");
  
  connectAWS();

  Serial.println("Setup Done!");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);

  /*
   * Setting up the BME280 Sensor
   */
  unsigned status;
  status = bme.begin(0x76);   // can also be 0x77
  if(!status) {
    Serial.println("Couldn't find a valid sensor connected");
    while(1) delay(10);
  }

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
  /*
   * Temperature, Pressure and Humidity readings
   */
  
  temperature = bme.readTemperature();
  delay(100);
  pressure = bme.readPressure() / 100.0F;
  delay(100);
  humidity = bme.readHumidity();
  delay(100);
  /*
  * Turbidity Sensor
  */
  turb_sensor_voltage = 0;
  // averaging over 100 samples
  for(int i=0; i<samples; i++) {
    turb_sensor_voltage += ((float)analogRead(turb_sensor_pin)/4096) * 3.3;
    turb_sensor_raw += analogRead(turb_sensor_pin);
    delay(10);
  }

  turb_sensor_raw = turb_sensor_raw/samples;
  

  /*
   * Now for the PH Sensor
   */
  static unsigned long SAMPLING_TIME = millis();
  static unsigned long PRINT_TIME = millis();
  static float VOLTAGE;
            
  if(millis() - SAMPLING_TIME > SAMPLING_INTERVAL) {
    PH_ARRAY[PH_ARRAY_INDEX++] = analogRead(ph_pin);
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
    Serial.println(ph_val);
  }

  for(int i=0; i<samples; i++) {
    ph_val += ((float)analogRead(ph_pin)/4096) * 3.3;
    ph_act += analogRead(ph_pin);
    delay(10);
  }
  
  ph_val = ph_val/samples;
  ph_act = ph_act/samples;
  /*
   * remove the following hard-coded elements
   * in the final program.
   */
  ph_act = 7;
  turb_sensor_raw = 200;

  // now get the time
  strftime(datentime, 20, "%F %T", &timeinfo);
  Serial.println("Sending the following Date and Time .");
  Serial.println(datentime);
  delay(100);
  

  Serial.print("Humidity: ");
  Serial.println(humidity, 2);
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Pressure: ");
  Serial.println(pressure);

  /*
   * format the values
   */

  

  publishMessage();
  client.loop();
  delay(1000);

  Serial.println("Powering OFF devices!");
  delay(100);
  digitalWrite(pwr_one, LOW);

  esp_sleep_enable_timer_wakeup(total_sleep);

  
  Serial.println("ESP set to wakeup after 30 minute/s");
  Serial.println("Going to sleep NOW!");
  Serial.println(temperature);
  Serial.println(humidity);
  Serial.println(pressure);
  digitalWrite(pwr_one, LOW);
  delay(1000);
  // delay(20000);
  
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
