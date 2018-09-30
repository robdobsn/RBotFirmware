// Particle_RdWebServerTest
// Rob Dobson 2012-2017

#include "RdWebServer.h"
#include "GenResourcesEx.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(AUTOMATIC);

SerialLogHandler logHandler(LOG_LEVEL_TRACE);

// Web server port
int webServerPort = 80;

// API Endpoints
#include "RestAPIEndpoints.h"
RestAPIEndpoints restAPIEndpoints;

// Web server
RdWebServer* pWebServer = NULL;

void restAPI_QueryStatus(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    retStr = "{\"rslt\":\"ok\"}";
}

void setup()
{
  Serial.begin(115200);
  delay(3000);
  Log.info("Particle_RdWebServerTest 2017Sep01");

  while(1)
  {
      if (WiFi.ready())
        break;
      delay(5000);
      Log.warn("Waiting for WiFi");
  }
  IPAddress myIp = WiFi.localIP();
  char myIpAddress[24];
  sprintf(myIpAddress, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);
  Particle.variable("ipAddress", myIpAddress, STRING);
  Serial.printlnf("IP Address %s", myIpAddress);

  // Add endpoint
  restAPIEndpoints.addEndpoint("Q", RestAPIEndpointDef::ENDPOINT_CALLBACK,
            restAPI_QueryStatus, "", "");

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

void loop()
{
  // Service the web server
  if (pWebServer)
    pWebServer->service();
}
