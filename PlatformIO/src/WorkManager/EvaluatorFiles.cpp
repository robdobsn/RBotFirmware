// RBotFirmware
// Rob Dobson 2017

#include <Arduino.h>
#include <ArduinoLog.h>
#include "EvaluatorFiles.h"
#include "RdJson.h"
#include "WorkManager.h"

static const char* MODULE_PREFIX = "EvaluatorFiles: ";

void EvaluatorFiles::setConfig(const char* configStr)
{
}

const char* EvaluatorFiles::getConfig()
{
    return "";
}

// Process WorkItem
bool EvaluatorFiles::execWorkItem(WorkItem& workItem)
{
    // Form the file name
    String fName = workItem.getString();

    // Start chunked file access
    bool retc = _fileManager.chunkedFileStart("SPIFFS", fName.c_str(), true);
    if (!retc)
        return false;
    Log.trace("%sstarted chunked file %s\n", MODULE_PREFIX, fName.c_str());
    _inProgress = true;
    return retc;
}

void EvaluatorFiles::service(WorkManager* pWorkManager)
{
    // Check in progress
    if (!_inProgress)
        return;

    // See if we can add to the queue
    if (!pWorkManager->canAcceptWorkItem())
        return;

    // Get next line from file
    String filename = "";
    int fileLen = 0;
    int chunkPos = 0;
    int chunkLen = 0;
    bool finalChunk = false;
    uint8_t* pLine = _fileManager.chunkFileNext(filename, fileLen, chunkPos, chunkLen, finalChunk);

    // Check if valid
    if (chunkLen > 0)
    {
        // Process the line
        String newLine = (char*)pLine;
        newLine.replace("\n", "");
        newLine.replace("\r", "");
        Log.verbose("%sservice new line %s\n", MODULE_PREFIX, newLine.c_str());
        String retStr;
        WorkItem workItem(newLine.c_str());
        pWorkManager->addWorkItem(workItem, retStr);
    }

    // Check for finished
    if (finalChunk)
    {
        // Process the line
        Log.trace("%sservice file finished\n", MODULE_PREFIX);
        _inProgress = false;
    }

}

void EvaluatorFiles::stop()
{
    _inProgress = "";
}
