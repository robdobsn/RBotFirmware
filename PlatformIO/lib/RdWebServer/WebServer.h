// WebServer
// Rob Dobson 2012-2018

#pragma once

#include <Arduino.h>
#include "ConfigBase.h"
#include "RestAPIEndpoints.h"

class AsyncWebServer;
class AsyncWebServerResponse;
class AsyncWebServerRequest;
class WebServerResource;
class AsyncEventSource;

class WebServer
{
public:
    AsyncWebServer* _pServer;
    bool _begun;
    bool _webServerEnabled;
    AsyncEventSource* _pAsyncEvents;

    WebServer();
    ~WebServer();

    void setup(ConfigBase& hwConfig);
    void addEndpoints(RestAPIEndpoints &endpoints);
    void begin(bool accessControlAllowOriginAll);
    // Add resources to the web server
    void addStaticResources(const WebServerResource *pResources, int numResources);
    static void parseAndAddHeaders(AsyncWebServerResponse *response, const char *pHeaders);
    static String recreatedReqUrl(AsyncWebServerRequest *request);
    void serveStaticFiles(const char* baseUrl, const char* baseFolder, const char* cache_control = NULL);
    // Async event handler (one-way text to browser)
    void enableAsyncEvents(const String& eventsURL);
    void sendAsyncEvent(const char* eventContent, const char* eventGroup);

private:
    void addStaticResource(const WebServerResource *pResource, const char *pAliasPath = NULL);
};
