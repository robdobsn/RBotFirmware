// FileManager 
// Rob Dobson 2018

#include "RdJson.h"
#include "FileManager.h"
#include "Utils.h"
#include <FS.h>
#include <SPIFFS.h>

static const char* MODULE_PREFIX = "FileManager: ";

void FileManager::setup(ConfigBase& config)
{
    // Get config
    ConfigBase fsConfig(config.getString("fileManager", "").c_str());
    Log.notice("%sconfig %s\n", MODULE_PREFIX, fsConfig.getConfigCStrPtr());

    // See if SPIFFS enabled
    _enableSPIFFS = fsConfig.getLong("spiffsEnabled", 0) != 0;
    bool spiffsFormatIfCorrupt = fsConfig.getLong("spiffsFormatIfCorrupt", 0) != 0;

    // Init if required
    if (_enableSPIFFS)
    {
        if (!SPIFFS.begin(spiffsFormatIfCorrupt))
            Log.warning("%sSPIFFS failed to initialise\n", MODULE_PREFIX);
    }
}
    
void FileManager::reformat(const String& fileSystemStr, String& respStr)
{
    // Reformat
    bool rslt = SPIFFS.format();
    Utils::setJsonBoolResult(respStr, rslt);
    Log.warning("%sReformat SPIFFS result %d\n", MODULE_PREFIX, rslt);
}

bool FileManager::getFilesJSON(const String& fileSystemStr, const String& folderStr, String& respStr)
{
    // Check file system supported
    if ((fileSystemStr.length() > 0) && (fileSystemStr != "SPIFFS"))
    {
        respStr = "{\"rslt\":\"fail\",\"error\":\"unknownfs\",\"files\":[]}";
        return false;
    }

    // respStr = "{\"rslt\":\"ok\",\"diskSize\":" + String(123456) + ",\"diskUsed\":" + 
    //              String(234567) + ",\"folder\":\"" + String("hello") + "\",\"files\":[]}";;
    // return true;

    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);

    // Only SPIFFS currently
    fs::FS fs = SPIFFS;
    size_t spiffsSize = SPIFFS.totalBytes();
    size_t spiffsUsed = SPIFFS.usedBytes();

    // Open folder
    File base = fs.open(folderStr.c_str());
    if (!base)
    {
        xSemaphoreGive(_fileSysMutex);
        respStr = "{\"rslt\":\"fail\",\"error\":\"nofolder\",\"files\":[]}";
        return false;
    }
    if (!base.isDirectory())
    {
        xSemaphoreGive(_fileSysMutex);
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
    xSemaphoreGive(_fileSysMutex);
    return true;
}

String FileManager::getFileContents(const char* fileSystem, const String& filename, int maxLen)
{
    // Only SPIFFS supported
    if (strcmp(fileSystem, "SPIFFS") != 0)
        return "";

    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);

    // Check file exists
    String rootFilename = (filename.startsWith("/") ? filename : ("/" + filename));
    if (!SPIFFS.exists(rootFilename))
    {
        xSemaphoreGive(_fileSysMutex);
        return "";
    }

    // Open file
    File file = SPIFFS.open(filename, FILE_READ);
    if (!file)
    {
        xSemaphoreGive(_fileSysMutex);
        Log.trace("%sfailed to open file to read %s\n", MODULE_PREFIX, filename.c_str());
        return "";
    }

    // Buffer
    uint8_t* pBuf = new uint8_t[maxLen+1];
    if (!pBuf)
    {
        xSemaphoreGive(_fileSysMutex);
        Log.trace("%sfailed to allocate %d\n", MODULE_PREFIX, maxLen);
        return "";
    }

    // Read
    size_t bytesRead = file.read(pBuf, maxLen);
    file.close();
    xSemaphoreGive(_fileSysMutex);
    pBuf[bytesRead] = 0;
    String readData = (char*)pBuf;
    delete [] pBuf;
    return readData;
}

bool FileManager::setFileContents(const char* fileSystem, const String& filename, String& fileContents)
{
    // Only SPIFFS supported
    if (strcmp(fileSystem, "SPIFFS") != 0)
        return false;

    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);

    // Open file
    File file = SPIFFS.open(filename, FILE_WRITE);
    if (!file)
    {
        xSemaphoreGive(_fileSysMutex);        
        Log.trace("%sfailed to open file to write %s\n", MODULE_PREFIX, filename.c_str());
        return "";
    }

    // Write
    size_t bytesWritten = file.write((uint8_t*)(fileContents.c_str()), fileContents.length());
    file.close();
    xSemaphoreGive(_fileSysMutex);
    return bytesWritten == fileContents.length();
}

void FileManager::uploadAPIBlockHandler(const char* fileSystem, const String& req, const String& filename, int fileLength, size_t index, uint8_t *data, size_t len, bool finalBlock)
{
    Log.trace("%suploadAPIBlockHandler fileSys %s, filename %s, total %d, idx %d, len %d, final %d\n", MODULE_PREFIX, 
                fileSystem, filename.c_str(), fileLength, index, len, finalBlock);
    if (strcmp(fileSystem, "SPIFFS") != 0)
        return;

    // Access type
    const char* accessType = FILE_WRITE;
    if (index > 0)
    {
        accessType = FILE_APPEND;
    }

    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);

    // Write file block
    File file = SPIFFS.open("/__tmp__", accessType);
    if (!file)
    {
        xSemaphoreGive(_fileSysMutex);        
        Log.trace("%sfailed to open __tmp__ file\n", MODULE_PREFIX);
        return;
    }
    if (!file.write(data, len))
    {
        Log.trace("%sfailed write to __tmp__ file\n", MODULE_PREFIX);
    }

    // Rename if last block
    if (finalBlock)
    {
        // Remove in case filename already exists
        SPIFFS.remove("/" + filename);
        // Rename
        if (!SPIFFS.rename("/__tmp__", "/" + filename))
        {
            Log.trace("%sfailed rename __tmp__ to %s\n", MODULE_PREFIX, filename.c_str());
        }
    }
    xSemaphoreGive(_fileSysMutex);
}

bool FileManager::deleteFile(const String& fileSystemStr, const String& filename)
{
    // Check file system supported
    if ((fileSystemStr.length() > 0) && (fileSystemStr != "SPIFFS"))
    {
        return false;
    }
    
    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);

    // Remove file
    if (!SPIFFS.remove("/" + filename))
    {
        Log.notice("%sfailed to remove file %s\n", MODULE_PREFIX, filename.c_str());
        return false; 
    }            

    xSemaphoreGive(_fileSysMutex);   
    return true;
}

bool FileManager::chunkedFileStart(const String& fileSystemStr, const String& filename, bool readByLine)
{
    // Check file system supported
    if ((fileSystemStr.length() > 0) && (fileSystemStr != "SPIFFS"))
    {
        return false;
    }

    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);

    // Check file exists
    String rootFilename = (filename.startsWith("/") ? filename : ("/" + filename));
    if (!SPIFFS.exists(rootFilename))
    {
        xSemaphoreGive(_fileSysMutex);  
        return false;
    }

    // Check file valid
    File file = SPIFFS.open(rootFilename, FILE_READ);
    if (!file)
    {
        xSemaphoreGive(_fileSysMutex);  
        Log.trace("%schunked failed to open %s file\n", MODULE_PREFIX, rootFilename.c_str());
        return false;
    }
    _chunkedFileLen = file.size();
    file.close();
    xSemaphoreGive(_fileSysMutex);  
    
    // Setup access
    _chunkedFilename = rootFilename;
    _chunkedFileInProgress = true;
    _chunkedFilePos = 0;
    _chunkOnLineEndings = readByLine;
    Log.trace("%schunkedFileStart filename %s size %d\n", MODULE_PREFIX, rootFilename.c_str(), _chunkedFileLen);
    return true;
}

uint8_t* FileManager::chunkFileNext(String& filename, int& fileLen, int& chunkPos, int& chunkLen, bool& finalChunk)
{
    // Check valid
    chunkLen = 0;
    if (!_chunkedFileInProgress)
        return NULL;

    // Return details
    filename = _chunkedFilename;
    fileLen = _chunkedFileLen;
    chunkPos = _chunkedFilePos;

    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);

    // Open file and seek
    File file = SPIFFS.open("/" + _chunkedFilename, FILE_READ);
    if (!file)
    {
        xSemaphoreGive(_fileSysMutex);  
        Log.trace("%schunkNext failed open %s\n", MODULE_PREFIX, _chunkedFilename.c_str());
        return NULL;
    }
    if ((_chunkedFilePos != 0) && (!file.seek(_chunkedFilePos)))
    {
        xSemaphoreGive(_fileSysMutex);  
        Log.trace("%schunkNext failed seek in filename %s to %d\n", MODULE_PREFIX, 
                        _chunkedFilename.c_str(), _chunkedFilePos);
        file.close();
        return NULL;
    }

    // Handle data type
    if (_chunkOnLineEndings)
    {
        // Read a line
        chunkLen = file.readBytesUntil('\n', _chunkedFileBuffer, CHUNKED_BUF_MAXLEN-1);
        if (chunkLen >= CHUNKED_BUF_MAXLEN)
            chunkLen = CHUNKED_BUF_MAXLEN-1;
        // Ensure line is terminated
        if (chunkLen >= 0)
        {
            _chunkedFileBuffer[chunkLen] = 0;
        }
        // Skip past the end of line character
        chunkLen++;
    }
    else
    {
        // Fill the buffer with file data
        chunkLen = file.read(_chunkedFileBuffer, CHUNKED_BUF_MAXLEN);
    }

    // Bump position and check if this was the final block
    _chunkedFilePos = _chunkedFilePos + chunkLen;
    if (_chunkedFileLen <= _chunkedFilePos)
    {
        finalChunk = true;
        _chunkedFileInProgress = false;
    }

    Log.trace("%schunkNext filename %s chunklen %d filePos %d fileLen %d inprog %d final %d curpos %d\n", MODULE_PREFIX, 
                    _chunkedFilename.c_str(), chunkLen, _chunkedFilePos, _chunkedFileLen, 
                    _chunkedFileInProgress, finalChunk, file.position());

    // Close
    file.close();
    xSemaphoreGive(_fileSysMutex);  
    return _chunkedFileBuffer;
}
