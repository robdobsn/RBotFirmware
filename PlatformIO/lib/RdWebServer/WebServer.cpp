// WebServer
// Rob Dobson 2012-2018

#include <WebServer.h>
#if defined (ESP8266)
#include "ESPAsyncTCP.h"
#else
#include "AsyncTCP.h"
#endif
#include <ESPAsyncWebServer.h>
#include "RestAPIEndpoints.h"
#include "ConfigPinMap.h"
#include "WebServerResource.h"
#include "AsyncStaticFileHandler.h"

static const char* MODULE_PREFIX = "WebServer: ";

WebServer::WebServer()
{
    _pServer = NULL;
    _begun = false;
    _webServerEnabled = false;
    _pAsyncEvents = NULL;
}

WebServer::~WebServer()
{
    if (_pServer)
        delete _pServer;
}

void WebServer::setup(ConfigBase& hwConfig)
{
    // Enable
    _webServerEnabled = hwConfig.getLong("webServerEnabled", 0) != 0;

    // Create server
    if (_webServerEnabled)
    {
        int port = hwConfig.getLong("webServerPort", 80);
        _pServer = new AsyncWebServer(port);
    }
}

String WebServer::recreatedReqUrl(AsyncWebServerRequest *request)
{
    String reqUrl = request->url();
    // Add parameters
    int numArgs = request->args();
    for(int i = 0; i < numArgs; i++)
    {
        if (i == 0)
            reqUrl += "?";
        else
            reqUrl += "&";
        reqUrl = reqUrl + request->argName(i) + "=" + request->arg(i);
    }
    return reqUrl;
}

void WebServer::addEndpoints(RestAPIEndpoints &endpoints)
{
    // Check enabled
    if (!_pServer)
        return;

    // Number of endpoints to add
    int numEndPoints = endpoints.getNumEndpoints();

    // Add each endpoint
    for (int i = 0; i < numEndPoints; i++)
    {
        // Get endpoint info
        RestAPIEndpointDef *pEndpoint = endpoints.getNthEndpoint(i);
        if (!pEndpoint)
            continue;

        // Mapping of GET/POST/ETC
        int webMethod = HTTP_GET;
        switch (pEndpoint->_endpointMethod)
        {
        case RestAPIEndpointDef::ENDPOINT_PUT:
            webMethod = HTTP_PUT;
            break;
        case RestAPIEndpointDef::ENDPOINT_POST:
            webMethod = HTTP_POST;
            break;
        case RestAPIEndpointDef::ENDPOINT_DELETE:
            webMethod = HTTP_DELETE;
            break;
        default:
            webMethod = HTTP_GET;
            break;
        }

        // Debug
        Log.trace("%sAdding API %s type %s\n", MODULE_PREFIX, pEndpoint->_endpointStr.c_str(), 
                    endpoints.getEndpointTypeStr(pEndpoint->_endpointType));

        // Handle the endpoint
        _pServer->on(("/" + pEndpoint->_endpointStr).c_str(), webMethod, 
        
            // Handler for main request URL
            [pEndpoint](AsyncWebServerRequest *request) {
                // Default response
                String respStr("{ \"rslt\": \"unknown\" }");

                // Make the required action
                if (pEndpoint->_endpointType == RestAPIEndpointDef::ENDPOINT_CALLBACK)
                {
                    String reqUrl = recreatedReqUrl(request);
                    Log.verbose("%sCalling %s url %s\n", MODULE_PREFIX,
                                    pEndpoint->_endpointStr.c_str(), request->url().c_str());
                    pEndpoint->callback(reqUrl, respStr);
                }
                else
                {
                    Log.trace("%sUnknown type %s url %s\n", MODULE_PREFIX,
                                    pEndpoint->_endpointStr.c_str(), request->url().c_str());
                }
                request->send(200, "application/json", respStr.c_str());
            },
            
            // Handler for upload (as in a file upload)
            [pEndpoint](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool finalBlock) {
                String reqUrl = recreatedReqUrl(request);
                pEndpoint->callbackUpload(reqUrl, filename, 
                            request ? request->contentLength() : 0, 
                            index, data, len, finalBlock);
            },
            
            // Handler for body
            [pEndpoint](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
                String reqUrl = recreatedReqUrl(request);
                pEndpoint->callbackBody(reqUrl, data, len, index, total);
            }
        );
    }
}

void WebServer::begin(bool accessControlAllowOriginAll)
{
    if (_pServer && !_begun)
    {
        if (accessControlAllowOriginAll)
            DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
        _pServer->begin();
        _begun = true;
        Log.notice("%sbegun\n", MODULE_PREFIX);
    }
}

// Add resources to the web server
void WebServer::addStaticResources(const WebServerResource *pResources, int numResources)
{
    // Add each resource
    for (int i = 0; i < numResources; i++)
    {
        String alias = pResources[i]._pResId;
        alias.trim();
        if (!alias.startsWith("/"))
            alias = "/" + alias;
        addStaticResource(&(pResources[i]), alias.c_str());
        // Check for index.html and alias to "/"
        if (strcasecmp(pResources[i]._pResId, "index.html") == 0)
        {
            addStaticResource(&(pResources[i]), "/");
        }
    }
}

void WebServer::parseAndAddHeaders(AsyncWebServerResponse *response, const char *pHeaders)
{
    String headerStr(pHeaders);
    int strPos = 0;
    while (strPos < headerStr.length())
    {
        int sepPos = headerStr.indexOf(":", strPos);
        if (sepPos == -1)
            break;
        String headerName = headerStr.substring(strPos, sepPos);
        headerName.trim();
        int lineEndPos = std::max(headerStr.indexOf("\r"), headerStr.indexOf("\n"));
        if (lineEndPos == -1)
            lineEndPos = headerStr.length() - 1;
        String headerVal = headerStr.substring(sepPos + 1, lineEndPos);
        headerVal.trim();
        if ((headerName.length() > 0) && (headerVal.length() > 0))
        {
            Log.notice("%sAdding header %s: %s", MODULE_PREFIX, headerName.c_str(), headerVal.c_str());
        }
        // Move to next header
        strPos = lineEndPos + 1;
    }
}

void WebServer::addStaticResource(const WebServerResource *pResource, const char *pAliasPath)
{
    // Check enabled
    if (!_pServer)
        return;

    // Check for alias
    const char *pPath = pResource->_pResId;
    if (pAliasPath != NULL)
        pPath = pAliasPath;

    // Debug
    Log.trace("%sAdding static resource %s mime %s encoding %s len %d\n", MODULE_PREFIX, pPath,
                pResource->_pMimeType, pResource->_pContentEncoding, pResource->_dataLen);

    // Static pages
    _pServer->on(pPath, HTTP_GET, [pResource](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", pResource->_pData, pResource->_dataLen);
        if ((pResource->_pContentEncoding != NULL) && (strlen(pResource->_pContentEncoding) != 0))
            response->addHeader("Content-Encoding", pResource->_pContentEncoding);
        if ((pResource->_pAccessControlAllowOrigin != NULL) && (strlen(pResource->_pAccessControlAllowOrigin) != 0))
            response->addHeader("Access-Control-Allow-Origin", pResource->_pAccessControlAllowOrigin);
        if (pResource->_noCache)
            response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        if ((pResource->_pExtraHeaders != NULL) && (strlen(pResource->_pExtraHeaders) != 0))
            parseAndAddHeaders(response, pResource->_pExtraHeaders);
        request->send(response);
    });
}

void WebServer::serveStaticFiles(const char* baseUrl, const char* baseFolder, const char* cache_control)
{
    // Check enabled
    if (!_pServer)
        return;
    
    // Handle file systems
    Log.trace("%sserveStaticFiles url %s folder %s\n", MODULE_PREFIX, baseUrl, baseFolder);
    AsyncStaticFileHandler* handler = new AsyncStaticFileHandler(baseUrl, baseFolder, cache_control);
    _pServer->addHandler(handler);
}

void WebServer::enableAsyncEvents(const String& eventsURL)
{
    // Enable events
    if (_pAsyncEvents)
        return;
    _pAsyncEvents = new AsyncEventSource(eventsURL);
    if (!_pAsyncEvents)
        return;

    // Handle connection
    // _pAsyncEvents->onConnect([](AsyncEventSourceClient *client) {
        // if(client->lastId())
        // {
        //     Log.trace("%sevent client reconn - last messageID got is: %d\n", MODULE_PREFIX,
        //             client->lastId());
        // }
    // });

    // Add handler for events
    _pServer->addHandler(_pAsyncEvents);
}

void WebServer::sendAsyncEvent(const char* eventContent, const char* eventGroup)
{
    if (_pAsyncEvents)
        _pAsyncEvents->send(eventContent, eventGroup, millis());
}

void WebServer::webSocketOpen(const String& websocketURL)
{
    // Check enabled
    if (!_pServer)
        return;

    // Add
    _pWebSocket = new AsyncWebSocket(websocketURL);
    _pServer->addHandler(_pWebSocket);
}

void WebServer::webSocketSend(const uint8_t* pBuf, uint32_t len)
{
    if (_pWebSocket)
        _pWebSocket->binaryAll(const_cast<uint8_t*>(pBuf), len);

}