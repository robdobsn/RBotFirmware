#pragma once

#include <Arduino.h>
#include <unity.h>
#include "../lib/RdCommandSerial/minihdlc.h"
#include <ArduinoLog.h>

class UnitTestMiniHDLC_IO
{
public:
    std::vector<uint8_t> _inFrame;
    std::vector<uint8_t> _outFrame;
};

std::vector<UnitTestMiniHDLC_IO> testIO = {
    {
        { 0x00, 0x00 },
        { 0x7e, 0x00, 0x00, 0x1d, 0x0f, 0x7e }
    },
    {
        { 0x7e, 0x00, 0x7f, 0x00, 0x01, 0x02, 0x03, 0xff, 0x7e },
        { 0x7e, 0x7d, 0x5e, 0x00, 0x7f, 0x00, 0x01, 0x02, 0x03, 0xff, 0x7d, 0x5e, 0x4c, 0x99, 0x7e }
    },
};

class UnitTestMiniHDLC
{
public:
    
    static const int MAX_VALID_CHARS_SENT = 1000;

    std::vector<uint8_t> _charsSent;
    std::vector<uint8_t> _frameReceived;

    void testDataClear()
    {
        _charsSent.clear();
        _frameReceived.clear();
    }

    int testDataNumCharsSent()
    {
        return _charsSent.size();
    }

    uint8_t* testDataSent()
    {
        return _charsSent.data();
    }

    void testDataDebugSent()
    {
        Serial.printf("Chars sent\n");
        for(int i = 0; i < _charsSent.size(); i++)
            Serial.printf("0x%02x, ", _charsSent[i]);
        Serial.printf("\n");
    }

    void testDataDebugReceived()
    {
        Serial.printf("Frame received\n");
        for(int i = 0; i < _frameReceived.size(); i++)
            Serial.printf("0x%02x, ", _frameReceived[i]);
        Serial.printf("\n");
    }

    int testDataNumBytesReceived()
    {
        return _frameReceived.size();
    }

    uint8_t* testDataReceived()
    {
        return _frameReceived.data();
    }

    void sendCharToCmdPort(uint8_t ch)
    {
        TEST_ASSERT_LESS_OR_EQUAL(MAX_VALID_CHARS_SENT, _charsSent.size());

        _charsSent.push_back(ch);
    }

    void frameHandler(const uint8_t *framebuffer, int framelength)
    {
        for (int i = 0; i < framelength; i++)
            _frameReceived.push_back(framebuffer[i]);
        testDataDebugReceived();
    }

    void runTests()
    {
        MiniHDLC miniHDLC(std::bind(&UnitTestMiniHDLC::sendCharToCmdPort, this, std::placeholders::_1), 
                std::bind(&UnitTestMiniHDLC::frameHandler, this, std::placeholders::_1, std::placeholders::_2),
                true, false);

        // TEST_ASSERT_FALSE(pMotionHoming->_commandInProgress);
        // TEST_ASSERT_EQUAL_STRING("", pMotionHoming->_homingSequence.c_str());

        /////////////////////////////////////////////////////////////////////////////////////
        //// TEST 1
        /////////////////////////////////////////////////////////////////////////////////////

        Serial.println("TEST1");

        // Loop through tests
        for (int i = 0; i < testIO.size(); i++)
        {
            for (int testInwards = 0; testInwards < 2; testInwards++)
            {
                testDataClear();
                if (testInwards == 1)
                {
                    for (int j = 0; j < testIO[i]._outFrame.size(); j++)
                    {
                        miniHDLC.handleChar(testIO[i]._outFrame[j]);
                        if (j != testIO[i]._outFrame.size()-1)
                            TEST_ASSERT_EQUAL(0, testDataNumBytesReceived());
                    }
                    TEST_ASSERT_EQUAL_UINT8_ARRAY(testIO[i]._inFrame.data(), testDataReceived(), testIO[i]._inFrame.size());
                }
                else
                {
                    miniHDLC.sendFrame(testIO[i]._inFrame.data(), testIO[i]._inFrame.size());
                    testDataDebugSent();
                    TEST_ASSERT_EQUAL(testIO[i]._outFrame.size(), testDataNumCharsSent());
                    TEST_ASSERT_EQUAL_UINT8_ARRAY(testIO[i]._outFrame.data(), testDataSent(), testIO[i]._outFrame.size());
                }
            }
        }
    }

};