// NetLog
// Used to log data to various places (MQTT, commandSerial, HTTP) from ArduinoLog module
// Rob Dobson 2018

#include "NetLog.h"

void NetLog::setLogLevel(const char* logLevelStr)
{
    // Get level
    int logLevel = LOG_LEVEL_SILENT;
    switch(toupper(*logLevelStr))
    {
        case 'F': case 1: logLevel = LOG_LEVEL_FATAL; break;
        case 'E': case 2: logLevel = LOG_LEVEL_ERROR; break;
        case 'W': case 3: logLevel = LOG_LEVEL_WARNING; break;
        case 'N': case 4: logLevel = LOG_LEVEL_NOTICE; break;
        case 'T': case 5: logLevel = LOG_LEVEL_TRACE; break;
        case 'V': case 6: logLevel = LOG_LEVEL_VERBOSE; break;
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
        if (_logToSerial && _serialPort == 0)
            Serial.printf("NetLog: LogLevel set to %d\n", _loggingThreshold);
    }
    else
    {
        if (_logToSerial && _serialPort == 0)
            Serial.printf("NetLog: LogLevel unchanged at %d\n", _loggingThreshold);
    }
}

void NetLog::setMQTT(bool mqttFlag, const char* mqttLogTopic)
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

void NetLog::setSerial(bool onOffFlag, const char* serialPortStr)
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

void NetLog::setCmdSerial(bool onOffFlag)
{
    // Set values
    bool dataChanged = (_logToCommandSerial != onOffFlag);
    _logToCommandSerial = onOffFlag;
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

void NetLog::setHTTP(bool httpFlag, const char* ipAddr, const char* portStr, const char* httpLogUrl)
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

void NetLog::setPapertrail(bool papertrailFlag, const char* hostStr, const char* portStr)
{
    // Set values
    String hostValidated = hostStr;
    if (hostValidated.length() == 0)
        hostValidated = _papertrailHost;
    int portValidated = String(portStr).toInt();
    if (strlen(portStr) == 0)
        portValidated = _papertrailPort;
    bool dataChanged = ((_logToPapertrail != papertrailFlag) || (_papertrailHost != hostValidated) || (_papertrailPort != portValidated));
    _logToPapertrail = papertrailFlag;
    _papertrailHost = hostValidated;
    _papertrailPort = portValidated;
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

void NetLog::getConfig(String& configStr)
{
    configStr = formConfigStr();
}

void NetLog::setup(ConfigBase *pConfig, const char* systemName)
{
    _systemName = systemName;
    _pConfigBase = pConfig;
    if (!pConfig)
        return;
    if (_logToSerial && _serialPort == 0)
        Serial.printf("NetLog: Setup from %s\n", pConfig->getConfigCStrPtr());
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
    _logToSerial = pConfig->getLong("SerialFlag", 1) != 0;
    _serialPort = pConfig->getLong("SerialPort", 0);
    // Get CommandSerial settings
    _logToCommandSerial = pConfig->getLong("CmdSerial", 0) != 0;
    // Get Papertrail settings
    _logToPapertrail = pConfig->getLong("PapertrailFlag", 0) != 0;
    _papertrailHost = pConfig->getString("PapertrailHost", "");
    _papertrailPort = pConfig->getLong("PapertrailPort", 38092);

    // Debug
    if (_logToSerial && _serialPort == 0)
        Serial.printf("NetLog: logLevel %d, mqttFlag %d topic %s, httpFlag %d, ip %s, port %d, url %s, serialFlag %d, serialPort %d, cmdSerial %d, papertrailFlag %d, host %s, port %d\n",
                _loggingThreshold, _logToMQTT, _mqttLogTopic.c_str(),
                _logToHTTP, _httpIpAddr.c_str(), _httpPort, _httpLogUrl.c_str(),
                _logToSerial, _serialPort, _logToCommandSerial,
                _logToPapertrail, _papertrailHost.c_str(), _papertrailPort);
}

String NetLog::formConfigStr()
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
                    "\",\"CmdSerial\":\"" + String(_logToCommandSerial) + 
                    "\",\"PapertrailFlag\":\"" + String(_logToPapertrail) + 
                    "\",\"PapertrailHost\":\"" + _papertrailHost + 
                    "\",\"PapertrailPort\":\"" + String(_papertrailPort) + 
                    "\"}";
}

void NetLog::pause()
{
    _isPaused = true;
    _pauseStartedMs = millis();
}

void NetLog::resume()
{
    if (_isPaused)
    {
        _isPaused = false;
        handleLoggedDuringPause();
    }
}

size_t NetLog::write(uint8_t ch)
{
    int retVal = 0;
    // Check if paused
    if (_isPaused)
    {
        // Check if we can put into the circular buffer
        if ((_pChBuffer != NULL) && _chBufferPosn.canPut())
        {
            _pChBuffer[_chBufferPosn.posToPut()] = ch;
            _chBufferPosn.hasPut();
        }
        // Serial.printf("<LEN%d>",_chBufferPosn.count());
        return retVal;
    }

    // Check for log to serial
    if (_logToSerial)
    {
        if (_serialPort == 0)
            retVal = Serial.write(ch);
    }

    // Check for log to MQTT or HTTP
    if (!(_logToMQTT || _logToHTTP || _logToCommandSerial || _logToPapertrail))
        return retVal;
    
    // Check for first char on line
    if (_firstChOnLine)
    {
        _firstChOnLine = false;
        // Get msg level from first char in message
        int msgLevel = LOG_LEVEL_SILENT;
        switch(ch)
        {
            case 'F': case 1: msgLevel = LOG_LEVEL_FATAL; break;
            case 'E': case 2: msgLevel = LOG_LEVEL_ERROR; break;
            case 'W': case 3: msgLevel = LOG_LEVEL_WARNING; break;
            case 'N': case 4: msgLevel = LOG_LEVEL_NOTICE; break;
            case 'T': case 5: msgLevel = LOG_LEVEL_TRACE; break;
            case 'V': case 6: msgLevel = LOG_LEVEL_VERBOSE; break;
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
                if (_logToPapertrail) {
                    String host = _papertrailHost;
                    host.trim();
                    if (host.length() != 0)
                    {
                        String logStr = "<22>" + _systemName + ": " + String(_msgToLog.c_str());
                        if (WiFi.isConnected())
                        {
                            int udpBeginPacketRslt = Udp.beginPacket(host.c_str(), _papertrailPort);
                        Udp.write((const uint8_t *) logStr.c_str(), logStr.length());
                            int udpRslt = Udp.endPacket();
                            Serial.printf("PAPERTRAIL %s %s %s %d %s\n",
                                        udpBeginPacketRslt ? "BEGINOK" : "BEGINFAIL",
                                        udpRslt ? "ENDOK" : "ENDFAIL", 
                                        host.c_str(), _papertrailPort, logStr.c_str());
                        }
                    }
                }
                if (_logToMQTT || _logToCommandSerial)
                {
                    String logStr = "{\"logLevel\":" + String(_curMsgLogLevel) + ",\"logMsg\":\"" + String(_msgToLog.c_str()) + "\"}";
                    logStr.replace("\n","");
                    if (_logToMQTT)
                        _mqttManager.reportSilent(logStr.c_str());
                    if (_logToCommandSerial)
                        _commandSerial.logMessage(logStr);
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

void NetLog::service(char xonXoffChar)
{
    // Handle WiFi connected - pump any data
    if (_wifiClient.connected())
    {
        // Check for data available on TCP socket
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
            Log.verbose("NetLog: wifiClient reading %d available %d read %d\n", numToRead, numAvail, numRead);
            // Currently just discard received data on the TCP socket
        }
    }

    // Check for busy indicator
    if (xonXoffChar == ASCII_XOFF)
    {
        // Serial.printf("<Logging paused>");
        pause();
    }
    else if (xonXoffChar == ASCII_XON)
    {
        // Serial.printf("<Logging resumed>");
        resume();
    }

    // Check for pause timeout
    if (_isPaused && Utils::isTimeout(millis(), _pauseStartedMs, _pauseTimeMs))
    {
        _isPaused = false;
        handleLoggedDuringPause();
    }
}

void NetLog::handleLoggedDuringPause()
{
    // Empty the circular buffer
    while ((_pChBuffer != NULL) && _chBufferPosn.canGet())
    {
        write(_pChBuffer[_chBufferPosn.posToGet()]);
        _chBufferPosn.hasGot();
    }
}
