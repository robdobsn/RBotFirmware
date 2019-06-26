// CommandSerial 
// Rob Dobson 2018

#pragma once
#include <Arduino.h>
#include "HardwareSerial.h"
#include "Utils.h"
#include "MiniHDLC.h"
#include "ConfigBase.h"
#include "FileManager.h"

typedef std::function<void (const uint8_t *framebuffer, int framelength)> CommandSerialFrameRxFnType;

class CommandSerial
{
private:
    // Serial
    HardwareSerial *_pSerial;

    // Serial port details
    int _serialPortNum;
    int _baudRate;

    // HDLC processor
    MiniHDLC _miniHDLC;

    // Upload of files
    bool _uploadFromFSInProgress;
    bool _uploadFromAPIInProgress;
    String _uploadFromFSRequest;
    int _blockCount;
    unsigned long _uploadStartMs;
    unsigned long _uploadLastBlockMs;
    String _uploadTargetCommandWhenComplete;
    String _uploadFileType;
    static const int MAX_UPLOAD_MS = 600000;
    static const int MAX_BETWEEN_BLOCKS_MS = 20000;
    static const int DEFAULT_BETWEEN_BLOCKS_MS = 10;

    // Frame handling callback
    CommandSerialFrameRxFnType _frameRxCallback;

    // File manager
    FileManager& _fileManager;

public:
    CommandSerial(FileManager& fileManager);
    void setup(ConfigBase& config);

    // Set callback on frame received
    void setCallbackOnRxFrame(CommandSerialFrameRxFnType frameRxCallback);
    
    // Log message
    void logMessage(String& msg);

    // Event message
    void eventMessage(String& msgJson);

    // Event message
    void responseMessage(String& reqStr, String& msgJson);

    // Upload in progress
    bool uploadInProgress();

    // Get HDLC stats
    MiniHDLCStats* getHDLCStats()
    {
        return _miniHDLC.getStats();
    }

    // Service 
    void service();

    void sendFileStartRecord(const char* fileType, const String& req, const String& filename, int fileLength);
    void sendFileBlock(size_t index, uint8_t *data, size_t len);
    void sendFileEndRecord(int blockCount, const char* pAdditionalJsonNameValues);
    void sendTargetCommand(const String& targetCmd, const String& reqStr);
    void sendTargetData(const String& cmdName, const uint8_t* pData, int len, int index);
    void uploadAPIBlockHandler(const char* fileType, const String& req, const String& filename, int fileLength, size_t index, uint8_t *data, size_t len, bool finalBlock);

    // Upload a file from the file system
    // Request is in the format of HTTP query parameters (e.g. "?baseAddr=1234")
    bool startUploadFromFileSystem(const String& fileSystemName, const String& request, const String& filename,
                    const char* pTargetCmdWhenDone);

private:
    void sendCharToCmdPort(uint8_t ch);
    void frameHandler(const uint8_t *framebuffer, int framelength);
    void uploadCommonBlockHandler(const char* fileType, const String& req, const String& filename, int fileLength, size_t index, uint8_t *data, size_t len, bool finalBlock);
};