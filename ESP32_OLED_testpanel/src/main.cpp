#include <Arduino.h>
#include <ezButton.h>
#include <adafruit_gfx.h>
#include <adafruit_ssd1306.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <random>

// _____________________SLEEP MODE_____________________
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 10       /* Time ESP32 will go to sleep (in seconds) */

// RTC_DATA_ATTR is used to store variables in RTC memory
RTC_DATA_ATTR int bootCount = 0;

int last_sleep = 0;

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

// _____________________WIFI AND MQTT SETUP_____________________
// Wifi name (ssid) and password
const char *ssid = "wifi_ssid";
const char *password = "wifi_password";

// MQTT server, add port and username
const char *mqtt_server = "MQTT_BROKER_IP_ADDRESS";
const int mqtt_port = 1883;

// declares name and variables for wifi and mqtt
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// BUTTON SETUP
#define BUTTON_PIN_1 19          // port from button to ESP32
ezButton button_1(BUTTON_PIN_1); // set up button
#define DEBOUNCE_TIME 50         // number of milliseconds to debounce

#define BUTTON_PIN_2 5           // port from button to ESP32
ezButton button_2(BUTTON_PIN_2); // set up button

#define BUTTON_PIN_3 17          // port from button to ESP32
ezButton button_3(BUTTON_PIN_3); // set up button

// buttonVariables has the 0 element as a fail safe
bool buttonVariables[4] = {false, false, false, false};

bool buttonVariables_old[4] = {false, false, false, false};

// LED SETUP
#define LED1_PIN 18         // port from LED1 to ESP32
const int LED1_CHANNEL = 1; // channel for LED1

// POTENTIOMETER SETUP
#define POT_PIN 36 // port from potentiometer to ESP32

// OLED SETUP
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// _________________________FUNCTIONS___________________________

// global int, first element is a fail safe
int timeParked_cars[4] = {0, 0, 0, 0};

// global int, battery percentage, first element is a fail safe
int battery_satus[4] = {0, 0, 0, 0};

// global bool, if true == charging, first element is a fail safe
bool charging_status[4] = {true, true, true, true};

// global bool, if true == decharging, first element is a fail safe
bool decharging_status[4] = {true, true, true, true};

// maximum W the grid can deliver (excluded batteries, in W)
#define MAX_GRID 100 * 1000 // 100 kW

// function to set up the button, takes the pin number and channel
void setupLED(int ledPin, int channelLED)
{
  pinMode(ledPin, OUTPUT);
  ledcSetup(channelLED, 2000, 8);
  ledcAttachPin(ledPin, channelLED);
  ledcWrite(channelLED, 0);
}

/*
function to connect to wifi
time_reconnect works well at 0
*/
void setup_wifi(int time_reconnect)
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    long now = millis();
    // read every 500ms
    if ((now - lastMsg > 500))
    {
      lastMsg = now;
      Serial.print(".");
    }
    if ((now - time_reconnect > 5000))
    {
      // restart if not connected after 5s, this is to avoid the ESP32 to get "stuck"
      Serial.print("Restarting esp32");
      time_reconnect = now;
      ESP.restart();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// function to sett callback for mqtt, this subscribes to the topic
void callback(char *topic, byte *message, unsigned int length)
{
  // print the topic and the message
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String dataMessage;

  // makes a variable "dataMessage" and adds char together
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    dataMessage += (char)message[i];
  }
  Serial.println();

  // if the message is on, turn on the LED, this just shows that two-way communication works
  if (String(topic) == "esp32/input")
  {
    Serial.print("Changing output to ");
    if (dataMessage == "on")
    {
      Serial.println("on");
      ledcWrite(1, 255);
    }
    else if (dataMessage == "off")
    {
      Serial.println("off");
      ledcWrite(1, 0);
    }
  }
}

void esp32_sleep_setup()
{
  // Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  // Print the wakeup reason for ESP32
  print_wakeup_reason();

  /*
  First we configure the wake up source
  We set our ESP32 to wake up every TIME_TO_SLEEP
  */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
                 " Seconds");
}

void setup()
{
  Serial.begin(9600);
  button_1.setDebounceTime(DEBOUNCE_TIME); // set debounce time
  button_2.setDebounceTime(DEBOUNCE_TIME); // set debounce time
  button_3.setDebounceTime(DEBOUNCE_TIME); // set debounce time
  setupLED(LED1_PIN, LED1_CHANNEL);        // set up LED1

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  // starts wifi:
  setup_wifi(0);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // setup for deep sleep
  esp32_sleep_setup();
}

// function that runs until mqtt is connected
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
      client.subscribe("esp32/input");
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

// function to display the potentiometer value on the OLED
void displayPot(int potValue, int button_1, int timeParked_1, int button_2, int timeParked_2, int button_3, int timeParked_3)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Potentiometer: ");
  display.println(potValue);
  display.print("Car 1: ");
  display.println(button_1);
  display.print("Time parked:");
  display.println(timeParked_1);
  display.print("Car 2: ");
  display.println(button_2);
  display.print("Time parked:");
  display.println(timeParked_2);
  display.print("Car 3: ");
  display.println(button_3);
  display.print("Time parked:");
  display.println(timeParked_3);
  display.display();
}

int bool_from_array_to_int(bool boolArray[], int index)
{
  if (boolArray[index] == true)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// function to increase the time parked for each car
void timeParked()
{
  // if the button is pressed, increase the time parked
  for (int i = 1; i < 4; i++)
  {
    if (buttonVariables[i] == true)
    {
      timeParked_cars[i] += 1;
    }
    else
    {
      timeParked_cars[i] = 0;
    }
  }
}

int parking_status_array(bool varArray[], int size)
{
  int sum = 0;
  for (int i = 1; i < size; i++)
  {
    sum += bool_from_array_to_int(varArray, i);
  }
  return sum;
}

// function to send the data to the server
void printMQTT(String topic, String msg, String owner)
{
  // print the topic and message to the serial monitor
  String mqtt_msg = "{\"owner\": \"" + owner + "\", \"message\": " + msg + "}";
  String mqtt_topic = "esp32/output/" + topic;
  client.publish(mqtt_topic.c_str(), mqtt_msg.c_str());
}

// function to send the data to the server
void printMQTT_parking(String topic, String msg, String owner, int timeParked, int battery_status)
{
  // print the topic and message to the serial monitor
  String mqtt_msg = "{\"owner\": \"" + owner + "\", \"amount\": " + msg + ", \"timeParked\": " + String(timeParked) + " , \"battery_status\": " + String(battery_status) + "}";
  String mqtt_topic = "esp32/output/" + topic;
  client.publish(mqtt_topic.c_str(), mqtt_msg.c_str());
}

int give_random_battery_status()
{
  int Wh = 3600;
  int random_number = int(random(20, 80)) * Wh;
  return random_number;
}

// changes the button value to true or false
void buttonState()
{
  if (button_1.isPressed())
  {
    buttonVariables[1] = !buttonVariables[1]; // toggle buttonVariable
    battery_satus[1] = give_random_battery_status();
  }
  if (button_2.isPressed())
  {
    buttonVariables[2] = !buttonVariables[2];
    battery_satus[2] = give_random_battery_status();
  }
  if (button_3.isPressed())
  {
    buttonVariables[3] = !buttonVariables[3];
    battery_satus[3] = give_random_battery_status();
  }
}

int battery_grid_need(int actual_grid, int max_grid)
{
  int battery_grid_need = 0;
  if ((actual_grid + 100000) > max_grid)
  {
    battery_grid_need = actual_grid + 100000 - max_grid;
  }
  return battery_grid_need;
}

/*
function to find the index of the biggest value in an array and checks
if that element is avalible
*/
int find_max_index(int array_with_elements[], bool array_with_bool[], int size)
{
  int biggest_element = 0;
  int index_biggest_element = 0;
  for (int i = 0; i < size; i++)
  {
    if (array_with_bool[i])
    {
      if (array_with_elements[i] > biggest_element)
      {
        biggest_element = array_with_elements[i];
        index_biggest_element = i;
      }
    }
  }
  return index_biggest_element;
}

// updates the global battery status of the cars
void update_battery_status(int actual_grid_status, int max_grid_status)
{
  int size = 4; // size of the arrays
  int Wh = 3600;
  int power_given_from_battery = 0;
  int battery_need = actual_grid_status;

  // arrays have 4 elements, but the index 0 is not used
  int battery_cars[size] = {};
  bool car_available[size] = {};
  for (int i = 0; i < size; i++)
  {
    battery_cars[i] = battery_satus[i];
    car_available[i] = buttonVariables[i];
    // set discharge to false, it will change to true if the battery is discharged
    decharging_status[i] = false;
  }

  // check if the avalible cars have over 10% battery left
  for (int i = 0; i < size; i++)
  {
    if (car_available[i])
    {
      if (battery_cars[i] < 10 * Wh)
      {
        car_available[i] = false;
      }
    }
  }

  // check if the cars have enough battery to give power to the grid
  int number_of_cars = parking_status_array(car_available, size);

  bool avalible_cars = false;
  if (number_of_cars > 0)
  {
    avalible_cars = true;
  }

  while (battery_need > 0 && number_of_cars > 0 && avalible_cars)
  {
    // find the biggest battery
    int biggest_battery_index = find_max_index(battery_cars, car_available, size);

    if (biggest_battery_index == 0)
    {
      // this eliminates battery1 beeing used even tough its not avalible
      avalible_cars = false;
    }
    // the biggest battery has index "biggest_battery_index"
    // check if battery_need is smaller than what the battery can give (5000 stands for 5kW)
    if (battery_need < 5000)
    {
      battery_cars[biggest_battery_index] -= battery_need;
      power_given_from_battery += battery_need;
      battery_need = 0;
    }
    if (battery_need >= 5000)
    {
      battery_cars[biggest_battery_index] -= 5000;
      power_given_from_battery += 5000;
      battery_need -= 5000;
    }

    decharging_status[biggest_battery_index] = true; // set the decharging status to true
    charging_status[biggest_battery_index] = false;  // set the charging status to false

    // the biggest battery has now been used, and we need to set it to unavalible
    car_available[biggest_battery_index] = false;
    number_of_cars--; // one car has been used
  }
  // send mqtt message to update battery status
  printMQTT("powergrid/need", String(battery_need), "grid");
  printMQTT("powergrid/batteryPark", String(power_given_from_battery), "grid");

  // update discharge status
  for (int i = 1; i < size; i++)
  {
    if (decharging_status[i] == true)
    {
      printMQTT("powergrid/decharging", String(i), "discharge");
    }
    if (decharging_status[i] == false)
    {
      printMQTT("powergrid/decharging", String(i), "standby");
    }
  }

  // update the battery variables
  for (int i = 0; i < 4; i++)
  {
    battery_satus[i] = battery_cars[i];
  }
}

// function to update the charging status of the cars
void update_battery_charging(int grid, int size)
{
  int Wh = 3600;
  // if there is power to charge batteries
  if (grid == 0)
  {
    for (int i = 1; i < size; i++)
    {
      // check if the car is parked
      if (buttonVariables[i] == true)
      {
        // check if the battery is not full
        if (battery_satus[i] < 100 * Wh)
        {
          // start charging
          charging_status[i] = true;
          battery_satus[i] += 5000;
          printMQTT("powergrid/charging", String(i), "charge");
        }
      }
      if (buttonVariables[i] == false)
      {
        // stop charging
        charging_status[i] = false;
        printMQTT("powergrid/charging", String(i), "standby");
      }
    }
  }
}

// function to check if array contains a true value
bool array_contains_true(bool array[], int size)
{
  for (int i = 0; i < size; i++)
  {
    if (array[i])
    {
      return true;
    }
  }
  return false;
}

void loop()
{
  // if mqtt is not connected, reconnect
  if (!client.connected())
  {
    Serial.print("disconnect mqtt");
    reconnect();
  }
  client.loop();

  button_1.loop(); // run the button loop
  button_2.loop(); // run the button loop
  button_3.loop(); // run the button loop

  int potValue = analogRead(POT_PIN);                     // read the potentiometer value
  int potValueMapped = map(potValue, 0, 4095, 0, 15000);  // map the potentiometer value to 0-115 (115kW)
  int potValueMappedLed = map(potValue, 0, 4095, 0, 100); // map the potentiometer value to 0-100 (100%)

  buttonState(); // run the buttonState function

  long now = millis();
  // leser av hvert sekund
  if ((now - lastMsg > 2000))
  {
    lastMsg = now;

    timeParked();                                    // run the timeParked function to update the time parked for each car
    update_battery_status(potValueMapped, MAX_GRID); // update the battery status
    update_battery_charging(potValueMapped, 4);      // update the charging status

    // map the battery status back to 0-100 by dividing by Wh (3600)
    int mapped_battery_status[4] = {};
    for (int i = 0; i < 4; i++)
    {
      mapped_battery_status[i] = battery_satus[i] / (3600);
    }

    printMQTT("battery", String(potValueMapped), "pot_meter");                                                                                    // send the battery value to the server
    printMQTT_parking("parking_1", String(bool_from_array_to_int(buttonVariables, 1)), "button_1", timeParked_cars[1], mapped_battery_status[1]); // send the car 1 value to the server
    printMQTT_parking("parking_2", String(bool_from_array_to_int(buttonVariables, 2)), "button_2", timeParked_cars[2], mapped_battery_status[2]); // send the car 2 value to the server
    printMQTT_parking("parking_3", String(bool_from_array_to_int(buttonVariables, 3)), "button_3", timeParked_cars[3], mapped_battery_status[3]); // send the car 3 value to the server
    printMQTT("parking_status", String(parking_status_array(buttonVariables, 4)), "parking_status");                                              // send the parking status to the server

    displayPot(potValueMapped, bool_from_array_to_int(buttonVariables, 1), timeParked_cars[1], bool_from_array_to_int(buttonVariables, 2), timeParked_cars[2],
               bool_from_array_to_int(buttonVariables, 3), timeParked_cars[3]); // display the potentiometer value on the OLED

    // esp32 deep sleep conditions
    int sleep_time = 1000;        // time in ms
    sleep_time = sleep_time * 15; // time in s

    // wait for sleep time and that the potentiometer is at 0, and no car is parked
    if (now - last_sleep > sleep_time && potValueMapped == 0 && parking_status_array(buttonVariables, 4) == 0)
    {
      last_sleep = now;
      // Now we enter the deep sleep mode.
      Serial.println("Going to sleep now");
      Serial.flush();
      esp_deep_sleep_start();
      Serial.println("This will never be printed");
    }
  }
}
