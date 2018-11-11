// Config using file system
// Rob Dobson 2016-2018

#pragma once

#include "ConfigBase.h"
#include "FileManager.h"

class ConfigFile : public ConfigBase
{
private:

    // File manager
    FileManager& _fileManager;

    // Config filesystem
    String _fileSystem;

    // Config filename
    String _filename;

public:
    ConfigFile(FileManager& fileManager, const char *fileSystem, const char* filename, int configMaxlen) :
            _fileManager(fileManager)
    {
        _fileSystem = fileSystem;
        _filename = filename;
        _configMaxDataLen = configMaxlen;
    }

    ~ConfigFile()
    {
    }

    // Get max length
    int getMaxLen();

    // Clear
    void clear();

    // Initialise
    bool setup();

    // Write configuration string
    bool writeConfig();
};
