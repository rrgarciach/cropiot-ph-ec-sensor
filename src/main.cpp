#include "Arduino.h"
#include "EEPROM.h"
#include "Wire.h"
#include "ESP8266WiFi.h"
#include "PubSubClient.h"
#include "Adafruit_ADS1015.h"

// https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino

// PH module Po   -> A0 ADS1115
// PH sensor GND  -> Nodemcu GND
// PH sensor V+   -> Nodemcu VU
// ADS1115 SDA    -> Nodemcu D2
// ADS1115 SCL    -> Nodemcu D1
// ADS1115 GND    -> Nodemcu GND
// ADS1115 VDD    -> Nodemcu VU

const char* mqtt_server = "demo.thingsboard.io";
#define ACCESS_TOKEN      "KjrfColRMYhfI9PYQTSZ"

#define MQTT_SERVER_ADDR 550
#define ACCESS_TOKEN_ADDR 500

WiFiClient wlanClient1;

PubSubClient client(wlanClient1);

Adafruit_ADS1115 ads;

int phSensorADCPin = 0;

float intercept = 20.6438; //change this value to calibrate
float slope = -0.2190; //change this value to calibrate
int sensorValue = 0;
unsigned long int avgValue;
float b;
int buf[10], temp;

void connectWiFi(String wlanSsid, String wlanPass);
void connectMQTT(String deviceId, String accessToken);
float calcSlope(float ph4Reading, float ph7Reading);
float calcIntercept(float slope, float ph7Reading, float phReference);
void writeMem(char add,String data);
String readMem(char add);

void setup() {
  Serial.println("Starting...");
  Serial.begin(115200);
  EEPROM.begin(512);
  connectWiFi("RiagaTechRRG", "RUyCHAVEz206");
  // writeMem(MQTT_SERVER_ADDR, "demo.thingsboard.io");
  // writeMem(ACCESS_TOKEN_ADDR, "KjrfColRMYhfI9PYQTSZ");
  String mqtt_server = readMem(MQTT_SERVER_ADDR);
  // String port = readMem(ACCESS_TOKEN_ADDR);
  Serial.println(mqtt_server);
  // Serial.println(port);
  client.setServer(mqtt_server.c_str(), 1883);
  connectMQTT("pH Sensor", ACCESS_TOKEN);
  ads.begin();
  Serial.println("Ready");
}

void loop() {
  for(int i=0;i<10;i++)
  {
    // buf[i]=analogRead(phSensorPin);
    buf[i] = ads.readADC_SingleEnded(phSensorADCPin);
    delay(30);
  }
  for(int i=0;i<9;i++)
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }
  avgValue=0;

  for(int i=2;i<8;i++)
    avgValue+=buf[i];

  float pHVol=(float)avgValue*5.0/1024/6;
  float phValue = slope * pHVol + intercept;
  Serial.print("sensor = ");
  Serial.println(phValue);

  String message = "{\"pH\": " + String(phValue) + "}";
  client.publish("v1/devices/me/telemetry", message.c_str());

  delay(10*1000);
}

void connectWiFi(String wlanSsid, String wlanPass) {
  // WiFi.setAutoConnect(true);
  // WiFi.setAutoReconnect(true);
  // WiFi.begin(wlanSsid, wlanPass);
  while (!WiFi.isConnected()) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected!");
  Serial.print("local IP address: ");
  Serial.println(WiFi.localIP());
  // Serial.print("AP IP address: ");
  // Serial.println(WiFi.softAPIP());
}

void connectMQTT(String deviceId, String accessToken) {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if ( client.connect(deviceId.c_str(), accessToken.c_str(), NULL) ) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

float calcSlope(float ph4Reading, float ph7Reading) {
  return (4.0 - 7.0) / (ph4Reading - ph7Reading);
}

float calcIntercept(float slope, float ph7Reading, float phReference) {
  return abs( (slope * ph7Reading) - phReference );
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
