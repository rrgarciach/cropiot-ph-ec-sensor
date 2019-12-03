#ifndef SETTINGS_ENDPOINTS_H
#define SETTINGS_ENDPOINTS_H

// Index (wifi setup) endpoints
struct {
  const char* FAVICON = "/favicon.ico";
  struct {
    const char* DEVICE = "/api/device";
    struct  {
      const char* SSID = "/api/settings/wifi/ssid";
      const char* STATUS = "/api/settings/wifi/status";
      const char* CONNECT = "/api/settings/wifi/connect";
      const char* DISCONNECT = "/api/settings/wifi/disconnect";
      const char* SCAN = "/api/settings/wifi/scan";
    } WIFI;
  } API;
  struct {
    struct  {
      const char* JQUERY_MIN_JS = "/src/jquery-3.3.1.min.js";
      const char* BOOTSTRAP_MIN_JS = "/src/bootstrap.bundle.min.js";
      const char* BOOTSTRAP_MIN_CSS = "/src/bootstrap.min.css";
      const char* INDEX_JS = "/src/index.js";
    } SRC;
    struct  {
      const char* LOGO_PNG = "/images/logo.png";
    } IMAGES;
  } STATICS;
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
  struct {
    struct {
      struct  {
        const char* INDEX = "/src/setup/index.html";
        const char* SETUP_JS = "/src/setup/setup.js";
        const char* SETUP_CSS = "/src/setup/setup.css";
      } SETUP;
    } SRC;
  } STATICS;
} SETUP_URLS;

#endif
