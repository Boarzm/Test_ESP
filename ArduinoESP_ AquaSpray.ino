#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>

// --- Configuration: WiFi Credentials ---
const char *ssid = "My-Network";
const char *password = "chaker123456";

// --- Configuration: Pin Definitions ---

// DHT Sensor Pin (GPIO 4 is often safe to use)
const int DHT_PIN = 4;
#define DHTTYPE DHT11 // DHT 11 sensor type
DHT dht(DHT_PIN, DHTTYPE);

// Analog pins for Moisture Sensors (Adjust pins based on your ESP32 board)
const int MOISTURE_PIN_G1 = 36; // Typically ADC1_CH0
const int MOISTURE_PIN_G2 = 39; // Typically ADC1_CH3

// Digital pins to control the Relays (Active LOW relays assumed: LOW = ON, HIGH = OFF)
const int RELAY_PIN_G1 = 25; 
const int RELAY_PIN_G2 = 26; 

// --- Configuration: Calibration Values ---
// ADJUST THESE VALUES FOR YOUR SENSOR (ESP32 ADC is 0-4095)
const int DRY_CALIBRATION = 4095; // Max reading when sensor is fully dry (low moisture)
const int WET_CALIBRATION = 1800; // Reading when sensor is submerged (high moisture)
// The threshold (in percentage) to consider the soil "dry" and trigger irrigation
const int DRY_THRESHOLD_PERCENT = 40; 

// --- Global Variables and Server Setup ---
WebServer server(80);

// --- Function Prototypes ---
float readDHTTemperature();
float readDHTHumidity();
int readMoisturePercent(int pin);
String controlAndGetIrrigationStatus(int moisturePercent, int relayPin);
String getMoistureState(int percent);
void handleRoot();

// --- Main HTTP Handler Function ---
void handleRoot() {
  
  // 1. Read all sensor values
  float t = readDHTTemperature();
  float h = readDHTHumidity();
  int moisturePercent1 = readMoisturePercent(MOISTURE_PIN_G1);
  int moisturePercent2 = readMoisturePercent(MOISTURE_PIN_G2);
  
  // 2. Control Relays and get status
  String irrigationStatus1 = controlAndGetIrrigationStatus(moisturePercent1, RELAY_PIN_G1);
  String irrigationStatus2 = controlAndGetIrrigationStatus(moisturePercent2, RELAY_PIN_G2); // Corrected function call
  
  // 3. Determine Garden States
  String state1 = getMoistureState(moisturePercent1);
  String state2 = getMoistureState(moisturePercent2);
  
  // 4. Serial Monitor Output 
  Serial.println("--- AquaSpray Dashboard Data ---");
  Serial.print("DHT: Temp="); Serial.print(t); Serial.print("C, Humidity="); Serial.print(h); Serial.println("%");
  Serial.print("Garden 1: Moisture="); Serial.print(moisturePercent1); Serial.print("% ("); Serial.print(state1); Serial.print("), Irrigation:"); Serial.println(irrigationStatus1);
  Serial.print("Garden 2: Moisture="); Serial.print(moisturePercent2); Serial.print("% ("); Serial.print(state2); Serial.print("), Irrigation:"); Serial.println(irrigationStatus2);

  char msg[3000]; 

  // 5. HTML Content for Web Dashboard (Includes inline-block CSS for side-by-side garden views)
  snprintf(msg, 3000,
           "<html>\
  <head>\
    <meta http-equiv='refresh' content='10'/>\
    <meta name='viewport' content='width=device-width, initial-scale=1'>\
    <link rel='stylesheet' href='https://use.fontawesome.com/releases/v5.7.2/css/all.css' integrity='sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr' crossorigin='anonymous'>\
    <title>AquaSpray Dashboard</title>\
    <style>\
    html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center;}\
    h2 { font-size: 2.5rem; color: #008744;}\
    \
    /* Container for DHT data */\
    .dht-container { margin-bottom: 20px; padding: 10px; border: 1px solid #ddd; border-radius: 8px;}\
    \
    /* Style for the individual garden blocks (Inline-Block) */\
    .garden-data { \
        display: inline-block; /* KEY: Makes blocks line up horizontally */\
        width: 45%%; /* Occupy less than half the screen */\
        min-width: 250px; /* Minimum size before wrapping */\
        margin: 10px; \
        padding: 15px; \
        border: 2px solid #008744; \
        border-radius: 12px; \
        box-shadow: 0 4px 8px rgba(0,0,0,0.1);\
        background-color: #f9fffb;\
    }\
    \
    .label { font-size: 1.5rem; vertical-align:middle; padding-bottom: 5px; font-weight: bold;}\
    .value { font-size: 2.0rem;}\
    .units { font-size: 1.2rem;}\
    .wet { color: #00add6; }\
    .dry { color: #ca3517; }\
    .on { color: green; font-weight: bold;}\
    .off { color: gray; font-weight: bold;}\
    </style>\
  </head>\
  <body>\
      <h2>💧 AquaSpray Dashboard 🌡️</h2>\
      \
      \
      <div class='dht-container'>\
        <p>\
          <i class='fas fa-thermometer-half' style='color:#ca3517;'></i>\
          <span class='label'>Temperature</span>\
          <span class='value'>%.2f</span>\
          <sup class='units'>&deg;C</sup>\
        </p>\
        <p>\
          <i class='fas fa-tint' style='color:#00add6;'></i>\
          <span class='label'>Humidity</span>\
          <span class='value'>%.2f</span>\
          <sup class='units'>&percnt;</sup>\
        </p>\
      </div>\
      \
      \
      <div class='garden-data'>\
        <p class='label'>**GARDEN 1**</p>\
        <p class='label'>Moisture: <span class='value %s'>%d%%</span></p>\
        <p class='label'>State: <span class='%s'>%s</span></p>\
        <p class='label'>Irrigation: <span class='%s'>%s</span></p>\
      </div>\
      \
      \
      <div class='garden-data'>\
        <p class='label'>**GARDEN 2**</p>\
        <p class='label'>Moisture: <span class='value %s'>%d%%</span></p>\
        <p class='label'>State: <span class='%s'>%s</span></p>\
        <p class='label'>Irrigation: <span class='%s'>%s</span></p>\
      </div>\
      \
  </body>\
</html>",
           // DHT Values
           t, h,
           
           // Garden 1 values
           (state1 == "Wet" ? "wet" : "dry"), moisturePercent1, 
           (state1 == "Wet" ? "wet" : "dry"), state1.c_str(), 
           (irrigationStatus1 == "ON" ? "on" : "off"), irrigationStatus1.c_str(),
           
           // Garden 2 values
           (state2 == "Wet" ? "wet" : "dry"), moisturePercent2, 
           (state2 == "Wet" ? "wet" : "dry"), state2.c_str(),
           (irrigationStatus2 == "ON" ? "on" : "off"), irrigationStatus2.c_str()
          );
  server.send(200, "text/html", msg);
}

// --- Sensor Reading Functions ---

// Reads Temperature from DHT11
float readDHTTemperature() {
  float t = dht.readTemperature();
  if (isnan(t)) {    
    return -1.0; 
  }
  return t;
}

// Reads Humidity from DHT11
float readDHTHumidity() {
  float h = dht.readHumidity();
  if (isnan(h)) {
    return -1.0; 
  }
  return h;
}

// Reads the analog moisture value and converts it to a percentage (0-100)
int readMoisturePercent(int pin) {
  int rawValue = analogRead(pin);

  // Map the raw value to a percentage (0-100)
  int percentage = map(rawValue, DRY_CALIBRATION, WET_CALIBRATION, 0, 100);
  
  return constrain(percentage, 0, 100);
}

// --- Logic Functions ---

// Determines the state (Wet/Dry) based on the moisture percentage
String getMoistureState(int percent) {
  if (percent < DRY_THRESHOLD_PERCENT) {
    return "Dry";
  } else {
    return "Wet";
  }
}

// Controls the relay based on moisture percentage and returns the status string
String controlAndGetIrrigationStatus(int moisturePercent, int relayPin) {
    // We assume Active LOW relays (LOW = ON, HIGH = OFF)
    if (moisturePercent < DRY_THRESHOLD_PERCENT) {
        digitalWrite(relayPin, LOW); // Turn ON pump
    } else {
        digitalWrite(relayPin, HIGH); // Turn OFF pump
    }
    
    // Return the current status for display
    return (digitalRead(relayPin) == LOW) ? "ON" : "OFF";
}

// --- Setup Function ---
void setup(void) {
  Serial.begin(115200);
  dht.begin(); 

  // Set up relay pins as OUTPUT and ensure they are OFF initially (Active LOW -> HIGH)
  pinMode(RELAY_PIN_G1, OUTPUT);
  pinMode(RELAY_PIN_G2, OUTPUT);
  digitalWrite(RELAY_PIN_G1, HIGH); 
  digitalWrite(RELAY_PIN_G2, HIGH);

  // Set up moisture pins as INPUT 
  pinMode(MOISTURE_PIN_G1, INPUT);
  pinMode(MOISTURE_PIN_G2, INPUT);
  
  // --- WiFi Connection ---
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // --- mDNS and Server Setup ---
  if (MDNS.begin("aquaspray")) {
    Serial.println("MDNS responder started at http://aquaspray.local");
  }
  server.on("/", handleRoot);

  server.begin();
  Serial.println("HTTP server started");
}

// --- Loop Function ---
void loop(void) {
  // 1. Control Garden 1 Irrigation
  int moisturePercent1 = readMoisturePercent(MOISTURE_PIN_G1);
  controlAndGetIrrigationStatus(moisturePercent1, RELAY_PIN_G1);
  
  // 2. Control Garden 2 Irrigation
  int moisturePercent2 = readMoisturePercent(MOISTURE_PIN_G2);
  controlAndGetIrrigationStatus(moisturePercent2, RELAY_PIN_G2);

  // 3. Handle web server requests
  server.handleClient();
  delay(1000); 
}
