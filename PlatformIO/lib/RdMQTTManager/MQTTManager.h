// MQTT Manager
// Rob Dobson 2018

#pragma once
#include "WiFiManager.h"
#include "RestAPIEndpoints.h"
#include "ConfigNVS.h"
#include <PubSubClient.h>

class MQTTManager
{
  public:
    // Default MQTT Port
    static constexpr int DEFAULT_MQTT_PORT = 1883;

  private:

    // Enabled
    bool _mqttEnabled;

    // Retry delay if connection fails
    static constexpr int CONNECT_RETRY_MS = 2000;

    // WiFi manager & client
    WiFiManager &_wifiManager;
    WiFiClient _wifiClient;

    // Endpoint handler (used when MQTT message is received)
    RestAPIEndpoints &_restAPIEndpoints;

    // MQTT client
    PubSubClient _mqttClient;

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

  public:
    MQTTManager(WiFiManager &wifiManager, RestAPIEndpoints &endpoints) : 
            _wifiManager(wifiManager), _restAPIEndpoints(endpoints), _mqttClient(_wifiClient)
    {
        _mqttEnabled = false;
        _mqttPort = DEFAULT_MQTT_PORT;
        _wasConnected = false;
        _lastConnectRetryMs = millis();
    }

    void onMqttMessage(char *topic, byte *payload, unsigned int len)
    {
        Log.notice("MQTTManager: rx topic: %s len %d\n", topic, len);
        const int MAX_RESTAPI_REQ_LEN = 200;
        if (len < MAX_RESTAPI_REQ_LEN)
        {
            char reqStr[len + 1];
            for (int i = 0; i < len; i++)
                reqStr[i] = payload[i];
            reqStr[len] = 0;
            // Log.notice("MQTTManager: rx payload: %s\n", reqStr);
            String respStr;
            _restAPIEndpoints.handleApiRequest(reqStr, respStr);
        }
    }

    void setup(ConfigBase& hwConfig, ConfigBase *pConfig)
    {
        // Check enabled
        _mqttEnabled = hwConfig.getLong("mqttEnabled", 0) != 0;
        _pConfigBase = pConfig;
        if ((!_mqttEnabled) || (!pConfig))
            return;

        // Get the server, port and in/out topics if available
        _mqttServer = pConfig->getString("MQTTServer", "");
        String mqttPortStr = pConfig->getString("MQTTPort", String(DEFAULT_MQTT_PORT).c_str());
        _mqttPort = mqttPortStr.toInt();
        _mqttInTopic = pConfig->getString("MQTTInTopic", "");
        _mqttOutTopic = pConfig->getString("MQTTOutTopic", "");
        if (_mqttServer.length() == 0)
            return;

        // Debug
        Log.trace("MQTTManager: server %s:%d, inTopic %s, outTopic %s\n", _mqttServer.c_str(), _mqttPort, _mqttInTopic.c_str(), _mqttOutTopic.c_str());

        // Setup server and callback on receive
        _mqttClient.setServer(_mqttServer.c_str(), _mqttPort);
        _mqttClient.setCallback(std::bind(&MQTTManager::onMqttMessage, this, std::placeholders::_1, std::placeholders::_2,
                                         std::placeholders::_3));
    }

    String formConfigStr()
    {
        // This string is stored in NV ram for configuration on power up
        return "{\"MQTTServer\":\"" + _mqttServer + "\",\"MQTTPort\":\"" + String(_mqttPort) + "\",\"MQTTInTopic\":\"" +
               _mqttInTopic + "\",\"MQTTOutTopic\":\"" + _mqttOutTopic + "\"}";
    }

    void setMQTTServer(String &mqttServer, String &mqttInTopic, String &mqttOutTopic, int mqttPort = DEFAULT_MQTT_PORT)
    {
        _mqttServer = mqttServer;
        if (mqttInTopic.length() > 0)
            _mqttInTopic = mqttInTopic;
        if (mqttOutTopic.length() > 0)
            _mqttOutTopic = mqttOutTopic;
        _mqttPort = mqttPort;
        if (_pConfigBase)
        {
            _pConfigBase->setConfigData(formConfigStr().c_str());
            _pConfigBase->writeConfig();
        }

        // Check if enabled
        if (!_mqttEnabled)
            return;
        // Disconnect so re-connection with new credentials occurs
        if (_mqttClient.connected())
        {
            Log.trace("MQTTManager: setMQTTServer disconnecting to allow new connection\n");
            _mqttClient.disconnect();
        }
        else
        {
            Log.trace("MQTTManager: setMQTTServer %s:%n\n", _mqttServer.c_str(), _mqttPort);
            _mqttClient.setServer(_mqttServer.c_str(), _mqttPort);
        }
    }

    void report(const char *reportStr)
    {
        // Check if enabled
        if (!_mqttEnabled)
            return;

        // Check if connected
        if (!_mqttClient.connected())
        {
            // Store until connected
            if (_mqttMsgToSendWhenConnected.length() == 0)
                _mqttMsgToSendWhenConnected = reportStr;
            return;
        }

        // Send immediately
        bool publishedOk = _mqttClient.publish(_mqttOutTopic.c_str(), reportStr, true);
        Log.verbose("MQTTManager: Published to %s at QoS 0, publishedOk %d\n", _mqttOutTopic.c_str(), publishedOk);
    }

    // Note do not put any Log messages in here as MQTT may be used for logging
    // and an infinite loop would result
    void reportSilent(const char *reportStr)
    {
        // Check if enabled
        if (!_mqttEnabled)
            return;

        // Check if connected
        if (!_mqttClient.connected())
        {
            // Store until connected
            if (_mqttMsgToSendWhenConnected.length() == 0)
                _mqttMsgToSendWhenConnected = reportStr;
            return;
        }

        // Send immediately
        _mqttClient.publish(_mqttOutTopic.c_str(), reportStr, true);
    }

    void service()
    {
        // Check if enabled
        if (!_mqttEnabled)
            return;

        // See if we are connected
        if (_mqttClient.connected())
        {
            // Service the client
            _mqttClient.loop();
        }
        else
        {

            // Check if we were previously connected
            if (_wasConnected)
            {
                // Set last connected time here so we hold off for a short time after disconnect
                // before trying to reconnect
                _lastConnectRetryMs = millis();
                Log.notice("MQTTManager: Disconnected, status code %d\n", _mqttClient.state());
                // Set server for next connection (in case it was changed)
                _mqttClient.setServer(_mqttServer.c_str(), _mqttPort);
                _wasConnected = false;
                return;
            }

            // Check if ready to reconnect
            if (_wifiManager.isConnected() && _mqttServer.length() > 0)
            {
                if (Utils::isTimeout(millis(), _lastConnectRetryMs, CONNECT_RETRY_MS))
                {
                    Log.notice("MQTTManager: Connecting to MQTT client\n");
                    if (_mqttClient.connect(_wifiManager.getHostname().c_str()))
                    {
                        // Connected
                        Log.notice("MQTTManager: Connected to MQTT\n");
                        _wasConnected = true;
                        
                        // Subscribe to in topic
                        if (_mqttInTopic.length() > 0)
                        {
                            bool subscribedOk = _mqttClient.subscribe(_mqttInTopic.c_str(), 0);
                            Log.notice("MQTTManager: Subscribing to %s at QoS 0, subscribe %s\n", _mqttInTopic.c_str(), 
                                            subscribedOk ? "OK" : "Failed");
                        }

                        // Re-send last message sent when disconnected (if any)
                        if (_mqttOutTopic.length() > 0 && _mqttMsgToSendWhenConnected.length() > 0)
                        {
                            // Send message "queued" to be sent
                            bool publishedOk = _mqttClient.publish(_mqttOutTopic.c_str(), _mqttMsgToSendWhenConnected.c_str(), true);
                            Log.trace("MQTTManager: Published to %s at QoS 0, result %s\n", _mqttOutTopic.c_str(), 
                                                    publishedOk ? "OK" : "Failed");
                            _mqttMsgToSendWhenConnected = "";
                        }

                    }
                    else
                    {
                        Log.notice("MQTTManager: Connect failed\n");
                    }
                    _lastConnectRetryMs = millis();
                }
            }
        }
    }
};

