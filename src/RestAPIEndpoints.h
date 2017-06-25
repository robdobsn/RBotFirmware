// REST API Endpoints
// Rob Dobson 2012-2016

#pragma once

// Information on received API request
typedef struct RestAPIEndpointMsg
{
    int _method;
    const char* _pEndpointStr;
    const char* _pArgStr;
    const char* _pMsgHeader;
    unsigned char* _pMsgContent;
    int _msgContentLen;
    RestAPIEndpointMsg(int method, const char* pEndpointStr, const char* pArgStr, const char* pMsgHeader)
    {
        _method = method;
        _pEndpointStr = pEndpointStr;
        _pArgStr = pArgStr;
        _pMsgHeader = pMsgHeader;
        _pMsgContent = NULL;
        _msgContentLen = 0;
    }
};

// Callback function for any endpoint
typedef void (*RestAPIEndpointCallbackType)(RestAPIEndpointMsg& restAPIEndpointMsg, String& retStr);

// Definition of an endpoint
class RestAPIEndpointDef
{
public:
    static const int ENDPOINT_CALLBACK = 1;
    RestAPIEndpointDef(const char *pStr, int endpointType, RestAPIEndpointCallbackType callback)
    {
        int stlen = strlen(pStr);

        _pEndpointStr = new char[stlen + 1];
        strcpy(_pEndpointStr, pStr);
        _endpointType = endpointType;
        _callback     = callback;
    };
    ~RestAPIEndpointDef()
    {
        delete _pEndpointStr;
    }
    char *_pEndpointStr;
    int  _endpointType;
    RestAPIEndpointCallbackType _callback;
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
    char *getNthEndpointStr(int n, int& endpointType, RestAPIEndpointCallbackType& callback)
    {
        if ((n >= 0) && (n < _numEndpoints))
        {
            endpointType = _pEndpoints[n]->_endpointType;
            callback     = _pEndpoints[n]->_callback;
            return _pEndpoints[n]->_pEndpointStr;
        }
        return NULL;
    }


    // Add an endpoint
    void addEndpoint(const char *pEndpointStr, int endpointType, RestAPIEndpointCallbackType callback)
    {
        // Check for overflow
        if (_numEndpoints >= MAX_WEB_SERVER_ENDPOINTS)
        {
            return;
        }

        // Create new command definition and add
        RestAPIEndpointDef *pNewEndpointDef = new RestAPIEndpointDef(pEndpointStr, endpointType, callback);

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
            if (strcasecmp(pEndpoint->_pEndpointStr, pEndpointStr) == 0)
            {
                return pEndpoint;
            }
        }
        return NULL;
    }


    // Handle an API request
    void handleApiRequest(const char *requestStr, String& retStr)
    {
        // Get the command
        static char *emptyStr       = (char *)"";
        String      requestEndpoint = getNthArgStr(requestStr, 0).toUpperCase();
        char        *argStart       = strstr(requestStr, "/");
        retStr = "";

        if (argStart == NULL)
        {
            argStart = emptyStr;
        }
        else
        {
            argStart++;
        }
        Log.trace("Endpoint: %s", requestStr);
        // Check against valid commands
        int numEndpoints = getNumEndpoints();
        for (int i = 0; i < numEndpoints; i++)
        {
            RestAPIEndpointCallbackType callback;
            int  endpointType;
            char *pEndpointStr = getNthEndpointStr(i, endpointType, callback);
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
                RestAPIEndpointMsg endpointMsg(0, NULL, argStart, NULL);
                callback(endpointMsg, retStr);
            }
        }
    }


    // Form a string from a char buffer with a fixed length
    static void formStringFromCharBuf(String& outStr, const char *pStr, int len)
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
        int        argLen = 0;
        String     oStr;
        const char *pStr = getArgPtrAndLen(argStr, argIdx, argLen);

        if (pStr)
        {
            formStringFromCharBuf(oStr, pStr, argLen);
        }
        oStr = unencodeHTTPChars(oStr);
        return oStr;
    }


    // Get position and length of nth arg
    static const char *getArgPtrAndLen(const char *argStr, int argIdx, int& argLen)
    {
        int        curArgIdx = 0;
        const char *pCh      = argStr;
        const char *pArg     = argStr;

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
        int        numArgs       = 0;
        int        numChSinceSep = 0;
        const char *pCh          = argStr;

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
    static String unencodeHTTPChars(String& inStr)
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


private:
    // Endpoint list
    RestAPIEndpointDef *_pEndpoints[MAX_WEB_SERVER_ENDPOINTS];
    int                _numEndpoints;
};
