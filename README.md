# Esp8266-Mqtt-Canary
Esp8266 Based Canary device alerting with Mqtt

This is a honeypot project for a local network. It is built using an ESP8266 WiFi module and Arduino programming language. The project is designed to detect unauthorized access to an FTP server and notify the user using MQTT messages.

# Features

	-Authentication for FTP server using predefined credentials.
	-MQTT client to publish messages on an MQTT server.
	-WiFi module to connect to a local network.
	-Custom MAC address configuration.
	-JSON message serialization to send alert messages to an MQTT server.
	-MQTT sync topic to synchronize state between Device and Home Assistant boot.
	
# Getting Started

	-Clone this repository to your local machine.
	-Open the project in Arduino IDE.
	-Install the required libraries (ESP8266WiFi.h, PubSubClient.h, ESP8266WiFiMulti.h, WiFiClient.h, and Arduino_JSON.h).
	-Configure the project parameters, such as WiFi credentials, MQTT server IP, MQTT credentials, and FTP server parameters.
	-Upload the sketch to the ESP8266 module using Arduino IDE.
	-Connect the ESP8266 module to the local network.
	-The project is now active and ready to detect unauthorized access to an FTP server.

# Home Assistant Config

Here's an example YAML configuration for a MQTT binary sensor in Home Assistant that sends a JSON payload with three data points:

```
binary_sensor:
  - platform: mqtt
    name: "Caller Alert Sensor"
    state_topic: "caller_alert"
    value_template: "{{ value_json.alert }}"
    payload_on: "ON"
    payload_off: "OFF"
    json_attributes_topic: "caller_alert"
    json_attributes_template: "{{ value_json | tojson }}"
```
