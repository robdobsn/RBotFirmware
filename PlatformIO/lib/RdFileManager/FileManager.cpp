// FileManager 
// Rob Dobson 2018

#include "RdJson.h"
#include "FileManager.h"
#include "Utils.h"
#include <FS.h>
#include <SPIFFS.h>

#include <sys/stat.h>
#include "vfs_api.h"

using namespace fs;

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
    // Check file system supported
    if ((fileSystemStr.length() > 0) && (fileSystemStr != "SPIFFS"))
    {
        respStr = "{\"rslt\":\"fail\",\"error\":\"unknownfs\",\"files\":[]}";
        return;
    }
    
    // Reformat
    bool rslt = SPIFFS.format();
    Utils::setJsonBoolResult(respStr, rslt);
    Log.warning("%sReformat SPIFFS result %d\n", MODULE_PREFIX, rslt);
}

bool FileManager::getFileInfo(const String& fileSystemStr, const String& filename, int& fileLength)
{
    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);

    // Check file exists
    struct stat st;
    String rootFilename = (filename.startsWith("/") ? "/spiffs" + filename : ("/spiffs/" + filename));
    if ((stat(rootFilename.c_str(), &st) != 0) || !S_ISREG(st.st_mode))
    {
        xSemaphoreGive(_fileSysMutex);
        return false;
    }
    xSemaphoreGive(_fileSysMutex);
    fileLength = st.st_size;
    return true;
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

String FileManager::getFileContents(const String& fileSystemStr, const String& filename, int maxLen)
{
    // Check file system supported
    if ((fileSystemStr.length() > 0) && (fileSystemStr != "SPIFFS"))
    {
        Log.trace("%sgetContents %s invalid file system %s\n", MODULE_PREFIX, filename.c_str(), fileSystemStr.c_str());
        return "";
    }

    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);

    // Get file info - to check length
    String rootFilename = (filename.startsWith("/") ? "/spiffs" + filename : ("/spiffs/" + filename));
    struct stat st;
    if (stat(rootFilename.c_str(), &st) != 0)
    {
        xSemaphoreGive(_fileSysMutex);
        Log.trace("%sgetContents %s cannot stat\n", MODULE_PREFIX, filename.c_str());
        return "";
    }
    if (!S_ISREG(st.st_mode))
    {
        xSemaphoreGive(_fileSysMutex);
        Log.trace("%sgetContents %s is a folder\n", MODULE_PREFIX, filename.c_str());
        return "";
    }

    // Check valid
    if (maxLen <= 0)
    {
        maxLen = ESP.getFreeHeap() / 3;
    }
    if (st.st_size >= maxLen-1)
    {
        xSemaphoreGive(_fileSysMutex);
        Log.trace("%sgetContents %s free heap %d size %d failed to read\n", MODULE_PREFIX, filename.c_str(), maxLen, st.st_size);
        return "";
    }
    int fileSize = st.st_size;

    // Open file
    FILE* pFile = fopen(rootFilename.c_str(), "r");
    if (!pFile)
    {
        xSemaphoreGive(_fileSysMutex);
        Log.trace("%sgetContents failed to open file to read %s\n", MODULE_PREFIX, filename.c_str());
        return "";
    }

    // Buffer
    uint8_t* pBuf = new uint8_t[fileSize+1];
    if (!pBuf)
    {
        fclose(pFile);
        xSemaphoreGive(_fileSysMutex);
        Log.trace("%sgetContents failed to allocate %d\n", MODULE_PREFIX, fileSize);
        return "";
    }

    // Read
    size_t bytesRead = fread((char*)pBuf, 1, fileSize, pFile);
    fclose(pFile);
    xSemaphoreGive(_fileSysMutex);
    pBuf[bytesRead] = 0;
    String readData = (char*)pBuf;
    delete [] pBuf;
    return readData;
}

bool FileManager::setFileContents(const String& fileSystemStr, const String& filename, String& fileContents)
{
    // Check file system supported
    if ((fileSystemStr.length() > 0) && (fileSystemStr != "SPIFFS"))
    {
        return false;
    }

    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);

    // Open file
    File file = SPIFFS.open(filename, FILE_WRITE);
    if (!file)
    {
        xSemaphoreGive(_fileSysMutex);        
        Log.trace("%sfailed to open file to write %s\n", MODULE_PREFIX, filename.c_str());
        return false;
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
    struct stat st;
    String rootFilename = (filename.startsWith("/") ? "/spiffs" + filename : ("/spiffs/" + filename));
    if ((stat(rootFilename.c_str(), &st) != 0) || !S_ISREG(st.st_mode))
    {
        Log.trace("%schunked file doesn't exist %s\n", MODULE_PREFIX, rootFilename.c_str());
        xSemaphoreGive(_fileSysMutex);
        return false;
    }
    _chunkedFileLen = st.st_size;
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
    FILE* pFile = NULL;
    if (_chunkOnLineEndings)
        pFile = fopen(_chunkedFilename.c_str(), "r");
    else
        pFile = fopen(_chunkedFilename.c_str(), "rb");
    if (!pFile)
    {
            xSemaphoreGive(_fileSysMutex);  
            Log.trace("%schunkNext failed open %s\n", MODULE_PREFIX, _chunkedFilename.c_str());
            return NULL;
    }    
    if ((_chunkedFilePos != 0) && (fseek(pFile, _chunkedFilePos, SEEK_SET) != 0))
    {
        xSemaphoreGive(_fileSysMutex);  
        Log.trace("%schunkNext failed seek in filename %s to %d\n", MODULE_PREFIX, 
                        _chunkedFilename.c_str(), _chunkedFilePos);
        fclose(pFile);
        return NULL;
    }

    // Handle data type
    if (_chunkOnLineEndings)
    {
        // Read a line
        char* pReadLine = fgets((char*)_chunkedFileBuffer, CHUNKED_BUF_MAXLEN-1, pFile);
        // Ensure line is terminated
        if (!pReadLine)
        {
            finalChunk = true;
            _chunkedFileInProgress = false;
            chunkLen = 0;
        }
        else
        {
            chunkLen = strlen((char*)_chunkedFileBuffer);
        }
        // Record position
        _chunkedFilePos = ftell(pFile);
    }
    else
    {
        // Fill the buffer with file data
        chunkLen = fread((char*)_chunkedFileBuffer, 1, CHUNKED_BUF_MAXLEN, pFile);

        // Record position and check if this was the final block
        _chunkedFilePos = ftell(pFile);
        if ((chunkLen != CHUNKED_BUF_MAXLEN) || (_chunkedFileLen <= _chunkedFilePos))
        {
            finalChunk = true;
            _chunkedFileInProgress = false;
        }

    }

    Log.trace("%schunkNext filename %s chunklen %d filePos %d fileLen %d inprog %d final %d\n", MODULE_PREFIX, 
                    _chunkedFilename.c_str(), chunkLen, _chunkedFilePos, _chunkedFileLen, 
                    _chunkedFileInProgress, finalChunk);

    // Close
    fclose(pFile);
    xSemaphoreGive(_fileSysMutex);  
    return _chunkedFileBuffer;
}

// Get file name extension
String FileManager::getFileExtension(String& fileName)
{
    String extn;
    // Find last .
    int dotPos = fileName.lastIndexOf('.');
    if (dotPos < 0)
        return extn;
    // Return substring
    return fileName.substring(dotPos+1);
}
