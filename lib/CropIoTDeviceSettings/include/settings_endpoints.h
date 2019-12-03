#ifndef SETTINGS_ENDPOINTS_H
#define SETTINGS_ENDPOINTS_H

// Index (wifi setup) endpoints
struct {
  const char* FAVICON = "/favicon.ico";
  struct {
    const char* DEVICE = "/api/device";
    struct  {
      const char* STATUS = "/api/settings/wifi/status";
      const char* CONNECT = "/api/settings/wifi/connect";
      const char* DISCONNECT = "/api/settings/wifi/disconnect";
      const char* SCAN = "/api/settings/wifi/scan";
    } WIFI;
  } API;
} URLS;

// MQTT setup endpoints
struct {
  struct {
    struct  {
      const char* STATUS = "/api/settings/mqtt/status";
      const char* CONNECT = "/api/settings/mqtt/connect";
      const char* SUBSCRIBE = "/api/settings/mqtt/subscribe";
      const char* PUBLISH = "/api/settings/mqtt/publish";
    } MQTT;
  } API;
} SETUP_URLS;

#endif
