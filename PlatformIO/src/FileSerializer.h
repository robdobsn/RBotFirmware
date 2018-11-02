// FileSerializer
// Rob Dobson 2016

#pragma once

#include "FileManager.h"
#include "CommandInterface.h"

class FileSerializer
{
private:
    FileManager& _fileManager;

public:
    FileSerializer(FileManager& fileManager) : 
            _fileManager(fileManager)
    {

    }

    void service(CommandInterface* pCommandInterface)
    {
        // Read a line from the file

    }

    bool procFile(const char* filename)
    {
        // Check if file exists in file system
        
    }

};
