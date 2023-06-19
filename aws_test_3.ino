#include <WiFiClient.h>
#include <HTTPClient.h>

#include <AmazonDynamoDBClient.h>
#include "EspAWSImplementations.h"
#include "AWSFoundationalTypes.h"
#include "AWSClient.h"
#include "time.h"

#include <stdlib.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define WIFI_SSID "LABS"
#define WIFI_PASSWORD "07081041"

// Configuration for AWS DynamoDb
static const char* TABLE_NAME = "NodeData";

static const char* PART_KEY = "NODE1"; // HASH_KEY_NAME is id HASH_KEY_VALUE is Node1
static const char* SORT_KEY; // RANGE_KEY_NAME is TakenAt and RANGE_KEY_VALUE is datetime.

static const char* HASH_KEY_NAME = "id";
static const char* HASH_KEY_VALUE = "Node1";
static const char* RANGE_KEY_NAME = "TakenAt";
static const char* RANGE_KEY_VALUE;
static const char* AWS_REGION = "us-east-1";
// static const char* AWS_ENDPOINT = "https://dynamodb.us-east-1.amazonaws.com";
static const char* AWS_ENDPOINT = "amazonaws.com";
static const int KEY_SIZE = 2;


static const char* temperature_val;
static const char* pressure_val;
static const char* humidity_val;


RTC_DATA_ATTR int boot_count = 0;
// 1 equals to 1 hour
RTC_DATA_ATTR unsigned long long total_sleep = 0.05 * 3600000000;

Adafruit_BME280 bme;


EspHttpClient http_client;
EspDateTimeProvider date_time_provider;

/* 
 * Objects for DynamoDb 
 */
AmazonDynamoDBClient dynamo_client;

TableDescription table_description;

PutRequest put_item_request;
PutItemInput put_item_input;
GetItemInput get_item_input;

AttributeValue partKey;
AttributeValue sortKey;
ActionError action_error;


float temp;
float humi;
float pressure;

struct tm timeinfo;
const char* ntp_server = "pool.ntp.org";
const long gmt_offset = 5 * 3600;
const int daylight_offset = 0;
char datentime[20];
char datesntime[20];
int wifi_retries = 5;


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
  Serial.begin(115200);
  delay(1000);

  ++boot_count;
  while(!Serial);
  Serial.println("Boot Number: "+String(boot_count));
  print_wakeup_reason();

  Serial.println("WiFi Connection Testing");
  initWiFi();

  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("Setup Done! Connected to");
  Serial.println(WIFI_SSID);

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
  strftime(datesntime, 20, "%F%T", &timeinfo);
  Serial.println();

  // Maybe we don't need domain & path
  // Copy domain into endpoint, if current setup doesn't work.
  // dynamo_client.setAWSDomain("dynamodb.us-east-1.amazonaws.com");
  dynamo_client.setAWSRegion(AWS_REGION);
  // from https://docs.aws.amazon.com/general/latest/gr/rande.html
  dynamo_client.setAWSEndpoint(AWS_ENDPOINT);
  dynamo_client.setAWSKeyID("AKIAZZZNWBLN3O3MH24V");
  dynamo_client.setAWSSecretKey("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");
  dynamo_client.setHttpClient(&http_client);
  dynamo_client.setDateTimeProvider(&date_time_provider);
}

void loop() {
  // Testing if the program can get data from the table
  
  
  Serial.println("Starting Testing!");
  delay(100);
  temp = bme.readTemperature();
  delay(100);
  humi = bme.readHumidity();
  delay(100);
  pressure = bme.readPressure() / 100.0F;
  delay(100);

  Serial.println(temp);
  Serial.println(humi);
  Serial.println(pressure);
  // get the time
  strftime(datesntime, 20, "%F %T", &timeinfo);
  delay(100);


  // MAYBE ASK KINDLY FIRST IF YOU CAN??


  
  // for our dynamodb putRequest we have to use
  // a Minimal Map which can then be pushed
  // into our database.
  Serial.println("Step 1: First Attribute Values");
  RANGE_KEY_VALUE = datesntime;
  // Create an ITEM
  char number_buffer[4];
  char num_b1[4];
  char num_b2[4];
  char num_b3[4];
  AttributeValue node_id;
  node_id.setS(HASH_KEY_VALUE);           // partiition Key
  AttributeValue taken_at;
  taken_at.setS(RANGE_KEY_VALUE);         // sort key

  Serial.println("Step 2: is sprintf working?");
  // Now for our values.
  AttributeValue temperature;
  sprintf(num_b1, "%d", temp);
  temperature.setN(num_b1);
  AttributeValue humidity;
  sprintf(num_b2, "%d", humi);
  humidity.setN(num_b2);
  AttributeValue pressure_a;
  sprintf(num_b3, "%d", pressure);
  pressure_a.setN(num_b3);

  Serial.println("Step 3: Key value pairing");

  // Creating Key-Value pair and making an array of them
  MinimalKeyValuePair < MinimalString, AttributeValue > att1(HASH_KEY_NAME, node_id);
  MinimalKeyValuePair < MinimalString, AttributeValue > att2(RANGE_KEY_NAME, taken_at);
  MinimalKeyValuePair < MinimalString, AttributeValue > att3("Temperature", temperature);
  MinimalKeyValuePair < MinimalString, AttributeValue > att4("Pressure", pressure_a);
  MinimalKeyValuePair < MinimalString, AttributeValue > att5("Humidity", humidity);

  MinimalKeyValuePair < MinimalString, AttributeValue > itemArray[] = { att1, att2, att3, att4, att5 };

  Serial.println("Step 4: Putting the values");

  // Set values for putItemInput
  put_item_request.setItem(MinimalMap < AttributeValue > (itemArray, 5));
  put_item_input.setItem(MinimalMap < AttributeValue > (itemArray, 5));
  put_item_input.setTableName(TABLE_NAME);

  Serial.print("Step 4a: Print the Table i.e. Count the items in it...  ");
  table_description.setTableName(TABLE_NAME);
  long how_many = table_description.getItemCount();
  
  Serial.println(how_many);
  // Serial.println(itemArray);

  // perform putItem and check for errors
  // Serial.println(dynamo_client);
  Serial.println("Step 5: PutItemOutput");
  PutItemOutput putItemOutput;
  // putItemOutput.retry = false;
  putItemOutput = dynamo_client.putItem(put_item_input, action_error);
  
  Serial.println("Step 6: Error Handling");
  switch (action_error) {
    case NONE_ACTIONERROR:
      Serial.println("PutItem Succeeded!");
      break;
    case INVALID_REQUEST_ACTIONERROR:
      Serial.print("ERROR: ");
      Serial.println(putItemOutput.getErrorMessage().getCStr());
      break;
    case MISSING_REQUIRED_ARGS_ACTIONERROR:
      Serial.println("ERROR: Required Arguments were not set for PutItemInput");
      break;
    case RESPONSE_PARSING_ACTIONERROR:
      Serial.println("ERROR: Problem parsing http response of PutItem");
      break;
    case CONNECTION_ACTIONERROR:
      Serial.println("ERROR: Connection Problem");
      break;
  }
  // wait not to double record
  delay(750);
  Serial.println("Step 7: Error Handled");
  
  //const char* data_push = dynamo_client.putRequest(sample_item);

  Serial.print("Is it WORKING?");
  delay(30000);  
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
