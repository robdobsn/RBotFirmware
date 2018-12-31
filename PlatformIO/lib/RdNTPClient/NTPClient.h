// NTPClient - handle NTP server setting
// Rob Dobson 2018

#pragma once

class ConfigBase;

class NTPClient
{
private:
    ConfigBase* _pDefaultConfig;
    ConfigBase* _pConfig;
    String _configName;
    bool _ntpSetup;
    String _ntpServer1;
    String _ntpServer2;
    String _ntpServer3;
    int _gmtOffsetSecs;
    int _dstOffsetSecs;
    int _betweenNTPChecksSecs;
    static const uint32_t BETWEEN_NTP_CHECKS_SECS_DEFAULT = 3600;
    uint32_t _lastNTPCheckTime;
    
public:
    NTPClient();
    void setup(ConfigBase* pDefaultConfig, const char* configName, ConfigBase* pConfig);
    void service();
    void getConfig(String& config);
    void setConfig(int gmtOffsetSecs, int dstOffsetSecs, 
            const char* server1, const char* server2, const char* server3);

private:
    void configChanged();
    void applySetup();
};
