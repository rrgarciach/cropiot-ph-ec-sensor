#include "Arduino.h"
#include "CropIoTDeviceSettings.h"
#include "../include/device_endpoints.h"
#include "Wire.h"
#include "Adafruit_ADS1015.h"
#include "DFRobot_EC.h"
// #include "DFRobot_PH.h"

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
#define PH4_READING_MEM_ADDR      480 // size of 20
#define PH7_READING_MEM_ADDR      460 // size of 20

WiFiClient wlanClient1;
PubSubClient mqttClient1(wlanClient1);

Adafruit_ADS1115 ads;
// DFRobot_PH ph;
DFRobot_EC ec;

float voltagePH,voltageEC,phValue,ecValue,temperature = 25;

// double slope = -0.2212389380530973;//-0.2190; //change this value to calibrate
// double intercept = 21.19026548672566;//20.6438; //change this value to calibrate
double slope = 0;
double intercept = 0;
int sensorValue = 0;
unsigned long int avgValue;
int buf[100], temp;

void readPhSensor();
void readECSensor();
double calcSlope();
double calcIntercept();
void loadDeviceEndpoints();

void setup() {
  DEVICE_TYPE = "ph_ec_sensor";
  Serial.begin(115200);
  Serial.println("Starting...");

  connectWiFi();
  wlanClient1 = generateWiFiClient();
  connectMQTT(mqttClient1);
  loadDeviceEndpoints();

  ec.begin();
  ads.begin();
  slope = calcSlope();
  intercept = calcIntercept();

  Serial.println("Ready");
}

void loop() {
  reconnectWiFi();
  reconnectMQTT(mqttClient1);
  readPhSensor();
  delay(10*1000);
}

void readPhSensor() {
  int numbOfSamples = 100;
  int sampleSize = 80;
  for(int i = 0; i < numbOfSamples; i++)
  {
    buf[i] = ads.readADC_SingleEnded(PH_PIN);
    delay(30);
  }
  for(int i = 0; i < numbOfSamples-1; i++)
  {
    for(int j = i + 1; j < numbOfSamples; j++)
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

  for(int i = numbOfSamples-sampleSize; i < sampleSize; i++)
    avgValue += buf[i];

  double pHVol = (double)avgValue * 5.0 / 1024 / ( sampleSize-(numbOfSamples-sampleSize) );
  double phValue = slope * pHVol + intercept;
  Serial.print("avgValue raw value = ");
  Serial.print(avgValue);
  Serial.print("\t pHVol value = ");
  Serial.print(pHVol);
  Serial.print("\t pH sensor = ");
  Serial.println(phValue);

  String message = "{\"pH\": " + String(phValue) + "}";
  mqttClient1.publish("v1/devices/me/telemetry", message.c_str());
}

void readECSensor() {
  voltageEC = ads.readADC_SingleEnded(EC_PIN)/1024.0*5000;   // read the voltage
  //temperature = readTemperature();          // read your temperature sensor to execute temperature compensation
  ecValue    = ec.readEC(voltageEC,temperature)*10;  // convert voltage to EC with temperature compensation
  Serial.print("raw:");
  Serial.print(ads.readADC_SingleEnded(EC_PIN));
  Serial.print("\t");
  Serial.print(", EC:");
  Serial.print(ecValue,2);
  Serial.println("μs/cm");

  // String message = "{\"ec\": " + StriXng(ecValue) + "}";
  // client.publish("v1/devices/me/telemetry", message.c_str());
}

double calcSlope() {
  double ph4Reading = readMem(PH4_READING_MEM_ADDR).toDouble();
  double ph7Reading = readMem(PH7_READING_MEM_ADDR).toDouble();
  Serial.print("Calculating slope...");
  double value = (4.0L - 7.0L) / (ph4Reading - ph7Reading);
  Serial.print(" Slope value: ");
  Serial.println(value, 16);
  return value;
}

double calcIntercept() {
  double ph4Reading = readMem(PH4_READING_MEM_ADDR).toDouble();
  double ph7Reading = readMem(PH7_READING_MEM_ADDR).toDouble();
  double slope = calcSlope();
  Serial.print("Calculating intercept...");
  double value = fabs( (slope * ph7Reading) - 7.0L);
  Serial.print(" Intercept value: ");
  Serial.println(value, 16);
  return value;
}

void loadDeviceEndpoints() {
  server.on(DEVICE_URLS.API.DEVICE.PH_CALIBRATE, HTTP_GET, [&](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["value"] = avgValue;
    root.printTo(*response);
    request->send(response);
  });
  server.on(DEVICE_URLS.API.DEVICE.PH_CALIBRATE, HTTP_POST, [&](AsyncWebServerRequest *request){
    if(request->hasParam("ph4Reading", true) && request->hasParam("ph7Reading", true)) {
      const String ph4Reading = request->getParam("ph4Reading", true)->value().c_str();
      const String ph7Reading = request->getParam("ph7Reading", true)->value().c_str();
      writeMem(PH4_READING_MEM_ADDR, ph4Reading);
      writeMem(PH7_READING_MEM_ADDR, ph7Reading);
      slope = calcSlope();
      intercept = calcIntercept();
      request->send(200);
    } else {
      request->send(400);
    }
  });
}
