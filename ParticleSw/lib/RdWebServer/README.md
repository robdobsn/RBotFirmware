# RdWebServer

Library for a simple web server which supports a REST API and static pages/images

## Usage

```c++
#include "RdWebServer.h"
#include "GenResources.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(AUTOMATIC);

// Web server
RdWebServer* pWebServer = NULL;

void setup()
{
    // Construct server
    pWebServer = new RdWebServer();

    // Configure web server
    if (pWebServer)
    {
      // Add resources to web server
      pWebServer->addStaticResources(genResources, genResourcesCount);

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
```

See the [examples](examples) folder for more details.

## Documentation

To add a REST API:

1. Include the RestAPIEndpoints helper

```C++
// API Endpoints
#include "RestAPIEndpoints.h"
RestAPIEndpoints restAPIEndpoints;
```

2. Create a method to handle the callback that you will receive from RdWebServer when the API endpoint is hit

```C++
void restAPI_QueryStatus(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    retStr = "{\"rslt\":\"ok\"}";
}
```

3. Register the endpoint with RestAPIEndpoint helper - do this anywhere in setup()

```C++
restAPIEndpoints.addEndpoint("Q", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_QueryStatus, "");
```

4. Add the endpoint definitions to the server

Do this after the pWebServer has been initialized to point to the instance of an RdWebServer

```C++
  // Add resources to web server
  pWebServer->addStaticResources(genResources, genResourcesCount);
```

## LICENSE
Copyright 2017 Rob Dobson

Licensed under the MIT license
