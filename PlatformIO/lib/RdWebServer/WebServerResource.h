// Web server resource
// Rob Dobson 2012-2018

#pragma once

class WebServerResource
{
public:
    WebServerResource(const char *pResId, const char *pMimeType,
                      const char *pContentEncoding,
                      const char *pAccessControlAllowOrigin,
                      const unsigned char *pData, int dataLen,
                      bool noCache = false,
                      const char *pExtraHeaders = NULL)
    {
        _pResId = pResId;
        _pMimeType = pMimeType;
        _pContentEncoding = pContentEncoding;
        _pAccessControlAllowOrigin = pAccessControlAllowOrigin;
        _pData = pData;
        _dataLen = dataLen;
        _noCache = noCache;
        _pExtraHeaders = pExtraHeaders;
    }
    const char *_pResId;
    const char *_pMimeType;
    const char *_pContentEncoding;
    const char *_pAccessControlAllowOrigin;
    const unsigned char *_pData;
    int _dataLen;
    bool _noCache;
    const char *_pExtraHeaders;
};
