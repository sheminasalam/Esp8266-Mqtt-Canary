#include <ESP8266WiFi.h>

#include <PubSubClient.h>

#include <ESP8266WiFiMulti.h>

#include <WiFiClient.h>

#include <Arduino_JSON.h>

//FTP server parameters
#define FTP_USERNAME "admin"
#define FTP_PASSWORD "password"
#define FTP_PORT 21

ESP8266WiFiMulti wifiMulti;
WiFiServer ftpServer(FTP_PORT);
WiFiClient ftpClient;
String incoming;
//WiFiClient wifiClient;
PubSubClient mqttClient(ftpClient);

//Wifi Paremters
const char * Wifissid = "Wifi_SSID";
const char * Wifipassword = "Wifi_password";
const char * WifiHost = "Wifi_Host_Name";

//Mqtt parameters
const char * mqttServer = "mqtt_sever_ip";
const int mqttPort = 1883;
const char * mqttUsername = "mqtt_username";
const char * mqttPassword = "mqtt_password";
const char * mqttTopic = "mqtt_topic";
const char * mqttsyncTopic = "mqtt_sync_topic"; //topic to sync state at Device and HA boot

//Define custom MacAddress
uint8_t newMACAddress[] = {
  0x00,
  0x11,
  0x32,
  0x53,
  0x9D,
  0x36
};

void setup() {
  Serial.begin(115200);
  wifiMulti.addAP(Wifissid, Wifipassword);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(WifiHost);
  wifi_set_macaddr(STATION_IF, & newMACAddress[0]);
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    delay(1000);
  }

  ftpServer.begin();
  Serial.print("FTP server started on port ");
  Serial.println(FTP_PORT);

  mqttConnect();
  JSONVar data; // create a JSON object to hold our data
  data["alert"] = "OFF";
  data["Login"] = "none";
  data["IP"] = "none";

  String jsonString = JSON.stringify(data); // convert the JSON object to a string

  mqttClient.publish(mqttTopic, jsonString.c_str()); // publish the serialized JSON message    
}

void loop() {
  WiFiClient client = ftpServer.available();

  if (client) {
    Serial.println("New client connected");

    if (ftpAuthenticate(client)) {
      Serial.println("Authentication successful");

      /*      IPAddress ip = client.remoteIP();
            String mqttMessage = "Successful login attempt from " + ip.toString();
            mqttClient.publish(mqttTopic, mqttMessage.c_str());
      */

      // Create a JSON object
      IPAddress ip = client.remoteIP();
      JSONVar data; // create a JSON object to hold our data
      data["alert"] = "ON";
      data["Login"] = "Success";
      data["IP"] = ip.toString();

      String jsonString = JSON.stringify(data); // convert the JSON object to a string

      mqttClient.publish(mqttTopic, jsonString.c_str()); // publish the serialized JSON message     
    } else {
      Serial.println("Authentication failed");

      // Create a JSON object
      IPAddress ip = client.remoteIP();
      JSONVar data; // create a JSON object to hold our data
      data["alert"] = "ON";
      data["Login"] = "Failed";
      data["IP"] = ip.toString();

      String jsonString = JSON.stringify(data); // convert the JSON object to a string

      mqttClient.publish(mqttTopic, jsonString.c_str()); // publish the serialized JSON message
      /*      IPAddress ip = client.remoteIP();
            String mqttMessage = "Failed login attempt from " + ip.toString();
            mqttClient.publish(mqttTopic, mqttMessage.c_str());
      */
    }
  }

  mqttClient.loop();
}

bool ftpAuthenticate(WiFiClient client) {
  client.print("220 Welcome\r\n");

  while (client.connected()) {
    if (client.available()) {
      incoming = client.readStringUntil('\n');
      incoming.trim();
      Serial.println(incoming);

      if (incoming.startsWith("USER ")) {
        if (incoming.substring(5) == FTP_USERNAME) {
          client.println("331 Password required");
          while (client.connected()) {
            if (client.available()) {
              incoming = client.readStringUntil('\n');
              incoming.trim();
              Serial.println(incoming);
              if (incoming.startsWith("PASS ")) {
                if (incoming.substring(5) == FTP_PASSWORD) {
                  client.println("230 Login successful");
                  return true;
                } else {
                  client.println("530 Login incorrect");
                  return false;
                }
              }
            }
          }
        } else {
          client.println("530 Login incorrect");
          return false;
        }
      }
    }
  }

  return false;
}

void mqttConnect() {
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  while (!mqttClient.connected()) {
    Serial.println("Connecting to MQTT server...");

    if (mqttClient.connect("ESP8266Client", mqttUsername, mqttPassword)) {
      Serial.println("Connected to MQTT server");

      mqttClient.subscribe(mqttsyncTopic);
    } else {
      Serial.print("MQTT connection failed with error code ");
      Serial.println(mqttClient.state());

      delay(5000);
    }
  }
}

void mqttCallback(char * topic, byte * payload, unsigned int length) {

  JSONVar data; // create a JSON object to hold our data
  data["alert"] = "OFF";
  data["Login"] = "none";
  data["IP"] = "none";

  String jsonString = JSON.stringify(data); // convert the JSON object to a string

  mqttClient.publish(mqttTopic, jsonString.c_str()); // publish the serialized JSON message  
}
