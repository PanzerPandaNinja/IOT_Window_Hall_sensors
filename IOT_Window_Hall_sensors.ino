
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>

#include <Msgflo.h>
#include <PubSubClient.h>

#include "./config.h"
/*
 The config file should contain:
      #define CFG_WIFI_SSID "SSID"
      #define CFG_WIFI_PASSWORD "password"
      #define CFG_MQTT_HOST "mqtt.server.no"
      #define CFG_MQTT_USER "USER"
      #define CFG_MQTT_PASSWORD "PASSWORD"
*/

struct Config {
  const int sensor1Pin = D3;
  const int sensor2Pin = D4;
  const int sensor3Pin = D5;
  const String role = "bitraf/windowsensor/workshop";

  const String wifiSsid = CFG_WIFI_SSID;
  const String wifiPassword = CFG_WIFI_PASSWORD;

  const char *mqttHost = CFG_MQTT_HOST;
  const int mqttPort = 1883;

  const char *mqttUsername = CFG_MQTT_USER;
  const char *mqttPassword = CFG_MQTT_PASSWORD;
} cfg;


const auto participant = msgflo::Participant("bitraf/WindowSensor", cfg.role);

WiFiClient wifiClient; // used by WiFi
PubSubClient mqttClient;
msgflo::Engine *engine;
msgflo::OutPort *sensorPort1;
msgflo::OutPort *sensorPort2;
msgflo::OutPort *sensorPort3;

long nextActivatedCheck = 0;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.printf("Configuring wifi: %s\r\n", cfg.wifiSsid.c_str());
  WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPassword.c_str());

  mqttClient.setServer(cfg.mqttHost, cfg.mqttPort);
  mqttClient.setClient(wifiClient);

  const String clientId = cfg.role + WiFi.macAddress();

  engine = msgflo::pubsub::createPubSubClientEngine(participant, &mqttClient, clientId.c_str());

  sensorPort1 = engine->addOutPort("sensor1", "boolean", "public/"+cfg.role+"/sensor1");
  //sensorPort2 = engine->addOutPort("sensor2", "boolean", "public/"+cfg.role+"/sensor2");
  //sensorPort3 = engine->addOutPort("sensor3", "boolean", "public/"+cfg.role+"/sensor3");

  pinMode(cfg.sensor1Pin, INPUT_PULLUP);
  pinMode(cfg.sensor2Pin, INPUT_PULLUP);
  pinMode(cfg.sensor3Pin, INPUT_PULLUP);
}

void loop() {
  static bool connected = false;

  if (WiFi.status() == WL_CONNECTED) {
    if (!connected) {
      Serial.printf("Wifi connected: ip=%s\r\n", WiFi.localIP().toString().c_str());
    }
    connected = true;
    engine->loop();
  } else {
    if (connected) {
      connected = false;
      Serial.println("Lost wifi connection.");
    }
  }

  // TODO: check for statechange. If changed, send right away. Else only send every 3 seconds or so
  if (millis() > nextActivatedCheck) {
    const bool activated1 = !digitalRead(cfg.sensor1Pin);
    const bool activated2 = !digitalRead(cfg.sensor2Pin);
    const bool activated3 = !digitalRead(cfg.sensor3Pin);

    if (activated1) Serial.print("Sensor 1 ");
    if (activated2) Serial.print("Sensor 2 ");
    if (activated3) Serial.print("Sensor 3 ");
    Serial.println("");
    
    sensorPort1->send(activated1 ? "true" : "false");
    //sensorPort2->send(activated2 ? "true" : "false");
    //sensorPort3->send(activated3 ? "true" : "false");

    
    
    nextActivatedCheck += 100;
  }

}

