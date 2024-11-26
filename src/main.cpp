#include <main.h>
#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32_SSL.h>


// * Blynk Device Wifi Provisioning, For use later

#define USE_ESP32_DEV_MODULE
#include "BlynkEdgent.h"

void setup()
{
  // * Blynk Wifi Provisioning
  BlynkEdgent.begin();
}

void loop()
{
  // * Blynk Wifi Provisioning
  BlynkEdgent.run();
}