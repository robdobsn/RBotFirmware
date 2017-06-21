// Web server resources
// Rob Dobson 2012-2017

#pragma once

class RdWebServerResourceDescr
{
public:
    RdWebServerResourceDescr(const char *pResId, const char *pMimeType, const unsigned char *pData, int dataLen)
    {
        _pResId    = pResId;
        _pMimeType = pMimeType;
        _pData     = pData;
        _dataLen   = dataLen;
    }
    const char          *_pResId;
    const char          *_pMimeType;
    const unsigned char *_pData;
    int                 _dataLen;
};
