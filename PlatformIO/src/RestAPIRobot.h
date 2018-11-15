// REST API for RBotFirmware
// Rob Dobson 2018

#pragma once
#include <Arduino.h>
#include "RestAPIEndpoints.h"
#include "WorkManager/WorkManager.h"
#include "RobotConfigurations.h"
#include "FileManager.h"

class RestAPIRobot
{
  private:
    WorkManager &_workManager;
    FileManager& _fileManager;

  public:
    RestAPIRobot(WorkManager &commandInterface, FileManager& fileManager) :
                _workManager(commandInterface), _fileManager(fileManager)
    {
    }
 
    void apiQueryStatus(String &reqStr, String &respStr);
    void apiGetRobotTypes(String &reqStr, String &respStr);
    void apiRobotConfiguration(String &reqStr, String &respStr);
    void apiGetSettings(String &reqStr, String &respStr);
    void apiPostSettings(String &reqStr, String &respStr);
    void apiSetLed(String &reqStr, String &respStr);
    void apiPostSettingsBody(String& reqStr, uint8_t *pData, size_t len, size_t index, size_t total);
    void apiSetLedBody(String& reqStr, uint8_t *pData, size_t len, size_t index, size_t total);
    void apiExec(String &reqStr, String &respStr);
    void apiPattern(String &reqStr, String &respStr);
    void apiSequence(String &reqStr, String &respStr);
    void apiPlayFile(String &reqStr, String &respStr);
    void setup(RestAPIEndpoints &endpoints);
};
