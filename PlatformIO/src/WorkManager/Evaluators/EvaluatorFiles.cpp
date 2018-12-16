// RBotFirmware
// Rob Dobson 2017

#include <Arduino.h>
#include <ArduinoLog.h>
#include "EvaluatorFiles.h"
#include "RdJson.h"
#include "../WorkManager.h"

static const char* MODULE_PREFIX = "EvaluatorFiles: ";

EvaluatorFiles::EvaluatorFiles(FileManager& fileManager) :
         _fileManager(fileManager)
{
    _inProgress = false;
    _fileType = FILE_TYPE_UNKNOWN;
}

void EvaluatorFiles::setConfig(const char* configStr)
{
}

const char* EvaluatorFiles::getConfig()
{
    return "";
}

// Is Busy
bool EvaluatorFiles::isBusy()
{
    return _inProgress;
}

int EvaluatorFiles::getFileTypeFromExtension(String& fileName)
{
    String fileExt = FileManager::getFileExtension(fileName);
    int fileType = FILE_TYPE_UNKNOWN;
    if (fileExt.equalsIgnoreCase("gcode"))
        fileType = FILE_TYPE_GCODE;
    if (fileExt.equalsIgnoreCase("thr"))
        fileType = FILE_TYPE_THETA_RHO;
    return fileType;
}

// Check if valid
bool EvaluatorFiles::isValid(WorkItem& workItem)
{
    // Form the file name
    String fileName = workItem.getString();
    // Check for supported extension
    int fileType = getFileTypeFromExtension(fileName);
    if (fileType == FILE_TYPE_UNKNOWN)
        return false;
    // Check on file system
    int fileLen = 0;
    bool rslt = _fileManager.getFileInfo("", fileName, fileLen);
    if (fileLen == 0)
        return false;
    return rslt;
}

// Process WorkItem
bool EvaluatorFiles::execWorkItem(WorkItem& workItem)
{
    // Form the file name
    String fileName = workItem.getString();
    int fileType = getFileTypeFromExtension(fileName);
    if (fileType == FILE_TYPE_UNKNOWN)
        return false;
    _fileType = fileType;

    // Start chunked file access
    bool retc = _fileManager.chunkedFileStart("", fileName, true);
    if (!retc)
        return false;
    Log.trace("%sstarted chunked file %s type is %s\n", MODULE_PREFIX, 
            fileName.c_str(), (_fileType == FILE_TYPE_GCODE ? "GCODE" : "THR"));
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
        newLine.trim();
        bool isComment = false;
        if (_fileType == FILE_TYPE_THETA_RHO)
            isComment = newLine.startsWith("#");
        else if (_fileType == FILE_TYPE_GCODE)
            isComment = newLine.startsWith(";");
        if (!isComment)
        {
            bool isValid = true;
            // Form GCode if Theta-Rho
            if (_fileType == FILE_TYPE_THETA_RHO)
            {
                int spacePos = newLine.indexOf(" ");
                if (spacePos > 0)
                {
                    newLine = (chunkPos == 0 ? "_THRLINE0_/" : "_THRLINEN_/") + newLine.substring(0,spacePos) + "/" + newLine.substring(spacePos+1);
                }
                else
                {
                    isValid = false;
                }
            }
            // Form the work item if valid
            if (isValid)
            {
                Log.verbose("%sservice new line %s\n", MODULE_PREFIX, newLine.c_str());
                String retStr;
                WorkItem workItem(newLine.c_str());
                pWorkManager->addWorkItem(workItem, retStr);
            }
        }
    }

    // Check for finished
    if (finalChunk)
    {
        // Process the line
        Log.verbose("%sservice file finished\n", MODULE_PREFIX);
        _inProgress = false;
    }

}

void EvaluatorFiles::stop()
{
    _inProgress = false;
}
