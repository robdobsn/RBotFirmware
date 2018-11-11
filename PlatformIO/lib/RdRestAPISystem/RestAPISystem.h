// REST API for System, WiFi, etc
// Rob Dobson 2012-2018

#pragma once
#include <Arduino.h>
#include <WiFiManager.h>
#include "RestAPIEndpoints.h"
#include "RdOTAUpdate.h"
#include "MQTTManager.h"
#include "NetLog.h"
#include "FileManager.h"

class RestAPISystem
{
private:
    bool _deviceRestartPending;
    unsigned long _deviceRestartMs;
    static const int DEVICE_RESTART_DELAY_MS = 1000;
    bool _updateCheckPending;
    unsigned long _updateCheckMs;
    // Delay before starting an update check
    // For some reason TCP connect fails if there is not a sufficient delay
    // But only when initiated from MQTT (web works ok)
    // 3s doesn't work, 5s seems ok
    static const int DEVICE_UPDATE_DELAY_MS = 7000;
    WiFiManager& _wifiManager;
    MQTTManager& _mqttManager;
    RdOTAUpdate& _otaUpdate;
    NetLog& _netLog;
    FileManager& _fileManager;
    String _systemType;
    String _systemVersion;
    
public:
    RestAPISystem(WiFiManager& wifiManager, MQTTManager& mqttManager,
                RdOTAUpdate& otaUpdate, NetLog& netLog,
                FileManager& fileManager,
                const char* systemType, const char* systemVersion) :
                _wifiManager(wifiManager), _mqttManager(mqttManager), 
                _otaUpdate(otaUpdate), _netLog(netLog),
                _fileManager(fileManager)
    {
        _deviceRestartPending = false;
        _deviceRestartMs = 0;
        _updateCheckPending = false;
        _updateCheckMs = 0;
        _systemType = systemType;
        _systemVersion = systemVersion;
    }

    // Call frequently
    void service();

    // WiFi settings
    void apiWifiSet(String &reqStr, String &respStr);
    void apiWifiClear(String &reqStr, String &respStr);
    void apiWifiExtAntenna(String &reqStr, String &respStr);
    void apiWifiIntAntenna(String &reqStr, String &respStr);

    // MQTT settings
    void apiMQTTSet(String &reqStr, String &respStr);

    // Reset machine
    void apiReset(String &reqStr, String& respStr);

    // Netlog settings
    void apiNetLogLevel(String &reqStr, String &respStr);
    void apiNetLogMQTT(String &reqStr, String &respStr);
    void apiNetLogSerial(String &reqStr, String &respStr);
    void apiNetLogCmdSerial(String &reqStr, String &respStr);
    void apiNetLogHTTP(String &reqStr, String &respStr);

    // Check for OTA updates
    void apiCheckUpdate(String &reqStr, String& respStr);
    
    // Get system version
    void apiGetVersion(String &reqStr, String& respStr);

    // Format file system
    void apiReformatFS(String &reqStr, String& respStr);

    // List files on a file system
    // Uses FileManager.h
    // In the reqStr the first part of the path is the file system name (e.g. SD or SPIFFS)
    // The second part of the path is the folder - note that / must be replaced with ~ in folder
    void apiFileList(String &reqStr, String& respStr);

    // Delete file on the file system
    // Uses FileManager.h
    // In the reqStr the first part of the path is the file system name (e.g. SD or SPIFFS)
    // The second part of the path is the filename - note that / must be replaced with ~ in filename
    void apiDeleteFile(String &reqStr, String& respStr);

    // Upload file to file system - completed
    void apiUploadToFileManComplete(String &reqStr, String &respStr);

    // Upload file to file system - part of file (from HTTP POST file)
    void apiUploadToFileManPart(String& req, String& filename, size_t contentLen, size_t index, 
                uint8_t *data, size_t len, bool finalBlock);

    void setup(RestAPIEndpoints &endpoints);

    static String getWifiStatusStr();
    static int reportHealth(int bitPosStart, unsigned long *pOutHash, String *pOutStr);
};
