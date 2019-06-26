// AsyncTelnetServer 
// Rob Dobson 2018

#pragma once

#include <Arduino.h>
#include "AsyncTCP.h"
#include <vector>

typedef std::function<void(void* cbArg, const char *pData, size_t numChars)> AsyncTelnetDataHandler;

class AsyncTelnetServer;

class AsyncTelnetSession
{
private:
    AsyncClient *_pClient;
    AsyncTelnetServer *_pServer;

    void _onError(int8_t error);
    void _onAck(size_t len, uint32_t time);
    void _onDisconnect();
    void _onTimeout(uint32_t time);
    void _onData(void *buf, size_t len);
    void _onPoll();

public:
    AsyncTelnetSession(AsyncTelnetServer *pServer, AsyncClient *pClient);
    ~AsyncTelnetSession();
    void forceClose();
    void sendChars(const char* pChars, int numChars);
};

class AsyncTelnetServer
{
protected:
    // Server and active sessions list
    AsyncServer _server;
    std::vector<AsyncTelnetSession*> _sessions;

    // Called by AsyncTelnetSession on events
    void _handleDisconnect(AsyncTelnetSession *pSess);
    void _handleData(const char* pData, int numChars);

    // Callback and argument for callback set in onData()
    AsyncTelnetDataHandler _rxDataCallback;
    void* _rxDataCallbackArg;

public:
    static const int MAX_SESSIONS = 3;

    // Construction/Destruction
    AsyncTelnetServer(int port);
    ~AsyncTelnetServer();

    // Begin listening for telnet sessions
    void begin();
    
    // Callback on data received from any telnet session
    void onData(AsyncTelnetDataHandler cb, void* arg = 0);

    // Send chars to all active telnet sessions
    void sendChars(const char* pChars, int numChars);

    friend class AsyncTelnetSession;
};
