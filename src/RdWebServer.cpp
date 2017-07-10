// Web Server (target Particle devices e.g. RedBear Duo)
// Rob Dobson 2012-2017

#include "application.h"
#include "RdWebServer.h"
#include "Utils.h"

RdWebClient::RdWebClient()
{
    _webClientState        = WEB_CLIENT_NONE;
    _webClientStateEntryMs = 0;
    _pResourceToSend       = NULL;
    _resourceSendIdx       = 0;
    _resourceSendBlkCount  = 0;
    _resourceSendMillis    = 0;
    _pHttpReqPayload       = NULL;
    _httpReqPayloadLen     = 0;
    _curHttpPayloadRxPos   = 0;
}

RdWebClient::~RdWebClient()
{
    cleanUp();
}

void RdWebClient::cleanUp()
{
    delete _pHttpReqPayload;
}

void RdWebClient::setState(WebClientState newState)
{
    _webClientState        = newState;
    _webClientStateEntryMs = millis();
    if (newState != WEB_CLIENT_SEND_RESOURCE && newState != WEB_CLIENT_SEND_RESOURCE_WAIT)
        Log.trace("WebClient %d State: %s", _clientIdx, connStateStr());
}


const char *RdWebClient::connStateStr()
{
    switch (_webClientState)
    {
    case WEB_CLIENT_NONE:
        return "None";

    case WEB_CLIENT_ACCEPTED:
        return "Accepted";

    case WEB_CLIENT_SEND_RESOURCE_WAIT:
        return "Wait";

    case WEB_CLIENT_SEND_RESOURCE:
        return "Send";
    }
    return "Unknown";
}


//////////////////////////////////
// Handle the client state machine
void RdWebClient::service(RdWebServer *pWebServer)
{
    // Handle different states
    switch (_webClientState)
    {
    case WEB_CLIENT_NONE:
        // See if a connection is ready to be accepted
        _TCPClient = pWebServer->available();
        if (_TCPClient)
        {
            // Now connected
            setState(WEB_CLIENT_ACCEPTED);
            _httpReqStr = "";
            _httpReqPayloadLen = 0;
            // Info
            IPAddress ip    = _TCPClient.remoteIP();
            String    ipStr = ip;
            Log.info("WebClient IP %s", ipStr.c_str());
        }
        break;

    case WEB_CLIENT_ACCEPTED:
       {
           // Check if client is still connected
           if (!_TCPClient.connected())
           {
               Log.trace("WebClient disconnected");
               _TCPClient.stop();
               setState(WEB_CLIENT_NONE);
               break;
           }

           // Anything available?
           int numBytesAvailable = _TCPClient.available();
           int numToRead         = numBytesAvailable;
           if (numToRead > 0)
           {
               // Make sure we don't overflow buffers
               if (numToRead > MAX_CHS_IN_SERVICE_LOOP)
               {
                   numToRead = MAX_CHS_IN_SERVICE_LOOP;
               }
               if (_httpReqStr.length() + numToRead > HTTPD_MAX_REQ_LENGTH)
               {
                   numToRead = HTTPD_MAX_REQ_LENGTH - _httpReqStr.length();
               }
           }

           // Check if we want to read
           if (numToRead > 0)
           {
               // Read from TCP client
               uint8_t *tmpBuf = new uint8_t[numToRead + 1];
               int     numRead = _TCPClient.read(tmpBuf, numToRead);
               tmpBuf[numToRead] = '\0';

               // Handle either by contactenating to the _httpReqStr or to the _pHttpReqPayload
               if (_httpReqPayloadLen == 0)
               {
                   if ((_httpReqStr.length() + strlen((const char*)tmpBuf) < HTTPD_MAX_REQ_LENGTH))
                        _httpReqStr.concat((char *)tmpBuf);
               }
               else
               {
                   for (int i = 0; i < numRead; i++)
                   {
                       if (_curHttpPayloadRxPos >= _httpReqPayloadLen)
                            break;
                       _pHttpReqPayload[_curHttpPayloadRxPos++] = tmpBuf[i];
                   }
                   // Ensure null terminated
                   _pHttpReqPayload[_curHttpPayloadRxPos] = 0;
               }
               delete [] tmpBuf;

               // Check if header complete
               bool headerComplete = _httpReqStr.indexOf("\r\n\r\n") >= 0;

               if (headerComplete && (_httpReqPayloadLen == 0))
               {
                  _httpReqPayloadLen = getContentLengthFromHeader(_httpReqStr);
                   _curHttpPayloadRxPos = 0;
                   // We have to ignore payloads that are too big for our memory
                   if (_httpReqPayloadLen > HTTP_MAX_PAYLOAD_LENGTH)
                   {
                       _httpReqPayloadLen = 0;
                   }
                   else
                   {
                       delete [] _pHttpReqPayload;
                       // Add space for null terminator
                       _pHttpReqPayload = new unsigned char[_httpReqPayloadLen+1];
                   }
               }

               // Check for completion
               if (headerComplete && (_httpReqPayloadLen == _curHttpPayloadRxPos))
               {
                   RdWebServerResourceDescr* pResToRespondWith = NULL;
                   Log.trace("WebClient received %d", _httpReqStr.length());
                   bool handledOk = false;
                   _pResourceToSend = handleReceivedHttp(handledOk, pWebServer);
                    // clean the received resources
                    _httpReqPayloadLen = 0;
                    delete _pHttpReqPayload;
                    _httpReqStr = "";
                    // Get ready to send the response (in sections as needed)
                    _resourceSendIdx = 0;
                    _resourceSendBlkCount = 0;
                    _resourceSendMillis   = millis();
                    // Wait until response complete
                    setState(WEB_CLIENT_SEND_RESOURCE_WAIT);
                   if (!handledOk)
                   {
                       Log.info("WebClient couldn't handle request");
                   }
               }
               else
               {
                   // Restart state timer to ensure timeout only happens when there is no data
                   _webClientStateEntryMs = millis();
               }
           }
           // Check for having been in this state for too long
           if (Utils::isTimeout(millis(), _webClientStateEntryMs, MAX_MS_IN_CLIENT_STATE_WITHOUT_DATA))
           {
               Log.info("WebClient no-data timeout");
               _TCPClient.stop();
               setState(WEB_CLIENT_NONE);
           }
           break;
       }

    case WEB_CLIENT_SEND_RESOURCE_WAIT:
        {
            // Check how long to wait
            unsigned long msToWait = MS_WAIT_AFTER_LAST_TCP_FRAME;
            if ((_pResourceToSend != NULL) && (_resourceSendIdx < _pResourceToSend->_dataLen))
                msToWait = MS_WAIT_BETWEEN_TCP_FRAMES;

            // Check for timeout on resource send
            if (Utils::isTimeout(millis(), _resourceSendMillis, msToWait))
            {
                setState(WEB_CLIENT_SEND_RESOURCE);
            }
            break;
        }
    case WEB_CLIENT_SEND_RESOURCE:
       {
           if (!_pResourceToSend)
           {
               // Close connection and finish
               _TCPClient.stop();
               setState(WEB_CLIENT_NONE);
               Log.info("WebClient resp complete");
               break;
           }
           // Send data in chunks based on limited buffer sizes in TCP stack
           if (_resourceSendIdx >= _pResourceToSend->_dataLen)
           {
               // Completed - close client
               _TCPClient.stop();
               setState(WEB_CLIENT_NONE);
               Log.info("WebClient Sent %s, %d bytes total, %d blocks",
                        _pResourceToSend->_pResId, _pResourceToSend->_dataLen, _resourceSendBlkCount);
               break;
           }

           // Get point and length of next chunk
           const unsigned char *pMem       = _pResourceToSend->_pData + _resourceSendIdx;
           int                 toSendBytes = _pResourceToSend->_dataLen - _resourceSendIdx;
           if (toSendBytes > HTTPD_MAX_RESP_CHUNK_SIZE)
               toSendBytes = HTTPD_MAX_RESP_CHUNK_SIZE;

           // Send next chunk
           /*Log.trace("WebClient Writing %d bytes (%d) = %02x %02x %02x %02x ... %02x %02x %02x", toSendBytes, _resourceSendIdx,
                     pMem[0], pMem[1], pMem[2], pMem[3], pMem[toSendBytes - 3], pMem[toSendBytes - 2], pMem[toSendBytes - 1]);*/

           _TCPClient.write(pMem, toSendBytes);
           _TCPClient.flush();
           _resourceSendIdx += toSendBytes;
           _resourceSendBlkCount++;
           _resourceSendMillis = millis();
           setState(WEB_CLIENT_SEND_RESOURCE_WAIT);
           break;
       }
    }
}


//////////////////////////////////////
// Handle an HTTP request
RdWebServerResourceDescr* RdWebClient::handleReceivedHttp(bool& handledOk, RdWebServer *pWebServer)
{
    handledOk = false;
    RdWebServerResourceDescr* pResourceToRespondWith = NULL;

    // Request string
    const char* pHttpReq = _httpReqStr.c_str();
    int httpReqLen = _httpReqStr.length();

    // Get HTTP method
    int httpMethod = METHOD_OTHER;
    if (strncmp(pHttpReq, "GET ", 4) == 0)
    {
        httpMethod = METHOD_GET;
    }
    else if (strncmp(pHttpReq, "POST", 4) == 0)
    {
        httpMethod = METHOD_POST;
    }
    else if (strncmp(pHttpReq, "OPTIONS", 7) == 0)
    {
        httpMethod = METHOD_OPTIONS;
    }

    // See if there is a valid HTTP command
    String endpointStr, argStr;
    if (extractEndpointArgs(pHttpReq + 3, endpointStr, argStr))
    {
        // Received cmd and arguments
        Log.trace("WebClient handleHTTP EndPtStr %s ArgStr %s", endpointStr.c_str(), argStr.c_str());

        // Handle REST API commands
        RestAPIEndpointDef *pEndpoint = pWebServer->getEndpoint(endpointStr);
        if (pEndpoint)
        {
            Log.trace("WebClient FoundEndpoint <%s> Type %d", endpointStr.c_str(), pEndpoint->_endpointType);
            if (pEndpoint->_endpointType == RestAPIEndpointDef::ENDPOINT_CALLBACK)
            {
                String retStr;
                RestAPIEndpointMsg apiMsg(httpMethod, endpointStr.c_str(), argStr.c_str(), pHttpReq);
                apiMsg._pMsgContent = _pHttpReqPayload;
                apiMsg._msgContentLen = _httpReqPayloadLen;
                (pEndpoint->_callback)(apiMsg, retStr);
                Log.trace("WebClient api response len %d", retStr.length());
                /*delay(50);*/
                formHTTPResponse(_httpRespStr, "200 OK", "application/json", retStr.c_str(), -1);
                Log.trace("WebClient http response len %d", _httpRespStr.length());
                /*delay(50);*/
                _TCPClient.write((uint8_t *)_httpRespStr.c_str(), _httpRespStr.length());
                Log.trace("WebClient write to tcp clientIdx %d len %d", _clientIdx, _httpRespStr.length());
                delay(50);
                _TCPClient.flush();
                handledOk = true;
            }
        }

        // Look for the command in the static resources
        if (!handledOk)
        {
            for (int wsResIdx = 0; wsResIdx < pWebServer->getNumResources(); wsResIdx++)
            {
                RdWebServerResourceDescr *pRes = pWebServer->getResource(wsResIdx);
                if ((strcasecmp(pRes->_pResId, endpointStr) == 0) ||
                    ((strlen(endpointStr) == 0) && (strcasecmp(pRes->_pResId, "index.html") == 0)))
                {
                    if (pRes->_pData != NULL)
                    {
                        Log.trace("WebClient sending resource %s, %d bytes, %s",
                                  pRes->_pResId, pRes->_dataLen, pRes->_pMimeType);
                        // Form header
                        formHTTPResponse(_httpRespStr, "200 OK", pRes->_pMimeType, "", pRes->_dataLen);
                        _TCPClient.write((uint8_t *)_httpRespStr.c_str(), _httpRespStr.length());
                        /*const char *pBuff   = _httpRespStr.c_str();
                        const int  bufffLen = strlen(pBuff);
                        Log.trace("Header %d = %02x %02x %02x ... %02x %02x %02x", _httpRespStr.length(),
                                  pBuff[0], pBuff[1], pBuff[2], pBuff[bufffLen - 3], pBuff[bufffLen - 2], pBuff[bufffLen - 1]);*/
                        _TCPClient.flush();
                        // Respond with static resource
                        pResourceToRespondWith = pRes;
                        handledOk             = true;
                    }
                    break;
                }
            }
        }

        // If not handled ok
        if (!handledOk)
        {
            Log.info("WebClient Endpoint %s not found or invalid", endpointStr.c_str());
        }
    }
    else
    {
        Log.info("WebClient Cannot find command or args");
    }

    // Handle situations where the command wasn't handled ok
    if (!handledOk)
    {
        Log.info("WebClient Returning 404 Not found");
        formHTTPResponse(_httpRespStr, "404 Not Found", "text/plain", "404 Not Found", -1);
        _TCPClient.write((uint8_t *)_httpRespStr.c_str(), _httpRespStr.length());
    }
    return pResourceToRespondWith;
}


//////////////////////////////////////
// Extract arguments from rest api string
bool RdWebClient::extractEndpointArgs(const char *buf, String& endpointStr, String& argStr)
{
    if (buf == NULL)
        return false;

    // Check for first slash
    char *pSlash1 = strchr(buf, '/');
    if (pSlash1 == NULL)
    {
        return false;
    }
    pSlash1++;
    // Extract command
    while (*pSlash1)
    {
        if ((*pSlash1 == '/') || (*pSlash1 == ' ') || (*pSlash1 == '\n') ||
            (*pSlash1 == '?') || (*pSlash1 == '&'))
        {
            break;
        }
        endpointStr.concat(*pSlash1++);
    }
    if ((*pSlash1 == '\0') || (*pSlash1 == ' ') || (*pSlash1 == '\n'))
    {
        return true;
    }
    // Now args
    pSlash1++;
    while (*pSlash1)
    {
        if ((*pSlash1 == ' ') || (*pSlash1 == '\n'))
        {
            break;
        }
        argStr.concat(*pSlash1++);
    }
    // Convert escaped characters
    RestAPIEndpoints::unencodeHTTPChars(endpointStr);
    RestAPIEndpoints::unencodeHTTPChars(argStr);
    return true;
}

int RdWebClient::getContentLengthFromHeader(const char *msgBuf)
{
    char *ptr = strstr(msgBuf, "Content-Length:");

    if (ptr)
    {
        ptr += 15;
        int contentLen = atoi(ptr);
        if (contentLen >= 0)
        {
            return contentLen;
        }
    }
    return 0;
}


// Form a header to respond
void RdWebClient::formHTTPResponse(String& respStr, const char *rsltCode,
                                   const char *contentType, const char *respBody, int contentLen)
{
    if (contentLen == -1)
    {
        contentLen = strlen(respBody);
    }
    respStr = String::format("HTTP/1.1 %s\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: %s\r\nConnection: close\r\nContent-Length: %d\r\n\r\n%s", rsltCode, contentType, contentLen, respBody);
}


RdWebServer::RdWebServer()
{
    _pTCPServer            = NULL;
    _pRestAPIEndpoints     = NULL;
    _pWebServerResources   = NULL;
    _numWebServerResources = 0;
    _TCPPort               = 80;
    _webServerState        = WEB_SERVER_STOPPED;
    _webServerStateEntryMs = 0;
    _numWebServerResources = 0;
    _webServerActiveLastUnixTime = 0;
    // Configure each client
    for (int clientIdx = 0; clientIdx < MAX_WEB_CLIENTS; clientIdx++)
        _webClients[clientIdx].setClientIdx(clientIdx);
}


// Destructor
RdWebServer::~RdWebServer()
{
}


const char *RdWebServer::connStateStr()
{
    switch (_webServerState)
    {
    case WEB_SERVER_STOPPED:
        return "Stopped";

    case WEB_SERVER_WAIT_CONN:
        return "WaitConn";

    case WEB_SERVER_BEGUN:
        return "Begun";
    }
    return "Unknown";
}


char RdWebServer::connStateChar()
{
    switch (_webServerState)
    {
    case WEB_SERVER_STOPPED:
        return '0';

    case WEB_SERVER_WAIT_CONN:
        return 'W';

    case WEB_SERVER_BEGUN:
        return 'B';
    }
    return 'K';
}


void RdWebServer::setState(WebServerState newState)
{
    _webServerState        = newState;
    _webServerStateEntryMs = millis();
    Log.trace("WebServerState: %s", connStateStr());
}


void RdWebServer::start(int port)
{
    Log.trace("WebServerStart");
    // Check if already started
    if (_pTCPServer)
    {
        stop();
    }
    // Create server and begin
    _TCPPort    = port;
    _pTCPServer = new TCPServer(_TCPPort);
    setState(WEB_SERVER_WAIT_CONN);
}


void RdWebServer::stop()
{
    if (_pTCPServer)
    {
        _pTCPServer->stop();
        // Delete previous server
        delete _pTCPServer;
        _pTCPServer = NULL;
    }
    setState(WEB_SERVER_STOPPED);
}


TCPClient RdWebServer::available()
{
    if (_pTCPServer)
    {
        return _pTCPServer->available();
    }
    return NULL;
}


//////////////////////////////////////
// Handle the connection state machine
void RdWebServer::service()
{
    // Handle different states
    switch (_webServerState)
    {
    case WEB_SERVER_STOPPED:
        break;

    case WEB_SERVER_WAIT_CONN:
        if (WiFi.ready())
        {
            start(_TCPPort);
            if (_pTCPServer)
            {
                Log.info("WebServer TCPServer Begin");
                _pTCPServer->begin();
                setState(WEB_SERVER_BEGUN);
            }
        }
        break;

    case WEB_SERVER_BEGUN:
        // Service the clients
        for (int clientIdx = 0; clientIdx < MAX_WEB_CLIENTS; clientIdx++)
        {
            _webClients[clientIdx].service(this);

            if (_webClients[clientIdx].clientIsActive())
                _webServerActiveLastUnixTime = Time.now();
        }
        break;
    }
}


//////////////////////////////////////
// Add resources to the web server
void RdWebServer::addStaticResources(RdWebServerResourceDescr *pResources, int numResources)
{
    _pWebServerResources   = pResources;
    _numWebServerResources = numResources;
}


// Add endpoints to the web server
void RdWebServer::addRestAPIEndpoints(RestAPIEndpoints *pRestAPIEndpoints)
{
    _pRestAPIEndpoints = pRestAPIEndpoints;
}
