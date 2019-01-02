// CommandScheduler
// Rob Dobson 2018

#include "CommandScheduler.h"
#include "RestAPIEndpoints.h"

// #define DEBUG_DISPLAY_TIME_CHECK 1

static const char* MODULE_PREFIX = "CommandScheduler: ";

CommandScheduler::CommandScheduler()
{
    _pDefaultConfig = NULL;
    _pConfig = NULL;    
    _lastCheckForJobsMs = 0;
    _pEndpoints = NULL;
}

void CommandScheduler::setup(ConfigBase* pDefaultConfig, const char* configName, 
            ConfigBase* pConfig, RestAPIEndpoints &endpoints)
{
    // Save config info
    _pEndpoints = &endpoints;
    _pDefaultConfig = pDefaultConfig;
    _configName = configName;
    _pConfig = pConfig;
    if (_pConfig)
        _pDefaultConfig->registerChangeCallback(std::bind(&CommandScheduler::configChanged, this));
    if (_pConfig)
        _pConfig->registerChangeCallback(std::bind(&CommandScheduler::configChanged, this));

    // Apply new setup
    applySetup();

}

// Service 
void CommandScheduler::service()
{
    // Check for a command to schedule
    if (Utils::isTimeout(millis(), _lastCheckForJobsMs, TIME_BETWEEN_CHECKS_MS))
    {
        _lastCheckForJobsMs = millis();
        // Get time
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo, 0))
        {
            Log.verbose("%sservice failed to get time\n", MODULE_PREFIX);
            return;
        }

        // Check each job
        for (int i = 0; i < _jobs.size(); i++)
        {
            if (_jobs[i]._doneInLast24Hours)
                continue;

#ifdef DEBUG_DISPLAY_TIME_CHECK
        Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
#endif

            if ((_jobs[i]._hour == timeinfo.tm_hour) && (_jobs[i]._min == timeinfo.tm_min))
            {
                Log.trace("%sservice performing job at hour %d min %d cmd %s\n", MODULE_PREFIX,
                    _jobs[i]._hour, _jobs[i]._min, _jobs[i]._command.c_str());
                _jobs[i]._doneInLast24Hours = true;
                if (_pEndpoints)
                {
                    String retStr;
                    _pEndpoints->handleApiRequest(_jobs[i]._command.c_str(), retStr);
                }
            }
        }
    }
}

void CommandScheduler::configChanged()
{
    // Reset config
    Log.trace("%sconfigChanged\n", MODULE_PREFIX);
    applySetup();
}

void CommandScheduler::applySetup()
{
    if ((!_pConfig) || (!_pDefaultConfig))
        return;

    // Get default config
    ConfigBase defConfig(_pDefaultConfig->getString(_configName.c_str(), "").c_str());

    // Get config
    Log.notice("%sconfig(s) %s, %s\n", MODULE_PREFIX, 
                defConfig.getConfigCStrPtr(), _pConfig->getConfigCStrPtr());

    // Clear schedule
    _jobs.clear();

    // Handle each config list in turn
    for (int i = 0; i < 2; i++)
    {
        // Get config
        ConfigBase* pConfig = &defConfig;
        if (i == 1)
            pConfig = _pConfig;

        // Extract jobs list
        String jobsListJson = pConfig->getString("jobs", "[]");
        int numJobs = 0;
        if (RdJson::getType(numJobs, jobsListJson.c_str()) != JSMNR_ARRAY)
            return;
        
        // Iterate array
        for (int i = 0; i < numJobs; i++)
        {
            // Extract job details
            String jobJson = RdJson::getString(("["+String(i)+"]").c_str(), "{}", jobsListJson.c_str());
            int hour = RdJson::getLong("hour",0,jobJson.c_str());
            int min = RdJson::getLong("minute",0,jobJson.c_str());
            String cmd = RdJson::getString("cmd","",jobJson.c_str());
            CommandSchedulerJob newjob(hour, min, cmd.c_str());

            // Add to list
            _jobs.push_back(newjob);
            Log.notice("%sapplySetup hour:%d min:%d cmd:%s\n", MODULE_PREFIX, 
                        hour, min, cmd.c_str());
        }
    }
}

void CommandScheduler::getConfig(String& config)
{
    config = "{}";
    if (_pConfig)
        config = _pConfig->getConfigString();
}

void CommandScheduler::setConfig(const char* configJson)
{
    // Save config
    if (_pConfig)
    {
        _pConfig->setConfigData(configJson);
        _pConfig->writeConfig();
        Log.trace("%setConfig %s\n", MODULE_PREFIX, _pConfig->getConfigCStrPtr());
    }
    applySetup();
}

void CommandScheduler::setConfig(uint8_t* pData, int len)
{
    if (!_pConfig)
        return;
    if (len >= _pConfig->getMaxLen())
        return;
    char* pTmp = new char[len + 1];
    if (!pTmp)
        return;
    memcpy(pTmp, pData, len);
    pTmp[len] = 0;
    // Make sure string is terminated
    setConfig(pTmp);
    delete[] pTmp;    
}
