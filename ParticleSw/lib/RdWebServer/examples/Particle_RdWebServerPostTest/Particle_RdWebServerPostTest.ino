/*
 * Project Particle_RdWebServerPostTest
 * Description:
 * Author:
 * Date:
 */

// Web server
#include "RdWebServer.h"
const int   webServerPort = 80;
RdWebServer *pWebServer   = NULL;
#include "GenResourcesEx.h"

// API Endpoints
RestAPIEndpoints restAPIEndpoints;

SerialLogHandler logHandler(LOG_LEVEL_TRACE);

String configStr = "{\"maxCfgLen\":2000, \"name\":\"Sand Table\",\"patterns\":{}, \"sequences\":{}, \"startup\":\"\"}";

// Post settings information via API
void restAPI_PostSettings(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    Log.trace("RestAPI PostSettings method %d contentLen %d", apiMsg._method, apiMsg._msgContentLen);
    if (apiMsg._pMsgHeader)
    {
        Log.trace("RestAPI PostSettings header len %d", strlen(apiMsg._pMsgHeader));
    }
    configStr = (const char *)apiMsg._pMsgContent;
    Log.trace(configStr.c_str());
    // Result
    retStr = "{\"ok\"}";
}


// Get settings information via API
void restAPI_GetSettings(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    Log.trace("RestAPI GetSettings method %d contentLen %d", apiMsg._method, apiMsg._msgContentLen);
    retStr = configStr;
}


// setup() runs once, when the device is first turned on.
void setup()
{
    Serial.begin(115200);
    delay(3000);
    Log.info("Particle_RdWebServerTestPostTest 2017Sep11");

    while (1)
    {
        if (WiFi.ready())
        {
            break;
        }
        delay(5000);
        Log.warn("Waiting for WiFi");
    }
    IPAddress myIp = WiFi.localIP();
    char      myIpAddress[24];
    sprintf(myIpAddress, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);
    Particle.variable("ipAddress", myIpAddress, STRING);
    Serial.printlnf("IP Address %s", myIpAddress);

    // Add endpoint
    restAPIEndpoints.addEndpoint("postsettings", RestAPIEndpointDef::ENDPOINT_CALLBACK,
                    restAPI_PostSettings, "", "");
    restAPIEndpoints.addEndpoint("getsettings", RestAPIEndpointDef::ENDPOINT_CALLBACK,
                    restAPI_GetSettings, "", "");

    // Construct server
    pWebServer = new RdWebServer();

    // Configure web server
    if (pWebServer)
    {
        // Add resources to web server
        pWebServer->addStaticResources(genResourcesEx, genResourcesExCount);

        // Endpoints
        pWebServer->addRestAPIEndpoints(&restAPIEndpoints);

        // Start the web server
        pWebServer->start(webServerPort);
    }
}


// loop() runs over and over again, as quickly as it can execute.
void loop()
{
    // Service the web server
    if (pWebServer)
    {
        pWebServer->service();
    }
}
