// CommandSerial 
// Rob Dobson 2018

#pragma once
#include "Arduino.h"
#include "FS.h"
#include "SPIFFS.h"

class FileManager
{
private:
    bool _enableSPIFFS;
    bool _defaultToSPIFFS;

public:
    FileManager()
    {
        _enableSPIFFS = false;
        _defaultToSPIFFS = true;
    }

    void setup(ConfigBase& config)
    {
        // Get config
        ConfigBase fsConfig(config.getString("fileManager", "").c_str());
        Log.notice("FileManager: config %s\n", fsConfig.getConfigData());

        // See if SPIFFS enabled
        _enableSPIFFS = fsConfig.getLong("spiffsEnabled", 0) != 0;
        bool spiffsFormatIfCorrupt = fsConfig.getLong("spiffsFormatIfCorrupt", 0) != 0;

        // Init if required
        if (_enableSPIFFS)
        {
            if (!SPIFFS.begin(spiffsFormatIfCorrupt))
                Log.warning("File system SPIFFS failed to initialise\n");
        }
    }
    
    bool getFilesJSON(String& fileSystemStr, String& folderStr, String& respStr)
    {
        // Check file system supported
        if ((fileSystemStr.length() > 0) && (fileSystemStr != "SPIFFS"))
        {
            respStr = "{\"rslt\":\"fail\",\"error\":\"unknownfs\",\"files\":[]}";
            return false;
        }

        // Only SPIFFS currently
        fs::FS fs = SPIFFS;
        size_t spiffsSize = SPIFFS.totalBytes();
        size_t spiffsUsed = SPIFFS.usedBytes();

        // Open folder
        File base = fs.open(folderStr.c_str());
        if (!base)
        {
            respStr = "{\"rslt\":\"fail\",\"error\":\"nofolder\",\"files\":[]}";
            return false;
        }
        if (!base.isDirectory())
        {
            respStr = "{\"rslt\":\"fail\",\"error\":\"notfolder\",\"files\":[]}";
            return false;
        }

        // Iterate over files
        File file = base.openNextFile();
        respStr = "{\"rslt\":\"ok\",\"diskSize\":" + String(spiffsSize) + ",\"diskUsed\":" + 
                    spiffsUsed + ",\"folder\":\"" + String(folderStr) + "\",\"files\":[";
        bool firstFile = true;
        while (file)
        {
            if (!file.isDirectory())
            {
                if (!firstFile)
                    respStr += ",";
                firstFile = false;
                respStr += "{\"name\":\"";
                respStr += file.name();
                respStr += "\",\"size\":";
                respStr += String(file.size());
                respStr += "}";
            }

            // Next
            file = base.openNextFile();
        }
        respStr += "]}";
        return true;
    }

    void fileUploadPart(String& filename, int fileLength, size_t index, uint8_t *data, size_t len, bool final)
    {
        Log.trace("FileManager: %s, total %d, idx %d, len %d, final %d\n", filename.c_str(), fileLength, index, len, final);
        if (!_defaultToSPIFFS)
            return;

        // Access type
        const char* accessType = FILE_WRITE;
        if (index > 0)
        {
            accessType = FILE_APPEND;
        }
        // Write file block
        File file = SPIFFS.open("/__tmp__", accessType);
        if (!file)
        {
            Log.trace("FileManager: failed to open to __tmp__ file\n");
            return;
        }
        if (!file.write(data, len))
        {
            Log.trace("FileManager: failed write to __tmp__ file\n");
        }

        // Rename if last block
        if (final)
        {
            // Remove in case filename already exists
            SPIFFS.remove("/" + filename);
            // Rename
            if (!SPIFFS.rename("/__tmp__", "/" + filename))
            {
                Log.trace("FileManager: failed rename __tmp__ to %s\n", filename.c_str());
            }
        }
    }

    bool deleteFile(String& fileSystemStr, String& filename)
    {
        // Check file system supported
        if ((fileSystemStr.length() > 0) && (fileSystemStr != "SPIFFS"))
        {
            return false;
        }
        if (!SPIFFS.remove("/" + filename))
        {
            Log.notice("FileManager: failed to remove file %s\n", filename.c_str());
            return false; 
        }               
        return true;
    }
};
