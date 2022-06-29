/**
 * Temperature / Env IoT node
 * 
 * ESP8266 core
 * SHT30 Env/Baro/Temp sensor
 * 3D printed case
 * MQTT client - broadcasting state
 * 
 * Author: Aaron S. Crandall <crandall@gonzaga.edu>
 * Copyright: 2022
 * 
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "Adafruit_SHT31.h"

// WiFi
// Values set by 3rd party tool as to not check them into the git repo here
//const char *ssid = "netssid";
//const char *password = "passwd";

// MQTT Broker
const char *mqtt_broker = "10.0.0.4";
const char *topic = "IoT/rawevents";
//const char *mqtt_username = "emqx";
//const char *mqtt_password = "public";
const int mqtt_port = 1883;
#define MQTT_HOSTNAME "broker.local"

WiFiClient espClient;
PubSubClient client(espClient);

const int trigPin = 12;
const int echoPin = 14;

//define sound velocity in cm/uS
#define SOUND_VELOCITY 0.034
#define CM_TO_INCH 0.393701

long duration;
float distanceCm;
float distanceInch;

float samples[10];
#define SAMPLES_COUNT 10
#define INF_DISTANCE_CM 100

enum DoorStatus {
  IS_CLOSED,
  IS_OPEN,
};

unsigned int last_report_epoch;
#define REPORT_TIMEOUT_MS 60000

char * g_location = "spaceport";
char * g_sensorType = "garagedoor";
char g_id[13] = "";

Adafruit_SHT31 sht31 = Adafruit_SHT31();

bool g_led_status = false;

// ** ************************************************************
// Onboard LED API / Controls
void ledON() {
  g_led_status = true;
  digitalWrite(LED_BUILTIN, LOW); // 8266's are backwards
}

void ledOFF() {
  g_led_status = false;
  digitalWrite(LED_BUILTIN, HIGH); // 8266's are backwards
}

void ledToggle() {
  if(g_led_status) {
    ledOFF();
  } else {
    ledON();
  }
}


// ** ************************************************************
void setupMQTTConnection() {
  //connecting to a mqtt broker
  Serial.println("Connecting to MQTT Broker");
  
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(mqtt_queue_callback);
  while (!client.connected()) {
    String client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str())) {
        Serial.println("MQTT broker connected");
    } else {
        Serial.print("failed with state ");
        Serial.print(client.state());
        delay(2000);
    }
  }

  // client.subscribe(topic);
}


// ** ************************************************************
void setupDeviceID() {
  byte mac[6];
  WiFi.macAddress(mac);
  sprintf(g_id, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.printf("Device uniqe id: %s\n", g_id);
  
}


// ** ************************************************************
void mqtt_queue_callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
      Serial.print((char) payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}


// ** ************************************************************
void handleNetworkStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    WiFi.begin();

    Serial.print("Connecting to WiFi (SSID: ");
    Serial.print(WiFi.SSID());
    Serial.print(") ...");
    while (WiFi.status() != WL_CONNECTED) {
      ledToggle();
      delay(500);
      Serial.print(".");
    }
    Serial.println("Connected to the WiFi network");

    setupMQTTConnection();
    ledOFF();
  }
}


// ** ************************************************************
void runReport() {
  char message[50];
  char value[10];

  float temperature_C = sht31.readTemperature();
  float humidity_pct = sht31.readHumidity();

  char * temperatureTypeName = "temperature";
  char * humidityTypeName = "humidity";
  

  sprintf(message, "%s:%s:%s:%0.2f", g_id, g_location, temperatureTypeName, temperature_C);
  client.publish(topic, message);

  Serial.println(message);

  sprintf(message, "%s:%s:%s:%0.2f", g_id, g_location, humidityTypeName, humidity_pct);
  client.publish(topic, message);
  
  Serial.println(message);

}


// ** ************************************************************
void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  for( int i = 0; i < 4; i++ ) {
    ledToggle();
    delay(1000);
  }

  setupDeviceID();

  handleNetworkStatus();

  Serial.println("SHT31 Connect and configure");
  if (! sht31.begin(0x45)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  last_report_epoch = millis();

  Serial.println("Beginning normal operations after Wifi Connect");

  delay(5000);
  runReport();
}


// ** ************************************************************
void loop() {
  handleNetworkStatus();  // Check for disconnection & handle it


  if (client.connected()==false){
    Serial.println("MQTT Broker connection is down");
    String client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());
    if (client.connect(client_id.c_str())) {
       Serial.println("MQTT Broker Connection Restarted");
    }
  }
  client.loop();    // Process/check MQTT queues

  
  if(last_report_epoch + REPORT_TIMEOUT_MS < millis()) {
    runReport();
    last_report_epoch = millis();
  }

}
