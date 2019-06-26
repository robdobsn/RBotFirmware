// NetLog
// Used to log data to various places (MQTT, commandSerial, HTTP) from ArduinoLog module
// Rob Dobson 2018

#pragma once

#include <ArduinoLog.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include "MQTTManager.h"
#include "CommandSerial.h"
#include "RingBufferPosn.h"

class NetLog : public Print
{
public:
    // Used to pause and resume logging
    static constexpr char ASCII_XOFF = 0x13;
    static constexpr char ASCII_XON = 0x11;

private:
    // Log message needs to be built up from parts
    String _msgToLog;
    bool _firstChOnLine;
    bool _collectLineForLog;
    int _curMsgLogLevel;
    const int LOG_LINE_MAXLEN = 250;

    // Logging goes to this sink always
    Print& _output;

    // Logging destinations
    MQTTManager& _mqttManager;
    bool _logToMQTT;
    String _mqttLogTopic;
    bool _logToHTTP;
    String _httpIpAddr;
    int _httpPort;
    String _httpLogUrl;
    int _logToSerial;
    int _serialPort;
    int _logToCommandSerial;
    CommandSerial& _commandSerial;
    bool _logToPapertrail;
    String _papertrailHost;
    int _papertrailPort;
    
    // Logging control
    int _loggingThreshold;
    
    // Configuration (object held elsewhere and pointer kept
    // to allow config changes to be written back)
    ConfigBase *_pConfigBase;

    // TCP client
    WiFiClient _wifiClient;
    const int MAX_RX_BUFFER_SIZE = 100;

    // UDP
    WiFiUDP Udp;

    // System name
    String _systemName;

    // Pause functionality
    bool _isPaused;
    uint32_t _pauseTimeMs;
    uint32_t _pauseStartedMs;
    uint8_t *_pChBuffer;
    RingBufferPosn _chBufferPosn;

public:
    NetLog(Print& output, MQTTManager& mqttManager, CommandSerial& commandSerial,
            int pauseBufferMaxChars = 1000, uint32_t pauseTimeMs = 15000) :
        _output(output),
        _mqttManager(mqttManager),
        _commandSerial(commandSerial),
        _chBufferPosn(pauseBufferMaxChars)
    {
        _firstChOnLine = true;
        _collectLineForLog = false;
        _msgToLog.reserve(250);
        _curMsgLogLevel = LOG_LEVEL_SILENT;
        _loggingThreshold = LOG_LEVEL_SILENT;
        _logToMQTT = false;
        _logToHTTP = false;
        _pConfigBase = NULL;
        _httpPort = 5076;
        _logToSerial = true;
        _logToCommandSerial = false;
        _serialPort = 0;
        _pauseStartedMs = 0;
        _pauseTimeMs = pauseTimeMs;
        _isPaused = false;
        _pChBuffer = new uint8_t[pauseBufferMaxChars];
    }

    void setLogLevel(const char* logLevelStr);
    void setMQTT(bool mqttFlag, const char* mqttLogTopic);
    void setSerial(bool onOffFlag, const char* serialPortStr);
    void setCmdSerial(bool onOffFlag);
    void setHTTP(bool httpFlag, const char* ipAddr, const char* portStr, const char* httpLogUrl);
    void setPapertrail(bool papertrailFlag, const char* hostStr, const char* portStr);
    void getConfig(String& configStr);
    void setup(ConfigBase *pConfig, const char* systemName);
    String formConfigStr();
    void pause();
    void resume();
    size_t write(uint8_t ch);
    void service(char xonXoffChar = 0);
private:
    void handleLoggedDuringPause();
};
