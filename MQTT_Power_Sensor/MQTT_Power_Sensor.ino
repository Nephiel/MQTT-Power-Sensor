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

#include "Custom_Settings.h"                // Connection details and customizable variables

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
  ReadPower(false); // do not show calibrate by default
}
void ReadPower(bool printCalibrationInfo) {
  int actualCurrent = 0;
  //int minCurrent = 1023;
  int maxCurrent = 0;

  // Needs to sample for at least one and half mains cycles (> 30mS at 50Hz), and the Wemos D1 takes aprox 0.15ms per sample
  int numSamples = 1000/mainsFreq * 1.5 / 0.15;

  unsigned long startMillis = millis();
  for (int j = 0 ; j <= numSamples ; j++)
  {
    // Read A/D input and record maximum and minimum current
    actualCurrent = analogRead(A0);
    if (actualCurrent > maxCurrent) {
      maxCurrent = actualCurrent;
    }
    /*
    if (actualCurrent < minCurrent) {
      minCurrent = actualCurrent;
    }
    */
  } // End of samples loop
  unsigned long sampledTime = millis() - startMillis;

  /*
  Serial.println("minCurrent = " + String(minCurrent) +
                 ", actualCurrent = " + String(actualCurrent) +
                 ", maxCurrent = " + String(maxCurrent)
                );
  */

  // Throw out the negative half of the AC sine wave
  if (maxCurrent < calibZero) {
    maxCurrent = calibZero;
  }

  // Print calibration info if requested
  if (printCalibrationInfo) {
    Serial.println("------ Calibration Info ----------------------");
    Serial.println("mainsFreq = " + String(mainsFreq) +
                   ", numSamples = " + String(numSamples) +
                   ", sampledTime = " + String(sampledTime) +
                   ", maxCurrent = " + String(maxCurrent) +
                   ", actualCurrent = " + String(actualCurrent) +
                   ", calibZero = " + String(calibZero)
                  );
    if (sampledTime < (1000/mainsFreq*1.5)) {
      Serial.println("Warning: numSamples is not enough for the given mainsFreq.");
    }
    if (actualCurrent > (calibZero + 5)) {
      Serial.println("Assuming no current is passing through the CT, to calibrate, try setting calibZero to a *higher* value (closer to actualCurrent).");
    } else if (actualCurrent < (calibZero - 5)) {
      Serial.println("Assuming no current is passing through the CT, to calibrate, try setting calibZero to a *lower* value (closer to actualCurrent).");
    }
    Serial.println(String("------ End Calibration Info ------------------"));
  }

  // http://www.referencedesigner.com/rfcal/cal_04.php
  rmsCurrent = ((maxCurrent - calibZero) * 0.707) / calibRMS; // Calculates RMS current based on maximum value and scales according to calibration
  rmsPower = mainsVoltage * rmsCurrent;                       // Calculates RMS Power

  kiloWattHours = kiloWattHours + ((double)rmsPower * ((double)sampledTime/3600000000)); // Calculates kilowatt hours used since last reboot. 3600000000 = 60*60*1000*1000

} // End of ReadPower

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

  ReadPower(true);                            // Measure current once, displaying calibration info

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

    // Sleep at least half the time.
    delay(1/2 * message_interval);

  } // End of messages timer

} // End of main loop
