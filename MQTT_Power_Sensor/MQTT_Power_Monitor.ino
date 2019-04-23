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

#include "Custom_Settings.h"                // Custom and Wifi Connection and board details.

WiFiClient espClient;                       // For ESP8266 boards
#include <PubSubClient.h>                   // http://pubsubclient.knolleary.net/api.html
PubSubClient psClient(espClient);           // ESP pubsub client
#include "WiFi_Functions.h"                 // WiFi and Serial Functions
#include "MQTT_Functions.h"                 // MQTT Functions

// Watchdog timer. Watchdog is reset after every succesful MQTT publish.
void ISRwatchdog() {
  watchdogCount++;
  if (watchdogCount == Watchdog_Timeout) {
    Serial.println();
    Serial.println("The watchdog bites!");
    ESP.restart();
  } // end of timeout if
} // end of watchdog count


// Initial setup
void setup(void) {
  delay(500);

  secondTick.attach(1, ISRwatchdog);          // Start watchdog timer

  pinMode(Status_LED, OUTPUT);                // Initialize Status LED
  Status_LED_Off;

  Init_Serial();                              // Initialize serial port for debugging

  Connect_To_Any_Known_WiFi();                // Connect to WiFi, trying each configured SSID. This must be done very soon after powerup!

  psClient.setServer(mqtt_server, mqtt_port); // Set the MQTT server and port
  
  //psClient.setCallback(callback);           // MQTT callback

  ms_since_last_message = millis();           // Reset time since sending last message

  Serial.println("Setup finished");
} // End of setup


// main loop
void loop() {

  // Check if we're still connected to MQTT broker
  if (!psClient.connected()) {
    // reconnect if not
    reconnect();
  }
  psClient.loop();

  // Enough time since sending last message, or Report Requested
  if ((millis() - ms_since_last_message) > message_interval || Report_Requested) {

    ms_since_last_message = millis();
    Report_Requested = false;

    // Update messages counter
    current_message_count++;
    if (current_message_count > max_message_count) {
      current_message_count = 1;
    }

    Status_LED_Off;
    ReadPower();
    Status_LED_On;
    Serial.println(String(RMSCurrent) + "A " + RMSPower + "W " + Kilos + "KWh");

    // Get a payload and build a char array
    String Payload = Build_Payload();
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

  } // End of messages timer

} // End of main loop
