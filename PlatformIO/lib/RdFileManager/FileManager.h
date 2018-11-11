// FileManager 
// Rob Dobson 2018

#pragma once

#include <Arduino.h>
#include "ConfigBase.h"

class FileManager
{
private:
    bool _enableSPIFFS;
    bool _defaultToSPIFFS;

    // Chunked file access
    static const int CHUNKED_BUF_MAXLEN = 1000;
    uint8_t _chunkedFileBuffer[CHUNKED_BUF_MAXLEN];
    int _chunkedFileInProgress;
    int _chunkedFilePos;
    String _chunkedFilename;
    int _chunkedFileLen;
    bool _chunkOnLineEndings;

public:
    FileManager()
    {
        _enableSPIFFS = false;
        _defaultToSPIFFS = true;
        _chunkedFileLen = 0;
        _chunkedFilePos = 0;
        _chunkedFileInProgress = false;
    }

    // Configure
    void setup(ConfigBase& config);

    // Reformat
    void reformat(const String& fileSystemStr, String& respStr);

    // Get a list of files on the file system as a JSON format string
    // {"rslt":"ok","diskSize":123456,"diskUsed":1234,"folder":"/","files":[{"name":"file1.txt","size":223},{"name":"file2.txt","size":234}]}
    bool getFilesJSON(const String& fileSystemStr, const String& folderStr, String& respStr);

    // Get/Set file contents as a string
    String getFileContents(const char* fileSystem, const String& filename, int maxLen);
    bool setFileContents(const char* fileSystem, const String& filename, String& fileContents);

    // Handle a file upload block - same API as ESPAsyncWebServer file handler
    void uploadAPIBlockHandler(const char* fileSystem, const String& req, const String& filename, int fileLength, size_t index, uint8_t *data, size_t len, bool finalBlock);

    // Delete file on file system
    bool deleteFile(const String& fileSystemStr, const String& filename);
    
    // Start access to a file in chunks
    bool chunkedFileStart(const String& fileSystemStr, const String& filename, bool readByLine);

    // Get next chunk of file
    uint8_t* chunkFileNext(String& filename, int& fileLen, int& chunkPos, int& chunkLen, bool& finalChunk);
};
