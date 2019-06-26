// CommandSerial
// Rob Dobson 2018

#include "CommandSerial.h"
#include "RdJson.h"

static const char* MODULE_PREFIX = "CommandSerial: ";

CommandSerial::CommandSerial(FileManager& fileManager) : 
        _miniHDLC(std::bind(&CommandSerial::sendCharToCmdPort, this, std::placeholders::_1), 
            std::bind(&CommandSerial::frameHandler, this, std::placeholders::_1, std::placeholders::_2),
            true, false),
        _fileManager(fileManager)
{
    _pSerial = NULL;
    _serialPortNum = -1;
    _uploadFromFSInProgress = false;
    _uploadFromAPIInProgress = false;
    _uploadStartMs = 0;
    _uploadLastBlockMs = 0;
    _blockCount = 0;
    _baudRate = 115200;
    _frameRxCallback = nullptr;
}

void CommandSerial::setup(ConfigBase& config)
{
    // Get config
    ConfigBase csConfig(config.getString("commandSerial", "").c_str());
    Log.notice("%sconfig %s\n", MODULE_PREFIX, csConfig.getConfigCStrPtr());

    // Get serial port
    _serialPortNum = csConfig.getLong("portNum", -1);
    _baudRate = csConfig.getLong("baudRate", 115200);

    // Setup port
    if (_serialPortNum == -1)
        return;

    // Setup serial port
    _pSerial = new HardwareSerial(_serialPortNum);
    if (_pSerial)
    {
        if (_serialPortNum == 1)
        {
            _pSerial->begin(_baudRate, SERIAL_8N1, 16, 17, false);
            Log.notice("%sportNum %d, baudRate %d, rxPin %d, txPin %d\n", MODULE_PREFIX,
                            _serialPortNum, _baudRate, 16, 17);
        }
        else if (_serialPortNum == 2)
        {
            _pSerial->begin(_baudRate, SERIAL_8N1, 26, 25, false);
            Log.notice("%sportNum %d, baudRate %d, rxPin %d, txPin %d\n", MODULE_PREFIX,
                            _serialPortNum, _baudRate, 26, 25);
        }
        else
        {
            _pSerial->begin(_baudRate);
            Log.notice("%sportNum %d, baudRate %d, rxPin %d, txPin %d\n", MODULE_PREFIX,
                            _serialPortNum, _baudRate, 3, 1);
        }

        // Set rx buffer size or leave as default
        int rxBufSize = csConfig.getLong("rxBufSize", -1);
        if (rxBufSize > 0)
        {
            _pSerial->setRxBufferSize(rxBufSize);
        }
    }
    else
    {
        Log.notice("%sfailed portNum %d, baudRate %d\n", MODULE_PREFIX,
                        _serialPortNum, _baudRate);
    }
}

// Set callback on frame received
void CommandSerial::setCallbackOnRxFrame(CommandSerialFrameRxFnType frameRxCallback)
{
    _frameRxCallback = frameRxCallback;
}

// Log message
void CommandSerial::logMessage(String& msg)
{
    // Don't use ArduinoLog logging here as CmdSerial can be used by NetLog and this becomes circular
    // Serial.printf("CommandSerial: send frame %s\n", msg.c_str());

    // Log over HDLC
    String frame = "{\"cmdName\":\"logMsg\",\"msg\":\"" + msg + "}\0";
    _miniHDLC.sendFrame((const uint8_t*)frame.c_str(), frame.length());
}

// Event message
void CommandSerial::eventMessage(String& msgJson)
{
    // Serial.printf("CommandSerial: event Msg %s\n", msgJson.c_str());

    String frame = "{\"cmdName\":\"eventMsg\"," + msgJson + "}\0";
    _miniHDLC.sendFrame((const uint8_t*)frame.c_str(), frame.length());
}

// Event message
void CommandSerial::responseMessage(String& reqStr, String& msgJson)
{
    // Serial.printf("CommandSerial: req %s ... response Msg %s\n", reqStr.c_str(), msgJson.c_str());
    String frame;
    if (msgJson.startsWith("{"))
    {
        frame = "{\"cmdName\":\"" + reqStr + "Resp\",\"msg\":" + msgJson + "}\0";
    }
    else
    {
        frame = "{\"cmdName\":\"" + reqStr + "Resp\"," + msgJson + "}\0";
    }
    _miniHDLC.sendFrame((const uint8_t*)frame.c_str(), frame.length());
}

// Upload in progress
bool CommandSerial::uploadInProgress()
{
    return _uploadFromAPIInProgress || _uploadFromFSInProgress;
}

// Service 
void CommandSerial::service()
{
    // Check if port enabled
    if (!_pSerial)
        return;

    // See if characters to be processed
    for (int rxCtr = 0; rxCtr < 5000; rxCtr++)
    {
        int rxCh = _pSerial->read();
        if (rxCh < 0)
            break;

        // Handle char
        _miniHDLC.handleChar(rxCh);
    }

    // Check if there's a file system upload in progress
    if (_uploadFromFSInProgress)
    {
        // See if ready to handle the next chunk
        if (Utils::isTimeout(millis(), _uploadLastBlockMs, DEFAULT_BETWEEN_BLOCKS_MS))
        {
            // Get next chunk of data
            String filename;
            int fileLen = 0; 
            int chunkPos = 0;
            int chunkLen = 0;
            bool finalChunk = false;
            uint8_t* pData = _fileManager.chunkFileNext(filename, fileLen, chunkPos, chunkLen, finalChunk);
            if (pData && (chunkLen != 0))
            {
                // Handle the chunk
                uploadCommonBlockHandler(_uploadFileType.c_str(), _uploadFromFSRequest, filename, 
                            fileLen, chunkPos, pData, chunkLen, finalChunk); 
            }
            else
            {
                // Tidy up if finished
                if (!finalChunk)
                    Log.warning("%supload 0 len but not final\n", MODULE_PREFIX);
                _uploadFromFSInProgress = false;
                // Log.notice("File upload from FS timed out lastBlockMs %u betweenBlocksMs %u chunkLen %u finalChunk %d", 
                //         _uploadLastBlockMs, DEFAULT_BETWEEN_BLOCKS_MS, chunkLen, finalChunk);
            }
        }
    }

    // Check for timeouts on any upload
    if (uploadInProgress())
    {
        // Check for timeouts
        uint32_t curMs = millis();
        if (Utils::isTimeout(curMs+1, _uploadLastBlockMs, MAX_BETWEEN_BLOCKS_MS))
        {
            _uploadFromFSInProgress = false;
            _uploadFromAPIInProgress = false;
            Log.notice("%sUpload block timed out millis %d lastBlockMs %d\n", MODULE_PREFIX, 
                        curMs, _uploadLastBlockMs);
        }
        if (Utils::isTimeout(curMs+1, _uploadStartMs, MAX_UPLOAD_MS))
        {
            _uploadFromFSInProgress = false;
            _uploadFromAPIInProgress = false;
            Log.notice("%sUpload timed out\n", MODULE_PREFIX);
        }
    }
}

void CommandSerial::sendFileStartRecord(const char* fileType, const String& req, const String& filename, int fileLength)
{
    String reqParams = Utils::getJSONFromHTTPQueryStr(req.c_str(), true);
    String frame = "{\"cmdName\":\"ufStart\",\"fileType\":\"" + String(fileType) + "\",\"fileName\":\"" + filename +
                "\",\"fileLen\":" + String(fileLength) + 
                ((reqParams.length() > 0) ? ("," + reqParams) : "") +
                "}";
    _miniHDLC.sendFrame((const uint8_t*)frame.c_str(), frame.length());
}

void CommandSerial::sendFileBlock(size_t index, uint8_t *data, size_t len)
{
    String header = "{\"cmdName\":\"ufBlock\",\"index\":" + String(index) + ",\"len\":" + String(len) + "}";
    int headerLen = header.length();
    uint8_t* pFrameBuf = new uint8_t[headerLen + len + 1];
    memcpy(pFrameBuf, header.c_str(), headerLen);
    pFrameBuf[headerLen] = 0;
    memcpy(pFrameBuf + headerLen + 1, data, len);
    _miniHDLC.sendFrame(pFrameBuf, headerLen + len + 1);
    delete [] pFrameBuf;
}

void CommandSerial::sendFileEndRecord(int blockCount, const char* pAdditionalJsonNameValues)
{
    String frame = "{\"cmdName\":\"ufEnd\",\"blockCount\":\"" + String(blockCount) + "\"" +
            (pAdditionalJsonNameValues ? ("," + String(pAdditionalJsonNameValues)) : "") + "}";
    _miniHDLC.sendFrame((const uint8_t*)frame.c_str(), frame.length());
}

void CommandSerial::sendTargetCommand(const String& targetCmd, const String& reqStr)
{
    // Serial.printf("CommandSerial: targetCommand Msg %s\n", targetCmd.c_str());

    String frame = "{\"cmdName\":\"" + targetCmd + "\",\"reqStr\":\"" + reqStr + "\"}\0";
    _miniHDLC.sendFrame((const uint8_t*)frame.c_str(), frame.length());
}

void CommandSerial::sendTargetData(const String& cmdName, const uint8_t* pData, int len, int index)
{
    String header = "{\"cmdName\":\"" + cmdName + "\",\"msgIdx\":" + String(index) + ",\"dataLen\":" + String(len) + "}";
    int headerLen = header.length();
    uint8_t* pFrameBuf = new uint8_t[headerLen + len + 1];
    memcpy(pFrameBuf, header.c_str(), headerLen);
    pFrameBuf[headerLen] = 0;
    memcpy(pFrameBuf + headerLen + 1, pData, len);
    _miniHDLC.sendFrame(pFrameBuf, headerLen + len + 1);
    delete [] pFrameBuf;
}

void CommandSerial::uploadCommonBlockHandler(const char* fileType, const String& req, 
            const String& filename, int fileLength, size_t index, uint8_t *data, size_t len, bool finalBlock)
{
    // Log.trace("%sblk filetype %s name %s, total %d, idx %d, len %d, final %d, fs %s api %s\n", MODULE_PREFIX, 
    //             fileType, filename.c_str(), fileLength, index, len, finalBlock, 
    //             (_uploadFromFSInProgress ? "yes" : "no"), (_uploadFromAPIInProgress ? "yes" : "no"));

    // For timeouts        
    _uploadLastBlockMs = millis();

    // Check if first block in an upload
    if (_blockCount == 0)
    {
        _uploadFileType = fileType;
        sendFileStartRecord(fileType, req, filename, fileLength);
        // Log.trace("%snew upload started millis %d blockLen %d final %d\n", MODULE_PREFIX, 
        //         _uploadLastBlockMs, len, finalBlock);
    }

    // Send the block
    sendFileBlock(index, data, len);
    // Log.trace("%sblock sent blockIdx %d len %d final %d\n", MODULE_PREFIX, _blockCount, len, finalBlock);
    _blockCount++;

    // Check if that was the final block
    if (finalBlock)
    {
        sendFileEndRecord(_blockCount, NULL);
    //    Log.trace("%sfile end sent\n", MODULE_PREFIX);
        if (_uploadTargetCommandWhenComplete.length() != 0)
        {
            sendTargetCommand(_uploadTargetCommandWhenComplete, "");
            // Log.trace("%spost-upload target command sent %s\n", MODULE_PREFIX,
            //         _uploadTargetCommandWhenComplete.c_str());
        }
        _uploadTargetCommandWhenComplete = "";
        _uploadFromFSInProgress = false;
        _uploadFromAPIInProgress = false;
    }
}

// Upload from API
void CommandSerial::uploadAPIBlockHandler(const char* fileType, const String& req, const String& filename, int fileLength, size_t index, uint8_t *data, size_t len, bool finalBlock)
{
    // Check there isn't an upload in progress from FS
    if (_uploadFromFSInProgress)
    {
        Log.notice("%suploadAPIBlockHandler upload already in progress\n", MODULE_PREFIX);
        return;
    }

    // Check upload from API already in progress
    if (!_uploadFromAPIInProgress)
    {
        // Upload now in progress
        _uploadFromAPIInProgress = true;
        _blockCount = 0;
        _uploadStartMs = millis();
        // Log.notice("%suploadAPIBlockHandler starting new - nothing in progress\n", MODULE_PREFIX);
    }
    
    // Commmon handler
    uploadCommonBlockHandler(fileType, req, filename, fileLength, index, data, len, finalBlock);
}

// Upload a file from the file system
// Request is in the format of HTTP query parameters (e.g. "?baseAddr=1234")
bool CommandSerial::startUploadFromFileSystem(const String& fileSystemName, 
                const String& uploadRequest, const String& filename,
                const char* pTargetCmdWhenDone)
{
    // Check no upload is already happening
    if (uploadInProgress())
    {
        Log.notice("%sstartUploadFromFileSystem upload already in progress\n", MODULE_PREFIX);
        return false;
    }

    // Start a chunked file session
    if (!_fileManager.chunkedFileStart(fileSystemName, filename, false))
    {
        // Log.trace("%sstartUploadFromFileSystem failed to start %s\n", MODULE_PREFIX, filename.c_str());
        return false;
    }

    // Upload now in progress
    // Log.trace("%sstartUploadFromFileSystem %s\n", MODULE_PREFIX, filename.c_str());
    _uploadFromFSInProgress = true;
    _uploadFileType = "target";
    _uploadFromFSRequest = uploadRequest;
    _blockCount = 0;
    _uploadStartMs = millis();
    _uploadLastBlockMs = millis();
    if (pTargetCmdWhenDone)
        _uploadTargetCommandWhenComplete = pTargetCmdWhenDone;
    return true;
}

void CommandSerial::sendCharToCmdPort(uint8_t ch)
{
    if (_pSerial)
        _pSerial->write(ch);
}

void CommandSerial::frameHandler(const uint8_t *framebuffer, int framelength)
{
    // Handle received frames
    if (_frameRxCallback)
        _frameRxCallback(framebuffer, framelength);
    // Serial.printf("HDLC frame received, len %d\n", framelength);
}
