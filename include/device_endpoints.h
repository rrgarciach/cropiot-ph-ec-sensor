#ifndef DEVICE_ENDPOINTS_H_
#define DEVICE_ENDPOINTS_H_

// Device setup endpoints
struct {
  struct {
    struct  {
      const char* PH_CALIBRATE = "/api/device/ph/calibrate";
      const char* EC_CALIBRATE = "/api/device/ec/calibrate";
    } DEVICE;
  } API;
} DEVICE_URLS;

#endif
