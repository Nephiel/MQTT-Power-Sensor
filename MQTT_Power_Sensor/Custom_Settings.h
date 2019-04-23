/*
 * Connection details for WiFi and board functions
 */

// To use Static IP Mode, uncomment the define line below
//#define STATIC_IP
IPAddress ip(192, 168, 1, 33);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Add your Wifi details below. Set SSID to NULL if unused.
const char* SSID_1 = "AP";
const char* Password_1 = "Password";

const char* SSID_2 = NULL;
const char* Password_2 = NULL;

const char* SSID_3 = NULL;
const char* Password_3 = NULL;

const char* SSID_4 = NULL;
const char* Password_4 = NULL;

// MQTT Broker address and port
const char* mqtt_server = "192.168.1.10";
const int mqtt_port = 1883;

// MQTT user and password. Set both to NULL if unused.
const char* mqtt_username = NULL;
const char* mqtt_password = NULL;

// MQTT Topics
const char* InStatus = "Power/Status";
const char* InControl = "Power/Control";

String Broadcast_All = "*ALL";                    // Message used to broadcast to all devices subscribed to above topic

double Calib = 10;                                // RMS Power correction. Default is 10. Change this value to calibrate

// message timing
int current_message_count = 0;                    // counter of messages sent
long ms_since_last_message = 0;                   // time elapsed since last message was sent
#define max_message_count 99                      // max value of message counter
#define message_interval 5000                     // time in ms between each message, 5000 ms = 5 seconds

// Watchdog items
volatile int Watchdog_Timeout = 300;              // Time in seconds before watchdog times out, 300 seconds = 5 minutes

// Custom default values
String WiFi_SSID = "None";                        // SSID string
String My_MAC = "";                               // MAC address, to be read from ESP8266
char MAC_array[13] = "000000000000";              // MAC definition to use as Client ID
volatile bool Report_Requested = false;           // Set to true if report required
//String Event = "Boot";                          // Default

// I/O
#define Status_LED 2                              // D4
#define Status_LED_On digitalWrite(Status_LED, HIGH)
#define Status_LED_Off digitalWrite(Status_LED, LOW)

// Power monitor items
double Kilos = 0;
double RMSCurrent = 0;
int RMSPower = 0;
int PeakPower = 0;

// Read information from CT
void ReadPower() {
  int Current = 0;
  int MaxCurrent = 0;
  int MedCurrent = 511; // AnalogRead value of the Zero line. Default is 511 (midway between 0 and 1023). Change this value to calibrate
  int MinCurrent = 1023;

  unsigned long StartMillis = millis();

  // Needs to sample for at least one and half mains cycles or > 30mS
  for (int j = 0 ; j <= 600 ; j++)
  {
    // Read A/D input and record maximum and minimum current
    Current = analogRead(A0);
    if (Current >= MaxCurrent) {
      MaxCurrent = Current;
    } else if (Current <= MinCurrent) {
      MinCurrent = Current;
    }
  } // End of samples loop

  // Throw out the negative half of the AC sine wave
  if (MaxCurrent < MedCurrent) {
    MaxCurrent = MedCurrent;
  }

  // http://www.referencedesigner.com/rfcal/cal_04.php
  RMSCurrent = ((MaxCurrent - MedCurrent) * 0.707) / Calib;    // Calculates RMS current based on maximum value and scales according to calibration
  int RMSPower = 230 * RMSCurrent;    // Calculates RMS Power assuming voltage is 230VAC
  if (RMSPower > PeakPower) {
    PeakPower = RMSPower;
  }

  unsigned long kTime = millis() - StartMillis;
  Kilos = Kilos + ((double)RMSPower * ((double)kTime/60/60/1000000)); // Calculates kilowatt hours used

  delay(2000);

} // End of function
