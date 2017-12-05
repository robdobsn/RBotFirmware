// Particle cloud handler
// Rob Dobson 2012-2017

#pragma once

typedef const char* (*ParticleRxCallbackType)(const char* cmdStr);

typedef char* (*ParticleCallbackType)(const char* idStr,
                const char* initialContentJsonElementList);
typedef unsigned long (*ParticleHashCallbackType)();

const int MAX_API_STR_LEN = 1000;

#include "application.h"

// Alive rate
const unsigned long ALIVE_EVENT_MILLIS = 30000;

// Status update rate
const unsigned long STATUS_UPDATE_MILLIS = 500;

static ParticleRxCallbackType __pParticleRxCallback = NULL;
static ParticleCallbackType __queryStatusFn = NULL;
static ParticleHashCallbackType __queryStatusHashFn = NULL;
static char __receivedApiBuf[MAX_API_STR_LEN];
static String __appStatusStr;
static unsigned long __lastStatusHashValue = 0xffffffff;
static bool __particleVariablesRegistered = false;

static int __particleApiCall(String cmd)
{
    // Check if there is a callback to call
    if (__pParticleRxCallback)
    {
        cmd.toCharArray(__receivedApiBuf, MAX_API_STR_LEN);
        __pParticleRxCallback(__receivedApiBuf);
    }
    return 0;
}

class ParticleCloud
{
public:
    // Last time an "Alive" event was sent to the particle cloud
    unsigned long _isAliveLastMillis;

    // Last time appStatusStr was updated
    unsigned long _statusUpdateLastMillis;

    // Name of application
    String _appName;

    ParticleCloud(const char* appName,
        ParticleRxCallbackType pParticleRxCallback,
        ParticleCallbackType queryStatusFn,
        ParticleHashCallbackType queryStatusHashFn)
    {
        _appName = appName;
        __pParticleRxCallback = pParticleRxCallback;
        __queryStatusFn = queryStatusFn;
        __queryStatusHashFn = queryStatusHashFn;
        _isAliveLastMillis = 0;
        _statusUpdateLastMillis = 0;
    }

    void RegisterVariables()
    {
        // Variable for application status
        Particle.variable("status", __appStatusStr);

        // Function for API Access
        Particle.function("apiCall", __particleApiCall);
    }

    void Service()
    {
        // Check if particle variables registered
        if (!__particleVariablesRegistered)
        {
            if (Particle.connected())
            {
                RegisterVariables();
                __particleVariablesRegistered = true;
            }
        }

        // Say we're alive to the particle cloud every now and then
        if (Utils::isTimeout(millis(), _isAliveLastMillis, ALIVE_EVENT_MILLIS))
        {
            // Serial.println("Publishing Event to Particle Cloud");
            if (Particle.connected())
                Particle.publish(_appName, __appStatusStr);
            _isAliveLastMillis = millis();
            // Serial.println("Published Event to Particle Cloud");
        }

        // Check if a new status check is needed
        if (__queryStatusFn != NULL)
        {
            if (Utils::isTimeout(millis(), _statusUpdateLastMillis, STATUS_UPDATE_MILLIS))
            {
                // Check for status change
                unsigned long statusHash = 0;
                if (__queryStatusHashFn != NULL)
                    statusHash = __queryStatusHashFn();
                if ((statusHash != __lastStatusHashValue) || (__queryStatusHashFn == NULL))
                {
                    // Serial.println("Particle var status has changed");
                    __appStatusStr = __queryStatusFn(NULL, NULL);
                    __lastStatusHashValue = statusHash;
                    // Serial.println("Particle var status updated ok");
                }
                _statusUpdateLastMillis = millis();
            }
        }
    }

};
