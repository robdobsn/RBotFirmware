// Over-The-Air (OTA) Firmware Update
// Rob Dobson 2018

#pragma once

#include <HTTPClient.h>
#include <WifiClient.h>
#include <ArduinoLog.h>
#include "Update.h"
#include "Utils.h"

class RdOTAUpdate
{
  private:

    // Enabled
    bool _otaEnabled;

    enum OTAUpdateState
    {
        OTA_UPDATE_STATE_IDLE,
        OTA_UPDATE_STATE_GET_VERSION,
        OTA_UPDATE_STATE_GET_DOWNLOAD_LEN,
        OTA_UPDATE_STATE_DOWNLOADING
    };

    static constexpr const char *HTTP_REQUEST_BASE =
        "GET /deployota/[project]/[filename] HTTP/1.1\r\n"
        "Host: [host]\r\n"
        "Accept-Language: en-us, en-gb\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "User-Agent: RdOTAUpdate\r\n\r\n";

    // Timeouts in update process
    const int TIMEOUT_GET_VERSION_MS = 2000;
    const int TIMEOUT_GET_DOWNLOAD_LENGTH_MS = 2000;
    const int TIMEOUT_DOWNLOAD_FIRMWARE_MS = 600000;

    // Max length of OTA file info
    const int OTA_FILEINFO_MAXLEN = 1000;

    // Max number of bytes in one chunk
    const int MAX_RX_BUFFER_SIZE = 100;

    // Master flag indicating update check is needed
    bool _firmwareCheckRequired;
    bool _updateHasBeenStarted;

    // Update server details
    String _updateServerName;
    uint16_t _updateServerPort;

    // State machine variables
    OTAUpdateState _otaUpdateState;
    unsigned long _updateStateEntryMs;

    // Header parsing
    bool _lastLineBlank;
    bool _headerComplete;
    String _responseHeader;
    int _contentDataLength;
    int _contentRxCount;

    // Update file info
    String _fileInfo;
    String _targetVersionStr;
    String _targetFilename;
    String _targetMD5;
    int _targetFileLength;

    // Progress
    int _updateBytesWritten;

    // Project name and current version
    String _projectName;
    String _currentVers;

    // TCP client
    WiFiClient _wifiClient;

  public:
    RdOTAUpdate()
    {
        _otaEnabled = false;
        // Header parsing
        _lastLineBlank = false;
        _headerComplete = false;
        _contentDataLength = 0;
        _contentRxCount = 0;
        // Progress
        _targetFileLength = 0;
        _updateBytesWritten = 0;
        _updateHasBeenStarted = false;
        // Flag indicating a firmware update check needed
        _firmwareCheckRequired = false;
        // Initially idle
        _otaUpdateState = OTA_UPDATE_STATE_IDLE;
    }

    void setup(ConfigBase& config, const char *projectName, const char *currentVers)
    {            
        // Get config
        ConfigBase otaConfig(config.getString("OTAUpdate", "").c_str());
        _otaEnabled = otaConfig.getLong("enabled", 0) != 0;
        // Project name must match the file store naming
        _projectName = projectName;
        _currentVers = currentVers;
        // Update server
        _updateServerName = otaConfig.getString("server","");
        _updateServerPort = otaConfig.getLong("port",80);
        // Init timer
        _updateStateEntryMs = millis();
    }

    void requestUpdateCheck()
    {
        _firmwareCheckRequired = true;
    }

    bool isInProgress()
    {
        return _otaUpdateState != OTA_UPDATE_STATE_IDLE;
    }

    void service()
    {
        // Check if enabled
        if (!_otaEnabled)
            return;

        // Time to check for firmware?
        if (_firmwareCheckRequired)
        {
            // Start update process
            startUpdateProcess();

            // No longer need to check
            _firmwareCheckRequired = false;
        }

        // Handle connected - pump any data
        if (_wifiClient.connected())
        {
            // Check for data available
            int numAvail = _wifiClient.available();
            int numToRead = numAvail;
            if (numAvail > MAX_RX_BUFFER_SIZE)
            {
                numToRead = MAX_RX_BUFFER_SIZE;
            }
            if (numToRead > 0)
            {
                uint8_t rxBuf[MAX_RX_BUFFER_SIZE];
                int numRead = _wifiClient.read(rxBuf, numToRead);
                // Log.verbose("OTAUpdate: wifiClient reading %d available %d read %d\n", numToRead, numAvail, numRead);
                if (numRead > 0)
                {
                    onDataReceived(rxBuf, numRead);
                }
            }
        }

        // State management
        switch(_otaUpdateState)
        {
            default:
            case OTA_UPDATE_STATE_IDLE:
            {
                break;
            }
            case OTA_UPDATE_STATE_GET_VERSION:
            {
                // Check timeout
                if (Utils::isTimeout(millis(), _updateStateEntryMs, TIMEOUT_GET_VERSION_MS))
                {
                    Log.notice("OTAUpdate: Timeout in getting version info\n");
                    abortUpdateProcess();
                }
                break;
            }
            case OTA_UPDATE_STATE_GET_DOWNLOAD_LEN:
            {
                // Check timeout
                if (Utils::isTimeout(millis(), _updateStateEntryMs, TIMEOUT_GET_DOWNLOAD_LENGTH_MS))
                {
                    Log.notice("OTAUpdate: Timeout in getting download length\n");
                    abortUpdateProcess();
                }
                break;
            }
            case OTA_UPDATE_STATE_DOWNLOADING:
            {
                // Check timeout
                if (Utils::isTimeout(millis(), _updateStateEntryMs, TIMEOUT_DOWNLOAD_FIRMWARE_MS))
                {
                    Log.notice("OTAUpdate: Failed update - timeout\n");
                    abortUpdateProcess();
                }

                // Check complete
                if (updateCheckComplete())
                {
                    setState(OTA_UPDATE_STATE_IDLE);

                    // Restart CPU to complete process
                    Log.notice("Restarting now .....\n");
                    ESP.restart();
                }
                break;
            }
        }
    }

  private:
    void startUpdateProcess()
    {
        // Check if enabled
        if (!_otaEnabled)
            return;

        // Abandon any existing connection
        if (_wifiClient.connected())
        {
            _wifiClient.stop();
            Log.verbose("OTAUpdate: Stopped existing TCP conn\n");            
        }

        // Connect
        Log.trace("OTAUpdate: TCP conn to %s:%d\n", _updateServerName.c_str(), _updateServerPort);
        bool rslt = _wifiClient.connect(_updateServerName.c_str(), _updateServerPort);
        Log.trace("OTAUpdate: TCP connect rslt %s\n", rslt ? "OK" : "FAIL");

        // Stop if we can't connect
        if (!rslt)
            return;

        // Start the version check
        startVersionCheck();
    }

    void startVersionCheck()
    {
        // Indicate starting
        Log.trace("OTAUpdate: Staring version check ..............................\n");
        // Log.verbose("OTAUpdate: Client space %d\n", _tcpClient.space());
        String requestStr = HTTP_REQUEST_BASE;
        requestStr.replace("[project]", _projectName);
        requestStr.replace("[filename]", "fileinfo.inf");
        requestStr.replace("[host]", _updateServerName);
        // Log.verbose("OTAUpdate: Request str ...\n");
        // Log.verbose("%s\n", requestStr.c_str());
        _wifiClient.write(requestStr.c_str(), requestStr.length());

        // Set state to get version info
        setState(OTA_UPDATE_STATE_GET_VERSION);
    }

    void startDownloadProcess()
    {
        Log.trace("OTAUpdate: Getting update binary ..............................\n");
        // Log.verbose("Client space %d\n", _tcpClient.space());
        String requestStr = HTTP_REQUEST_BASE;
        requestStr.replace("[project]", _projectName);
        requestStr.replace("[filename]", _targetFilename);
        requestStr.replace("[host]", _updateServerName);
        // Set state machine
        setState(OTA_UPDATE_STATE_GET_DOWNLOAD_LEN);
        // Make the request
        _wifiClient.write(requestStr.c_str(), requestStr.length());
    }

    void abortUpdateProcess()
    {
        // Abort update
        if (_updateHasBeenStarted)
            Update.abort();

        // Close TCP if connected
        if (_wifiClient.connected())
        {
            _wifiClient.stop();
            Log.verbose("OTAUpdate: Stopped existing TCP conn\n");
        }

        // Now idle again
        setState(OTA_UPDATE_STATE_IDLE);

        // Debug
        Log.trace("OTAUpdate: Update process aborted\n");
    }

    void setState(OTAUpdateState newState)
    {
        _otaUpdateState = newState;
        _updateStateEntryMs = millis();
        if (newState == OTA_UPDATE_STATE_GET_VERSION)
        {
            _headerComplete = false;
            _lastLineBlank = false;
            _responseHeader = "";
            _contentDataLength = 0;
            _contentRxCount = 0;
            _fileInfo = "";
            _responseHeader.reserve(300);
            Log.trace("OTAUpate: SetState GetVersion\n");
        }
        else if (newState == OTA_UPDATE_STATE_GET_DOWNLOAD_LEN)
        {
            Log.trace("OTAUpate: SetState Download Pending\n");
            _headerComplete = false;
            _lastLineBlank = false;
            _responseHeader = "";
            _contentDataLength = 0;
            _contentRxCount = 0;
        }
        else if (newState == OTA_UPDATE_STATE_DOWNLOADING)
        {
            Log.trace("OTAUpate: SetState Downloading\n");
        }
        else
        {
            if (_wifiClient.connected())
            {
                _wifiClient.stop();
                Log.verbose("OTAUpdate: Stopped existing TCP conn\n");            
            }
            _responseHeader.reserve(1);
            Log.trace("OTAUpate: SetState Idle\n");
        }
    }

    bool checkVersionString(const char *pNewVersionStr)
    {
        return (strcmp(pNewVersionStr, _currentVers.c_str()) > 0);
    }

    bool updateStart(unsigned int updateFileLen)
    {
        // Check the update can be started
        _targetFileLength = updateFileLen;
        _updateBytesWritten = 0;
        bool enoughSpace = Update.begin(updateFileLen, U_FLASH);
        if (!enoughSpace)
        {
            Log.notice("OTAUpdate: Not enough space for file length %d\n", _targetFileLength);
            return false;
        }

        // Started ok
        Log.notice("OTAUpdate: Update has enough space\n");
        Update.setMD5(_targetMD5.c_str());
        _updateHasBeenStarted = true;
        return true;
    }

    void updateChunk(uint8_t *pData, int dataReceivedLen)
    {
        bool bytesWritten = Update.write(pData, dataReceivedLen);
        if (bytesWritten > 0)
        {
            // Note the write method always seems to return 1 but it works ok so I guess it must
            // be writing the remaining data ok
            // if (bytesWritten != dataReceivedLen)
            // {
            //     Log.verbose("Flash write returned %d != %d\n", bytesWritten, dataReceivedLen);
            // }
            _updateBytesWritten += bytesWritten;
            // Log.verbose("Progress %d\n", (100 * _updateBytesWritten) / _targetFileLength);
        }
        else
        {
            Log.trace("Flashing failed - no data written - should be %d\n", dataReceivedLen);
        }
    }

    bool updateCheckComplete()
    {
        if (Update.isFinished())
        {
            Update.end();
            _updateHasBeenStarted = false;
            return true;
        }
        return false;
    }

    String extractValueByName(String &nameValuesStr, const char *pValueName)
    {
        // Convert to lower case
        String lcNameValuesStr = nameValuesStr;
        lcNameValuesStr.toLowerCase();

        // Find value by name
        String valueName = pValueName;
        valueName.toLowerCase();
        int headerPos = lcNameValuesStr.indexOf(valueName);
        // Log.verbose("OTAUpdate: Extract value %s\n", valueName.c_str());
        if (headerPos >= 0)
        {
            // Extract the contents
            String headerContent;
            int colonPos = nameValuesStr.indexOf(":", headerPos);
            // Log.verbose("Colon: %d\n",colonPos );
            if (colonPos >= 0)
            {
                int eolPos = nameValuesStr.indexOf("\n", colonPos);
                if (eolPos < 0)
                    headerContent = nameValuesStr.substring(colonPos + 1);
                else
                    headerContent = nameValuesStr.substring(colonPos + 1, eolPos);
                headerContent.trim();
                // Log.verbose("OTAUpdate: Value extracted === %s\n", headerContent.c_str());
                return headerContent;
            }
        }
        Log.verbose("%s\n", nameValuesStr.c_str());
        Log.verbose("Value %s not found in info file\n", pValueName);
        return "";
    }

    void onDataReceived(uint8_t *pDataReceived, size_t dataReceivedLen)
    {
        // Log.verbose("OTAUpdate: Data callback len %d .....\n", dataReceivedLen);
        // for (int i = 0; i < dataReceivedLen; i++)
        // {
        //     Serial.printf("%02x ", ((unsigned char*)pDataReceived)[i]);
        // }
        // Serial.println();

        // If idle do nothing
        if (_otaUpdateState == OTA_UPDATE_STATE_IDLE)
            return;

        // Process the received pDataReceived
        for (int i = 0; i < dataReceivedLen; i++)
        {
            char ch = ((unsigned char *)pDataReceived)[i];
            if (_headerComplete)
            {
                if (_otaUpdateState == OTA_UPDATE_STATE_GET_VERSION)
                {
                    _fileInfo += ch;
                    _contentRxCount++;
                    if (_contentRxCount >= _contentDataLength)
                    {
                        // Check the received code is not "File not found"
                        if (_fileInfo.equalsIgnoreCase("file not found"))
                        {
                            Log.notice("OTAUpdate: File not found\n");
                            setState(OTA_UPDATE_STATE_IDLE);
                            return;
                        }
                        // Extract version
                        String fileVersionStr = extractValueByName(_fileInfo, "version");
                        String fileName = extractValueByName(_fileInfo, "filename");
                        String fileMD5 = extractValueByName(_fileInfo, "MD5");
                        Log.trace("OTAUpdate: Version %s, Filename %s, ND5 %s\n", fileVersionStr.c_str(), fileName.c_str(), fileMD5.c_str());
                        if (fileVersionStr.length() == 0)
                        {
                            Log.trace("OTAUpdate: Cannot extract version from fileInfo\n");
                            setState(OTA_UPDATE_STATE_IDLE);
                            return;
                        }
                        if (fileName.length() == 0)
                        {
                            Log.trace("OTAUpdate: File name invalid\n");
                            setState(OTA_UPDATE_STATE_IDLE);
                            return;
                        }
                        // Check version against current
                        bool versionNeedsUpdating = checkVersionString(fileVersionStr.c_str());
                        if (versionNeedsUpdating)
                        {
                            _targetVersionStr = fileVersionStr;
                            _targetFilename = fileName;
                            _targetMD5 = fileMD5;
                            Log.notice("OTAUpdate: Starting update to version %s with MD5 %s\n", _targetVersionStr.c_str(), _targetMD5.c_str());
                            startDownloadProcess();
                            return;
                        }
                        else
                        {
                            Log.trace("OTAUpdate: No update required\n");
                            setState(OTA_UPDATE_STATE_IDLE);
                            return;
                        }
                    }
                }
                else
                {
                    // Send chunk of file to Update
                    uint8_t *pDataChunk = ((uint8_t *)pDataReceived) + i;
                    int dataChunkLen = dataReceivedLen - i;
                    updateChunk(pDataChunk, dataChunkLen);

                    // Break out of looping over chars as we're handling in chunks now
                    break;
                }
            }
            else
            {
                _responseHeader += ch;

                // Handle blank-line checking
                if (ch == '\n')
                {
                    if (_lastLineBlank)
                    {
                        _headerComplete = true;
                        String contentLenStr = extractValueByName(_responseHeader, "content-length");
                        _contentDataLength = contentLenStr.toInt();
                        // Log.verbose("OTAUpdate: Content-Length: %s = %d\n", contentLenStr.c_str(), _contentDataLength);
                        _contentRxCount = 0;
                        if (_otaUpdateState == OTA_UPDATE_STATE_GET_VERSION)
                        {
                            _fileInfo = "";
                            if (_contentDataLength > OTA_FILEINFO_MAXLEN)
                                _contentDataLength = OTA_FILEINFO_MAXLEN;
                            _fileInfo.reserve(_contentDataLength + 1);
                        }
                        else
                        {
                            Log.trace("OTAUpdate: Downloading started\n");
                            // Start the update process
                            if (updateStart(_contentDataLength))
                            {
                                setState(OTA_UPDATE_STATE_DOWNLOADING);
                            }
                            else
                            {
                                abortUpdateProcess();
                                return;
                            }
                        }
                    }
                    _lastLineBlank = true;
                }
                else if (ch != '\r')
                {
                    _lastLineBlank = false;
                }
            }
        }
    }
};
