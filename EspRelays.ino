// --- Required Libraries (from user image) ---
// Note: These libraries are included as requested, 
// but are not necessary for this specific GPIO timing task.
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// --- Pin Definitions ---
// Define the GPIO pins connected to the relays.
const int RELAY_1 = 25;
const int RELAY_2 = 26;
const int RELAY_3 = 27;

// --- Signal Logic Adjustment for Active-Low Relays connected to NO terminals ---
// IMPORTANT: Most cheap relay modules are "Active-Low," meaning the relay activates 
// when the input signal is LOW.
// Since you are connected to the Normally Open (NO) terminals:
//  - RELAY_ON (LOW) will CLOSE the NO contact (circuit is complete).
//  - RELAY_OFF (HIGH) will OPEN the NO contact (circuit is broken/off).
const int RELAY_ON = LOW;
const int RELAY_OFF = HIGH;

void setup() {
  // Initialize the serial communication for debugging
  Serial.begin(115200);
  Serial.println("Relay Sequencer Initializing...");

  // Set the relay pins as outputs
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  pinMode(RELAY_3, OUTPUT);

  // --- Initial Setup State ---
  // Requested state: "turn low 3 relays" (i.e., turn them ON).
  // Since LOW = ON for active-low modules, all relays are turned ON immediately.
  digitalWrite(RELAY_1, RELAY_ON);
  digitalWrite(RELAY_2, RELAY_ON);
  digitalWrite(RELAY_3, RELAY_ON);
  
  Serial.println("Setup complete: All relays are initially ON (NO contact CLOSED).");
}

void loop() {
  Serial.println("\n--- Start of Loop Cycle ---");

  // Step 1: Turn all relays OFF (HIGH) to begin the sequential cycle cleanly.
  digitalWrite(RELAY_1, RELAY_OFF);
  digitalWrite(RELAY_2, RELAY_OFF);
  digitalWrite(RELAY_3, RELAY_OFF);
  Serial.println("All relays OFF (NO contact OPEN).");
  delay(100); // Small pause to ensure state change

  // --- Sequential Activation (2 second intervals) ---

  // 1. Turn on Relay 1 (LOW signal closes NO contact)
  digitalWrite(RELAY_1, RELAY_ON);
  Serial.println("Relay 1 ON.");
  delay(2000); // Wait 2 seconds

  // 2. Turn on Relay 2 (LOW signal closes NO contact)
  digitalWrite(RELAY_2, RELAY_ON);
  Serial.println("Relay 2 ON.");
  delay(2000); // Wait 2 seconds

  // 3. Turn on Relay 3 (LOW signal closes NO contact)
  digitalWrite(RELAY_3, RELAY_ON);
  Serial.println("Relay 3 ON. All relays are now ON (NO contacts CLOSED).");
  // The request states: "by the time the last relay turns on..." - this is that moment.

  // --- All Off Period (3 seconds) ---

  // Turn all relays OFF (HIGH signal opens NO contact)
  Serial.println("Turning all relays OFF for 3 seconds.");
  digitalWrite(RELAY_1, RELAY_OFF);
  digitalWrite(RELAY_2, RELAY_OFF);
  digitalWrite(RELAY_3, RELAY_OFF);

  delay(3000); // Wait 3 seconds
  
  Serial.println("--- End of Loop Cycle (3s OFF period completed) ---");
  // The loop will now repeat, starting the sequential activation again.
}
