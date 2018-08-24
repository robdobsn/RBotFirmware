// REST API for System, WiFi, etc
// Rob Dobson 2012-2018

#pragma once
#include <Arduino.h>
#include "RestAPIEndpoints.h"

class RestAPISystem
{
  public:
    void apiWifiSet(String &reqStr, String &respStr)
    {
        bool rslt = false;
        // Get SSID
        String ssid = RestAPIEndpoints::getNthArgStr(reqStr.c_str(), 1);
        Log.trace("RestAPISystem: WiFi SSID %s\n", ssid.c_str());
        // Get pw
        String pw = RestAPIEndpoints::getNthArgStr(reqStr.c_str(), 2);
        Log.trace("RestAPISystem: WiFi PW %s\n", pw.c_str());
        // Get hostname
        String hostname = RestAPIEndpoints::getNthArgStr(reqStr.c_str(), 3);
        Log.trace("RestAPISystem: Hostname %s\n", hostname.c_str());
        // Check if both SSID and pw have now been set
        if (ssid.length() != 0 && pw.length() != 0)
        {
            Log.notice("RestAPISystem: WiFi Credentials Added SSID %s\n", ssid.c_str());
            wifiManager.setCredentials(ssid, pw, hostname);
            rslt = true;
        }
        respStr = Utils::setJsonBoolResult(rslt);
    }

    void apiWifiClear(String &reqStr, String &respStr)
    {
        // Clear stored SSIDs
        // wifiManager.clearCredentials();
        Log.notice("RestAPISystem: Cleared WiFi Credentials\n");
        respStr = Utils::setJsonBoolResult(true);
    }

    void apiWifiExtAntenna(String &reqStr, String &respStr)
    {
        Log.notice("RestAPISystem: Set external antenna - not supported\n");
        respStr = Utils::setJsonBoolResult(true);
    }

    void apiWifiIntAntenna(String &reqStr, String &respStr)
    {
        Log.notice("RestAPISystem: Set internal antenna - not supported\n");
        respStr = Utils::setJsonBoolResult(true);
    }

    void apiMQTTSet(String &reqStr, String &respStr)
    {
        bool rslt = false;
        // Get Server
        String server = RestAPIEndpoints::getNthArgStr(reqStr.c_str(), 1);
        Log.trace("RestAPISystem: MQTTServer %s\n", server.c_str());
        // Get port
        String port = RestAPIEndpoints::getNthArgStr(reqStr.c_str(), 2);
        Log.trace("RestAPISystem: MQTTPort %s\n", port.c_str());
        // Get mqtt in topic
        String inTopic = RestAPIEndpoints::getNthArgStr(reqStr.c_str(), 3);
        inTopic.replace("~", "/");
        Log.trace("RestAPISystem: MQTTInTopic %s\n", inTopic.c_str());
        // Get mqtt out topic
        String outTopic = RestAPIEndpoints::getNthArgStr(reqStr.c_str(), 4);
        outTopic.replace("~", "/");
        Log.trace("RestAPISystem: MQTTOutTopic %s\n", outTopic.c_str());
        mqttManager.setMQTTServer(server, port, inTopic, outTopic);
        rslt = true;
        respStr = Utils::setJsonBoolResult(rslt);
    }

    void apiReset(String &reqStr, String& retStr)
    {
        // Restart the device
        ESP.restart();
    }


    void setup(RestAPIEndpoints &endpoints)
    {
        endpoints.addEndpoint("w", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET, 
                        std::bind(&RestAPISystem::apiWifiSet, this, std::placeholders::_1, std::placeholders::_2),
                        "Setup WiFi SSID/password/hostname");
        endpoints.addEndpoint("wc", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET, 
                        std::bind(&RestAPISystem::apiWifiClear, this, std::placeholders::_1, std::placeholders::_2),
                        "Clear WiFi settings");
        endpoints.addEndpoint("wax", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET, 
                        std::bind(&RestAPISystem::apiWifiExtAntenna, this, std::placeholders::_1, std::placeholders::_2), 
                        "Set external WiFi Antenna");
        endpoints.addEndpoint("wai", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET, 
                        std::bind(&RestAPISystem::apiWifiIntAntenna, this, std::placeholders::_1, std::placeholders::_2), 
                        "Set internal WiFi Antenna");
        endpoints.addEndpoint("m", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET, 
                        std::bind(&RestAPISystem::apiMQTTSet, this, std::placeholders::_1, std::placeholders::_2),
                        "Setup MQTT server/port/intopic/outtopic .. not ~ replaces / in topics");
        endpoints.addEndpoint("reset", RestAPIEndpointDef::ENDPOINT_CALLBACK, RestAPIEndpointDef::ENDPOINT_GET, 
                        std::bind(&RestAPISystem::apiReset, this, std::placeholders::_1, std::placeholders::_2), 
                        "Restart program");
    }

    String getWifiStatusStr()
    {
        if (WiFi.status() == WL_CONNECTED)
            return "C";
        if (WiFi.status() == WL_NO_SHIELD)
            return "4";
        if (WiFi.status() == WL_IDLE_STATUS)
            return "I";
        if (WiFi.status() == WL_NO_SSID_AVAIL)
            return "N";
        if (WiFi.status() == WL_SCAN_COMPLETED)
            return "S";
        if (WiFi.status() == WL_CONNECT_FAILED)
            return "F";
        if (WiFi.status() == WL_CONNECTION_LOST)
            return "L";
        return "D";
    }

    int reportHealth(int bitPosStart, unsigned long *pOutHash, String *pOutStr)
    {
        // Generate hash if required
        if (pOutHash)
        {
            unsigned long hashVal = (WiFi.status() == WL_CONNECTED);
            hashVal = hashVal << bitPosStart;
            *pOutHash += hashVal;
            *pOutHash ^= WiFi.localIP();
        }
        // Generate JSON string if needed
        if (pOutStr)
        {
            byte mac[6];
            WiFi.macAddress(mac);
            String macStr = String(mac[0], HEX) + ":" + String(mac[1], HEX) + ":" + String(mac[2], HEX) + ":" +
                            String(mac[3], HEX) + ":" + String(mac[4], HEX) + ":" + String(mac[5], HEX);
            String sOut = "\"wifiIP\":\"" + WiFi.localIP().toString() + "\",\"wifiConn\":\"" + getWifiStatusStr() + "\","
                                                                                                                    "\"ssid\":\"" +
                          WiFi.SSID() + "\",\"MAC\":\"" + macStr + "\",\"RSSI\":" + String(WiFi.RSSI());
            *pOutStr = sOut;
        }
        // Return number of bits in hash
        return 8;
    }
};
