// HDLC Bit and Bytewise
// Rob Dobson 2018
// This HDLC implementation doesn't completely conform to HDLC
// Currently STX and ETX are not sent
// There is no flow control
// Bit and Byte oriented HDLC is supported with appropriate bit/byte stuffing

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define USE_STD_FUNCTION_AND_BIND 1

#ifdef USE_STD_FUNCTION_AND_BIND
#include <functional>
// Put byte or bit callback function type
typedef std::function<void(uint8_t ch)> MiniHDLCPutChFnType;
// Received frame callback function type
typedef std::function<void(const uint8_t *framebuffer, int framelength)> MiniHDLCFrameRxFnType;
#else
typedef void (*MiniHDLCPutChFnType)(uint8_t ch);
typedef void (*MiniHDLCFrameRxFnType)(const uint8_t *framebuffer, int framelength);
#endif

class MiniHDLCStats
{
public:
    MiniHDLCStats()
    {
        clear();
    }
    void clear()
    {
        _rxFrameCount = 0;
        _frameCRCErrCount = 0;
        _frameTooLongCount = 0;
    }
    uint32_t _rxFrameCount;
    uint32_t _frameCRCErrCount;
    uint32_t _frameTooLongCount;
};

// MiniHDLC
class MiniHDLC
{
public:
    // Constructor for HDLC
    // If bitwise HDLC then the first parameter will receive bits not bytes 
    MiniHDLC(MiniHDLCPutChFnType putChFn, MiniHDLCFrameRxFnType frameRxFn,
				bool bigEndianCRC = true, bool bitwiseHDLC = false);

    // Called by external function that has byte-wise data to process
    void handleChar(uint8_t ch);
    void handleBuffer(const uint8_t* pBuf, int numBytes);

    // Called by external function that has bit-wise data to process
    void handleBit(uint8_t bit);

    // Called to send a frame
    void sendFrame(const uint8_t *pData, int frameLen);

    // Get stats
    MiniHDLCStats* getStats()
    {
        return &_stats;
    }

private:
    // If either of the following two octets appears in the transmitted data, an escape octet is sent,
    // followed by the original data octet with bit 5 inverted

    // The frame boundary octet is 01111110, (7E in hexadecimal notation)
    static constexpr uint8_t FRAME_BOUNDARY_OCTET = 0x7E;

    // Control escape octet", has the bit sequence '01111101', (7D hexadecimal) */
    static constexpr uint8_t CONTROL_ESCAPE_OCTET = 0x7D;

    // Invert octet explained above
    static constexpr uint8_t INVERT_OCTET = 0x20;

    // The frame check sequence (FCS) is a 16-bit CRC-CCITT
    // AVR Libc CRC function is _crc_ccitt_update()
    // Corresponding CRC function in Qt (www.qt.io) is qChecksum()
    static constexpr uint16_t CRC16_CCITT_INIT_VAL = 0xFFFF;

    // Max FRAME length
    static constexpr int MINIHDLC_MAX_FRAME_LENGTH = 20000;

    // CRC table
    static const uint16_t _CRCTable[256];

    // Callback functions for PutCh/PutBit and FrameRx
#ifdef USE_STD_FUNCTION_AND_BIND
    MiniHDLCPutChFnType _putChFn;
    MiniHDLCFrameRxFnType _frameRxFn;
#else
    static MiniHDLCPutChFnType _putChFn;
    static MiniHDLCFrameRxFnType _frameRxFn;
#endif

    // Bitwise HDLC flag (otherwise byte-wise)
    bool _bitwiseHDLC;

    // Send FCS (CRC) big-endian - i.e. high byte first
    bool _bigEndianCRC;

    // State vars
    int _framePos;
    uint16_t _frameCRC;
    bool _inEscapeSeq;

    // Bitwise state
    uint8_t _bitwiseLast8Bits;
    uint8_t _bitwiseByte;
    int _bitwiseBitCount;
    int _bitwiseSendOnesCount;

    // Receive buffer
    uint8_t _rxBuffer[MINIHDLC_MAX_FRAME_LENGTH + 1];

    // Stats
    MiniHDLCStats _stats;

private:
    uint16_t crcUpdateCCITT(unsigned short fcs, unsigned char value);
    void sendChar(uint8_t ch);
    void sendCharWithStuffing(uint8_t ch);
    void sendEscaped(uint8_t ch);

};
