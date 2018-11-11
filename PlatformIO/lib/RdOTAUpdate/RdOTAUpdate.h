// Over-The-Air (OTA) Firmware Update
// Rob Dobson 2018

#pragma once

#include <HTTPClient.h>
#include <WifiClient.h>
#include <ArduinoLog.h>
#include "Utils.h"
#include "ConfigBase.h"

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

    // Setup
    void setup(ConfigBase& config, const char *projectName, const char *currentVers);

    // Request update check - actually done in the service loop
    void requestUpdateCheck();

    // Check if update in progress
    bool isInProgress();

    // Call this frequently
    void service();

private:
    // Start an update process
    void startUpdateProcess();

    // Start a version check
    void startVersionCheck();

    // Start download process
    void startDownloadProcess();

    // Abort the update process
    void abortUpdateProcess();

    // Set the state
    void setState(OTAUpdateState newState);

    // Check version string
    bool checkVersionString(const char *pNewVersionStr);

    // Update start
    bool updateStart(unsigned int updateFileLen);

    // Handle a chunk in the update flow
    void updateChunk(uint8_t *pData, int dataReceivedLen);

    // Update check complete
    bool updateCheckComplete();

    // Extract a value by name
    String extractValueByName(String &nameValuesStr, const char *pValueName);

    // Handle received data
    void onDataReceived(uint8_t *pDataReceived, size_t dataReceivedLen);
};
