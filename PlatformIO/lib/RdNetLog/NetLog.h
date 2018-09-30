// MQTT Log
// Used to log data to MQTT from ArduinoLog module
// Rob Dobson 2018

#pragma once

#include <ArduinoLog.h>
#include <WiFiClient.h>
#include "MQTTManager.h"

class NetLog : public Print
{
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
    
    // Logging control
    int _loggingThreshold;
    
    // MQTT configuration (object held elsewhere and pointer kept
    // to allow config changes to be written back)
    ConfigBase *_pConfigBase;

    // TCP client
    WiFiClient _wifiClient;
    const int MAX_RX_BUFFER_SIZE = 100;

    // System name
    String _systemName;

  public:
    NetLog(Print& output, MQTTManager& mqttManager) :
        _output(output),
        _mqttManager(mqttManager)
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
        _serialPort = 0;
    }

    void setLogLevel(const char* logLevelStr)
    {
        // Get level
        int logLevel = LOG_LEVEL_SILENT;
        switch(toupper(*logLevelStr))
        {
            case 'F': logLevel = LOG_LEVEL_FATAL; break;
            case 'E': logLevel = LOG_LEVEL_ERROR; break;
            case 'W': logLevel = LOG_LEVEL_WARNING; break;
            case 'N': logLevel = LOG_LEVEL_NOTICE; break;
            case 'T': logLevel = LOG_LEVEL_TRACE; break;
            case 'V': logLevel = LOG_LEVEL_VERBOSE; break;
        }
        // Store result
        bool logLevelChanged = (_loggingThreshold != logLevel);
        _loggingThreshold = logLevel;
        // Persist if changed
        if (logLevelChanged)
        {
            if (_pConfigBase)
            {
                _pConfigBase->setConfigData(formConfigStr().c_str());
                _pConfigBase->writeConfig();
            }
        }
        else
        {
            if (_logToSerial && _serialPort == 0)
                Serial.printf("NetLog: LogLevel unchanged\n");
        }
    }

    void setMQTT(bool mqttFlag, const char* mqttLogTopic)
    {
        // Set values
        bool dataChanged = ((_logToMQTT != mqttFlag) || (_mqttLogTopic != mqttLogTopic));
        _logToMQTT = mqttFlag;
        _mqttLogTopic = mqttLogTopic;
        // Persist if changed
        if (dataChanged)
        {
            if (_pConfigBase)
            {
                _pConfigBase->setConfigData(formConfigStr().c_str());
                _pConfigBase->writeConfig();
            }
        }
    }

    void setSerial(bool onOffFlag, const char* serialPortStr)
    {
        // Set values
        bool dataChanged = ((_logToSerial != onOffFlag) || (String(_serialPort) != String(serialPortStr)));
        _logToSerial = onOffFlag;
        _serialPort = atoi(serialPortStr);
        // Persist if changed
        if (dataChanged)
        {
            if (_pConfigBase)
            {
                _pConfigBase->setConfigData(formConfigStr().c_str());
                _pConfigBase->writeConfig();
            }
        }
    }

    void setHTTP(bool httpFlag, const char* ipAddr, const char* portStr, const char* httpLogUrl)
    {
        // Set values
        String ipAddrValidated = ipAddr;
        if (ipAddrValidated.length() == 0)
            ipAddrValidated = _httpIpAddr;
        int portValidated = String(portStr).toInt();
        if (strlen(portStr) == 0)
            portValidated = _httpPort;
        String httpLogUrlValidated = httpLogUrl;
        if (httpLogUrlValidated.length() == 0)
            httpLogUrlValidated = _httpLogUrl;
        bool dataChanged = ((_logToHTTP != httpFlag) || (_httpLogUrl != httpLogUrlValidated) ||
                    (_httpIpAddr != ipAddrValidated) || (_httpPort != portValidated));
        _logToHTTP = httpFlag;
        _httpIpAddr = ipAddrValidated;
        _httpPort = portValidated;
        _httpLogUrl = httpLogUrlValidated;
        // Persist if changed
        if (dataChanged)
        {
            if (_pConfigBase)
            {
                _pConfigBase->setConfigData(formConfigStr().c_str());
                _pConfigBase->writeConfig();
            }
        }
        else
        {
            if (_logToSerial && _serialPort == 0)
                Serial.printf("NetLog: Config data unchanged\n");
        }
    }

    void setup(ConfigBase *pConfig, const char* systemName)
    {
        _systemName = systemName;
        _pConfigBase = pConfig;
        if (!pConfig)
            return;
        if (_logToSerial && _serialPort == 0)
            Serial.printf("NetLog: Setup from %s\n", pConfig->getConfigData());
        // Get the log level
        _loggingThreshold = pConfig->getLong("LogLevel", LOG_LEVEL_SILENT);
        // Get MQTT settings
        _logToMQTT = pConfig->getLong("MQTTFlag", 0) != 0;
        _mqttLogTopic = pConfig->getString("MQTTTopic", "");
        // Get HTTP settings
        _logToHTTP = pConfig->getLong("HTTPFlag", 0) != 0;
        _httpIpAddr = pConfig->getString("HTTPAddr", "");
        _httpPort = pConfig->getLong("HTTPPort", 5076);
        _httpLogUrl = pConfig->getString("HTTPUrl", "");
        // Get Serial settings
        _logToSerial = pConfig->getLong("SerialFlag", 0) != 0;
        _serialPort = pConfig->getLong("SerialPort", 5076);
        
        // Debug
        if (_logToSerial && _serialPort == 0)
            Serial.printf("NetLog: logLevel %d, mqttFlag %d topic %s, httpFlag %d, ip %s, port %d, url %s, serialFlag %d, serialPort %d\n",
                    _loggingThreshold, _logToMQTT, _mqttLogTopic.c_str(),
                    _logToHTTP, _httpIpAddr.c_str(), _httpPort, _httpLogUrl.c_str(),
                    _logToSerial, _serialPort);
    }

    String formConfigStr()
    {
        // This string is stored in NV ram for configuration on power up
        return "{\"LogLevel\":\"" + String(_loggingThreshold) +
                        "\",\"MQTTFlag\":\"" + String(_logToMQTT ? 1 : 0) + 
                        "\",\"MQTTTopic\":\"" + _mqttLogTopic +
                        "\",\"HTTPFlag\":\"" + String(_logToHTTP ? 1 : 0) + 
                        "\",\"HTTPAddr\":\"" + _httpIpAddr + 
                        "\",\"HTTPPort\":\"" + String(_httpPort) + 
                        "\",\"HTTPUrl\":\"" + _httpLogUrl +
                        "\",\"SerialFlag\":\"" + _logToSerial + 
                        "\",\"SerialPort\":\"" + String(_serialPort) + 
                        "\"}";
    }

    size_t write(uint8_t ch)
    {
        // Check for log to serial
        int retVal = 0;
        if (_logToSerial)
        {
            if (_serialPort == 0)
                retVal = Serial.write(ch);
        }

        // Check for log to MQTT or HTTP
        if (!(_logToMQTT || _logToHTTP))
            return retVal;
        
        // Check for first char on line
        if (_firstChOnLine)
        {
            _firstChOnLine = false;
            // Get msg level from first char in message
            int msgLevel = LOG_LEVEL_SILENT;
            switch(ch)
            {
                case 'F': msgLevel = LOG_LEVEL_FATAL; break;
                case 'E': msgLevel = LOG_LEVEL_ERROR; break;
                case 'W': msgLevel = LOG_LEVEL_WARNING; break;
                case 'N': msgLevel = LOG_LEVEL_NOTICE; break;
                case 'T': msgLevel = LOG_LEVEL_TRACE; break;
                case 'V': msgLevel = LOG_LEVEL_VERBOSE; break;
            }
            if (msgLevel <= _loggingThreshold)
            {
                _collectLineForLog = true;
                _curMsgLogLevel = msgLevel;
                _msgToLog = (char)ch;
            }
        }
        else if (_collectLineForLog)
        {
            if (_msgToLog.length() < LOG_LINE_MAXLEN)
                _msgToLog += (char)ch;
        }

        // Check for EOL
        if (ch == '\n')
        {
            _firstChOnLine = true;
            if (_collectLineForLog)
            {
                if (_msgToLog.length() > 0)
                {
                    // Remove linefeeds
                    _msgToLog.replace("\n","");
                    _msgToLog.replace("\r","");
                    // Form str
                    if (_logToMQTT)
                    {
                        String logStr = "{\"logLevel\":" + String(_curMsgLogLevel) + ",\"logMsg\":\"" + String(_msgToLog.c_str()) + "\"}";
                        logStr.replace("\n","");
                        _mqttManager.reportSilent(logStr.c_str());
                    }
                    if (_logToHTTP)
                    {
                        // Abandon any existing connection
                        if (_wifiClient.connected())
                        {
                            _wifiClient.stop();
                            // Serial.println("NetLog: Stopped existing TCP conn");
                        }

                        // Connect
                        // Serial.printf("NetLog: TCP conn to %s:%d\n", _httpIpAddr.c_str(), _httpPort);
                        bool connOk = _wifiClient.connect(_httpIpAddr.c_str(), _httpPort);
                        // Serial.printf("NetLog: TCP connect rslt %s\n", connOk ? "OK" : "FAIL");
                        if (connOk)
                        {
                            String logStr = "[{\"logCat\":" + String(_curMsgLogLevel) + ",\"eventText\":\"" + _msgToLog + "\"}]\r\n";
                            static const char* headers = "Content-Type: application/json\r\nAccept: application/json\r\n"
                                        "Host: NetLogger\r\nConnection: close\r\n\r\n";
                            String reqStr = "POST /" + _httpLogUrl + "/" + _systemName + "/ HTTP/1.1\r\nContent-Length:" + String(logStr.length()) + "\r\n";
                            _wifiClient.print(reqStr + headers + logStr);
                        }
                        else
                        {
                            if (_logToSerial && _serialPort == 0)
                                Serial.printf("NetLog: Couldn't connect to %s:%d\n", _httpIpAddr.c_str(), _httpPort);
                        }
                    }
                }
                _msgToLog = "";
            }
            _collectLineForLog = false;
        }
        return retVal;
    }

    void service()
    {
        // Handle connected - pump any data
        if (_wifiClient.connected())
        {
            // Check for data available
            int numAvail = _wifiClient.available();
            int numToRead = numAvail;
            if (numAvail > MAX_RX_BUFFER_SIZE)
            {
                numToRead = MAX_RX_BUFFER_SIZE;
            }
            if (numToRead > 0)
            {
                uint8_t rxBuf[MAX_RX_BUFFER_SIZE];
                int numRead = _wifiClient.read(rxBuf, numToRead);
                Log.verbose("OTAUpdate: wifiClient reading %d available %d read %d\n", numToRead, numAvail, numRead);
            }
        }
    }
};
