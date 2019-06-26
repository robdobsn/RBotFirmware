// FileManager 
// Rob Dobson 2018

#include "RdJson.h"
#include "FileManager.h"
#include "Utils.h"
#include <sys/stat.h>
#include "vfs_api.h"
#include "esp_spiffs.h"
#include "ConfigPinMap.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"

using namespace fs;

static const char* MODULE_PREFIX = "FileManager: ";

void FileManager::setup(ConfigBase& config, const char* pConfigPath)
{
    // Init
    _spiffsIsOk = false;
    _sdIsOk = false;
    _cachedFileListValid = false;

    // Get config
    String pathStr = "fileManager";
    if (pConfigPath)
        pathStr = pConfigPath;
    ConfigBase fsConfig(config.getString(pathStr.c_str(), "").c_str());
    Log.notice("%ssetup %s\n", MODULE_PREFIX, fsConfig.getConfigCStrPtr());

    // See if SPIFFS enabled
    _enableSPIFFS = fsConfig.getLong("spiffsEnabled", 0) != 0;

    // Init SPIFFS if required
    if (_enableSPIFFS)
    {
        // Begin SPIFFS using arduino functions as web server relies on that file system
        bool spiffsFormatIfCorrupt = fsConfig.getLong("spiffsFormatIfCorrupt", 0) != 0;

        // Using ESP32 native SPIFFS support rather than arduino as potential bugs encountered in some
        // arduino functions
        esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = spiffsFormatIfCorrupt
        };        
        // Use settings defined above to initialize and mount SPIFFS filesystem.
        // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
        esp_err_t ret = esp_vfs_spiffs_register(&conf);
        if (ret != ESP_OK)
        {
            if (ret == ESP_FAIL)
                Log.warning("%ssetup failed mount/format SPIFFS\n", MODULE_PREFIX);
            else if (ret == ESP_ERR_NOT_FOUND)
                Log.warning("%ssetup failed to find SPIFFS partition\n", MODULE_PREFIX);
            else
                Log.warning("%ssetup failed to init SPIFFS (error %s)\n", MODULE_PREFIX, esp_err_to_name(ret));
        }
        else
        {
            // Get SPIFFS info
            size_t total = 0, used = 0;
            esp_err_t ret = esp_spiffs_info(NULL, &total, &used);
            if (ret != ESP_OK)
                Log.warning("%ssetup failed to get SPIFFS info (error %s)\n", MODULE_PREFIX, esp_err_to_name(ret));
            else
                Log.notice("%ssetup SPIFFS partition size total %d, used %d\n", MODULE_PREFIX, total, used);

            // Default to SPIFFS
            _defaultToSPIFFS = true;
            _spiffsIsOk = true;
        }
    }

    // See if SD enabled
    _enableSD = fsConfig.getLong("sdEnabled", 0) != 0;

    // Init SD if enabled
    if (_enableSD)
    {
        // Get settings
        String pinName = fsConfig.getString("sdMOSI", "");
        int sdMOSIPin = ConfigPinMap::getPinFromName(pinName.c_str());
        pinName = fsConfig.getString("sdMISO", "");
        int sdMISOPin = ConfigPinMap::getPinFromName(pinName.c_str());
        pinName = fsConfig.getString("sdCLK", "");
        int sdCLKPin = ConfigPinMap::getPinFromName(pinName.c_str());
        pinName = fsConfig.getString("sdCS", "");
        int sdCSPin = ConfigPinMap::getPinFromName(pinName.c_str());
        
        // Check valid
        if (sdMOSIPin == -1 || sdMISOPin == -1 || sdCLKPin == -1 || sdCSPin == -1)
        {
            Log.warning("%ssetup SD pins invalid\n", MODULE_PREFIX);
        }
        else
        {
            sdmmc_host_t host = SDSPI_HOST_DEFAULT();
            sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
            slot_config.gpio_miso = (gpio_num_t)sdMISOPin;
            slot_config.gpio_mosi = (gpio_num_t)sdMOSIPin;
            slot_config.gpio_sck  = (gpio_num_t)sdCLKPin;
            slot_config.gpio_cs   = (gpio_num_t)sdCSPin;
            // Options for mounting the filesystem.
            // If format_if_mount_failed is set to true, SD card will be partitioned and formatted
            // in case when mounting fails.
            esp_vfs_fat_sdmmc_mount_config_t mount_config = {
                .format_if_mount_failed = false,
                .max_files = 5
            };

            sdmmc_card_t* pCard;
            esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sd", &host, &slot_config, &mount_config, &pCard);
            if (ret != ESP_OK) {
                if (ret == ESP_FAIL)
                    Log.warning("%ssetup failed mount SD\n", MODULE_PREFIX);
                else
                    Log.warning("%ssetup failed to init SD (error %s)\n", MODULE_PREFIX, esp_err_to_name(ret));
            }
            else
            {
                _pSDCard = pCard;
                Log.notice("%ssetup SD ok\n", MODULE_PREFIX);
                // Default to SD
                _defaultToSPIFFS = false;
                _sdIsOk = true;
            }

            // DEBUG SD print SD card info
            // sdmmc_card_print_info(stdout, pCard);
        }
    }
}
    
void FileManager::reformat(const String& fileSystemStr, String& respStr)
{
    // Check file system supported
    String nameOfFS;
    if (!checkFileSystem(fileSystemStr, nameOfFS))
    {
        respStr = "{\"rslt\":\"fail\",\"error\":\"invalidfs\",\"files\":[]}";
        return;
    }
    
    // Reformat - need to disable Watchdog timer while formatting
    // Watchdog is not enabled on core 1 in Arduino according to this
    // https://www.bountysource.com/issues/44690700-watchdog-with-system-reset
    _cachedFileListValid = false;
    disableCore0WDT();
    esp_err_t ret = esp_spiffs_format(NULL);
    enableCore0WDT();
    Utils::setJsonBoolResult(respStr, ret == ESP_OK);
    Log.warning("%sReformat SPIFFS result %s\n", MODULE_PREFIX, (ret == ESP_OK ? "OK" : "FAIL"));
}

bool FileManager::getFileInfo(const String& fileSystemStr, const String& filename, int& fileLength)
{
    String nameOfFS;
    if (!checkFileSystem(fileSystemStr, nameOfFS)) {
        Log.trace("%sgetFileInfo %s invalid file system %s\n", MODULE_PREFIX, filename.c_str(), fileSystemStr.c_str());
        return false;
    }

    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);

    // Check file exists
    struct stat st;
    String rootFilename = getFilePath(nameOfFS, filename);

    if (stat(rootFilename.c_str(), &st) != 0)
    {
        xSemaphoreGive(_fileSysMutex);
        Log.trace("%sgetFileInfo %s cannot stat\n", MODULE_PREFIX, rootFilename.c_str());
        return false;
    }
    if (!S_ISREG(st.st_mode))
    {
        xSemaphoreGive(_fileSysMutex);
        Log.trace("%sgetFileInfo %s is a folder\n", MODULE_PREFIX, rootFilename.c_str());
        return false;
    }
    xSemaphoreGive(_fileSysMutex);
    fileLength = st.st_size;
    return true;
}

bool FileManager::getFilesJSON(const String& fileSystemStr, const String& folderStr, String& respStr)
{
    // Check file system supported
    String nameOfFS;
    if (!checkFileSystem(fileSystemStr, nameOfFS))
    {
        respStr = "{\"rslt\":\"fail\",\"error\":\"unknownfs\",\"files\":[]}";
        return false;
    }

    // Check if cached version can be used
    if ((_cachedFileListValid) && (_cachedFileListResponse.length() != 0))
    {
        respStr = _cachedFileListResponse;
        return true;
        
    }
    // Take mutex
    if (xSemaphoreTake(_fileSysMutex, 0) != pdTRUE)
    {
        respStr = "{\"rslt\":\"fail\",\"error\":\"fsbusy\",\"files\":[]}";
        return false;
    }

    // Get size of file systems
    String baseFolderForFS;
    double fsSizeBytes = 0, fsUsedBytes = 0;
    if (nameOfFS == "spiffs")
    {
        uint32_t sizeBytes = 0, usedBytes = 0;
        esp_err_t ret = esp_spiffs_info(NULL, &sizeBytes, &usedBytes);
        if (ret != ESP_OK)
        {
            xSemaphoreGive(_fileSysMutex);
            Log.warning("%sgetFilesJSON Failed to get SPIFFS info (error %s)\n", MODULE_PREFIX, esp_err_to_name(ret));
            respStr = "{\"rslt\":\"fail\",\"error\":\"SPIFFSINFO\",\"files\":[]}";
            return false;
        }
        // FS settings
        fsSizeBytes = sizeBytes;
        fsUsedBytes = usedBytes;
        nameOfFS = "spiffs";
        baseFolderForFS = "/spiffs";
    }
    else if (nameOfFS == "sd")
    {
        // Get size info
        sdmmc_card_t* pCard = (sdmmc_card_t*)_pSDCard;
        if (pCard)
        {
            fsSizeBytes = ((double) pCard->csd.capacity) * pCard->csd.sector_size;
        	FATFS* fsinfo;
            DWORD fre_clust;
            if(f_getfree("0:",&fre_clust,&fsinfo) == 0)
            {
                fsUsedBytes = ((double)(fsinfo->csize))*((fsinfo->n_fatent - 2) - (fsinfo->free_clst))
            #if _MAX_SS != 512
                    *(fsinfo->ssize);
            #else
                    *512;
            #endif
            }
        }
        // Set FS info
        nameOfFS = "sd";
        baseFolderForFS = "/sd";
    }

    // Check file system is valid
    if (fsSizeBytes == 0)
    {
        Log.warning("%sgetFilesJSON No valid file system\n", MODULE_PREFIX);
        respStr = "{\"rslt\":\"fail\",\"error\":\"NOFS\",\"files\":[]}";
        return false;
    }

    // Open directory
    String rootFolder = (folderStr.startsWith("/") ? baseFolderForFS + folderStr : (baseFolderForFS + "/" + folderStr));
    DIR* dir = opendir(rootFolder.c_str());
    if (!dir)
    {
        xSemaphoreGive(_fileSysMutex);
        Log.warning("%sgetFilesJSON Failed to open base folder %s\n", MODULE_PREFIX, rootFolder.c_str());
        respStr = "{\"rslt\":\"fail\",\"error\":\"nofolder\",\"files\":[]}";
        return false;
    }

    // Start response JSON
    respStr = "{\"rslt\":\"ok\",\"fsName\":\"" + nameOfFS + "\",\"fsBase\":\"" + baseFolderForFS + 
                "\",\"diskSize\":" + String(fsSizeBytes) + ",\"diskUsed\":" + fsUsedBytes +
                ",\"folder\":\"" + String(rootFolder) + "\",\"files\":[";
    bool firstFile = true;

    // Read directory entries
    struct dirent* ent = NULL;
    while ((ent = readdir(dir)) != NULL) 
    {
        // Check for unwanted files
        String fName = ent->d_name;
        if (fName.equalsIgnoreCase("System Volume Information"))
            continue;
        if (fName.equalsIgnoreCase("thumbs.db"))
            continue;

        // Get file info including size
        size_t fileSize = 0;
        struct stat st;
        String filePath = (rootFolder.endsWith("/") ? rootFolder + fName : rootFolder + "/" + fName);
        if (stat(filePath.c_str(), &st) == 0) 
        {
            fileSize = st.st_size;
        }

        // Form the JSON list
        if (!firstFile)
            respStr += ",";
        firstFile = false;
        respStr += "{\"name\":\"";
        respStr += ent->d_name;
        respStr += "\",\"size\":";
        respStr += String(fileSize);
        respStr += "}";
    }

    // Finished with file list
    closedir(dir);
    xSemaphoreGive(_fileSysMutex);

    // Complete string and replenish cache
    respStr += "]}";
    _cachedFileListResponse = respStr;
    _cachedFileListValid = true;
    return true;
}

String FileManager::getFileContents(const String& fileSystemStr, const String& filename, int maxLen)
{
    // Check file system supported
    String nameOfFS;
    if (!checkFileSystem(fileSystemStr, nameOfFS))
    {
        Log.trace("%sgetContents %s invalid file system %s\n", MODULE_PREFIX, filename.c_str(), fileSystemStr.c_str());
        return "";
    }

    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);

    // Get file info - to check length
    String rootFilename = getFilePath(nameOfFS, filename);
    struct stat st;
    if (stat(rootFilename.c_str(), &st) != 0)
    {
        xSemaphoreGive(_fileSysMutex);
        Log.trace("%sgetContents %s cannot stat\n", MODULE_PREFIX, rootFilename.c_str());
        return "";
    }
    if (!S_ISREG(st.st_mode))
    {
        xSemaphoreGive(_fileSysMutex);
        Log.trace("%sgetContents %s is a folder\n", MODULE_PREFIX, rootFilename.c_str());
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
        Log.trace("%sgetContents %s free heap %d size %d too big to read\n", MODULE_PREFIX, rootFilename.c_str(), maxLen, st.st_size);
        return "";
    }
    int fileSize = st.st_size;

    // Open file
    FILE* pFile = fopen(rootFilename.c_str(), "rb");
    if (!pFile)
    {
        xSemaphoreGive(_fileSysMutex);
        Log.trace("%sgetContents failed to open file to read %s\n", MODULE_PREFIX, rootFilename.c_str());
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
    String nameOfFS;
    if (!checkFileSystem(fileSystemStr, nameOfFS))
    {
        return false;
    }

    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);

    // Open file for writing
    String rootFilename = getFilePath(nameOfFS, filename);
    FILE* pFile = fopen(rootFilename.c_str(), "wb");
    if (!pFile)
    {
        xSemaphoreGive(_fileSysMutex);
        Log.trace("%ssetContents failed to open file to write %s\n", MODULE_PREFIX, rootFilename.c_str());
        return "";
    }

    // Write
    size_t bytesWritten = fwrite((uint8_t*)(fileContents.c_str()), 1, fileContents.length(), pFile);
    fclose(pFile);

    // Clean up
    _cachedFileListValid = false;
    xSemaphoreGive(_fileSysMutex);
    return bytesWritten == fileContents.length();
}

void FileManager::uploadAPIBlocksComplete()
{
    // Cached file list now invalid
    _cachedFileListValid = false;
}

void FileManager::uploadAPIBlockHandler(const char* fileSystem, const String& req, const String& filename, 
                    int fileLength, size_t index, uint8_t *data, size_t len, bool finalBlock)
{
    Log.trace("%suploadAPIBlockHandler fileSys %s, filename %s, total %d, idx %d, len %d, final %d\n", MODULE_PREFIX, 
                fileSystem, filename.c_str(), fileLength, index, len, finalBlock);

    // Check file system supported
    String nameOfFS;
    if (!checkFileSystem(String(fileSystem), nameOfFS))
        return;

    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);
    String tempFileName = "/__tmp__";
    String tmpRootFilename = getFilePath(nameOfFS, tempFileName);
    FILE* pFile = NULL;

    // Check if we should overwrite or append
    if (index > 0)
        pFile = fopen(tmpRootFilename.c_str(), "ab");
    else
        pFile = fopen(tmpRootFilename.c_str(), "wb");
    if (!pFile)
    {
        xSemaphoreGive(_fileSysMutex);
        Log.trace("%suploadBlock failed to open file to write %s\n", MODULE_PREFIX, tmpRootFilename.c_str());
        return;
    }

    // Write file block to temporary file
    size_t bytesWritten = fwrite(data, 1, len, pFile);
    fclose(pFile);
    if (bytesWritten != len)
    {
        Log.trace("%suploadBlock write failed %s (written %d != len %d)\n", MODULE_PREFIX, tmpRootFilename.c_str(), bytesWritten, len);
    }

    // Rename if last block
    if (finalBlock)
    {
        // Check if destination file exists before renaming
        struct stat st;
        String rootFilename = getFilePath(nameOfFS, filename);
        if (stat(rootFilename.c_str(), &st) == 0) 
        {
            // Remove in case filename already exists
            unlink(rootFilename.c_str());
        }

        // Rename
        if (rename(tmpRootFilename.c_str(), rootFilename.c_str()) != 0)
        {
            Log.trace("%sfailed rename %s to %s\n", MODULE_PREFIX, tmpRootFilename.c_str(), rootFilename.c_str());
        }
    }

    // Restore semaphore
    xSemaphoreGive(_fileSysMutex);
}

bool FileManager::deleteFile(const String& fileSystemStr, const String& filename)
{
    // Check file system supported
    String nameOfFS;
    if (!checkFileSystem(fileSystemStr, nameOfFS))
    {
        return false;
    }
    
    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);

    // Remove file
    struct stat st;
    String rootFilename = getFilePath(nameOfFS, filename);
    if (stat(rootFilename.c_str(), &st) == 0) 
    {
        unlink(rootFilename.c_str());
    }

    _cachedFileListValid = false;
    xSemaphoreGive(_fileSysMutex);   
    return true;
}

bool FileManager::chunkedFileStart(const String& fileSystemStr, const String& filename, bool readByLine)
{
    // Check file system supported
    String nameOfFS;
    if (!checkFileSystem(fileSystemStr, nameOfFS))
    {
        return false;
    }

    // Take mutex
    xSemaphoreTake(_fileSysMutex, portMAX_DELAY);

    // Check file exists
    struct stat st;
    String rootFilename = getFilePath(nameOfFS, filename);
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
    Log.trace("%schunkedFileStart filename %s size %d byLine %s\n", MODULE_PREFIX, 
            rootFilename.c_str(), _chunkedFileLen, (readByLine ? "Y" : "N"));
    return true; 
}

char* FileManager::readLineFromFile(char* pBuf, int maxLen, FILE* pFile)
{
    // Iterate over chars
    pBuf[0] = 0;
    char* pCurPtr = pBuf;
    int curLen = 0;
    while (true)
    {
        if (curLen >= maxLen-1)
            break;
        int ch = fgetc(pFile);
        if (ch == EOF)
        {
            if (curLen != 0)
                break;
            return NULL;
        }
        if (ch == '\n')
            break;
        if (ch == '\r')
            continue;
        *pCurPtr++ = ch;
        *pCurPtr = 0;
        curLen++;
    }
    return pBuf;
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
        char* pReadLine = readLineFromFile((char*)_chunkedFileBuffer, CHUNKED_BUF_MAXLEN-1, pFile);
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

    Log.verbose("%schunkNext filename %s chunklen %d filePos %d fileLen %d inprog %d final %d byLine %s\n", MODULE_PREFIX, 
                    _chunkedFilename.c_str(), chunkLen, _chunkedFilePos, _chunkedFileLen, 
                    _chunkedFileInProgress, finalChunk, (_chunkOnLineEndings ? "Y" : "N"));

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

// Get file system and check ok
bool FileManager::checkFileSystem(const String& fileSystemStr, String& fsName)
{
    // Check file system
    fsName = fileSystemStr;
    fsName.trim();
    fsName.toLowerCase();

    // Check for default
    if (fsName.length() == 0)
    {
        if (_defaultToSPIFFS)
            fsName = "spiffs";
        else
            fsName = "sd";
    }

    if (fsName == "spiffs")
    {
        if (!_spiffsIsOk)
            return false;
        return true;
    }
    if (fsName == "sd")
    {
        if (!_sdIsOk)
            return false;
        return true;
    }

    // Unknown FS
    return false;
}

String FileManager::getFilePath(const String& nameOfFS, const String& filename)
{
    // Check if filename already contains file system
    if ((filename.indexOf("spiffs/") >= 0) || (filename.indexOf("sd/") >= 0))
        return (filename.startsWith("/") ? filename : ("/" + filename));
    return (filename.startsWith("/") ? "/" + nameOfFS + filename : ("/" + nameOfFS + "/" + filename));
}
