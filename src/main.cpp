#include "Arduino.h"
#include "ArduinoOTA.h"
#include "CropIoTDeviceSettings.h"
#include "../include/device_endpoints.h"
#include "Wire.h"
#include "Adafruit_ADS1015.h"
#include "OneWire.h"
// #include "GravityTDS.h"
// #include "DFRobot_EC.h"
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
#define TDS_PIN 1
#define DS18S20_PIN D3
#define PH4_READING_MEM_ADDR          440 // size of 20
#define PH7_READING_MEM_ADDR          410 // size of 20
#define TDS_700_READING_MEM_ADDR      380 // size of 20
#define TDS_2000_READING_MEM_ADDR     350 // size of 20

Adafruit_ADS1115 ads;
// DFRobot_PH ph;
// DFRobot_EC ec;
OneWire ds(DS18S20_PIN);

float voltagePH,voltageTDS,phValue,ecValue,tdsValue,temperature = 30;

// double slope = -0.2212389380530973;//-0.2190; //change this value to calibrate
// double intercept = 21.19026548672566;//20.6438; //change this value to calibrate
double pHslope = 0;
double pHintercept = 0;
double TDSslope = 0;
double TDSintercept = 0;
double pHVoltage = 0;
double tdsVoltage = 0;
// int sensorValue = 0;
// unsigned long int avgValue;
// int buf[100], temp;

void readPhSensor();
void readTDSSensor();
float readTemperature();
long int calcAvgValue(int pin, int numbOfSamples, int sampleSize);
double calcPHSlope();
double calcPHIntercept();
double calcTDSSlope();
double calcTDSIntercept();
void loadDeviceEndpoints();

void setup() {
  DEVICE_TYPE = "ph_ec_sensor";
  Serial.begin(115200);
  Serial.println("Starting...");

  connectWiFi();
  connectMQTT();
  loadDeviceEndpoints();
  ads.begin();
  pHslope = calcPHSlope();
  pHintercept = calcPHIntercept();
  // TDSslope = calcTDSSlope();
  // TDSintercept = calcTDSIntercept();

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
}

void loop() {
  ArduinoOTA.handle();
  reconnectWiFi();
  reconnectMQTT();
  readPhSensor();
  // readTDSSensor();
  delay(10*1000);
}

void readPhSensor() {
  int numbOfSamples = 100;
  int sampleSize = 80;
  long int pHAvgValue = calcAvgValue(PH_PIN, numbOfSamples, sampleSize);
  pHVoltage = (double)pHAvgValue * 5.0 / 1024 / ( sampleSize-(numbOfSamples-sampleSize) );
  double phValue = pHslope * pHVoltage + pHintercept;
  Serial.print("pHAvgValue raw value = ");
  Serial.print(pHAvgValue);
  Serial.print("\t pHVoltage value = ");
  Serial.print(pHVoltage, 6);
  Serial.print("\t pH = ");
  Serial.println(phValue);

  if (phValue < 0) phValue = 0;
  else if (phValue > 14) phValue = 14;

  String message = "{\"pH\": " + String(phValue) + "}";
  boolean succeed = mqttClient.publish("v1/devices/me/telemetry", message.c_str());
  if (!succeed) Serial.println("Publish message failed!");
}

void readTDSSensor() {
  int numbOfSamples = 100;
  int sampleSize = 80;
  long int tdsAvgValue = calcAvgValue(TDS_PIN, numbOfSamples, sampleSize);
  tdsVoltage = (double)tdsAvgValue * 5.0 / 1024 / ( sampleSize-(numbOfSamples-sampleSize) );
  temperature = readTemperature(); // read your temperature sensor to execute temperature compensation
  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
  float compensationVoltage = tdsVoltage / compensationCoefficient;  //temperature compensation
  double ecValue = (133.42 * pow(compensationVoltage, 3) - 255.86 * pow(compensationVoltage, 2) + 857.39 * compensationVoltage) * 0.5; //convert voltage value to tds value
  ecValue = TDSslope * compensationVoltage + TDSintercept;

  tdsValue = (ecValue / 500); // convert TDS to EC and then to μs/cm

  Serial.print("tdsAvgValue raw value = ");
  Serial.print(tdsAvgValue);
  Serial.print("\t tdsVoltage value = ");
  Serial.print(tdsVoltage, 6);
  Serial.print("\t compensationVoltage value = ");
  Serial.print(compensationVoltage, 6);
  Serial.print("\t TDS = ");
  Serial.print(tdsValue);
  Serial.print(" ppm");
  Serial.print("\t EC = ");
  Serial.print(ecValue);
  Serial.println(" μs/cm");

  if (tdsValue < 0) tdsValue = 0;
  else if (tdsValue > 9999) tdsValue = 9999;
  ecValue = (tdsValue / 500) * 1000; // convert TDS to EC and then to μs/cm

  String message = "{\"tds\": " + String(tdsValue) + ", \"ec\": " + String(ecValue) + ", \"temperature\": " + String(temperature) + "}";
  // Serial.println(message.c_str());
  boolean succeed = mqttClient.publish("v1/devices/me/telemetry", message.c_str());
  if (!succeed) Serial.println("Publish message failed!");
}

float readTemperature() {
  // returns the temperature from one DS18S20 in DEG Celsius
  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
      return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
      return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  Serial.print("Liquid temperature = ");
  Serial.print(TemperatureSum);
  Serial.println(" °C");

  return TemperatureSum;
}

long int calcAvgValue(int pin, int numbOfSamples, int sampleSize) {
  int buf[100], temp;
  unsigned long int avgValue = 0;

  for(int i = 0; i < numbOfSamples; i++)
  {
    buf[i] = ads.readADC_SingleEnded(pin);
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

  for(int i = numbOfSamples-sampleSize; i < sampleSize; i++)
    avgValue += buf[i];

  return avgValue;
}

double calcPHSlope() {
  double ph4Reading = readMem(PH4_READING_MEM_ADDR).toDouble();
  double ph7Reading = readMem(PH7_READING_MEM_ADDR).toDouble();
  Serial.print("Calculating pH slope...");
  double value = (4.0L - 7.0L) / (ph4Reading - ph7Reading);
  Serial.print(" pH Slope value: ");
  Serial.println(value, 16);
  return value;
}

double calcPHIntercept() {
  double ph7Reading = readMem(PH7_READING_MEM_ADDR).toDouble();
  double slope = calcPHSlope();
  Serial.print("Calculating pH intercept...");
  double value = fabs( (slope * ph7Reading) - 7.0L);
  Serial.print(" pH Intercept value: ");
  Serial.println(value, 16);
  return value;
}

double calcTDSSlope() {
  double tds700Reading = readMem(TDS_700_READING_MEM_ADDR).toInt();
  double tds2000Reading = readMem(TDS_2000_READING_MEM_ADDR).toInt();
  Serial.print("Calculating TDS slope...");
  double value = (700 - 2000) / (tds700Reading - tds2000Reading);
  Serial.print(" TDS Slope value: ");
  Serial.println(value, 16);
  return value;
}

double calcTDSIntercept() {
  double tds1000Reading = readMem(TDS_2000_READING_MEM_ADDR).toInt();
  double slope = calcTDSSlope();
  Serial.print("Calculating TDS intercept...");
  double value = abs( (slope * tds1000Reading) - 1000);
  Serial.print(" TDS Intercept value: ");
  Serial.println(value, 16);
  return value;
}

void loadDeviceEndpoints() {
  server.on(DEVICE_URLS.API.DEVICE.PH_CALIBRATE, HTTP_GET, [&](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["value"] = pHVoltage;
    root.printTo(*response);
    request->send(response);
  });
  server.on(DEVICE_URLS.API.DEVICE.PH_CALIBRATE, HTTP_POST, [&](AsyncWebServerRequest *request){
    if(request->hasParam("ph4Reading", true) && request->hasParam("ph7Reading", true)) {
      const String ph4Reading = request->getParam("ph4Reading", true)->value().c_str();
      const String ph7Reading = request->getParam("ph7Reading", true)->value().c_str();
      writeMem(PH4_READING_MEM_ADDR, ph4Reading);
      writeMem(PH7_READING_MEM_ADDR, ph7Reading);
      pHslope = calcPHSlope();
      pHintercept = calcPHIntercept();
      request->send(200);
    } else {
      request->send(400);
    }
  });
  server.on(DEVICE_URLS.API.DEVICE.TDS_CALIBRATE, HTTP_GET, [&](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["value"] = tdsVoltage;
    root.printTo(*response);
    request->send(response);
  });
  server.on(DEVICE_URLS.API.DEVICE.TDS_CALIBRATE, HTTP_POST, [&](AsyncWebServerRequest *request){
    if(request->hasParam("tds700Reading", true) && request->hasParam("tds2000Reading", true)) {
      const String tds700Reading = request->getParam("tds700Reading", true)->value().c_str();
      const String tds2000Reading = request->getParam("tds2000Reading", true)->value().c_str();
      writeMem(TDS_700_READING_MEM_ADDR, tds700Reading);
      writeMem(TDS_2000_READING_MEM_ADDR, tds2000Reading);
      TDSslope = calcTDSSlope();
      TDSintercept = calcTDSIntercept();
      request->send(200);
    } else {
      request->send(400);
    }
  });
  server.on(DEVICE_URLS.API.DEVICE.TDS_TEMPERATURE, HTTP_GET, [&](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["temperature"] = temperature;
    root.printTo(*response);
    request->send(response);
  });
}
