// Web Server
// Rob Dobson 2012-2017

#pragma once

#include "RdWebServerResources.h"
#include "RestAPIEndpoints.h"

class RdWebServer;

class RdWebClient
{
private:
    // Max length of an http request
    static const int HTTPD_MAX_REQ_LENGTH = 2048;

    // Max payload of a message
    static const int HTTP_MAX_PAYLOAD_LENGTH = 2048;

    // Each call to service() process max this number of chars received from a TCP connection
    static const int MAX_CHS_IN_SERVICE_LOOP = 500;

    // Timeouts
    static const unsigned long MAX_MS_IN_CLIENT_STATE_WITHOUT_DATA = 2000;

    // Time between TCP frames
    static const unsigned long MS_WAIT_BETWEEN_TCP_FRAMES = 10;
    static const unsigned long MS_WAIT_AFTER_LAST_TCP_FRAME = 200;

    // Max chunk size of HTTP response sending
    static const int HTTPD_MAX_RESP_CHUNK_SIZE = 500;

    // TCP client
    TCPClient _TCPClient;

    // Methods
    static const int METHOD_OTHER   = 0;
    static const int METHOD_GET     = 1;
    static const int METHOD_POST    = 2;
    static const int METHOD_OPTIONS = 3;

public:
    RdWebClient();
    ~RdWebClient();

    void setClientIdx(int clientIdx)
    {
        _clientIdx = clientIdx;
    }
    void service(RdWebServer *pWebServer);

    enum WebClientState
    {
        WEB_CLIENT_NONE, WEB_CLIENT_ACCEPTED, WEB_CLIENT_SEND_RESOURCE_WAIT, WEB_CLIENT_SEND_RESOURCE
    };

    void setState(WebClientState newState);
    const char *connStateStr();
    WebClientState clientConnState()
    {
        return _webClientState;
    }

    // Process HTTP Request
    RdWebServerResourceDescr* handleReceivedHttp(bool& handledOk, RdWebServer *pWebServer);
private:
    // Current client state
    WebClientState _webClientState;
    unsigned long  _webClientStateEntryMs;

    // HTTP Request
    String _httpReqStr;

    // HTTP Request payload
    unsigned char* _pHttpReqPayload;
    int _httpReqPayloadLen;

    // HTTP payload while being received
    int _curHttpPayloadRxPos;

    // HTTP response
    String _httpRespStr;

    // Resource to send
    RdWebServerResourceDescr* _pResourceToSend;
    int _resourceSendIdx;
    int _resourceSendBlkCount;
    unsigned long _resourceSendMillis;

    // Index of client - for debug
    int _clientIdx;

private:
    // cleanUp
    void cleanUp();

    // Helpers
    static int getContentLengthFromHeader(const char *msgBuf);

    // Extract endpoint arguments
    static bool extractEndpointArgs(const char *buf, String& endpointStr, String& argStr);

    // Utility
    static void formStringFromCharBuf(String& outStr, char *pStr, int len);

    // Form HTTP response
    static void formHTTPResponse(String& respStr, const char *rsltCode,
                          const char *contentType, const char *respBody, int contentLen);

};

class RdWebServer
{
public:
    RdWebServer();
    virtual ~RdWebServer();

    void start(int port);
    void stop();
    void service();
    const char *connStateStr();
    char connStateChar();

    int serverConnState()
    {
        return _webServerState;
    }


    int clientConnections()
    {
        int connCount = 0;
        for (int clientIdx = 0; clientIdx < MAX_WEB_CLIENTS; clientIdx++)
        {
            if (_webClients[clientIdx].clientConnState() == RdWebClient::WebClientState::WEB_CLIENT_ACCEPTED)
            {
                connCount++;
            }
        }
        return connCount;
    }

    // Add endpoints to the web server
    void addRestAPIEndpoints(RestAPIEndpoints *pRestAPIEndpoints);

    // Add resources to the web server
    void addStaticResources(RdWebServerResourceDescr *pResources, int numResources);

    // resources
    int getNumResources()
    {
        return _numWebServerResources;
    }

    RdWebServerResourceDescr* getResource(int resIdx)
    {
        if (!_pWebServerResources)
            return NULL;
        if ((resIdx >= 0) && (resIdx < _numWebServerResources))
            return _pWebServerResources + resIdx;
        return NULL;
    }

    // Endpoints
    RestAPIEndpointDef* getEndpoint(const char* endpointStr)
    {
        if (!_pRestAPIEndpoints)
            return NULL;
        return _pRestAPIEndpoints->getEndpoint(endpointStr);
    }

public:
    // Get an available client
    TCPClient available();

private:
    // Clients
    static const int MAX_WEB_CLIENTS = 1;

private:
    // Port
    int _TCPPort;

    // TCP server
    TCPServer *_pTCPServer;

    // Client
    RdWebClient _webClients[MAX_WEB_CLIENTS];

    // Possible states of web server
public:
    enum WebServerState
    {
        WEB_SERVER_STOPPED, WEB_SERVER_WAIT_CONN, WEB_SERVER_BEGUN
    };

private:
    // Current web-server state
    WebServerState _webServerState;
    unsigned long  _webServerStateEntryMs;

    // REST API Endpoints
    RestAPIEndpoints *_pRestAPIEndpoints;

    // Web server resources
    RdWebServerResourceDescr *_pWebServerResources;
    int _numWebServerResources;

    // Utility
    void setState(WebServerState newState);


};
