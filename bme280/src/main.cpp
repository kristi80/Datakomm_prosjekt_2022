#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <PubSubClient.h>

// BME280 setup
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;

// Wifi name (ssid) and password
const char *ssid = "zenbook-kristian";
const char *password = "hansen123";

// MQTT Broker IP-address
// const char *mqtt_server = "10.22.226.56"; school
const char *mqtt_server = "10.22.226.91"; // home
const int mqtt_port = 1883;

// declare the mqtt client
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// function to connect to wifi
void setup_wifi(int time_reconnect)
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) // wait for connection
  {
    long now = millis();
    if ((now - lastMsg > 500)) // every 500 ms
    {
      lastMsg = now;
      Serial.print(".");
    }
    if ((now - time_reconnect > 5000)) // every 5 s
    {
      // restarer esp etter 5 sekunder for raskere oppkobling til wifi
      Serial.print("Restarter esp32");
      time_reconnect = now;
      ESP.restart();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Listens for messages on subscribed topics
void callback(char *topic, byte *message, unsigned int length)
{
  // prints the topic and the message
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String dataMessage;

  // makes a string from the message
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    dataMessage += (char)message[i];
  }
}

void setup()
{
  Serial.begin(9600); // start serial for output

  if (!bme.begin(0x76)) // start BME280
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1)
      ;
  }

  // starter wifi:
  setup_wifi(0);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// MQTT reconnect function
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client"))
    {
      Serial.println("connected");
      // Subscribe
      // client.subscribe("esp32/input");
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

void printMQTT(String topic, String msg, String owner)
{
  bool debug = false;
  // print the topic and message to the serial monitor
  String mqtt_msg = "{\"owner\": \"" + owner + "\", \"message\": " + msg + "}";
  String mqtt_topic = "esp32/output/" + topic;
  client.publish(mqtt_topic.c_str(), mqtt_msg.c_str());
  if (debug)
  {
    Serial.print("Topic: ");
    Serial.println(mqtt_topic);
    Serial.print("Message: ");
    Serial.println(mqtt_msg);
  }
}
void loop()
{
  // if mqtt is not connected, reconnect
  if (!client.connected())
  {
    Serial.print("disconnect mqtt");
    reconnect();
  }

  long now = millis();

  if ((now - lastMsg > 1000)) // every second
  {
    // send data to mqtt
    printMQTT("temperature", String(bme.readTemperature()), "ESP32");
    printMQTT("humidity", String(bme.readHumidity()), "ESP32");
    printMQTT("pressure", String(bme.readPressure() / 100.0F), "ESP32");
    printMQTT("altitude", String(bme.readAltitude(SEALEVELPRESSURE_HPA)), "ESP32");

    // print the data to the serial monitor
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println("*C");

    Serial.print("Pressure = ");
    Serial.print(bme.readPressure() / 100.0F);
    Serial.println("hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println("m");

    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println("%");

    Serial.println();
  }
  client.loop(); // listen for incoming messages
}