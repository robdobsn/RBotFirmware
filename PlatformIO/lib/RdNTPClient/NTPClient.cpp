// NTPClient - handle NTP server setting
// Rob Dobson 2018

#include <Arduino.h>
#include <ArduinoLog.h>
#include <NTPClient.h>
#include <WiFi.h>
#include "time.h"
#include "Utils.h"
#include "RdJson.h"
#include <ArduinoLog.h>
#include "ConfigBase.h"

static const char* MODULE_PREFIX = "NTPClient: ";

NTPClient::NTPClient()
{
    _pDefaultConfig = NULL;
    _pConfig = NULL;
    _ntpSetup = false;
    _lastNTPCheckTime = 0;
    _gmtOffsetSecs = 0;
    _dstOffsetSecs = 0;
    _betweenNTPChecksSecs = BETWEEN_NTP_CHECKS_SECS_DEFAULT;
}

void NTPClient::setup(ConfigBase* pDefaultConfig, const char* configName, ConfigBase* pConfig)
{
    // Save config and register callback on config changed
    _pDefaultConfig = pDefaultConfig;
    _configName = configName;
    _pConfig = pConfig;
    if (_pConfig)
        _pConfig->registerChangeCallback(std::bind(&NTPClient::configChanged, this));
    applySetup();
}

void NTPClient::service()
{
    if (Utils::isTimeout(millis(), _lastNTPCheckTime, _ntpSetup ? _betweenNTPChecksSecs*1000 : 1000))
    {
        _lastNTPCheckTime = millis();
        if (!_ntpSetup && _ntpServer1.length() > 0)
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                configTime(_gmtOffsetSecs, _dstOffsetSecs, _ntpServer1.c_str(), _ntpServer2.c_str(), _ntpServer3.c_str());
                _ntpSetup = true;
                Log.notice("%sservice - configTime set\n", MODULE_PREFIX);
            }
        }
    }
}

void NTPClient::configChanged()
{
    // Reset config
    Log.trace("%sconfigChanged\n", MODULE_PREFIX);
    applySetup();
}

void NTPClient::applySetup()
{
    if (!_pConfig)
        return;
    ConfigBase defaultConfig;
    if (_pDefaultConfig)
        defaultConfig.setConfigData(_pDefaultConfig->getString(_configName.c_str(), "").c_str());
    Log.trace("%ssetup name %s configStr %s\n", MODULE_PREFIX, _configName.c_str(),
                    _pConfig->getConfigCStrPtr());
    // Extract server, etc
    _ntpServer1 = _pConfig->getString("ntpServer", defaultConfig.getString("ntpServer", "").c_str());
    _ntpServer2 = _pConfig->getString("ntpServer2", defaultConfig.getString("ntpServer2", "").c_str());
    _ntpServer3 = _pConfig->getString("ntpServer3", defaultConfig.getString("ntpServer3", "").c_str());
    _gmtOffsetSecs = _pConfig->getLong("gmtOffsetSecs", defaultConfig.getLong("gmtOffsetSecs", 0));
    _dstOffsetSecs = _pConfig->getLong("dstOffsetSecs", defaultConfig.getLong("dstOffsetSecs", 0));
    _lastNTPCheckTime = 0;
    _ntpSetup = false;
    Log.notice("%ssetup servers %s, %s, %s, GMT Offset %d, DST Offset %d\n", MODULE_PREFIX, 
                _ntpServer1.c_str(), _ntpServer2.c_str(), _ntpServer3.c_str(),
                _gmtOffsetSecs, _dstOffsetSecs);
}

void NTPClient::setConfig(int gmtOffsetSecs, int dstOffsetSecs, 
            const char* server1, const char* server2, const char* server3)
{
    String jsonStr;
    jsonStr += "{";
    jsonStr += "\"gmtOffsetSecs\":";
    jsonStr += String(gmtOffsetSecs);
    jsonStr += ",";
    jsonStr += "\"dstOffsetSecs\":";
    jsonStr += String(dstOffsetSecs);
    jsonStr += ",";
    jsonStr += "\"ntpServer\":\"";
    jsonStr += server1;
    jsonStr += "\",";
    jsonStr += "\"ntpServer2\":\"";
    jsonStr += server2;
    jsonStr += "\",";
    jsonStr += "\"ntpServer3\":\"";
    jsonStr += server3;
    jsonStr += "\"";
    jsonStr += "}";
    if (_pConfig)
    {
        _pConfig->setConfigData(jsonStr.c_str());
        _pConfig->writeConfig();
        Log.trace("%setConfig %s\n", MODULE_PREFIX, _pConfig->getConfigCStrPtr());
    }
    applySetup();
}

void NTPClient::getConfig(String& config)
{
    config = "{}";
    if (_pConfig)
        config = _pConfig->getConfigString();
}
