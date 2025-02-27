#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define read_pin D5
#define sense_pin D0
#define relay_pin_1 D2
#define relay_pin_2 D3
#define BUILTIN_LED D4


// Update these with values suitable for your network.

const char* ssid = "hacker";
const char* password = "sreejamadhu";
const char* mqtt_server = "raspberrypi";
String node_id = "null";
long tick_1 = 0, tick_2 = 0;
boolean relay_state = false, publish_flag = false;
int max_number_nodes = 16;

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[4];
int node_grid_number = 0;
String msg_received = "";
boolean sensor_value_current = 0, sensor_value_prev = 0;
long max_wait_for_node_id;
int blink_delay = 50;

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(sense_pin, INPUT);
  pinMode(read_pin, INPUT);
  pinMode(relay_pin_1, OUTPUT);
  pinMode(relay_pin_2, OUTPUT);
  Serial.begin(115200);
  max_wait_for_node_id = max_number_nodes*(1000+blink_delay);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  int node_id_count = -1;
  tick_1 = millis();
  tick_2 = millis();
  while (digitalRead(read_pin) && (node_id_count < max_number_nodes))
  {
    if (millis() - tick_1 > 1000)
    {
      digitalWrite(BUILTIN_LED, LOW);
      delay(blink_delay);
      digitalWrite(BUILTIN_LED, HIGH);
      Serial.println("Blink");
      if (digitalRead(read_pin))
      {
        node_id_count += 1;
        Serial.println("Read Pin: High " + (String)node_id_count);
      }
      tick_1 = millis();
    }
    if (millis() - tick_2 > max_wait_for_node_id)
    {
      break;
    }
  }
  if (node_id_count > -1)
    node_id = node_id_count;
  else
    node_id = (String)(max_number_nodes-1);
  Serial.println("Setting Node ID:" + (String)node_id);
}

void callback(char* topic, byte * payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  if (topic == "relay")
  {
    for (int i = 0; i < length; i++) {
      msg_received = msg_received + (String)payload[i];
    }
    Serial.println(msg_received);
    // Switch on the LED if an 1 was received as first character
    if (msg_received == "1") {
      digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
      // but actually the LED is on; this is because
      // it is acive low on the ESP-01)
    } else {
      digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
    }
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected())
  {
    String temp = "client_id_" + (String)node_id;
    char client_id[20];
    temp.toCharArray(client_id, 20);

    if (client.connect(client_id))
    {
      Serial.println("mqtt Connected!!!, Client ID:" + (String)client_id);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  if (millis() - tick_1 > 1000)
  {
    digitalWrite(BUILTIN_LED, LOW);
    delay(blink_delay);
    digitalWrite(BUILTIN_LED, HIGH);
    Serial.println("Blink count : "+(String)(tick_1/1000));
    tick_1 = millis();
  }

  sensor_value_current = digitalRead(sense_pin);

  if ((sensor_value_prev ^ sensor_value_current))
  {
    digitalWrite(relay_pin_1, relay_state);
    digitalWrite(relay_pin_2, !relay_state);
    digitalWrite(BUILTIN_LED, relay_state);
    relay_state = !relay_state;
    if (sensor_value_current)
      publish_flag = true;
    sensor_value_prev = sensor_value_current;
  }

  if (publish_flag)
  {
    char message_char_array[10];
    node_id.toCharArray(message_char_array, 10);
    client.publish("outTopic", message_char_array);
    Serial.println("Message published!!!: " + (String)message_char_array);
    publish_flag = false;
  }
}
