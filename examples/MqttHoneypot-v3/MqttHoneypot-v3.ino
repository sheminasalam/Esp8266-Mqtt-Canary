/*
  ESPCanary by AnotherMaker
  This script will turn an ESP8266 or an ESP32 into a honeypot FTP server.

  After setting your basic configuration below, this script will join your network and present
  itself as a standard FTP server (Synology). If someone connects to it using the username admin and
  ANY password, it will hit a webhook.  Most people will just want to go to https://opencanary.org
  and generate a free "Web Bug/URL token" and paste that url below. This token will automatically email you when someone hits the FTP server.

  If you want to get a little fancier, you can setup your own webhook url on your own server and
  ESPCanary will POST the ip address that tried to access the FTP server to that webhook.

  You can also use the SPIFFS file system to upload typical honeypot type files such as fake API keys and anything else you can get creative and generate.
*/

#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#include "SPIFFS.h"
#endif
#include <ESP8266WiFiMulti.h>

#include <ESPCanary.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// Define MQTT parameters
const char* mqttServer = "mqttServer";
const int mqttPort = 1883;
const char* mqttUsername = "mqttUsername";
const char* mqttPassword = "mqttPassword";
const char* mqttTopic = "esp/canary-alert";
const char* mqttSyncTopic = "esp/canary-sync"; //this is to read the state of switch when mqtt reconnects
const char* deviceId = "esp09344";
int attempts = 0;
int trigger = 0;


//Wifi Paremters
const char * Wifissid = "Wifissid";
const char * Wifipassword = "Wifipassword";
const char * WifiHost = "Synology";
uint8_t newMACAddress[] = {0x00, 0x11, 0x32, 0x07, 0x6D, 0x96};
// if you would like to create your own mac address for your canary...
// by default we're using a Synology mac address (0x00, 0x11, 0x32)
// must be unique per network




WiFiClient ftpClient1;
//WiFiClient wifiClient;
PubSubClient mqttClient1(ftpClient1);

String canary = "PASTE_CANARY TOKEN HERE";  //grab FREE web bug/URL tokens at http://canarytokens.org
String ftp_user = "admin";    //if you replace this with "%" it will accept ANY username
String ftp_pass = "password"; //if you replace this with "%" it will accept ANY password
bool append_ip = false;       //if you are using a canary token, leave this as false
String append_char = "?";     //if you are using a canary token, this doesn't matter
//if you are using your own webhook,with a bunch of GET
//parameters then you would want this to be "&" so the IP
//address becomes the final GET parameter

ESP8266WiFiMulti wifiMulti;
FtpServer ftpSrv;   //set #define FTP_DEBUG in ESPCanary.h to see ftp verbose on serial

void setup(void) {
  Serial.begin(115200);

  wifiMulti.addAP(Wifissid, Wifipassword);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(WifiHost);
  wifi_set_macaddr(STATION_IF, & newMACAddress[0]);
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    delay(1000);
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());

  /////FTP Setup, ensure SPIFFS is started before ftp;  /////////
#ifdef ESP32       //esp32 we send true to format spiffs if cannot mount
  if (SPIFFS.begin(true)) {
#elif defined ESP8266
  if (SPIFFS.begin()) {
#endif
    Serial.println("SPIFFS opened!");
    ftpSrv.begin(ftp_user, ftp_pass, canary, append_ip, append_char); //username, password for ftp.  set ports in ESPCanary.h  (default 21, 50009 for PASV)
  }
  mqttConnect();
}
void mqttConnect() {

  mqttClient1.setServer(mqttServer, mqttPort);
  mqttClient1.setCallback(mqttCallback1);
  while (!mqttClient1.connected() && attempts < 30) {
    Serial.println("Connecting to MQTT server...");
    if (mqttClient1.connect(deviceId, mqttUsername, mqttPassword) && trigger == 1) {
      attempts = 0;
      trigger = 0;
      delay(200);
      mqttClient1.subscribe(mqttSyncTopic);
      Serial.println("Connected to MQTT server and Subscirbed");

    }
    else if (mqttClient1.connect(deviceId, mqttUsername, mqttPassword)) {
      Serial.println("Connected to MQTT server");
      attempts = 0;
      mqttConnect();
      StaticJsonDocument<256> doc;
      doc["alert"] = "OFF";
      doc["IP"] = "none";

      String message;
      serializeJson(doc, message);

      Serial.println("Publishing MQTT message:");
      Serial.println(message);

      // Publish the MQTT message
      mqttClient1.publish(mqttTopic, message.c_str());

      mqttClient1.subscribe(mqttSyncTopic);
    } else {
      if (  attempts < 30) {
        attempts++;
        Serial.print("MQTT connection failed with error code ");
        Serial.println(mqttClient1.state());

        delay(10000);
      }
      else if (  attempts > 30) {
        attempts++;
        Serial.print("MQTT connection failed with error code ");
        Serial.println(mqttClient1.state());
        delay(60000);
      }

      else if (  attempts > 60) {
        attempts++;
        Serial.print("Restarting Esp8266");
        ESP.restart();
        delay(200);
      }
    }

  }
}

void mqttCallback1(char * topic, byte * payload, unsigned int length) {

  StaticJsonDocument<256> doc;
  doc["alert"] = "OFF";
  doc["IP"] = "none";

  String message;
  serializeJson(doc, message);

  Serial.println("Publishing MQTT message:");
  Serial.println(message);

  // Publish the MQTT message
  mqttClient1.publish(mqttTopic, message.c_str());
}
void loop() {
  ftpSrv.handleFTP();        //make sure in loop you call handleFTP()!!
  // server.handleClient();   //example if running a webserver you still need to call .handleClient();
  if (!mqttClient1.connected()) { // check if MQTT client is disconnected
    mqttConnect(); // call the function to reconnect MQTT
  }
  mqttClient1.loop();

}
