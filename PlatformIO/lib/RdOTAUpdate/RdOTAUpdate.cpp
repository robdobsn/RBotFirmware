// Over-The-Air (OTA) Firmware Update
// Rob Dobson 2018

#include <RdOTAUpdate.h>
#include "Update.h"

static const char* MODULE_PREFIX = "OTAUpdate: ";

RdOTAUpdate::RdOTAUpdate()
{
    _otaEnabled = false;
    _otaDirectEnabled = false;
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
    // Restart pending
    _directUpdateRestartPending = false;
    _directUpdateRestartPendingStartMs = 0;
    // Direct update status
    _otaDirectUpdateHandle = -1;
    _otaDirectInProgress = false;
}

void RdOTAUpdate::setup(ConfigBase &config, const char *projectName, const char *currentVers)
{
    // Get config
    ConfigBase otaConfig(config.getString("OTAUpdate", "").c_str());
    _otaEnabled = otaConfig.getLong("enabled", 0) != 0;
    _otaDirectEnabled = (otaConfig.getLong("directOk", 1) != 0) && _otaEnabled;
    // Project name must match the file store naming
    _projectName = projectName;
    _currentVers = currentVers;
    // Update server
    _updateServerName = otaConfig.getString("server", "");
    _updateServerPort = otaConfig.getLong("port", 80);
    // Init timer
    _updateStateEntryMs = millis();
}

void RdOTAUpdate::requestUpdateCheck()
{
    _firmwareCheckRequired = true;
}

bool RdOTAUpdate::isInProgress()
{
    return _otaUpdateState != OTA_UPDATE_STATE_IDLE;
}

void RdOTAUpdate::service()
{
    // Check if enabled
    if (!_otaEnabled)
        return;

    // Check if OTA direct restart is pending
    if (_directUpdateRestartPending && 
            Utils::isTimeout(millis(), _directUpdateRestartPendingStartMs, TIME_TO_WAIT_BEFORE_RESTART_MS))
    {
        ESP.restart();
    }

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
    switch (_otaUpdateState)
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
            Log.notice("%sTimeout in getting version info\n", MODULE_PREFIX);
            abortUpdateProcess();
        }
        break;
    }
    case OTA_UPDATE_STATE_GET_DOWNLOAD_LEN:
    {
        // Check timeout
        if (Utils::isTimeout(millis(), _updateStateEntryMs, TIMEOUT_GET_DOWNLOAD_LENGTH_MS))
        {
            Log.notice("%sTimeout in getting download length\n", MODULE_PREFIX);
            abortUpdateProcess();
        }
        break;
    }
    case OTA_UPDATE_STATE_DOWNLOADING:
    {
        // Check timeout
        if (Utils::isTimeout(millis(), _updateStateEntryMs, TIMEOUT_DOWNLOAD_FIRMWARE_MS))
        {
            Log.notice("%sFailed update - timeout\n", MODULE_PREFIX);
            abortUpdateProcess();
        }

        // Check complete
        if (updateCheckComplete())
        {
            setState(OTA_UPDATE_STATE_IDLE);

            // Restart CPU to complete process
            Log.notice("%sRestarting now .....\n", MODULE_PREFIX);
            ESP.restart();
        }
        break;
    }
    }
}

void RdOTAUpdate::startUpdateProcess()
{
    // Check if enabled
    if (!_otaEnabled)
        return;

    // Abandon any existing connection
    if (_wifiClient.connected())
    {
        _wifiClient.stop();
        Log.verbose("%sStopped existing TCP conn\n", MODULE_PREFIX);
    }

    // Connect
    Log.trace("%sTCP conn to %s:%d\n", MODULE_PREFIX, _updateServerName.c_str(), _updateServerPort);
    bool rslt = _wifiClient.connect(_updateServerName.c_str(), _updateServerPort);
    Log.trace("%sTCP connect rslt %s\n", MODULE_PREFIX, rslt ? "OK" : "FAIL");

    // Stop if we can't connect
    if (!rslt)
        return;

    // Start the version check
    startVersionCheck();
}

void RdOTAUpdate::startVersionCheck()
{
    // Indicate starting
    Log.notice("%sStaring version check ..............................\n", MODULE_PREFIX);
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

void RdOTAUpdate::startDownloadProcess()
{
    Log.notice("%sGetting update binary ..............................\n", MODULE_PREFIX);
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

void RdOTAUpdate::abortUpdateProcess()
{
    // Abort update
    if (_updateHasBeenStarted)
        Update.abort();

    // Close TCP if connected
    if (_wifiClient.connected())
    {
        _wifiClient.stop();
        Log.verbose("%sStopped existing TCP conn\n", MODULE_PREFIX);
    }

    // Now idle again
    setState(OTA_UPDATE_STATE_IDLE);

    // Aborted
    Log.notice("%sUpdate process aborted\n", MODULE_PREFIX);
}

void RdOTAUpdate::setState(OTAUpdateState newState)
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
        Log.trace("%sSetState GetVersion\n", MODULE_PREFIX);
    }
    else if (newState == OTA_UPDATE_STATE_GET_DOWNLOAD_LEN)
    {
        Log.trace("%sSetState Download Pending\n", MODULE_PREFIX);
        _headerComplete = false;
        _lastLineBlank = false;
        _responseHeader = "";
        _contentDataLength = 0;
        _contentRxCount = 0;
    }
    else if (newState == OTA_UPDATE_STATE_DOWNLOADING)
    {
        Log.trace("%sSetState Downloading\n", MODULE_PREFIX);
    }
    else
    {
        if (_wifiClient.connected())
        {
            _wifiClient.stop();
            Log.verbose("%sStopped existing TCP conn\n", MODULE_PREFIX);
        }
        _responseHeader.reserve(1);
        Log.trace("%sSetState Idle\n", MODULE_PREFIX);
    }
}

bool RdOTAUpdate::checkVersionString(const char *pNewVersionStr)
{
    return (strcmp(pNewVersionStr, _currentVers.c_str()) > 0);
}

bool RdOTAUpdate::updateStart(unsigned int updateFileLen)
{
    // Check the update can be started
    _targetFileLength = updateFileLen;
    _updateBytesWritten = 0;
    bool enoughSpace = Update.begin(updateFileLen, U_FLASH);
    if (!enoughSpace)
    {
        Log.notice("%sNot enough space for file length %d\n", MODULE_PREFIX, _targetFileLength);
        return false;
    }

    // Started ok
    Log.notice("%sUpdate has enough space\n", MODULE_PREFIX);
    Update.setMD5(_targetMD5.c_str());
    _updateHasBeenStarted = true;
    return true;
}

void RdOTAUpdate::updateChunk(uint8_t *pData, int dataReceivedLen)
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
        Log.trace("%sFlashing failed - no data written - should be %d\n", MODULE_PREFIX, dataReceivedLen);
    }
}

bool RdOTAUpdate::updateCheckComplete()
{
    if (Update.isFinished())
    {
        Update.end();
        _updateHasBeenStarted = false;
        return true;
    }
    return false;
}

String RdOTAUpdate::extractValueByName(String &nameValuesStr, const char *pValueName)
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
    Log.verbose("%sValue %s not found in info file\n", MODULE_PREFIX, pValueName);
    return "";
}

void RdOTAUpdate::onDataReceived(uint8_t *pDataReceived, size_t dataReceivedLen)
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
                        Log.notice("%sFile not found\n", MODULE_PREFIX);
                        setState(OTA_UPDATE_STATE_IDLE);
                        return;
                    }
                    // Extract version
                    String fileVersionStr = extractValueByName(_fileInfo, "version");
                    String fileName = extractValueByName(_fileInfo, "filename");
                    String fileMD5 = extractValueByName(_fileInfo, "MD5");
                    Log.notice("%sLatestVersion %s, Filename %s, MD5 %s\n", MODULE_PREFIX, fileVersionStr.c_str(), fileName.c_str(), fileMD5.c_str());
                    if (fileVersionStr.length() == 0)
                    {
                        Log.trace("%sCannot extract version from fileInfo\n", MODULE_PREFIX);
                        setState(OTA_UPDATE_STATE_IDLE);
                        return;
                    }
                    if (fileName.length() == 0)
                    {
                        Log.trace("%sFile name invalid\n", MODULE_PREFIX);
                        setState(OTA_UPDATE_STATE_IDLE);
                        return;
                    }
                    // Check version against current
                    Log.notice("%sRunningVersion is %s\n", MODULE_PREFIX, _currentVers.c_str());
                    bool versionNeedsUpdating = checkVersionString(fileVersionStr.c_str());
                    if (versionNeedsUpdating)
                    {
                        _targetVersionStr = fileVersionStr;
                        _targetFilename = fileName;
                        _targetMD5 = fileMD5;
                        Log.notice("%sStarting update to version %s with MD5 %s\n", MODULE_PREFIX,
                                   _targetVersionStr.c_str(), _targetMD5.c_str());
                        startDownloadProcess();
                        return;
                    }
                    else
                    {
                        Log.notice("%sNo update required\n", MODULE_PREFIX);
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
                        Log.notice("%sDownloading started\n", MODULE_PREFIX);
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

void RdOTAUpdate::directFirmwareUpdatePart(String &filename, size_t contentLen, size_t index,
                                           uint8_t *data, size_t len, bool finalBlock)
{
    // Check enabled
    if (!_otaDirectEnabled)
    {
        Log.warning("%sapiESPFirmwarePart OTA Direct Disabled\n", MODULE_PREFIX);
        return;
    }
    // Log.trace("%sapiESPFirmwarePart %d, %d, %d, %d\n", MODULE_PREFIX, contentLen, index, len, finalBlock);
    const esp_partition_t* update_partition = esp_ota_get_next_update_partition(NULL);
    // Check if first part
    if (index == 0)
    {

        Log.warning("%sapiESPFirmwarePart first block contentLen %d len %d\n", MODULE_PREFIX, contentLen, len);

        const esp_partition_t *configured = esp_ota_get_boot_partition();
        const esp_partition_t *running = esp_ota_get_running_partition();

        if (configured != running) 
        {
            Log.warning("%sapiESPFirmwarePart configured OTA boot partition at offset 0x%x, but running from offset 0x%x\n",
                        MODULE_PREFIX, configured->address, running->address);
            Log.warning("%sapiESPFirmwarePart (this can happen if either the OTA boot data or preferred boot image become corrupted somehow.)\n",
                        MODULE_PREFIX);
        }
        Log.warning("%sapiESPFirmwarePart running partition type %d subtype %d (offset 0x%x)\n",
                    MODULE_PREFIX, running->type, running->subtype, running->address);

        Log.warning("%swriting to partition subtype %d at offset 0x%x\n",
                MODULE_PREFIX, update_partition->subtype, update_partition->address);

        esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &_otaDirectUpdateHandle);
        if (err != ESP_OK) 
        {
            Log.warning("%sesp_ota_begin failed, error=%d\n", MODULE_PREFIX, err);
        }
        else
        {
            _otaDirectInProgress = true;
            Log.warning("%sesp_ota_begin succeeded\n", MODULE_PREFIX);
        }
    }

    // Check if in progress
    if (_otaDirectInProgress)
    {
        esp_err_t err = esp_ota_write(_otaDirectUpdateHandle, (const void *)data, len);
        if (err != ESP_OK) 
        {
            Log.warning("%sesp_ota_write failed! err=0x%x\n", MODULE_PREFIX, err);
        }
    }

    // Check if final block
    if (finalBlock && _otaDirectInProgress)
    {
        _otaDirectInProgress = false;
        if (esp_ota_end(_otaDirectUpdateHandle) != ESP_OK) 
        {
            Log.warning("%sesp_ota_end failed!\n", MODULE_PREFIX);
        }
        else
        {
            esp_err_t err = esp_ota_set_boot_partition(update_partition);
            if (err != ESP_OK) 
            {
                Log.warning("%sesp_ota_set_boot_partition failed! err=0x%x\n", MODULE_PREFIX, err);
            }
            else
            {
                Log.notice("%sesp_ota_set_boot_partition ok ... reboot pending\n", MODULE_PREFIX, err);
                _directUpdateRestartPendingStartMs = millis();
                _directUpdateRestartPending = true;
            }
        }
    }
}

void RdOTAUpdate::directFirmwareUpdateDone()
{
    if (_otaDirectInProgress)
    {
        esp_ota_end(_otaDirectUpdateHandle);
    }
    Log.trace("%sapiESPFirmwareUpdate DONE\n", MODULE_PREFIX);
}
