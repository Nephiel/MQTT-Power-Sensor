/*
  MQTT with ESP8266 and Power Monitor 5th December 2017
  David Mottram
  Updated by Nephiel, 22nd April 2019
  See Custom_Settings.h for Wifi and MQTT settings
*/

#include <ESP8266WiFi.h>                    // Needed for ESP8266
#include <WiFiClient.h>                     // WiFi client

// Watchdog items
#include <Ticker.h>
Ticker secondTick;
volatile int watchdogCount = 0;

volatile bool birthMsgRequested = false;    // True when ready to begin taking sensor readings

double kiloWattHours = 0;                   // Measured energy in KWh
double rmsCurrent = 0;                      // Measured current in A
int rmsPower = 0;                           // Measured power in W

#include "Custom_Settings.h"                // Custom Wifi connection and details.

// I/O
#define Status_LED 2 // D4
#define Status_LED_On digitalWrite(Status_LED, HIGH)
#define Status_LED_Off digitalWrite(Status_LED, LOW)

WiFiClient espClient;                       // For ESP8266 boards
#include <PubSubClient.h>                   // http://pubsubclient.knolleary.net/api.html
PubSubClient psClient(espClient);           // ESP pubsub client
#include "WiFi_Functions.h"                 // WiFi and Serial Functions

// Watchdog timer. Watchdog is reset after every succesful MQTT publish.
void ISRwatchdog() {
  watchdogCount++;
  if (watchdogCount == watchdogTimeout) {
    Serial.println();
    Serial.println("The watchdog bites!");
    ESP.restart();
  } // end of timeout if
} // end of watchdog count

// Read information from CT
void ReadPower() {
  int actualCurrent = 0;
  int maxCurrent = 0;
  int medCurrent = 511; // AnalogRead value of the Zero line. Default is 511 (midway between 0 and 1023). Change this value to calibrate
  int minCurrent = 1023;

  unsigned long startMillis = millis();

  // Needs to sample for at least one and half mains cycles or > 30mS
  for (int j = 0 ; j <= 600 ; j++)
  {
    // Read A/D input and record maximum and minimum current
    actualCurrent = analogRead(A0);
    if (actualCurrent >= maxCurrent) {
      maxCurrent = actualCurrent;
    } else if (actualCurrent <= minCurrent) {
      minCurrent = actualCurrent;
    }
  } // End of samples loop

  // Throw out the negative half of the AC sine wave
  if (maxCurrent < medCurrent) {
    maxCurrent = medCurrent;
  }

  // http://www.referencedesigner.com/rfcal/cal_04.php
  rmsCurrent = ((maxCurrent - medCurrent) * 0.707) / calib;    // Calculates RMS current based on maximum value and scales according to calibration
  int rmsPower = mainsVoltage * rmsCurrent;    // Calculates RMS Power

  unsigned long kTime = millis() - startMillis;
  kiloWattHours = kiloWattHours + ((double)rmsPower * ((double)kTime/60/60/1000000)); // Calculates kilowatt hours used

  //delay(2000);

} // End of function

// Initial setup
void setup(void) {
  delay(500);

  secondTick.attach(1, ISRwatchdog);          // Start watchdog timer

  pinMode(Status_LED, OUTPUT);                // Initialize Status LED
  Status_LED_Off;

  Init_Serial();                              // Initialize serial port for debugging

  Connect_To_Any_Known_WiFi();                // Connect to WiFi, trying each configured SSID. This must be done very soon after powerup!

  psClient.setServer(mqtt_server, mqtt_port); // Set the MQTT server and port
  
  ms_since_last_message = millis();           // Reset time since sending last message

  Serial.println("Setup finished");
} // End of setup


// main loop
void loop() {

  // Check if we're still connected to MQTT broker
  if ( (!psClient.loop()) || (!psClient.connected()) ) {
    // reconnect if not
    reconnect();
  }

  // If enough time since sending last message, or if birth message must be published
  if ((millis() - ms_since_last_message) > message_interval || birthMsgRequested) {

    ms_since_last_message = millis();
    bool mustSpecifyBirth = birthMsgRequested;
    birthMsgRequested = false;

    // Update messages counter
    message_count++;
    if (message_count > max_message_count) {
      message_count = 1;
    }

    Status_LED_Off;
    ReadPower();
    Status_LED_On;
    Serial.println(String(rmsCurrent) + "A " + rmsPower + "W " + kiloWattHours + "KWh");

    // Get a payload and build a char array
    String Payload = Build_Payload(mustSpecifyBirth);
    char Payload_array[(Payload.length() + 1)];
    Payload.toCharArray(Payload_array, (Payload.length() + 1));

    Serial.println(String("") + "Publishing to topic: " + mqtt_topic + " - Payload: " + Payload_array);

    // Publish a MQTT message with the payload
    if (psClient.publish(mqtt_topic, Payload_array)) {
      // Clear watchdog
      watchdogCount = 0;
    } else {
      Serial.println("Something went wrong, unable to publish.");
    }

    /*
    // WIP: for large messages (over 128 bytes including headers)
    psClient.beginPublish(mqtt_topic, (Payload.length() + 1), false);

    for (int c = 0 ; c < Payload.length() ; c++) {
      psClient.write(Payload_array[c]);
    }

    if (psClient.endPublish()) {
      // clear watchdog, etc.
    }
    */

    // Sleep for at least 3/4ths of the time
    delay(3/4 * message_interval);

  } // End of messages timer

} // End of main loop
