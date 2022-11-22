#include <Arduino.h>
#include "UbidotsEsp32Mqtt.h"
#include <ezButton.h>

/****************************************
 * Define Constants
 ****************************************/
const char *UBIDOTS_TOKEN = "token";                               // Put here your Ubidots TOKEN
const char *WIFI_SSID = "ssid";                                    // Put here your Wi-Fi SSID
const char *WIFI_PASS = "password";                                // Put here your Wi-Fi password
const char *DEVICE_LABEL = "esp32";                                // Put here your Device label to which data  will be published
const char *VARIABLE_LABEL = "parking";                            // Put here your Variable label to which data  will be published

long lastMsg = 0;

const int PUBLISH_FREQUENCY = 5000; // Update rate in milliseconds

// BUTTON SETUP
#define BUTTON_PIN 19        // port from button to ESP32
ezButton button(BUTTON_PIN); // set up button
#define DEBOUNCE_TIME 50     // number of milliseconds to debounce

bool buttonVariable = false;
int buttonValue = 0;

Ubidots ubidots(UBIDOTS_TOKEN);

/****************************************
 * Auxiliar Functions
 ****************************************/

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

/****************************************
 * Main Functions
 ****************************************/

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  // ubidots.setDebug(true);  // uncomment this to make debug messages available
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();

  button.setDebounceTime(DEBOUNCE_TIME); // set debounce time
}

void buttonState()
{
  if (button.isPressed())
  {
    buttonVariable = !buttonVariable;
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
  button.loop(); // run the button loop
  buttonState(); // run the buttonState function

  if (buttonVariable == true)
  {
    buttonValue = 1;
  }
  else
  {
    buttonValue = 0;
  }

  if (!ubidots.connected())
  {
    ubidots.reconnect();
  }
  long now = millis();
  if (now - lastMsg > PUBLISH_FREQUENCY) // triggers the routine every 5 seconds
  {
    lastMsg = now;

    ubidots.add(VARIABLE_LABEL, buttonValue); // Insert your variable Labels and the value to be sent
    ubidots.publish(DEVICE_LABEL);
  }
  ubidots.loop();
}
