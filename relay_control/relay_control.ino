#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
extern "C" {
#include "user_interface.h"
}

#include "secrets.h"
const long utcOffsetInSeconds = 0;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

byte relayON[] = {0xA0, 0x01, 0x01, 0xA2};  //Hex command to send to serial for open relay
byte relayOFF[] = {0xA0, 0x01, 0x00, 0xA1}; //Hex command to send to serial for close relay

byte targetTemperature = 0;
byte temperature = 0;
String lastTemperatureUpdate = "No updates from sensor, yet...";
byte operationMode = 0; // 0 = auto 1 = manual
byte relayStatus = 0; // 0 = Off 1 = On

// Set web server port number to 80
ESP8266WebServer server(80);


void relayOn() {
  Serial.write(relayON, sizeof(relayON));
  Serial.flush();
  relayStatus = 1;
}

void handleRelayOn() {
  relayOn();
  server.send(200, "text/html", "Relay is ON"); //Send ADC value only to client ajax request
}

void relayOff() {
  Serial.write(relayOFF, sizeof(relayOFF));
  Serial.flush();
  relayStatus = 0;
}

void handleRelayOff() {
  relayOff();
  server.send(200, "text/html", "Relay is OFF");
}

void handleTemperature() {
  String answer;
  if (server.method() == HTTP_POST) {
    String message = "POST form was:\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    if (server.argName(0) == "current") {
      temperature = server.arg(0).toInt();
      lastTemperatureUpdate = timeClient.getFormattedTime();
    }
    server.send(200, "text/plain", message);
  } else if (server.method() == HTTP_GET) {
    String answer = String(temperature, DEC) + " " + lastTemperatureUpdate;
    server.send(200, "text/plain", answer);
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleOperationMode() {
  if (server.method() == HTTP_POST) {
    String message = "POST form was:\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    if (server.argName(0) == "mode") {
      operationMode = server.arg(0).toInt();
    }
    server.send(200, "text/plain", message);
  } else if (server.method() == HTTP_GET) {
    String answer = String(operationMode, DEC);
    server.send(200, "text/plain", answer);
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleTargetTemperature() {
  if (server.method() == HTTP_POST) {
    String message = "POST form was:\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    if (server.argName(0) == "targetTemperature") {
      targetTemperature = server.arg(0).toInt();
    }
    server.send(200, "text/plain", message);
  } else if (server.method() == HTTP_GET) {
    String answer = String(targetTemperature, DEC);
    server.send(200, "text/plain", answer);
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleRoot() {

  const String postForms = String("<html>\
  <head>\
    <title>Temperature control</title>\
    <style>\
      body { font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Temperature control data</h1><br>\
    <p>Current status: ") + String(relayStatus,DEC) + String("</p>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/targetTemperature\">\
      <a>targetTemperature</a> \
      <input type=\"text\" name=\"targetTemperature\" value=\"") + String(targetTemperature) + String("\"><br>\
      <input type=\"submit\" value=\"Submit\">\
    </form>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/operationMode\">\
      <a>operationMode 0 = auto | 1 = manual</a> \
      <input type=\"text\" name=\"mode\" value=\"") + String(operationMode) + String("\"><br>\
      <input type=\"submit\" value=\"Submit\">\
    </form>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/temperature\">\
      <a>current temperature</a> \
      <input type=\"text\" name=\"current\" value=\"") + String(temperature) + String("\"> last update: ") + lastTemperatureUpdate + String("<br>\
      <input type=\"submit\" value=\"Submit\">\
    </form>\
  </body>\
</html>");
  server.send(200, "text/html", postForms);
}

void setup() {
  Serial.begin(9600);
  Serial.setDebugOutput(false);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  // Connect to Wi-Fi network with SSID and password
  WiFi.disconnect();
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.mode(WIFI_STA);
  WiFi.setPhyMode(WIFI_PHY_MODE_11B);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  MDNS.begin("relay");

  // Start the server
  server.on("/relayOn", handleRelayOn);
  server.on("/relayOff", handleRelayOff);
  server.on("/temperature", handleTemperature);
  server.on("/targetTemperature", handleTargetTemperature);
  server.on("/operationMode", handleOperationMode);
  server.on("/", handleRoot);
  server.begin();
  ArduinoOTA.begin();
  timeClient.begin();
}

void loop() {
  ArduinoOTA.handle();
  MDNS.update();
  timeClient.update();
  server.handleClient();          //Handle client requests
  if (temperature > targetTemperature && operationMode == 0 && relayStatus == 1) {
    relayOff();
  } else if (temperature < targetTemperature && operationMode == 0 && relayStatus == 0) {
    relayOn();
  }
}
