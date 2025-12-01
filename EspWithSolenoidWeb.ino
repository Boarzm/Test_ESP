#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>

// --- Configuration: WiFi Credentials ---
const char *ssid = "My-Network";
const char *password = "chaker123456";

// --- Configuration: Pin Definitions ---

// DHT Sensor Pin 
const int DHT_PIN = 4;
#define DHTTYPE DHT11 // DHT 11 sensor type
DHT dht(DHT_PIN, DHTTYPE);

// Analog pins for Moisture Sensors 
const int MOISTURE_PIN_G1 = 36; 
const int MOISTURE_PIN_G2 = 39; 

// Digital pins for Solenoid Valves (G1 & G2)
const int SOLENOID_PIN_G1 = 25; 
const int SOLENOID_PIN_G2 = 26; 

// NEW Digital pin for the Main Water Pump
const int PUMP_PIN = 27; 

// --- Configuration: Calibration Values ---
const int DRY_CALIBRATION = 4095; 
const int WET_CALIBRATION = 1800; 
const int DRY_THRESHOLD_PERCENT = 40; 

// --- Global Variables and Server Setup ---
WebServer server(80);

// --- State Variables for Sequencing ---
// Tracks the time (in ms) when a valve was opened or closed.
unsigned long valve_G1_time = 0;
unsigned long valve_G2_time = 0;
// Half-second delay requirement (500ms)
const unsigned long SEQUENCING_DELAY = 500; 

// --- Function Prototypes ---
float readDHTTemperature();
float readDHTHumidity();
int readMoisturePercent(int pin);
void controlIrrigationSystem(int moisturePercent1, int moisturePercent2);
String getIrrigationStatus(int solenoidPin, int pumpPin);
String getMoistureState(int percent);
void handleRoot();

// --- Main HTTP Handler Function ---
void handleRoot() {
  
  // 1. Read all sensor values
  float t = readDHTTemperature();
  float h = readDHTHumidity();
  int moisturePercent1 = readMoisturePercent(MOISTURE_PIN_G1);
  int moisturePercent2 = readMoisturePercent(MOISTURE_PIN_G2);
  
  // 2. Control System (Handles sequencing and overall pump control)
  controlIrrigationSystem(moisturePercent1, moisturePercent2);
  
  // 3. Get Status for Display
  String irrigationStatus1 = getIrrigationStatus(SOLENOID_PIN_G1, PUMP_PIN);
  String irrigationStatus2 = getIrrigationStatus(SOLENOID_PIN_G2, PUMP_PIN);
  String state1 = getMoistureState(moisturePercent1);
  String state2 = getMoistureState(moisturePercent2);
  
  // 4. Serial Monitor Output 
  Serial.println("--- AquaSpray Dashboard Data ---");
  Serial.print("DHT: Temp="); Serial.print(t); Serial.print("C, Humidity="); Serial.print(h); Serial.println("%");
  Serial.print("Garden 1: Moisture="); Serial.print(moisturePercent1); Serial.print("% ("); Serial.print(state1); Serial.print("), Solenoid:"); Serial.print((digitalRead(SOLENOID_PIN_G1) == LOW ? "OPEN" : "CLOSED")); Serial.print(", Pump: "); Serial.println((digitalRead(PUMP_PIN) == LOW ? "ON" : "OFF"));
  Serial.print("Garden 2: Moisture="); Serial.print(moisturePercent2); Serial.print("% ("); Serial.print(state2); Serial.print("), Solenoid:"); Serial.print((digitalRead(SOLENOID_PIN_G2) == LOW ? "OPEN" : "CLOSED")); Serial.print(", Pump: "); Serial.println((digitalRead(PUMP_PIN) == LOW ? "ON" : "OFF"));

  char msg[3000]; 

  // 5. HTML Content for Web Dashboard 
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
        display: inline-block; \
        width: 45%%; \
        min-width: 250px; \
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
        <p class='label'>Irrigation Status: <span class='%s'>%s</span></p>\
      </div>\
      \
      \
      <div class='garden-data'>\
        <p class='label'>**GARDEN 2**</p>\
        <p class='label'>Moisture: <span class='value %s'>%d%%</span></p>\
        <p class='label'>State: <span class='%s'>%s</span></p>\
        <p class='label'>Irrigation Status: <span class='%s'>%s</span></p>\
      </div>\
      \
      \
      <p class='label' style='margin-top: 20px;'>\
        <i class='fas fa-water' style='color:#00add6;'></i>\
        <span class='label'>Main Pump Status:</span>\
        <span class='%s'>%s</span>\
      </p>\
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
           (irrigationStatus2 == "ON" ? "on" : "off"), irrigationStatus2.c_str(),
           
           // Overall Pump Status
           (digitalRead(PUMP_PIN) == LOW ? "on" : "off"), (digitalRead(PUMP_PIN) == LOW ? "ON" : "OFF")
          );
  server.send(200, "text/html", msg);
}

// --- Sensor Reading Functions ---

float readDHTTemperature() {
  float t = dht.readTemperature();
  if (isnan(t)) {    
    return -1.0; 
  }
  return t;
}

float readDHTHumidity() {
  float h = dht.readHumidity();
  if (isnan(h)) {
    return -1.0; 
  }
  return h;
}

int readMoisturePercent(int pin) {
  int rawValue = analogRead(pin);
  // Map the raw value to a percentage (0-100). DRY=4095 (0%), WET=1800 (100%)
  int percentage = map(rawValue, DRY_CALIBRATION, WET_CALIBRATION, 0, 100);
  
  return constrain(percentage, 0, 100);
}

// --- Sequencing and Control Logic ---

void controlIrrigationSystem(int m1, int m2) {
    unsigned long currentMillis = millis();

    // Determine if EITHER garden needs watering
    bool needWaterG1 = (m1 < DRY_THRESHOLD_PERCENT);
    bool needWaterG2 = (m2 < DRY_THRESHOLD_PERCENT);

    // --- Garden 1 Solenoid Control ---
    if (needWaterG1 && digitalRead(SOLENOID_PIN_G1) == HIGH) {
        // If dry and solenoid is closed, open the solenoid and start timer
        digitalWrite(SOLENOID_PIN_G1, LOW); // Open Solenoid (Active LOW)
        valve_G1_time = currentMillis; // Record open time
    } else if (!needWaterG1 && digitalRead(SOLENOID_PIN_G1) == LOW) {
        // If wet and solenoid is open, start timer for pump-off sequence
        valve_G1_time = currentMillis; // Record close request time
    }
    
    // --- Garden 2 Solenoid Control ---
    if (needWaterG2 && digitalRead(SOLENOID_PIN_G2) == HIGH) {
        // If dry and solenoid is closed, open the solenoid and start timer
        digitalWrite(SOLENOID_PIN_G2, LOW); // Open Solenoid (Active LOW)
        valve_G2_time = currentMillis; // Record open time
    } else if (!needWaterG2 && digitalRead(SOLENOID_PIN_G2) == LOW) {
        // If wet and solenoid is open, start timer for pump-off sequence
        valve_G2_time = currentMillis; // Record close request time
    }

    // --- Pump Control and Sequencing ---
    
    // Check if the pump needs to be ON due to a solenoid opening (Turn-ON Sequence)
    if (digitalRead(PUMP_PIN) == HIGH) { // Pump is currently OFF
        // Check G1's ON Sequence
        if (digitalRead(SOLENOID_PIN_G1) == LOW && (currentMillis - valve_G1_time >= SEQUENCING_DELAY)) {
            digitalWrite(PUMP_PIN, LOW); // Turn ON pump (500ms AFTER valve opened)
        }
        // Check G2's ON Sequence
        else if (digitalRead(SOLENOID_PIN_G2) == LOW && (currentMillis - valve_G2_time >= SEQUENCING_DELAY)) {
            digitalWrite(PUMP_PIN, LOW); // Turn ON pump (500ms AFTER valve opened)
        }
    }

    // Check if the pump needs to be OFF due to a solenoid closing (Shut-OFF Sequence)
    if (digitalRead(PUMP_PIN) == LOW) { // Pump is currently ON
        bool g1_ready_to_close = (!needWaterG1 && digitalRead(SOLENOID_PIN_G1) == LOW && (currentMillis - valve_G1_time >= SEQUENCING_DELAY));
        bool g2_ready_to_close = (!needWaterG2 && digitalRead(SOLENOID_PIN_G2) == LOW && (currentMillis - valve_G2_time >= SEQUENCING_DELAY));

        // If EITHER valve is in the process of closing (g1_ready_to_close or g2_ready_to_close)
        // We only shut the pump off IF NO other solenoid is actively open.
        
        bool pump_still_needed = (digitalRead(SOLENOID_PIN_G1) == LOW && !g1_ready_to_close) || (digitalRead(SOLENOID_PIN_G2) == LOW && !g2_ready_to_close);

        if (!pump_still_needed) {
            // Pump is not needed by any open solenoid. Shut it off.
            digitalWrite(PUMP_PIN, HIGH); // Turn OFF pump
            
            // Now that the pump is off, execute the solenoid CLOSE action (500ms later)
            // Reset timers to prevent immediate re-trigger on next cycle
            valve_G1_time = currentMillis;
            valve_G2_time = currentMillis;
        }
    }
    
    // --- Final Solenoid Close Action (Delayed until pump is guaranteed OFF) ---
    // If G1 requested closure (is wet, solenoid is LOW, and 500ms passed since pump-off or request)
    if (digitalRead(SOLENOID_PIN_G1) == LOW && !needWaterG1 && digitalRead(PUMP_PIN) == HIGH) {
        // Now that pump is OFF, close the solenoid
        digitalWrite(SOLENOID_PIN_G1, HIGH); // Close Solenoid
    }

    // If G2 requested closure
    if (digitalRead(SOLENOID_PIN_G2) == LOW && !needWaterG2 && digitalRead(PUMP_PIN) == HIGH) {
        // Now that pump is OFF, close the solenoid
        digitalWrite(SOLENOID_PIN_G2, HIGH); // Close Solenoid
    }
}

// Returns the Irrigation Status for Display
String getIrrigationStatus(int solenoidPin, int pumpPin) {
    // If the solenoid is OPEN AND the pump is ON, then irrigation is active for this garden.
    if (digitalRead(solenoidPin) == LOW && digitalRead(pumpPin) == LOW) {
        return "ON";
    }
    return "OFF";
}

// Determines the state (Wet/Dry) based on the moisture percentage
String getMoistureState(int percent) {
  if (percent < DRY_THRESHOLD_PERCENT) {
    return "Dry";
  } else {
    return "Wet";
  }
}

// --- Setup Function ---
void setup(void) {
  Serial.begin(115200);
  dht.begin(); 

  // Set all control pins as OUTPUT
  pinMode(SOLENOID_PIN_G1, OUTPUT);
  pinMode(SOLENOID_PIN_G2, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  
  // Ensure all relays are OFF/CLOSED initially (Active LOW -> HIGH)
  digitalWrite(SOLENOID_PIN_G1, HIGH); 
  digitalWrite(SOLENOID_PIN_G2, HIGH);
  digitalWrite(PUMP_PIN, HIGH);

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
  
  // Read moisture just before controlling the system
  int moisturePercent1 = readMoisturePercent(MOISTURE_PIN_G1);
  int moisturePercent2 = readMoisturePercent(MOISTURE_PIN_G2);
  
  // Call the main control function (handles sequencing and pump)
  controlIrrigationSystem(moisturePercent1, moisturePercent2);

  // Handle web server requests
  server.handleClient();
  delay(100); // Check status more frequently for sequencing accuracy
}
