// WiFi Manager
// Rob Dobson 2018

#pragma once

#include <Arduino.h>
#include "WiFi.h"

class StatusIndicator;
class ConfigBase;

class WiFiManager
{
private:
    bool _wifiEnabled;
    String _ssid;
    String _password;
    static String _hostname;
    String _defaultHostname;
    unsigned long _lastWifiBeginAttemptMs;
    bool _wifiFirstBeginDone;
    static constexpr unsigned long TIME_BETWEEN_WIFI_BEGIN_ATTEMPTS_MS = 60000;
    ConfigBase* _pConfigBase;
    static StatusIndicator* _pStatusLed;

public:
    WiFiManager()
    {
        _wifiEnabled = false;
        _lastWifiBeginAttemptMs = 0;
        _wifiFirstBeginDone = false;
        _pConfigBase = NULL;
        _pStatusLed = NULL;
    }

    bool isEnabled();
    String getHostname();
    void setup(ConfigBase& hwConfig, ConfigBase *pSysConfig, const char *defaultHostname, StatusIndicator *pStatusLed);
    void service();
    bool isConnected();
    String formConfigStr();
    void setCredentials(String &ssid, String &pw, String &hostname);
    void clearCredentials();
    static void wiFiEventHandler(WiFiEvent_t event);
    static const char* getEventName(WiFiEvent_t event);
};
