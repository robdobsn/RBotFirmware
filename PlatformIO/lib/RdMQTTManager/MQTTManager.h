// MQTT Manager
// Rob Dobson 2018

#pragma once

#define MQTT_USE_ASYNC_MQTT 1

#include "WiFiManager.h"
#include "RestAPIEndpoints.h"
#include "ConfigNVS.h"
#ifdef MQTT_USE_ASYNC_MQTT
#include <AsyncMqttClient.h>
#else
#include <PubSubClient.h>
#endif

// #define STRESS_TEST_MQTT 1

class MQTTManager
{
public:
    // Default MQTT Port
    static constexpr int DEFAULT_MQTT_PORT = 1883;

private:

    // Max accepted payload length
    static const int MAX_PAYLOAD_LEN = 5000;

    // Enabled
    bool _mqttEnabled;

    // Retry delay if connection fails
    static constexpr int CONNECT_RETRY_MS = 10000;

    // WiFi manager & client
    WiFiManager &_wifiManager;
    WiFiClient _wifiClient;

    // Endpoint handler (used when MQTT message is received)
    RestAPIEndpoints &_restAPIEndpoints;

    // MQTT client
#ifdef MQTT_USE_ASYNC_MQTT
    AsyncMqttClient _mqttClient;
#else
    PubSubClient _mqttClient;
#endif

    // MQTT details
    String _mqttServer;
    int _mqttPort;
    String _mqttInTopic;
    String _mqttOutTopic;

    // Connection details
    bool _wasConnected;
    unsigned long _lastConnectRetryMs;

    // MQTT configuration (object held elsewhere and pointer kept
    // to allow config changes to be written back)
    ConfigBase *_pConfigBase;

    // Pending MQTT messages to be published when connected
    String _mqttMsgToSendWhenConnected;

#ifdef STRESS_TEST_MQTT
int curRxHelloCount;
unsigned long  _stressTestLastSendTime;
int _stressTestCounts[3];
#endif

public:
#ifdef MQTT_USE_ASYNC_MQTT
    MQTTManager(WiFiManager &wifiManager, RestAPIEndpoints &endpoints) : 
            _wifiManager(wifiManager), _restAPIEndpoints(endpoints)
#else
    MQTTManager(WiFiManager &wifiManager, RestAPIEndpoints &endpoints) : 
            _wifiManager(wifiManager), _restAPIEndpoints(endpoints), 
            _mqttClient(_wifiClient)
#endif
    {
        _mqttEnabled = false;
        _mqttPort = DEFAULT_MQTT_PORT;
        _wasConnected = false;
        _lastConnectRetryMs = 0;
#ifdef STRESS_TEST_MQTT
        curRxHelloCount = -1;
        _stressTestLastSendTime = millis();
        _stressTestCounts[0] = _stressTestCounts[1] = _stressTestCounts[2] = 0;
#endif
    }

#ifdef MQTT_USE_ASYNC_MQTT

    void onMqttConnect(bool sessionPresent);
    void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
    void onMqttSubscribe(uint16_t packetId, uint8_t qos);
    void onMqttUnsubscribe(uint16_t packetId);
    void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
    void onMqttPublish(uint16_t packetId);

#else

    void onMqttMessage(char *topic, byte *payload, unsigned int len);

#endif

    void setup(ConfigBase& hwConfig, ConfigBase *pConfig);
    String formConfigStr();
    void setMQTTServer(String &mqttServer, String &mqttInTopic, String &mqttOutTopic, int mqttPort = DEFAULT_MQTT_PORT);
    void reportJson(String& msg);
    void report(const char *reportStr);
    void reportSilent(const char *reportStr);
    void service();
};

