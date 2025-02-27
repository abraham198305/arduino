/*
  IPLocation by Meeo

  This example will make use of Meeo. If you haven't already,
  visit Meeo at https://meeo.io and create an account. Then
  check how to get started with the Meeo library through
  https://github.com/meeo/meeo-arduino

  OTHER REQUIREMENTS
  Under Sketch > Include Library > Manage Libraries...
  Search and install the following:
  * ESP8266RestClient by fabianofranca
  * ArduinoJson by Benoit Blanchon

  Copyright: Meeo
  Author: Terence Anton Dela Fuente
  License: MIT
*/
#include <Math.h>
#include <Meeo.h>
#include <RestClient.h>
#include <ArduinoJson.h>

// Uncomment if you wish to see the events on the Meeo dashboard
#define LOGGER_CHANNEL "logger"

String nameSpace = "apaa-p3k4tojn";
String accessKey = "user_wOZhnuxnibIOHNLD";
String ssid = "JioFiAbraham";
String pass = "0110401611";
String variableValueChannel = "variable-value";

// ipapi.co will serve as our GPS tracker based on the current IP address of the
// device
RestClient client = RestClient("ipapi.co");
DynamicJsonBuffer jsonBuffer;
float i=0;
unsigned long previous = 0;

void setup() {
  Serial.begin(115200);

  Meeo.setEventHandler(meeoEventHandler);
  Meeo.setDataReceivedHandler(meeoDataHandler);
  Meeo.begin(nameSpace, accessKey, ssid, pass);

  #ifdef LOGGER_CHANNEL
    Meeo.setLoggerChannel(LOGGER_CHANNEL);
  #endif
}

void loop() {
  Meeo.run();

  unsigned long now = millis();
  // Check value every 1 seconds
  if (now - previous >= 1000) 
  {
    previous = now;
    String variableValue = (String)(0.5 + 0.5*sin(i));  
    i+=0.1;
    Meeo.publish(variableValueChannel, variableValue);

    #ifdef LOGGER_CHANNEL
      Meeo.println(variableValue);
    #endif
  }
}

void meeoDataHandler(String topic, String payload) {
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(payload);
}

void meeoEventHandler(MeeoEventType event) {
  switch (event) {
    case WIFI_DISCONNECTED:
      Serial.println("Not Connected to WiFi");
      break;
    case WIFI_CONNECTING:
      Serial.println("Connecting to WiFi");
      break;
    case WIFI_CONNECTED:
      Serial.println("Connected to WiFi");
      break;
    case MQ_DISCONNECTED:
      Serial.println("Not Connected to MQTT Server");
      break;
    case MQ_CONNECTED:
      Serial.println("Connected to MQTT Server");
      break;
    case MQ_BAD_CREDENTIALS:
      Serial.println("Bad Credentials");
      break;
    case AP_MODE:
      Serial.println("AP Mode");
      break;
    default:
      break;
  }
}
