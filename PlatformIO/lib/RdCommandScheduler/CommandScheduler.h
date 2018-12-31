// CommandScheduler 
// Rob Dobson 2018

#pragma once
#include <Arduino.h>
#include "Utils.h"
#include "ConfigBase.h"

class RestAPIEndpoints;

class CommandSchedulerJob
{
public:
    int _hour;
    int _min;
    bool _doneInLast24Hours;
    String _command;

public:
    CommandSchedulerJob()
    {
        _hour = 0;
        _min = 0;
        _doneInLast24Hours = false;
    }
    CommandSchedulerJob(const CommandSchedulerJob& other)
    {
        _hour = other._hour;
        _min = other._min;
        _doneInLast24Hours = other._doneInLast24Hours;
        _command = other._command;
    }
    CommandSchedulerJob(int hour, int min, const char* command)
    {
        _hour = hour;
        _min = min;
        _command = command;
        _doneInLast24Hours = false;
    }
};

class CommandScheduler
{
private:
    // List of jobs
    std::vector<CommandSchedulerJob> _jobs;

    // Time checking
    uint32_t _lastCheckForJobsMs;
    static const int TIME_BETWEEN_CHECKS_MS = 1000;

    // Config
    ConfigBase* _pDefaultConfig;
    ConfigBase* _pConfig;
    String _configName;

    // Endpoints
    RestAPIEndpoints* _pEndpoints;

public:
    CommandScheduler();
    void setup(ConfigBase* pDefaultConfig, const char* configName, ConfigBase* pConfig,
                RestAPIEndpoints &endpoints);

    // Service 
    void service();

    // Get and update config
    void getConfig(String& config);
    void setConfig(const char* configJson);
    void setConfig(uint8_t* pData, int len);

private:
    void configChanged();
    void applySetup();
};
