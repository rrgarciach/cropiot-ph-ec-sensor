#ifndef DEVICE_ENDPOINTS_H_
#define DEVICE_ENDPOINTS_H_

// Device setup endpoints
struct {
  struct {
    struct  {
      const char* PH_CALIBRATE = "/api/device/ph/calibrate";
      const char* TDS_CALIBRATE = "/api/device/tds/calibrate";
      const char* TDS_TEMPERATURE = "/api/device/tds/temperature";
    } DEVICE;
  } API;
} DEVICE_URLS;

#endif
