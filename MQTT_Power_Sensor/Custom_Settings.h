/*
 * Connection details for WiFi and board functions
 */

// To use Static IP Mode, uncomment the define line below
//#define STATIC_IP
IPAddress ip(192, 168, 1, 33);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Add your Wifi details below. Set SSID to NULL if unused.
// WiFi slot 1
const char* ssid_1 = "AP";
const char* password_1 = "Password";
// WiFi slot 2
const char* ssid_2 = NULL;
const char* password_2 = NULL;
// WiFi slot 3
const char* ssid_3 = NULL;
const char* password_3 = NULL;
// WiFi slot 4
const char* ssid_4 = NULL;
const char* password_4 = NULL;

// MQTT Broker address and port
const char* mqtt_server = "192.168.1.10";
const int mqtt_port = 1883;

// MQTT user and password. Set both to NULL if unused.
const char* mqtt_username = NULL;
const char* mqtt_password = NULL;

// MQTT Topic
const char* mqtt_topic = "Power/Status";

// Mains voltage and calibration
const double calibRMS = 3.3877083;     // RMS Power correction. Default is 10.00. Change this value to calibrate.
const int calibZero = 545;             // AnalogRead value when there is no current (i.e. when the CT is not clamped around any wire).
                                       // Default is (1023-0)/2 = 511. Change this value to calibrate.
                                       // My sensor board read 545, the same as with the CT jack unplugged. YMMV.
const int mainsVoltage = 230;          // Mains voltage in Volts. Change this value according to your mains.
const int mainsFreq = 50;              // Mains frequency in Hertz. Change this value according to your mains.

// Message timing
int message_count = 0;                 // Counter of messages sent
long ms_since_last_message = 0;        // Time elapsed since last message was sent, in ms
const int max_message_count = 99;      // Max value of counter of messages sent. Will wrap back to 0 after this value.
const int message_interval = 5000;     // Time in ms between each message. 5000 ms = 5 seconds

// Watchdog timing
volatile int watchdogTimeout = 300;    // Time in seconds before watchdog times out, 300 seconds = 5 minutes
