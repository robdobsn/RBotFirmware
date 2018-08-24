// REST API Endpoints
// Rob Dobson 2012-2016

#pragma once
#include <Arduino.h>
#include <ArduinoLog.h>
#include <functional>

// Callback function for any endpoint
typedef std::function<void(String &reqStr, String &respStr)> RestAPIFunction;

// Definition of an endpoint
class RestAPIEndpointDef
{
  public:
    enum EndpointType
    {
        ENDPOINT_NONE = 0,
        ENDPOINT_CALLBACK = 1,
    };

    enum EndpointMethod
    {
        ENDPOINT_GET = 0,
        ENDPOINT_POST = 1,
        ENDPOINT_PUT = 2,
        ENDPOINT_DELETE = 3
    };

    RestAPIEndpointDef(const char *pStr, EndpointType endpointType,
                       EndpointMethod endpointMethod,
                       RestAPIFunction callback,
                       const char *pDescription,
                       const char *pContentType,
                       const char *pContentEncoding,
                       bool noCache,
                       const char *pExtraHeaders)
    {
        _endpointStr = pStr;
        _endpointType = endpointType;
        _endpointMethod = endpointMethod;
        _callback = callback;
        _description = pDescription;
        if (pContentType)
            _contentType = pContentType;
        if (pContentEncoding)
            _contentEncoding = pContentEncoding;
        _noCache = noCache;
        if (pExtraHeaders)
            _extraHeaders = pExtraHeaders;
    };
    String _endpointStr;
    EndpointType _endpointType;
    EndpointMethod _endpointMethod;
    String _description;
    String _contentType;
    String _contentEncoding;
    RestAPIFunction _callback;
    bool _noCache;
    String _extraHeaders;

    void callFn(String &req, String &resp)
    {
        if (_callback)
            _callback(req, resp);
    }
};

// Collection of endpoints
class RestAPIEndpoints
{
  public:
    // Max endpoints we can accommodate
    static const int MAX_WEB_SERVER_ENDPOINTS = 50;

    RestAPIEndpoints()
    {
        _numEndpoints = 0;
    }

    ~RestAPIEndpoints()
    {
        // Clean-up
        for (int i = 0; i < _numEndpoints; i++)
        {
            delete _pEndpoints[i];
        }
        _numEndpoints = 0;
    }

    // Get number of endpoints
    int getNumEndpoints()
    {
        return _numEndpoints;
    }

    // Get nth endpoint
    const char *getNthEndpointStr(int n)
    {
        if ((n >= 0) && (n < _numEndpoints))
        {
            return _pEndpoints[n]->_endpointStr.c_str();
        }
        return "";
    }

    // Get nth endpoint description
    const char *getNthEndpointDescr(int n)
    {
        if ((n >= 0) && (n < _numEndpoints))
        {
            return _pEndpoints[n]->_description.c_str();
        }
        return "";
    }

    // Get nth endpoint
    const char *getNthEndpointStr(int n, RestAPIEndpointDef::EndpointType &endpointType,
                                  RestAPIEndpointDef::EndpointMethod &endpointMethod,
                                  RestAPIFunction &callback)
    {
        if ((n >= 0) && (n < _numEndpoints))
        {
            endpointType = _pEndpoints[n]->_endpointType;
            endpointMethod = _pEndpoints[n]->_endpointMethod;
            callback = _pEndpoints[n]->_callback;
            return _pEndpoints[n]->_endpointStr.c_str();
        }
        return NULL;
    }

    // Add an endpoint
    void addEndpoint(const char *pEndpointStr, RestAPIEndpointDef::EndpointType endpointType,
                     RestAPIEndpointDef::EndpointMethod endpointMethod,
                     RestAPIFunction callback,
                     const char *pDescription,
                     const char *pContentType = NULL,
                     const char *pContentEncoding = NULL,
                     bool pNoCache = true,
                     const char *pExtraHeaders = NULL)
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
                                   pNoCache, pExtraHeaders);
        _pEndpoints[_numEndpoints] = pNewEndpointDef;
        _numEndpoints++;
    }

    // Get the endpoint definition corresponding to a requested endpoint
    RestAPIEndpointDef *getEndpoint(const char *pEndpointStr)
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
    void handleApiRequest(const char *requestStr, String &retStr)
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
        int numEndpoints = getNumEndpoints();
        Log.trace("RestAPIEndpoints: reqStr %s requestEndpoint %s, num endpoints %d\n", 
                    requestStr, requestEndpoint.c_str(), numEndpoints);
        for (int i = 0; i < numEndpoints; i++)
        {
            RestAPIFunction callback;
            RestAPIEndpointDef::EndpointType endpointType;
            RestAPIEndpointDef::EndpointMethod endpointMethod;
            const char *pEndpointStr = getNthEndpointStr(i, endpointType, endpointMethod, callback);
            if (!pEndpointStr)
            {
                continue;
            }
            if (endpointType != RestAPIEndpointDef::ENDPOINT_CALLBACK)
            {
                continue;
            }
            if (requestEndpoint.equalsIgnoreCase(pEndpointStr))
            {
                String reqStr(requestStr);
                callback(reqStr, retStr);
            }
        }
    }

    // Form a string from a char buffer with a fixed length
    static void formStringFromCharBuf(String &outStr, const char *pStr, int len)
    {
        outStr = "";
        outStr.reserve(len + 1);
        for (int i = 0; i < len; i++)
        {
            outStr.concat(*pStr);
            pStr++;
        }
    }

    // Get Nth argument from a string
    static String getNthArgStr(const char *argStr, int argIdx)
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
    static const char *getArgPtrAndLen(const char *argStr, int argIdx, int &argLen)
    {
        int curArgIdx = 0;
        const char *pCh = argStr;
        const char *pArg = argStr;

        while (true)
        {
            if ((*pCh == '/') || (*pCh == '\0'))
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
    static int getNumArgs(const char *argStr)
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
    static String unencodeHTTPChars(String &inStr)
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

    static const char *getEndpointTypeStr(RestAPIEndpointDef::EndpointType endpointType)
    {
        if (endpointType == RestAPIEndpointDef::ENDPOINT_CALLBACK)
            return "Callback";
        return "Unknown";
    }

    static const char *getEndpointMethodStr(RestAPIEndpointDef::EndpointMethod endpointMethod)
    {
        if (endpointMethod == RestAPIEndpointDef::ENDPOINT_POST)
            return "POST";
        if (endpointMethod == RestAPIEndpointDef::ENDPOINT_PUT)
            return "PUT";
        if (endpointMethod == RestAPIEndpointDef::ENDPOINT_DELETE)
            return "DELETE";
        return "GET";
    }

  private:
    // Endpoint list
    RestAPIEndpointDef *_pEndpoints[MAX_WEB_SERVER_ENDPOINTS];
    int _numEndpoints;
};
