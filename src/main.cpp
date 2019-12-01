#include "Arduino.h"
#include "CropIoTDeviceSettings.h"
#include "../include/device_endpoints.h"
#include "Wire.h"
#include "Adafruit_ADS1015.h"
#include "DFRobot_EC.h"
// #include "DFRobot_PH.h"

// https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino

// PH module Po   -> A0 ADS1115
// PH sensor GND  -> Nodemcu GND
// PH sensor V+   -> Nodemcu VU
// ADS1115 SDA    -> Nodemcu D2
// ADS1115 SCL    -> Nodemcu D1
// ADS1115 GND    -> Nodemcu GND
// ADS1115 VDD    -> Nodemcu VU

// apply assets
// implement MQTT UI
// Implement pH calibration UI setup and API
// Implement EC reading
// Implement EC calibration UI setup and API
// Implement pumps control
// Implement pumps calibration UI setup and API

#define PH_PIN 0
#define EC_PIN 1
// #define SLOPE_MEM_ADDR          490 // size of 2
// #define INTERCEPT_MEM_ADDR      495 // size of 5
#define PH4_READING_MEM_ADDR      490 // size of 2
#define PH7_READING_MEM_ADDR      495 // size of 5

WiFiClient wlanClient1;
PubSubClient mqttClient1(wlanClient1);

Adafruit_ADS1115 ads;
// DFRobot_PH ph;
DFRobot_EC ec;

float voltagePH,voltageEC,phValue,ecValue,temperature = 25;

// float slope = -0.2212389380530973;//-0.2190; //change this value to calibrate
// float intercept = 21.19026548672566;//20.6438; //change this value to calibrate
float slope = 0;//-0.2190; //change this value to calibrate
float intercept = 0;//20.6438; //change this value to calibrate
int sensorValue = 0;
unsigned long int avgValue;
float b;
int buf[100], temp;

float readPhSensor();
float readECSensor();
// bool readSerial(char result[]);
float calcSlope();
float calcIntercept();
void loadDeviceEndpoints();

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");
  connectWiFi();
  wlanClient1 = generateWiFiClient();
  connectMQTT(mqttClient1);
  // ph.begin();
  ec.begin();
  ads.begin();

  slope = calcSlope();
  intercept = calcIntercept();
  Serial.println("Ready");
}

void loop() {
  connectMQTT(mqttClient1);
  readPhSensor();
  delay(10*1000);
  // char cmd[10];
  // static unsigned long timepoint = millis();

  // if(millis()-timepoint>1000U){                            //time interval: 1s
    // readPhSensor();
    // delay(10*1000);
    // readECSensor();
  // }
  //
  // if(readSerial(cmd)){
  //   strupr(cmd);
  //   if(strstr(cmd,"PH")){
  //     ph.calibration(voltagePH,temperature,cmd);       //PH calibration process by Serail CMD
  //   }
  //   if(strstr(cmd,"EC")){
  //     ec.calibration(voltageEC,temperature,cmd);       //EC calibration process by Serail CMD
  //   }
  // }
  reconnectWiFi();
  reconnectMQTT(mqttClient1);
}

float readPhSensor() {
  for(int i = 0; i < 10; i++)
  {
    buf[i] = ads.readADC_SingleEnded(PH_PIN);
    delay(30);
  }
  for(int i = 0; i < 9; i++)
  {
    for(int j = i + 1; j < 10; j++)
    {
      if(buf[i] > buf[j])
      {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }
  avgValue = 0;

  for(int i = 2; i < 8; i++)
    avgValue += buf[i];

  float pHVol = (float)avgValue * 5.0 / 1024 / 6;
  float phValue = slope * pHVol + intercept;
  Serial.print("avgValue raw value = ");
  Serial.print(avgValue);
  Serial.print("\t pHVol value = ");
  Serial.print(pHVol);
  Serial.print("\t pH sensor = ");
  Serial.println(phValue);

  // //temperature = readTemperature();                   // read your temperature sensor to execute temperature compensation
  // voltagePH = ads.readADC_SingleEnded(PH_PIN)/1024.0*5000;          // read the ph voltage
  // phValue    = ph.readPH(voltagePH,temperature);       // convert voltage to pH with temperature compensation
  // Serial.print("raw:");
  // Serial.print(ads.readADC_SingleEnded(PH_PIN));
  // Serial.print("\t");
  // Serial.print("pH:");
  // Serial.print(phValue,2);
  // voltageEC = analogRead(EC_PIN)/1024.0*5000;

  String message = "{\"pH\": " + String(phValue) + "}";
  mqttClient1.publish("v1/devices/me/telemetry", message.c_str());
  return avgValue;
}

float readECSensor() {
  voltageEC = ads.readADC_SingleEnded(EC_PIN)/1024.0*5000;   // read the voltage
  //temperature = readTemperature();          // read your temperature sensor to execute temperature compensation
  ecValue    = ec.readEC(voltageEC,temperature)*10;  // convert voltage to EC with temperature compensation
  Serial.print("raw:");
  Serial.print(ads.readADC_SingleEnded(EC_PIN));
  Serial.print("\t");
  Serial.print(", EC:");
  Serial.print(ecValue,2);
  Serial.println("μs/cm");
  return voltageEC;

  // String message = "{\"ec\": " + StriXng(ecValue) + "}";
  // client.publish("v1/devices/me/telemetry", message.c_str());
}

// int i = 0;
// bool readSerial(char result[]) {
//     while(Serial.available() > 0){
//         char inChar = Serial.read();
//         if(inChar == '\n'){
//              result[i] = '\0';
//              Serial.flush();
//              i=0;
//              return true;
//         }
//         if(inChar != '\r'){
//              result[i] = inChar;
//              i++;
//         }
//         delay(1);
//     }
//     return false;
// }

float calcSlope() {
  float ph4Reading = readMem(PH4_READING_MEM_ADDR).toFloat();
  float ph7Reading = readMem(PH7_READING_MEM_ADDR).toFloat();
  float value = (4.0 - 7.0) / (ph4Reading - ph7Reading);
  Serial.print("Slope value: ");
  Serial.println(value);
  return value;
}

float calcIntercept() {
  float ph4Reading = readMem(PH4_READING_MEM_ADDR).toFloat();
  float ph7Reading = readMem(PH7_READING_MEM_ADDR).toFloat();
  float slope = calcSlope();
  float value = abs( (slope * ph7Reading) - 7);
  Serial.print("Intercept value: ");
  Serial.println(value);
  return value;
}

void loadDeviceEndpoints() {
  // device setup endpoints:
  server.on(DEVICE_URLS.API.DEVICE.STATUS, HTTP_GET, [&](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["ssid"] = WiFi.SSID();
    root.printTo(*response);
    request->send(response);
  });
  server.on(DEVICE_URLS.API.DEVICE.PH_CALIBRATE, HTTP_GET, [&](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["value"] = readPhSensor();
    root.printTo(*response);
    request->send(response);
  });
  server.on(DEVICE_URLS.API.DEVICE.PH_CALIBRATE, HTTP_POST, [&](AsyncWebServerRequest *request){
    if(request->hasParam("ph4Reading", true) && request->hasParam("ph7Reading", true)) {
      const String ph4Reading = request->getParam("ph4Reading", true)->value().c_str();
      const String ph7Reading = request->getParam("ph7Reading", true)->value().c_str();
      writeMem(PH4_READING_MEM_ADDR, ph4Reading);
      writeMem(PH7_READING_MEM_ADDR, ph7Reading);
      request->send(200);
      ESP.reset();
    } else {
      request->send(400);
    }
  });
}
