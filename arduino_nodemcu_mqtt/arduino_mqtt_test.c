/***************************************************
Adafruit MQTT Library ESP8266 Example
Must use ESP8266 Arduino from:
https://github.com/esp8266/Arduino
Works great with Adafruit’s Huzzah ESP board & Feather
—-> https://www.adafruit.com/product/2471
—-> https://www.adafruit.com/products/2821
Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!
Written by Tony DiCola for Adafruit Industries.
MIT license, all text above must be included in any redistribution
****************************************************/
#include <ESP8266WiFi.h>
#include “Adafruit_MQTT.h”
#include “Adafruit_MQTT_Client.h”
#include “DHT.h”
/************************* DHT Type And Data Pin *****************************/
#define DHTPIN 2
#define DHTTYPE DHT11
/************************* WiFi Access Point *********************************/
#define WLAN_SSID       “JioFiPollayil” \\Replace “x” with the value
#define WLAN_PASS       “pollayil”
/************************* Adafruit.io Setup *********************************/
#define AIO_SERVER      “io.adafruit.com”
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    “abeyalexander”
#define AIO_KEY         “938f74dec434480b8cd1138c9a49ba8b”
/************ Global State (you don’t need to change this!) ******************/
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// or… use WiFiFlientSecure for SSL
//WiFiClientSecure client;
// Store the MQTT server, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_USERNAME, MQTT_PASSWORD);
/****************************** Feeds ***************************************/
// Setup a feed called ‘photocell’ for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char PHOTOCELL_FEED[] PROGMEM = AIO_USERNAME “/feeds/photocell”;
Adafruit_MQTT_Publish photocell = Adafruit_MQTT_Publish(&mqtt, PHOTOCELL_FEED);
const char TEMPANDHUMID_FEED[] PROGMEM = AIO_USERNAME “/feeds/TempandHumid”;
Adafruit_MQTT_Publish TempandHumid = Adafruit_MQTT_Publish(&mqtt, TEMPANDHUMID_FEED);
// Setup a feed called ‘onoff’ for subscribing to changes.
const char ONOFF_FEED[] PROGMEM = AIO_USERNAME “/feeds/onoff”;
Adafruit_MQTT_Subscribe onoff = Adafruit_MQTT_Subscribe(&mqtt, ONOFF_FEED);
/*************************** Sketch Code ************************************/
// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();
DHT dht(DHTPIN, DHTTYPE);
void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println(“DHT11 test!”);
  Serial.println(F(“Adafruit MQTT demo”));
// Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print(“Connecting to “);
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(“.”);
  }
  Serial.println();
Serial.println(“WiFi connected”);
Serial.println(“IP address: “); Serial.println(WiFi.localIP());
dht.begin();
// Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&onoff);
}
uint32_t x=0;
void loop() {
// Ensure the connection to the MQTT server is alive (this will make the first
// connection and automatically reconnect when disconnected).  See the MQTT_connect
// function definition further below.
  MQTT_connect();
// this is our ‘wait for incoming subscription packets’ busy subloop
// try to spend your time here
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(10000))) {
    if (subscription == &onoff) {
      Serial.print(F(“Got: “));
      Serial.println((char *)onoff.lastread);
    }
  }
// Sensor readings may also be up to 2 seconds ‘old’ (its a very slow sensor)
  float h = dht.readHumidity();
// Read temperature as Celsius (the default)
  float t = dht.readTemperature();
// Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
// Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(“Failed to read from DHT sensor!”);
    return;
  }
// Now we can publish stuff!
  Serial.print(F(“\nSending photocell val “));
  Serial.print(x);
  Serial.print(“…”);
  if (! photocell.publish(x++)) {
    Serial.println(F(“Failed”));
  } else {
    Serial.println(F(“OK!”));
  }
char message_buff[160];
// Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
// Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);
  Serial.print(F(“Humidity: “));
  Serial.print(h);
  Serial.print(F(” %\t”));
  Serial.print(F(“Temperature: “));
  Serial.print(t);
  Serial.print(F(” *C “));
  Serial.print(f);
  Serial.print(F(” *F\t”));
  String humid = String(h);
  String TempC = String(t);
  String TempF = String(f);
  String pub = “Humidity: ” + humid+ ” %”+”     Temperature: “+ TempC + ” C   “+TempF+”  F”;
 pub.toCharArray(message_buff, pub.length()+1);
  if (! TempandHumid.publish(message_buff)) {
    Serial.println(F(“Failed”));
  } else {
    Serial.println(F(“OK!”));
  }
// ping the server to keep the mqtt connection alive
// NOT required if you are publishing once every KEEPALIVE seconds
/*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
*/
}
// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;
// Stop if already connected.
  if (mqtt.connected()) {
    return;
  }
  Serial.print(“Connecting to MQTT… “);
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println(“Retrying MQTT connection in 5 seconds…”);
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries–;
       if (retries == 0) {
// basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println(“MQTT Connected!”);
}
