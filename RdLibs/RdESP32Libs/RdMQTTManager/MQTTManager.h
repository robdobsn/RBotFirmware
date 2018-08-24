// MQTT Manager
// Rob Dobson 2018

#include <AsyncMqttClient.h>
#include "WiFiManager.h"
#include "RestAPIEndpoints.h"
#include "ConfigNVS.h"

class MQTTManager
{
  public:
    static constexpr int DEFAULT_MQTT_PORT = 1883;
    static constexpr int RETRY_CONNECT_MS = 2000;
    AsyncMqttClient _mqttClient;
    WiFiManager &_wifiManager;
    bool _isConnected;
    unsigned long _lastDisconnectMs;
    RestAPIEndpoints &_restAPIEndpoints;
    String _mqttServer;
    int _mqttPort;
    String _mqttInTopic;
    String _mqttOutTopic;
    ConfigBase *_pConfigBase;

    MQTTManager(WiFiManager &wifiManager, RestAPIEndpoints &endpoints) : _wifiManager(wifiManager), _restAPIEndpoints(endpoints)
    {
        _isConnected = false;
        _lastDisconnectMs = 0;
        _mqttPort = DEFAULT_MQTT_PORT;
    }

    void onMqttConnect(bool sessionPresent)
    {
        _isConnected = true;
        Log.notice("MQTTManager: Connected to MQTT\n");
        Log.notice("MQTTManager: Session present = %d\n", sessionPresent);
        const char *subUrl = "home/office/shades";
        uint16_t packetIdSub = _mqttClient.subscribe(subUrl, 2);
        Log.notice("MQTTManager: Subscribing to %s at QoS 2, packetId: %d\n", subUrl, packetIdSub);
        // mqttClient.publish("test/lol", 0, true, "test 1");
        // Serial.println("Publishing at QoS 0");
        // uint16_t packetIdPub1 = mqttClient.publish("test/lol", 1, true, "test 2");
        // Serial.print("Publishing at QoS 1, packetId: ");
        // Serial.println(packetIdPub1);
        // uint16_t packetIdPub2 = mqttClient.publish("test/lol", 2, true, "test 3");
        // Serial.print("Publishing at QoS 2, packetId: ");
        // Serial.println(packetIdPub2);
    }

    void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
    {
        _isConnected = false;
        _lastDisconnectMs = millis();
        Log.notice("MQTTManager: Disconnected from MQTT\n");

        // if (WiFi.isConnected()) {
        //   xTimerStart(mqttReconnectTimer, 0);
        // }
    }

    void onMqttSubscribe(uint16_t packetId, uint8_t qos)
    {
        Log.notice("MQTTManager: Subscribe acknowledged packetId: %d qos: %d\n", packetId, qos);
    }

    void onMqttUnsubscribe(uint16_t packetId)
    {
        Log.notice("MQTTManager: Unsubscribe acknowledged packetId: %d\n", packetId);
    }

    void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
    {
        Log.notice("MQTTManager: Publish received topic: %s qos %d dup %d retain %d len %d idx %d total %d\n",
                   topic, properties.qos, properties.dup, properties.retain,
                   len, index, total);
        const int MAX_RESTAPI_REQ_LEN = 200;
        if (len < MAX_RESTAPI_REQ_LEN)
        {
            char reqStr[len + 1];
            for (int i = 0; i < len; i++)
                reqStr[i] = payload[i];
            reqStr[len] = 0;
            Log.notice("MQTTManager: Publish received payload: %s\n", reqStr);
            String respStr;
            _restAPIEndpoints.handleApiRequest(reqStr, respStr);
        }
    }

    void onMqttPublish(uint16_t packetId)
    {
        Log.notice("MQTTManager: Publish acknowledged packetId: %d\n", packetId);
    }

    void setup(ConfigBase *pConfig)
    {
        _pConfigBase = pConfig;
        if (!pConfig)
            return;
        // Get the server, port and in/out topics if available
        _mqttServer = pConfig->getString("MQTTServer", "");
        String mqttPortStr = pConfig->getString("MQTTPort", String(DEFAULT_MQTT_PORT).c_str());
        _mqttPort = mqttPortStr.toInt();
        _mqttInTopic = pConfig->getString("MQTTInTopic", "");
        _mqttOutTopic = pConfig->getString("MQTTOutTopic", "");
        if (_mqttServer.length() == 0)
            return;

        // Setup handlers for MQTT events
        _mqttClient.onConnect(std::bind(&MQTTManager::onMqttConnect, this, std::placeholders::_1));
        _mqttClient.onDisconnect(std::bind(&MQTTManager::onMqttDisconnect, this, std::placeholders::_1));
        _mqttClient.onSubscribe(std::bind(&MQTTManager::onMqttSubscribe, this, std::placeholders::_1, std::placeholders::_2));
        _mqttClient.onUnsubscribe(std::bind(&MQTTManager::onMqttUnsubscribe, this, std::placeholders::_1));
        _mqttClient.onMessage(std::bind(&MQTTManager::onMqttMessage, this, std::placeholders::_1, std::placeholders::_2,
                                        std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
        _mqttClient.onPublish(std::bind(&MQTTManager::onMqttPublish, this, std::placeholders::_1));

        // Set the server
        _mqttClient.setServer(_mqttServer.c_str(), _mqttPort);
    }

    String formConfigStr()
    {
        return "{\"MQTTServer\":\"" + _mqttServer + "\",\"MQTTPort\":\"" + String(_mqttPort) + "\",\"MQTTInTopic\":\"" +
                         _mqttInTopic + "\",\"MQTTOutTopic\":\"" + _mqttOutTopic + "\"}";
    }

    void setMQTTServer(String &mqttServer, String &mqttPort, String &mqttInTopic, String &mqttOutTopic)
    {
        String oldServer = _mqttServer;
        int oldPort = _mqttPort;
        _mqttServer = mqttServer;
        if (mqttPort.length() > 0)
            _mqttPort = mqttPort.toInt();
        if (mqttInTopic.length() > 0)
            _mqttInTopic = mqttInTopic;
        if (mqttOutTopic.length() > 0)
            _mqttOutTopic = mqttOutTopic;
        if (_pConfigBase)
        {
            _pConfigBase->setConfigData(formConfigStr().c_str());
            _pConfigBase->writeConfig();
        }
        // Disconnect so re-connection with new credentials occurs
        Log.trace("MQTTManager: setMQTTServer disconnecting to allow new connection\n");
        if (_isConnected)
        {
            _mqttClient.disconnect(true);
            _isConnected = false;
        }
        if (oldServer != _mqttServer || oldPort != _mqttPort)
        {
            _mqttClient.setServer(_mqttServer.c_str(), _mqttPort);
        }
    }

    void report(const char *reportStr)
    {
        if (!_isConnected)
            return;
        uint16_t packetIdPub1 = _mqttClient.publish(_mqttOutTopic.c_str(), 1, true, reportStr);
        Log.trace("MQTTManager: Publishing to %s at QoS 1, packetId: %d\n", _mqttOutTopic.c_str(), packetIdPub1);
    }

    void service()
    {
        if (_isConnected)
            return;
        if (_wifiManager.isConnected() && _mqttServer.length() > 0)
        {
            if (Utils::isTimeout(millis(), _lastDisconnectMs, RETRY_CONNECT_MS))
            {
                Log.notice("MQTTManager: Connecting to MQTT client\n");
                _mqttClient.connect();
                _lastDisconnectMs = millis();
            }
        }
    }
};
