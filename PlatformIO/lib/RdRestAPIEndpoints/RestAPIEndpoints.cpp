// REST API Endpoints
// Rob Dobson 2012-2016

#include "RestAPIEndpoints.h"

static const char* MODULE_PREFIX = "RestAPIEndpoints: ";

RestAPIEndpoints::~RestAPIEndpoints()
{
    // Clean-up
    for (int i = 0; i < _numEndpoints; i++)
    {
        delete _pEndpoints[i];
    }
    _numEndpoints = 0;
}

// Get number of endpoints
int RestAPIEndpoints::getNumEndpoints()
{
    return _numEndpoints;
}

// Get nth endpoint
RestAPIEndpointDef *RestAPIEndpoints::getNthEndpoint(int n)
{
    if ((n >= 0) && (n < _numEndpoints))
    {
        return _pEndpoints[n];
    }
    return NULL;
}

// Add an endpoint
void RestAPIEndpoints::addEndpoint(const char *pEndpointStr, RestAPIEndpointDef::EndpointType endpointType,
                    RestAPIEndpointDef::EndpointMethod endpointMethod,
                    RestAPIFunction callback,
                    const char *pDescription,
                    const char *pContentType,
                    const char *pContentEncoding,
                    bool pNoCache,
                    const char *pExtraHeaders,
                    RestAPIFnBody callbackBody,
                    RestAPIFnUpload callbackUpload)
{
    // Check for overflow
    if (_numEndpoints >= MAX_WEB_SERVER_ENDPOINTS)
    {
        return;
    }

    // Create new command definition and add
    RestAPIEndpointDef *pNewEndpointDef =
        new RestAPIEndpointDef(pEndpointStr, endpointType,
                                endpointMethod, callback,
                                pDescription,
                                pContentType, pContentEncoding,
                                pNoCache, pExtraHeaders,
                                callbackBody, callbackUpload);
    _pEndpoints[_numEndpoints] = pNewEndpointDef;
    _numEndpoints++;
}

// Get the endpoint definition corresponding to a requested endpoint
RestAPIEndpointDef* RestAPIEndpoints::getEndpoint(const char *pEndpointStr)
{
    // Look for the command in the registered callbacks
    for (int endpointIdx = 0; endpointIdx < _numEndpoints; endpointIdx++)
    {
        RestAPIEndpointDef *pEndpoint = _pEndpoints[endpointIdx];
        if (strcasecmp(pEndpoint->_endpointStr.c_str(), pEndpointStr) == 0)
        {
            return pEndpoint;
        }
    }
    return NULL;
}

// Handle an API request
void RestAPIEndpoints::handleApiRequest(const char *requestStr, String &retStr)
{
    // Get the command
    static char *emptyStr = (char *)"";
    String requestEndpoint = getNthArgStr(requestStr, 0);
    requestEndpoint.toUpperCase();
    char *argStart = strstr(requestStr, "/");
    retStr = "";

    if (argStart == NULL)
    {
        argStart = emptyStr;
    }
    else
    {
        argStart++;
    }
    // Check against valid commands
    bool endpointMatched = false;
    int numEndpoints = getNumEndpoints();
    Log.verbose("%sreqStr %s requestEndpoint %s, num endpoints %d\n", MODULE_PREFIX, 
                requestStr, requestEndpoint.c_str(), numEndpoints);
    for (int i = 0; i < numEndpoints; i++)
    {
        RestAPIEndpointDef* pEndpoint = getNthEndpoint(i);
        if (!pEndpoint)
        {
            continue;
        }
        if (pEndpoint->_endpointType != RestAPIEndpointDef::ENDPOINT_CALLBACK)
        {
            continue;
        }
        if (requestEndpoint.equalsIgnoreCase(pEndpoint->_endpointStr))
        {
            String reqStr(requestStr);
            pEndpoint->callback(reqStr, retStr);
            endpointMatched = true;
        }
    }
    if (!endpointMatched)
    {
        Log.notice("%sendpoint %s not found\n", MODULE_PREFIX, requestEndpoint.c_str());
    }
}

// Form a string from a char buffer with a fixed length
void RestAPIEndpoints::formStringFromCharBuf(String &outStr, const char *pStr, int len)
{
    outStr = "";
    outStr.reserve(len + 1);
    for (int i = 0; i < len; i++)
    {
        outStr.concat(*pStr);
        pStr++;
    }
}

// Remove first argument from string
String RestAPIEndpoints::removeFirstArgStr(const char *argStr)
{
    // Get location of / (excluding first char if needed)
    String oStr = argStr;
    oStr = unencodeHTTPChars(oStr);
    int idxSlash = oStr.indexOf('/', 1);
    if (idxSlash == -1)
        return String("");
    return oStr.substring(idxSlash+1);
}

// Get Nth argument from a string
String RestAPIEndpoints::getNthArgStr(const char *argStr, int argIdx)
{
    int argLen = 0;
    String oStr;
    const char *pStr = getArgPtrAndLen(argStr, *argStr == '/' ? argIdx + 1 : argIdx, argLen);

    if (pStr)
    {
        formStringFromCharBuf(oStr, pStr, argLen);
    }
    oStr = unencodeHTTPChars(oStr);
    return oStr;
}

// Get position and length of nth arg
const char* RestAPIEndpoints::getArgPtrAndLen(const char *argStr, int argIdx, int &argLen)
{
    int curArgIdx = 0;
    const char *pCh = argStr;
    const char *pArg = argStr;

    while (true)
    {
        if ((*pCh == '/') || (*pCh == '?') || (*pCh == '\0'))
        {
            if (curArgIdx == argIdx)
            {
                argLen = pCh - pArg;
                return pArg;
            }
            if (*pCh == '\0')
            {
                return NULL;
            }
            pArg = pCh + 1;
            curArgIdx++;
        }
        pCh++;
    }
    return NULL;
}

// Num args from an argStr
int RestAPIEndpoints::getNumArgs(const char *argStr)
{
    int numArgs = 0;
    int numChSinceSep = 0;
    const char *pCh = argStr;

    // Count args
    while (*pCh)
    {
        if (*pCh == '/')
        {
            numArgs++;
            numChSinceSep = 0;
        }
        pCh++;
        numChSinceSep++;
    }
    if (numChSinceSep > 0)
    {
        return numArgs + 1;
    }
    return numArgs;
}

// Convert encoded URL
String RestAPIEndpoints::unencodeHTTPChars(String &inStr)
{
    inStr.replace("+", " ");
    inStr.replace("%20", " ");
    inStr.replace("%21", "!");
    inStr.replace("%22", "\"");
    inStr.replace("%23", "#");
    inStr.replace("%24", "$");
    inStr.replace("%25", "%");
    inStr.replace("%26", "&");
    inStr.replace("%27", "^");
    inStr.replace("%28", "(");
    inStr.replace("%29", ")");
    inStr.replace("%2A", "*");
    inStr.replace("%2B", "+");
    inStr.replace("%2C", ",");
    inStr.replace("%2D", "-");
    inStr.replace("%2E", ".");
    inStr.replace("%2F", "/");
    inStr.replace("%3A", ":");
    inStr.replace("%3B", ";");
    inStr.replace("%3C", "<");
    inStr.replace("%3D", "=");
    inStr.replace("%3E", ">");
    inStr.replace("%3F", "?");
    inStr.replace("%5B", "[");
    inStr.replace("%5C", "\\");
    inStr.replace("%5D", "]");
    inStr.replace("%5E", "^");
    inStr.replace("%5F", "_");
    inStr.replace("%60", "`");
    inStr.replace("%7B", "{");
    inStr.replace("%7C", "|");
    inStr.replace("%7D", "}");
    inStr.replace("%7E", "~");
    return inStr;
}

const char* RestAPIEndpoints::getEndpointTypeStr(RestAPIEndpointDef::EndpointType endpointType)
{
    if (endpointType == RestAPIEndpointDef::ENDPOINT_CALLBACK)
        return "Callback";
    return "Unknown";
}

const char* RestAPIEndpoints::getEndpointMethodStr(RestAPIEndpointDef::EndpointMethod endpointMethod)
{
    if (endpointMethod == RestAPIEndpointDef::ENDPOINT_POST)
        return "POST";
    if (endpointMethod == RestAPIEndpointDef::ENDPOINT_PUT)
        return "PUT";
    if (endpointMethod == RestAPIEndpointDef::ENDPOINT_DELETE)
        return "DELETE";
    return "GET";
}

