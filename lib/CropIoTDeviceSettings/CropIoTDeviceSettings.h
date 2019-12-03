#ifndef CROPIOTDEVICESETTINGS_H_
#define CROPIOTDEVICESETTINGS_H_

#include "Arduino.h"
#include "include/settings_endpoints.h"
#include "ESP8266WiFi.h"
#include "ESPAsyncWebServer.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "PubSubClient.h"
#include "EEPROM.h"

#define DEVICE_NAME_MEM_ADDR      500 // size of 20
#define DEVICE_PASS_MEM_ADDR      520 // size of 20
#define MQTT_SERVER_MEM_ADDR    540 // size of 50
#define MQTT_KEY_MEM_ADDR       690 // size of 20
String DEVICE_TYPE = "";

IPAddress apLocalIP(192,168,4,1);
IPAddress apGateway(192,168,4,0);
IPAddress apSubnet(255,255,255,0);

boolean shouldReboot = false;

AsyncWebServer server(80);

void loadSettingsEndpoints();
void logIpAddress();
String readMem(char add);
void writeMem(char add,String data);

WiFiClient generateWiFiClient() {
  WiFiClient wlanClient;
  return wlanClient;
}

PubSubClient generateMqttClient(WiFiClient wlanClient) {
  PubSubClient mqttClient(wlanClient);
  return mqttClient;
}

void startAP() {
  String apName = readMem(DEVICE_NAME_MEM_ADDR);
  String apPass = readMem(DEVICE_PASS_MEM_ADDR);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apLocalIP, apGateway, apSubnet);
  if (apPass == "")
    WiFi.softAP(apName);
  else
    WiFi.softAP(apName, apPass);
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  WiFi.begin();
  server.begin();
}

void connectWiFi() {
  EEPROM.begin(512);
  Serial.print("Device Name: \"");Serial.print(readMem(DEVICE_NAME_MEM_ADDR));Serial.println("\"");
  Serial.print("Device Password: \"");Serial.print(readMem(DEVICE_PASS_MEM_ADDR));Serial.println("\"");
  Serial.print("MQTT server: \"");Serial.print(readMem(MQTT_SERVER_MEM_ADDR));Serial.println("\"");
  Serial.print("MQTT Key: \"");Serial.print(readMem(MQTT_KEY_MEM_ADDR));Serial.println("\"");
  loadSettingsEndpoints();
  // WiFi.begin(wlanSsid, wlanPass);
  startAP();
  while (!WiFi.isConnected() && !shouldReboot) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  Serial.println("Connected!");
  logIpAddress();
}

void reconnectWiFi() {
  if(shouldReboot){
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }
  if (!WiFi.isConnected()) {
    delay(1000);
    Serial.println("Reconnecting to WiFi..");
  }
  if (!WiFi.isConnected()) {
    Serial.println("Reconnected!");
    logIpAddress();
  }
}

void logIpAddress() {
  Serial.print("local IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void connectMQTT(PubSubClient& mqttClient) {
  String deviceName = readMem(DEVICE_NAME_MEM_ADDR);
  String mqttServer = readMem(MQTT_SERVER_MEM_ADDR);
  String mqttKey = readMem(MQTT_KEY_MEM_ADDR);

  mqttClient.setServer(mqttServer.c_str(), 1883);

  while (!mqttClient.connected() && !shouldReboot) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if ( mqttClient.connect(deviceName.c_str(), mqttKey.c_str(), NULL) ) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("outTopic", "hello world");
      // ... and resubscribe
      mqttClient.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void reconnectMQTT(PubSubClient& mqttClient) {
  String deviceName = readMem(DEVICE_NAME_MEM_ADDR);
  String mqttServer = readMem(MQTT_SERVER_MEM_ADDR);
  String mqttKey = readMem(MQTT_KEY_MEM_ADDR);

  mqttClient.setServer(mqttServer.c_str(), 1883);

  if (!mqttClient.connected()) {
    Serial.print("Attempting MQTT reconnection...");
    // Attempt to connect
    if ( mqttClient.connect(deviceName.c_str(), mqttKey.c_str(), NULL) ) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("outTopic", "hello world");
      // ... and resubscribe
      mqttClient.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in next loop");
    }
  }
}

void beginMem() {
  EEPROM.begin(512);
}

void writeMem(char add,String data){
  int _size = data.length();
  int i;
  for(i=0; i < _size; i++)
  EEPROM.write(add+i,data[i]);

  EEPROM.write(add+_size,'\0');   //Add termination null character for String Data
  EEPROM.commit();
}

String readMem(char add){
  char data[100]; //Max 100 Bytes
  int len = 0;
  unsigned char k;
  k = EEPROM.read(add);
  while(k != '\0' && len < 500)   //Read until null character
  {
    k = EEPROM.read(add+len);
    data[len] = k;
    len++;
  }
  data[len] = '\0';
  return String(data);
}

void loadSettingsEndpoints() {
  // statics endpoints:
  server.serveStatic("/", SPIFFS, "/");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on(URLS.API.DEVICE, HTTP_GET, [&](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    String deviceName = readMem(DEVICE_NAME_MEM_ADDR);
    root["deviceName"] = deviceName.c_str();
    root["type"] = DEVICE_TYPE;
    root.printTo(*response);
    request->send(response);
  });
  server.on(URLS.API.DEVICE, HTTP_POST, [&](AsyncWebServerRequest *request){
    if(request->hasParam("deviceName", true) && request->hasParam("devicePass", true)) {
      const String deviceName = request->getParam("deviceName", true)->value().c_str();
      const String devicePass = request->getParam("devicePass", true)->value().c_str();
      writeMem(DEVICE_NAME_MEM_ADDR, deviceName);
      writeMem(DEVICE_PASS_MEM_ADDR, devicePass);
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
      request->send(200);
      shouldReboot = true;
    } else {
      request->send(400);
    }
  });

  // WiFi setup endpoints:
  server.on(URLS.API.WIFI.STATUS, HTTP_GET, [&](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["ssid"] = WiFi.SSID();
    root.printTo(*response);
    request->send(response);
  });
  server.on(URLS.API.WIFI.CONNECT, HTTP_POST, [&](AsyncWebServerRequest *request){
    String wlanSsid;
    String wlanPass;
    if(request->hasParam("ssid", true) && request->hasParam("pass", true)) {
      AsyncWebParameter* ssid = request->getParam("ssid", true);
      wlanSsid = ssid->value().c_str();
      AsyncWebParameter* pass = request->getParam("pass", true);
      wlanPass = pass->value().c_str();
      WiFi.setAutoConnect(true);
      WiFi.setAutoReconnect(true);
      WiFi.begin(wlanSsid, wlanPass);
      request->send(201);
      shouldReboot = true;
    } else {
      request->send(400);
    }
  });
  // server.on(URLS.API.WIFI.WPS, HTTP_POST, [&](AsyncWebServerRequest *request){
  //   Serial.print("Beginning WPS (press WPS button on your router) ... ");
  //   Serial.println(WiFi.beginWPSConfig() ? "Success" : "Failed");
  //   request->send(200);
  // });
  server.on(URLS.API.WIFI.DISCONNECT, HTTP_POST, [](AsyncWebServerRequest *request){
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(false);
    request->send(201);
    WiFi.disconnect();
  });
  server.on(URLS.API.WIFI.SCAN, HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "[";
    int n = WiFi.scanComplete();
    if(n == -2){
      WiFi.scanNetworks(true);
    } else if(n){
      for (int i = 0; i < n; ++i){
        if(i) json += ",";
        json += "{";
        json += "\"ssid\":\""+WiFi.SSID(i)+"\"";
        json += "}";
      }
      WiFi.scanDelete();
      if(WiFi.scanComplete() == -2){
        WiFi.scanNetworks(true);
      }
    }
    json += "]";
    request->send(200, "application/json", json);
    json = String();
  });

  // MQTT setup endpoints:
  // server.on(SETUP_URLS.STATICS.SRC.SETUP.SETUP_CSS, HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send(SPIFFS, SETUP_URLS.STATICS.SRC.SETUP.SETUP_CSS, "text/css");
  // });
  // server.on(SETUP_URLS.STATICS.SRC.SETUP.SETUP_JS, HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send(SPIFFS, SETUP_URLS.STATICS.SRC.SETUP.SETUP_JS, "text/javascript");
  // });
  // server.on(SETUP_URLS.STATICS.SRC.SETUP.INDEX, HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send(SPIFFS, SETUP_URLS.STATICS.SRC.SETUP.INDEX, "text/html");
  // });
  server.on(SETUP_URLS.API.MQTT.STATUS, HTTP_GET, [&](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["ssid"] = WiFi.SSID();
    root.printTo(*response);
    request->send(response);
  });
  server.on(SETUP_URLS.API.MQTT.CONNECT, HTTP_POST, [&](AsyncWebServerRequest *request){
    if(request->hasParam("host", true) &&
    // request->hasParam("port", true) &&
    request->hasParam("token", true)) {
      const String mqttHost = request->getParam("host", true)->value().c_str();
      const String mqttToken = request->getParam("token", true)->value().c_str();
      // uint16_t mqttPort = strtol(request->getParam("port", true)->value().c_str(), NULL, 0);
      writeMem(MQTT_SERVER_MEM_ADDR, mqttHost);
      writeMem(MQTT_KEY_MEM_ADDR, mqttToken);
      request->send(201);
      shouldReboot = true;
    } else {
      request->send(400);
    }
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404);
  });
}

#endif
