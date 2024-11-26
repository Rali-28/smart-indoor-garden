#include <main.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32_SSL.h>

// * Blynk Device Wifi Provisioning
#define USE_ESP32_DEV_MODULE
#include "BlynkEdgent.h"

// Pins
#define soilMoistureSensor 34
#define waterPump 4 // * Active-Low Relay
#define s1_trigPin 25
#define s1_echoPin 26
#define s2_trigPin 16
#define s2_echoPin 17
// Pins End

// Soil Moisture Sensor Values
int sensorValue;
int moisturePercentage;
int drySoil = 4095;
int wetSoil = 3500;
int moisturePerLow = 20;  // low moisture percentage
int moisturePerHigh = 80; // High moisture percentage
// Soil Moisture Sensor Values End

// Water Sensor Values
int distance_1;
int distance_2;
// Boolean for getting the waterpump state
bool waterPumpState = true; // Pump is off (active-LOW relay)
static bool lastPumpState = waterPumpState;
static bool drySoilLogged = false;
bool manualOverride = false;
int lastTankStatus = -1;

BlynkTimer timer;


// Shows state of Water Pump to Blynk
// Manual override of water pump
BLYNK_WRITE(V1)
{
  int controlValue = param.asInt(); // Read value from app (0 or 1)

  if (controlValue == 1)
  {
    manualOverride = true;        // Enable manual override
    digitalWrite(waterPump, LOW); // Turn on water pump manually
    Blynk.virtualWrite(V1, 1);    // Set virtual button to "on" (pump on)
    Blynk.logEvent("pump_on");
  }
  else
  {
    manualOverride = false;        // Disable manual override
    digitalWrite(waterPump, HIGH); // Ensure pump is off
    Blynk.virtualWrite(V1, 0);     // Set virtual button to "off" (pump off)
    Blynk.logEvent("pump_off");
  }
}


// Water Pump Function
void pumpOn()
{
  digitalWrite(waterPump, LOW); // *Turn pump on (active-LOW relay)
  waterPumpState = false;       // update state to pump on
}

void pumpOff()
{
  digitalWrite(waterPump, HIGH); // *Turn pump off (active-LOW relay)
  waterPumpState = true;         // update state to pump off
}
// Water Pump Function End

// Water Tank Status
int waterTankStatus()
{
  int currentStatus;

  if (distance_1 <= 20 && distance_2 <= 20)
  {
    // Water Tank Full
    currentStatus = 2;
  }
  else if (distance_1 <= 20)
  {
    // Water Tank Filled
    currentStatus = 1;
  }
  else if (distance_1 > 20 && distance_2 > 20)
  {
    // Water Tank Empty
    currentStatus = 0;
  }
  else
  {
    currentStatus = -1; // Invalid status
  }

  // Only update Blynk and log events if the status changes
  if (currentStatus != lastTankStatus)
  {
    lastTankStatus = currentStatus; // Update the last status

    // Update Blynk Virtual Pin (V2) with the current status
    if (currentStatus == 2)
    {
      Blynk.virtualWrite(V2, "FULL"); // change label_widget to full
    }
    else if (currentStatus == 1)
    {
      Blynk.virtualWrite(V2, "FILLED"); // change label_widget to filled
    }
    else if (currentStatus == 0)
    {
      Blynk.virtualWrite(V2, "EMPTY"); // change label_widget to empty
      Blynk.logEvent("tank_empty"); // Log only when tank becomes empty
    }
  }

  return currentStatus; // Return the current status for other use if needed
}

// Water Sensor Function
void firstSensor()
{
  // Clear the trigPin by setting it LOW:
  digitalWrite(s1_trigPin, LOW);
  delayMicroseconds(5);

  // Trigger the sensor by setting the s1_trigPin high for 10 microseconds:
  digitalWrite(s1_trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(s1_trigPin, LOW);

  // Read the echoPin. pulseIn() returns the duration (length of the pulse) in microseconds:
  int duration_1 = pulseIn(s1_echoPin, HIGH);

  // Calculate the distance:
  distance_1 = (duration_1 / 2) / 29.1;
}

void secondSensor()
{
  digitalWrite(s2_trigPin, LOW);
  delayMicroseconds(5);

  digitalWrite(s2_trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(s2_trigPin, LOW);

  int duration_2 = pulseIn(s2_echoPin, HIGH);

  distance_2 = (duration_2 / 2) / 29.1;
}
// Water Sensor Function End

// Soil Moisture Sensor Function
void soilSensorDataSend()
{
  sensorValue = analogRead(soilMoistureSensor);   // takes analog value of sensor
  moisturePercentage = map(sensorValue, drySoil, wetSoil, 0, 100); // maps value of 3500-4095 = 0-100
  moisturePercentage = constrain(moisturePercentage, 0, 100); // contrain to 0-100 values only
  Blynk.virtualWrite(V0, moisturePercentage); // Shows value to Blynk widget
  int tankStatus = waterTankStatus(); // gets state of water tank

  // Turns on water pump if moisture gets too low
  if (moisturePercentage < moisturePerLow && waterPumpState == true && (tankStatus == 1 || tankStatus == 2))
  {
    pumpOn(); // turn on water pump
    Blynk.virtualWrite(V1, 1); // turns on Button indicator for water pump
    Blynk.logEvent("pump_on"); // send log event pump_on
  }
  // Turns off water pump at the right moisture level
  // Will not turn on water pump if water tank is empty
  else if ((tankStatus == 0 || moisturePercentage > moisturePerHigh) && waterPumpState == false)
  {
    pumpOff(); // turn off water pump
    Blynk.virtualWrite(V1, 0); // turn off button indicator for water pump
    Blynk.logEvent("pump_off"); // send log event pump_off
  }
  // log event for dry soil
  // solves problem of redundant call of blynk.logevent
  if (moisturePercentage <= moisturePerLow && !drySoilLogged)
  {
    Blynk.logEvent("dry_soil");
    drySoilLogged = true; // Log only once
  }
  // Reset when moisture goes above the threshold
  if (moisturePercentage > moisturePerLow)
  {
    drySoilLogged = false; // Allow logging again if moisture is sufficient
  }
}
// Soil Moisture Sensor Function End

void setup()
{
  Serial.begin(115200);
  // * Blynk Wifi Provisioning
  BlynkEdgent.begin();

  // Pinmodes
  pinMode(soilMoistureSensor, INPUT);
  pinMode(s1_trigPin, OUTPUT);
  pinMode(s1_echoPin, INPUT);
  pinMode(s2_trigPin, OUTPUT);
  pinMode(s2_echoPin, INPUT);
  pinMode(waterPump, OUTPUT);
  digitalWrite(waterPump, HIGH); // set initial state of pump to Off

  timer.setInterval(10000L, soilSensorDataSend);
  timer.setInterval(5000L, firstSensor);  // First water sensor (Ultrasonic 1)
  timer.setInterval(5000L, secondSensor); // First water sensor (Ultrasonic 2)
}

void loop()
{
  // * Main Blynk function
  Blynk.run();
  // * Blynk Wifi Provisioning
  BlynkEdgent.run();
  // * Run set timer
  timer.run();
}