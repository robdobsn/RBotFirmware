// HDLC Bit and Bytewise
// Rob Dobson 2018
// Very loosely based on https://github.com/mengguang/minihdlc

#include "MiniHDLC.h"

// CRC Lookup table
const uint16_t MiniHDLC::_CRCTable[256] = { 
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
    0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
    0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
    0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
    0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
    0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
    0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
    0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
    0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
    0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
    0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
    0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

#ifndef USE_STD_FUNCTION_AND_BIND
    MiniHDLCPutChFnType MiniHDLC::_putChFn = NULL;
    MiniHDLCFrameRxFnType MiniHDLC::_frameRxFn = NULL;
#endif

// If bitwise HDLC then the first parameter will receive bits not bytes 
MiniHDLC::MiniHDLC(MiniHDLCPutChFnType putChFn, MiniHDLCFrameRxFnType frameRxFn,
            bool bigEndianCRC, bool bitwiseHDLC)
{
    _putChFn = putChFn;
    _frameRxFn = frameRxFn;
    _framePos = 0;
    _frameCRC = CRC16_CCITT_INIT_VAL;
    _inEscapeSeq = false;
    _bigEndianCRC = bigEndianCRC;
    _bitwiseHDLC = bitwiseHDLC;
    _bitwiseLast8Bits = 0;
    _bitwiseByte = 0;
    _bitwiseBitCount = 0;
    _bitwiseSendOnesCount = 0;
}

// Function to handle a single bit received
void MiniHDLC::handleBit(uint8_t bit)
{
	// Shift previous bits up to make space and add new
	_bitwiseLast8Bits = _bitwiseLast8Bits >> 1;
	_bitwiseLast8Bits |= (bit ? 0x80 : 0);

	// Check for frame start flag
	if (_bitwiseLast8Bits == FRAME_BOUNDARY_OCTET)
	{
		// Handle with the byte-based handler
		handleChar(FRAME_BOUNDARY_OCTET);
		_bitwiseByte = 0;
		_bitwiseBitCount = 0;
		return;
	}

	// Check for bit stuffing - HDLC abhors a sequence of more
	// than 5 ones in regular data and stuffs a 0 in this case
	// So here we detect that situation and ignore that 0
	if ((_bitwiseLast8Bits & 0xfc) == 0x7c)
		return;

	// Add the received bit into the byte
	_bitwiseByte = _bitwiseByte >> 1;
	_bitwiseByte |= (bit ? 0x80 : 0);

	// Count the bits received and handle each byte-full
	_bitwiseBitCount++;
	if (_bitwiseBitCount == 8)
	{
		// Handle byte-wise
		handleChar(_bitwiseByte);
		_bitwiseByte = 0;
		_bitwiseBitCount = 0;
	}
}

// Function to find valid HDLC frame from incoming data
void MiniHDLC::handleChar(uint8_t ch)
{
    // Check boundary
    if (ch == FRAME_BOUNDARY_OCTET) 
    {
        if (_framePos >= 2) 
        {
            // Valid frame ?
            uint16_t rxcrc = _rxBuffer[_framePos - 2] | (((uint16_t)_rxBuffer[_framePos-1]) << 8);
            if (_bigEndianCRC)
                rxcrc = _rxBuffer[_framePos - 1] | (((uint16_t)_rxBuffer[_framePos - 2]) << 8);
            // Log.trace("...len %d calc %x rxcrc %x\n", _framePos, _frameCRC, rxcrc);
            // for (int i = 0; i < _framePos-2; i++)
            // {
            //     if (_rxBuffer[i] == 0)
            //         Serial.printf("\\0");
            //     else
            //         Serial.printf("%c", _rxBuffer[i]);
            // }
            // for (int i = _framePos-2; i < _framePos; i++)
            // {
            //     Serial.printf(" %02x", _rxBuffer[i]);
            // }
            // Serial.println("");
            if (rxcrc == _frameCRC)
            {
                // Null terminate the frame (in case used as a string)
                _rxBuffer[_framePos-2] = 0;

                // Handle the frame
                if(_frameRxFn)
                    _frameRxFn(_rxBuffer, _framePos - 2);
            }
            else
            {
                _stats._frameCRCErrCount++;
            }
        }

        // Ready for new frame
        _inEscapeSeq = false;
        _framePos = 0;
        _frameCRC = CRC16_CCITT_INIT_VAL;
        _stats._rxFrameCount++;
        return;
    }

    // Check escape
    if (_inEscapeSeq)
    {
        _inEscapeSeq = false;
        ch ^= INVERT_OCTET;
    }
    else if (ch == CONTROL_ESCAPE_OCTET)
    {
        _inEscapeSeq = true;
        return;
    }

    // Store in buffer
    _rxBuffer[_framePos] = ch;

    // Update checksum if needed
    if (_framePos >= 2) 
    {
        _frameCRC = crcUpdateCCITT(_frameCRC, _rxBuffer[_framePos - 2]);
    }

    // Bump position
    _framePos++;

    // Check for max
    if (_framePos == MINIHDLC_MAX_FRAME_LENGTH)
    {
        // Discard and start again
        _framePos = 0;
        _frameCRC = CRC16_CCITT_INIT_VAL;
        _stats._frameTooLongCount++;
    }
}

void MiniHDLC::handleBuffer(const uint8_t* pBuf, int numBytes)
{
    // Iterate bytes
    for (int i = 0; i < numBytes; i++)
        handleChar(pBuf[i]);
}

// Wrap given data in HDLC frame and send it out byte at a time
void MiniHDLC::sendFrame(const uint8_t *pFrame, int frameLen)
{
    uint16_t fcs = CRC16_CCITT_INIT_VAL;

    // Initial boundary
    sendChar(FRAME_BOUNDARY_OCTET);

    // Loop over frame
    int bytesLeft = frameLen;
    while (bytesLeft)
    {
        // Handle escapes
        uint8_t data = *pFrame++;
        fcs = crcUpdateCCITT(fcs, data);
        sendEscaped(data);
        bytesLeft--;
    }

    // Get CRC in the correct order
    uint8_t fcs1 = fcs & 0xff;
    uint8_t fcs2 = (fcs >> 8) & 0xff;
    if (_bigEndianCRC)
    {
        fcs1 = (fcs >> 8) & 0xff;
        fcs2 = fcs & 0xff;
    }

    // Send the FCS
    sendEscaped(fcs1);
    sendEscaped(fcs2);

    // Boundary
    sendChar(FRAME_BOUNDARY_OCTET);
}

uint16_t MiniHDLC::crcUpdateCCITT(unsigned short fcs, unsigned char value)
{
	return (fcs << 8) ^ _CRCTable[((fcs >> 8) ^ value) & 0xff];
}

void MiniHDLC::sendChar(uint8_t ch)
{
	if (_bitwiseHDLC)
	{
		// Send each bit
		uint8_t bitData = ch;
		for (int i = 0; i < 8; i++)
		{
			if (_putChFn)
				_putChFn(bitData & 0x01);
			bitData = bitData >> 1;
		}
	}
	else
	{
		// Send byte-wise
		if (_putChFn)
			_putChFn(ch);
	}
}

void MiniHDLC::sendCharWithStuffing(uint8_t ch)
{
	if (_bitwiseHDLC)
	{
		// Send making sure we don't exceed 5 x 1s in a row
		uint8_t bitData = ch;
		for (int i = 0; i < 8; i++)
		{
			// Put the actual bit
			if (_putChFn)
				_putChFn(bitData & 0x01);

			// Handle bit stuffing
			if (bitData & 0x01)
			{
				// Count 1s
				_bitwiseSendOnesCount++;
				if (_bitwiseSendOnesCount == 5)
				{
					// Stuff a 0 to avoid 6 consecutive 1s
					if (_putChFn)
						_putChFn(0);
					_bitwiseSendOnesCount = 0;
				}
			}
			else
			{
				// Reset count of consecutive 1s
				_bitwiseSendOnesCount = 0;
			}
			// Shift to next bit
			bitData = bitData >> 1;
		}
	}
	else
	{
		// Just send it
		sendChar(ch);
	}
}

void MiniHDLC::sendEscaped(uint8_t ch)
{
	if ((ch == CONTROL_ESCAPE_OCTET) || (ch == FRAME_BOUNDARY_OCTET))
	{
		sendCharWithStuffing(CONTROL_ESCAPE_OCTET);
		ch ^= INVERT_OCTET;
	}
	sendCharWithStuffing(ch);
}
