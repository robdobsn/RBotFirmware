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
    Log.notice("%sconfig %s\n", MODULE_PREFIX, fsConfig.getConfigData());

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

String FileManager::getFileContents(const char* fileSystem, const String& filename, int maxLen)
{
    // Only SPIFFS supported
    if (strcmp(fileSystem, "SPIFFS") != 0)
        return "";

    // Open file
    File file = SPIFFS.open(filename, FILE_READ);
    if (!file)
    {
        Log.trace("%sfailed to open file to read %s\n", MODULE_PREFIX, filename.c_str());
        return "";
    }

    // Buffer
    uint8_t* pBuf = new uint8_t[maxLen+1];
    if (!pBuf)
    {
        Log.trace("%sfailed to allocate %d\n", MODULE_PREFIX, maxLen);
        return "";
    }

    // Read
    size_t bytesRead = file.read(pBuf, maxLen);
    file.close();
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

    // Open file
    File file = SPIFFS.open(filename, FILE_WRITE);
    if (!file)
    {
        Log.trace("%sfailed to open file to write %s\n", MODULE_PREFIX, filename.c_str());
        return "";
    }

    // Write
    size_t bytesWritten = file.write((uint8_t*)(fileContents.c_str()), fileContents.length());
    file.close();
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
    // Write file block
    File file = SPIFFS.open("/__tmp__", accessType);
    if (!file)
    {
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
}

bool FileManager::deleteFile(const String& fileSystemStr, const String& filename)
{
    // Check file system supported
    if ((fileSystemStr.length() > 0) && (fileSystemStr != "SPIFFS"))
    {
        return false;
    }
    if (!SPIFFS.remove("/" + filename))
    {
        Log.notice("%sfailed to remove file %s\n", MODULE_PREFIX, filename.c_str());
        return false; 
    }               
    return true;
}

bool FileManager::chunkedFileStart(const String& fileSystemStr, const String& filename, bool readByLine)
{
    // Check file system supported
    if ((fileSystemStr.length() > 0) && (fileSystemStr != "SPIFFS"))
    {
        return false;
    }

    // Check file valid
    File file = SPIFFS.open("/" + filename, FILE_READ);
    if (!file)
    {
        Log.trace("%schunked failed to open %s file\n", MODULE_PREFIX, filename.c_str());
        return false;
    }
    _chunkedFileLen = file.size();
    file.close();
    
    // Setup access
    _chunkedFilename = filename;
    _chunkedFileInProgress = true;
    _chunkedFilePos = 0;
    _chunkOnLineEndings = readByLine;
    Log.verbose("%schunkedFileStart filename %s size %d\n", MODULE_PREFIX, filename.c_str(), _chunkedFileLen);
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

    // Open file and seek
    File file = SPIFFS.open("/" + _chunkedFilename, FILE_READ);
    if (!file)
    {
        Log.trace("%schunkNext failed open %s\n", MODULE_PREFIX, _chunkedFilename.c_str());
        return NULL;
    }
    if ((_chunkedFilePos != 0) && (!file.seek(_chunkedFilePos)))
    {
        Log.trace("%schunkNext failed seek in filename %s to %d\n", MODULE_PREFIX, 
                        _chunkedFilename.c_str(), _chunkedFilePos);
        file.close();
        return NULL;
    }

    // Fill the buffer with file data
    chunkLen = file.read(_chunkedFileBuffer, CHUNKED_BUF_MAXLEN);

    // Bump position and check if this was the final block
    _chunkedFilePos = _chunkedFilePos + CHUNKED_BUF_MAXLEN;
    if ((file.position() < _chunkedFilePos) || (chunkLen < CHUNKED_BUF_MAXLEN))
    {
        finalChunk = true;
        _chunkedFileInProgress = false;
    }

    // Close
    file.close();
    return _chunkedFileBuffer;
}

