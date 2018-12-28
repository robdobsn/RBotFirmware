// FileManager 
// Rob Dobson 2018

#pragma once

#include <Arduino.h>
#include "ConfigBase.h"

class FileManager
{
private:
    // File system controls
    bool _enableSPIFFS;
    bool _spiffsIsOk;
    bool _enableSD;
    bool _defaultToSPIFFS;
    bool _sdIsOk;
    bool _cachedFileListValid;

    // SD card
    void* _pSDCard;

    // Chunked file access
    static const int CHUNKED_BUF_MAXLEN = 1000;
    uint8_t _chunkedFileBuffer[CHUNKED_BUF_MAXLEN];
    int _chunkedFileInProgress;
    int _chunkedFilePos;
    String _chunkedFilename;
    int _chunkedFileLen;
    bool _chunkOnLineEndings;

    // Cached file list response
    String _cachedFileListResponse;

    // Mutex controlling access to file system
    SemaphoreHandle_t _fileSysMutex;

public:
    FileManager()
    {
        _enableSPIFFS = false;
        _spiffsIsOk = false;
        _enableSD = false;
        _sdIsOk = false;
        _cachedFileListValid = false;
        _defaultToSPIFFS = true;
        _chunkedFileLen = 0;
        _chunkedFilePos = 0;
        _chunkedFileInProgress = false;
        _pSDCard = NULL;
        _fileSysMutex = xSemaphoreCreateMutex();
    }

    // Configure
    void setup(ConfigBase& config, const char* pConfigPath = NULL);

    // Reformat
    void reformat(const String& fileSystemStr, String& respStr);

    // Get a list of files on the file system as a JSON format string
    // {"rslt":"ok","diskSize":123456,"diskUsed":1234,"folder":"/","files":[{"name":"file1.txt","size":223},{"name":"file2.txt","size":234}]}
    bool getFilesJSON(const String& fileSystemStr, const String& folderStr, String& respStr);

    // Get/Set file contents as a string
    String getFileContents(const String& fileSystemStr, const String& filename, int maxLen=0);
    bool setFileContents(const String& fileSystemStr, const String& filename, String& fileContents);

    // Handle a file upload block - same API as ESPAsyncWebServer file handler
    void uploadAPIBlockHandler(const char* fileSystem, const String& req, const String& filename, int fileLength, size_t index, uint8_t *data, size_t len, bool finalBlock);
    void uploadAPIBlocksComplete();

    // Delete file on file system
    bool deleteFile(const String& fileSystemStr, const String& filename);
    
    // Test file exists and get info
    bool getFileInfo(const String& fileSystemStr, const String& filename, int& fileLength);

    // Start access to a file in chunks
    bool chunkedFileStart(const String& fileSystemStr, const String& filename, bool readByLine);

    // Get next chunk of file
    uint8_t* chunkFileNext(String& filename, int& fileLen, int& chunkPos, int& chunkLen, bool& finalChunk);

    // Get file name extension
    static String getFileExtension(String& filename);

    // Read line from file
    char* readLineFromFile(char* pBuf, int maxLen, FILE* pFile);

private:
    bool checkFileSystem(const String& fileSystemStr, String& fsName);
    String getFilePath(const String& nameOfFS, const String& filename);

};
