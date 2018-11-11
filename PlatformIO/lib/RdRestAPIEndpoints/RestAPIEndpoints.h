// REST API Endpoints
// Rob Dobson 2012-2016

#pragma once
#include <Arduino.h>
#include <ArduinoLog.h>
#include <functional>

// Callback function for any endpoint
typedef std::function<void(String &reqStr, String &respStr)> RestAPIFunction;
typedef std::function<void(String &reqStr, uint8_t *pData, size_t len, size_t index, size_t total)> RestAPIFnBody;
typedef std::function<void(String &reqStr, String& filename, size_t contentLen, size_t index, uint8_t *data, size_t len, bool finalBlock)> RestAPIFnUpload;

// Definition of an endpoint
class RestAPIEndpointDef
{
public:
    enum EndpointType
    {
        ENDPOINT_NONE = 0,
        ENDPOINT_CALLBACK = 1
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
                       const char *pExtraHeaders,
                       RestAPIFnBody callbackBody,
                       RestAPIFnUpload callbackUpload
                       )
    {
        _endpointStr = pStr;
        _endpointType = endpointType;
        _endpointMethod = endpointMethod;
        _callback = callback;
        _callbackBody = callbackBody;
        _callbackUpload = callbackUpload;
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
    RestAPIFnBody _callbackBody;
    RestAPIFnUpload _callbackUpload;
    bool _noCache;
    String _extraHeaders;

    void callback(String &req, String &resp)
    {
        if (_callback)
            _callback(req, resp);
    }

    void callbackBody(String&req, uint8_t *pData, size_t len, size_t index, size_t total)
    {
        if (_callbackBody)
            _callbackBody(req, pData, len, index, total);
    }

    void callbackUpload(String&req, String &filename, size_t contentLen, size_t index,
                         uint8_t *data, size_t len, bool finalBlock)
    {
        if (_callbackUpload)
            _callbackUpload(req, filename, contentLen, index, data, len, finalBlock);
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

    ~RestAPIEndpoints();

    // Get number of endpoints
    int getNumEndpoints();

    // Get nth endpoint
    RestAPIEndpointDef *getNthEndpoint(int n);

    // Add an endpoint
    void addEndpoint(const char *pEndpointStr, RestAPIEndpointDef::EndpointType endpointType,
                     RestAPIEndpointDef::EndpointMethod endpointMethod,
                     RestAPIFunction callback,
                     const char *pDescription,
                     const char *pContentType = NULL,
                     const char *pContentEncoding = NULL,
                     bool pNoCache = true,
                     const char *pExtraHeaders = NULL,
                     RestAPIFnBody callbackBody = NULL,
                     RestAPIFnUpload callbackUpload = NULL);

    // Get the endpoint definition corresponding to a requested endpoint
    RestAPIEndpointDef *getEndpoint(const char *pEndpointStr);

    // Handle an API request
    void handleApiRequest(const char *requestStr, String &retStr);

    // Form a string from a char buffer with a fixed length
    static void formStringFromCharBuf(String &outStr, const char *pStr, int len);

    // Remove first argument from string
    static String removeFirstArgStr(const char *argStr);

    // Get Nth argument from a string
    static String getNthArgStr(const char *argStr, int argIdx);

    // Get position and length of nth arg
    static const char *getArgPtrAndLen(const char *argStr, int argIdx, int &argLen);

    // Num args from an argStr
    static int getNumArgs(const char *argStr);

    // Convert encoded URL
    static String unencodeHTTPChars(String &inStr);

    static const char *getEndpointTypeStr(RestAPIEndpointDef::EndpointType endpointType);

    static const char *getEndpointMethodStr(RestAPIEndpointDef::EndpointMethod endpointMethod);

private:
    // Endpoint list
    RestAPIEndpointDef *_pEndpoints[MAX_WEB_SERVER_ENDPOINTS];
    int _numEndpoints;
};
