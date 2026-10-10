#include "HX711.h"

// --- PINS ---
const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 27;

HX711 scale;

void setup() {
  Serial.begin(115200);
  delay(1000); // Δίνουμε 1 δευτερόλεπτο για να ανοίξει το Serial Monitor
  
  Serial.println("\n--- INITIATING HX711 DIAGNOSTICS ---");
  Serial.println("1. Testing Connection...");

  // Ξεκινάμε την επικοινωνία
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  // Ελέγχουμε αν το τσιπάκι ανταποκρίνεται
  if (scale.is_ready()) {
    Serial.println("SUCCESS! HX711 Found.");
  } else {
    Serial.println("FAILED! HX711 not found.");
    Serial.println("ERROR: Check your VCC, GND, DT(26), and SCK(27) wiring.");
    while (1); // Παγώνει το πρόγραμμα εδώ αν υπάρχει σφάλμα
  }

  Serial.println("2. Taring the scale (Do NOT touch the load cell)...");
  scale.set_scale(1.0); // Βάζουμε το συντελεστή στο 1 για να δούμε τα ΚΑΘΑΡΑ RAW νούμερα
  scale.tare();         // Μηδενίζουμε
  
  Serial.println("SUCCESS! Scale is tared to 0.");
  Serial.println("\n--- LIVE SENSOR DATA ---");
  Serial.println("Press down on the load cell with your finger to see the numbers change!");
}

void loop() {
  if (scale.is_ready()) {
    // Παίρνουμε το μέσο όρο από 5 μετρήσεις για πιο σταθερό νούμερο
    long raw_reading = scale.get_units(5); 
    
    Serial.print("Raw Value: ");
    Serial.println(raw_reading);
  } else {
    Serial.println("HX711 disconnected or not ready.");
  }
  
  delay(500); // Περιμένουμε μισό δευτερόλεπτο για να προλαβαίνουμε να διαβάζουμε την οθόνη
}