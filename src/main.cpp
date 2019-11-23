#include "Arduino.h"
#include "Wire.h"
#include "Adafruit_ADS1015.h"

// PH module Po   -> A0 ADS1115
// PH sensor GND  -> Nodemcu GND
// PH sensor V+   -> Nodemcu VU
// ADS1115 SDA    -> Nodemcu D2
// ADS1115 SCL    -> Nodemcu D1
// ADS1115 GND    -> Nodemcu GND
// ADS1115 VDD    -> Nodemcu VU

Adafruit_ADS1115 ads;

float calibration = 20.6438; //change this value to calibrate
float slope = -0.2190; //change this value to calibrate
const int analogInPin = A0;
int sensorValue = 0;
unsigned long int avgValue;
float b;
int buf[10],temp;

void setup() {
  Serial.begin(115200);
  ads.begin();
}

void loop() {
  for(int i=0;i<10;i++)
  {
    // buf[i]=analogRead(analogInPin);
    buf[i] = ads.readADC_SingleEnded(0);
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
  float phValue = slope * pHVol + calibration;
  Serial.print("sensor = ");
  Serial.println(phValue);

  delay(500);
}
