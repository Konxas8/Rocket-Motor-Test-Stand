// --- PINS ---
const int MOSFET_PIN = 4;

void setup() {
  Serial.begin(115200);
  
  // 1. Ορισμός του Pin και ΚΛΕΙΔΩΜΑ σε κατάσταση OFF με το που ανοίγει
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW); 

  delay(2000); // Περιμένουμε λίγο να ανοίξει το Serial Monitor
  
  Serial.println("\n==================================");
  Serial.println("   MOSFET ENDLESS DIAGNOSTIC");
  Serial.println("==================================");
  Serial.println("WARNING: Ensure NO real igniters are connected!");
  Serial.println("Use a Multimeter or LED to verify power.");
  Serial.println("Starting in 3 seconds...\n");
  delay(3000);
}

void loop() {
  // --- 1. ΑΝΑΜΜΑ ΓΙΑ 2 ΔΕΥΤΕΡΟΛΕΠΤΑ ---
  Serial.println("[MOSFET ON] - Firing for 2 seconds...");
  digitalWrite(MOSFET_PIN, HIGH);
  delay(2000); 

  // --- 2. ΣΒΗΣΙΜΟ ΚΑΙ ΑΝΑΜΟΝΗ 30 ΔΕΥΤΕΡΟΛΕΠΤΑ ---
  Serial.println("[MOSFET OFF] - Cooling down for 30 seconds...");
  digitalWrite(MOSFET_PIN, LOW);

  // Κάνουμε μια αντίστροφη μέτρηση στην οθόνη για να ξέρεις ότι δεν κόλλησε
  Serial.print("Next fire in: ");
  for (int i = 30; i > 0; i--) {
    Serial.print(i);
    Serial.print("... ");
    delay(1000); // Περιμένει 1 δευτερόλεπτο ανά βήμα
  }
  Serial.println("\n"); 
}