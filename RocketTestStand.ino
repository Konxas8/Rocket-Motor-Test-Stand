#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <SD.h>
#include "HX711.h"

// ================= ΡΥΘΜΙΣΕΙΣ PINS =================
const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 27;
const int MOSFET_PIN = 4;
const int CS_PIN = 5; 
//SCK=18, MISO=19, MOSI=23 για την SD.

// ================= ΡΥΘΜΙΣΕΙΣ WIFI & ΑΣΦΑΛΕΙΑΣ =================
const char* ssid = "Rocket_Stand_AP";  
const char* password = "12345678"; 
const String FIRE_PIN = "4321"; 

// ================= ΜΕΤΑΒΛΗΤΕΣ =================
HX711 scale;
WebServer server(80);

// Συντελεστής 
float calibration_factor = 2280.0; 

bool isSDRecording = false;
bool isWiFiStreaming = false;
File logFile;
unsigned long lastFlushTime = 0;

// ================= HTML ΣΕΛΙΔΑ =================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Rocket Test Stand</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; background-color: #222; color: #fff; }
    h1 { color: #f39c12; }
    .btn { display: block; width: 80%; margin: 10px auto; padding: 15px; font-size: 20px; border: none; border-radius: 5px; cursor: pointer; }
    .btn-cal { background-color: #3498db; color: white; }
    .btn-wifi { background-color: #9b59b6; color: white; }
    .btn-sd { background-color: #2ecc71; color: white; }
    .btn-fire { background-color: #e74c3c; color: white; font-weight: bold; height: 60px; font-size: 25px;}
    .data { font-size: 40px; margin: 20px; color: #00e676; }
    input[type="password"] { font-size: 20px; padding: 10px; text-align: center; width: 50%; margin-bottom: 10px;}
  </style>
</head>
<body>
  <h1>ROCKET CONTROL</h1>
  <div class="data">Thrust: <span id="thrust">0.0</span> kg</div>
  
  <button class="btn btn-cal" onclick="sendCommand('calibrate')">1. CALIBRATE (TARE TO 0)</button>
  <button class="btn btn-wifi" onclick="toggleWiFi()">2. WIFI STREAM: <span id="wifiStatus">OFF</span></button>
  <button class="btn btn-sd" onclick="toggleSD()">3. SD RECORD: <span id="sdStatus">OFF</span></button>
  <br>
  
  <div style="background:#333; padding:15px; border-radius:10px; width:80%; margin:auto;">
    <h3>ARMING SEQUENCE</h3>
    <input type="password" id="authPin" placeholder="Enter PIN to Arm"><br>
    <button class="btn btn-fire" onclick="fireSequence()">4. FIRE !!!</button>
  </div>

<script>
  let wifiActive = false;
  let sdActive = false;

  function sendCommand(cmd) {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/" + cmd, true);
    xhr.send();
  }

  function toggleWiFi() {
    wifiActive = !wifiActive;
    document.getElementById("wifiStatus").innerText = wifiActive ? "ON" : "OFF";
    sendCommand(wifiActive ? "wifi_on" : "wifi_off");
  }

  function toggleSD() {
    sdActive = !sdActive;
    document.getElementById("sdStatus").innerText = sdActive ? "ON" : "OFF";
    sendCommand(sdActive ? "sd_on" : "sd_off");
  }

  function fireSequence() {
    let pinCode = document.getElementById("authPin").value;
    if (pinCode === "") {
        alert("Enter the PIN to arm the system.");
        return;
    }
    if (confirm("ΕΙΣΑΙ ΣΙΓΟΥΡΟΣ; Ο ΚΙΝΗΤΗΡΑΣ ΘΑ ΑΝΑΨΕΙ!")) {
       var xhr = new XMLHttpRequest();
       xhr.onreadystatechange = function() {
         if (this.readyState == 4) {
             if(this.status == 200) { alert("FIRING SEQUENCE STARTED!"); }
             else if(this.status == 401) { alert("ERROR: INCORRECT PIN!"); }
         }
       };
       xhr.open("GET", "/fire?pin=" + pinCode, true);
       xhr.send();
    }
  }

  setInterval(function() {
    if(wifiActive) {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          document.getElementById("thrust").innerHTML = this.responseText;
        }
      };
      xhr.open("GET", "/read", true);
      xhr.send();
    }
  }, 200);
</script>
</body>
</html>
)rawliteral";

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // Ρύθμιση Pins
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW); //MOSFET LOW!!!

  // Εκκίνηση WiFi AP
  WiFi.softAP(ssid, password);
  Serial.println("WiFi Access Point started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Ρύθμιση Διαδρομών Server
  server.on("/", []() { server.send(200, "text/html", index_html); });
  server.on("/calibrate", handleCalibrate);
  server.on("/wifi_on", []() { isWiFiStreaming = true; server.send(200, "text/plain", "OK"); });
  server.on("/wifi_off", []() { isWiFiStreaming = false; server.send(200, "text/plain", "OK"); });
  server.on("/sd_on", startSD);
  server.on("/sd_off", stopSD);
  server.on("/read", handleRead);
  server.on("/fire", handleFire);

  // --- NEW ROUTES TO SERVE SD FILES TO THE JAVASCRIPT PLOTTER ---
  server.on("/LOG.TXT", HTTP_GET, []() {
    File f = SD.open("/LOG.TXT");
    if (!f) { server.send(404, "text/plain", "File not found"); return; }
    server.streamFile(f, "text/plain");
    f.close();
  });
  
  server.on("/FIRE_LOG.TXT", HTTP_GET, []() {
    File f = SD.open("/FIRE_LOG.TXT");
    if (!f) { server.send(404, "text/plain", "File not found"); return; }
    server.streamFile(f, "text/plain");
    f.close();
  });

  // Εκκίνηση HX711
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare(); //Μηδενισμός

  // Εκκίνηση SD
  if (!SD.begin(CS_PIN)) {
    Serial.println("SD Card Mount Failed");
  } else {
    Serial.println("SD Card Initialized");
  }

  server.begin();
}

// ================= LOOP =================
void loop() {
  server.handleClient();

  if (isSDRecording && logFile) {
    if (scale.is_ready()) {
      float reading = scale.get_units(1); 
      logFile.print(millis());
      logFile.print(",");
      logFile.println(reading, 3);
      
      //flush κάθε 500ms
      if (millis() - lastFlushTime > 500) {
        logFile.flush();
        lastFlushTime = millis();
      }
    }
  }
}

// ================= ΛΕΙΤΟΥΡΓΙΕΣ =================

void handleCalibrate() {
  scale.tare();
  server.send(200, "text/plain", "Calibrated");
  Serial.println("Scale Tared to 0 kg");
}

void handleRead() {
  float weight = scale.get_units(1);
  server.send(200, "text/plain", String(weight, 3)); // 3 δεκαδικά ψηφία για κιλά
}

void startSD() {
  if (isSDRecording) return; 
  
  logFile = SD.open("/LOG.TXT", FILE_APPEND);
  if(logFile){
    isSDRecording = true;
    logFile.println("---- NEW SESSION ----");
    logFile.println("Time(ms), Force(kg)");
    server.send(200, "text/plain", "SD Started");
  } else {
    Serial.println("Failed to open file for recording");
    server.send(500, "text/plain", "SD Error");
  }
}

void stopSD() {
  isSDRecording = false;
  if(logFile) {
    logFile.close();
  }
  server.send(200, "text/plain", "SD Stopped");
}

// 2FA & WATCHDOG YIELD
void handleFire() {
  //(2FA)
  if (!server.hasArg("pin") || server.arg("pin") != FIRE_PIN) {
    server.send(401, "text/plain", "Unauthorized - Wrong PIN");
    Serial.println("Failed ignition attempt - Wrong PIN");
    return;
  }

  server.send(200, "text/plain", "FIRING");
  
  File fireFile = SD.open("/FIRE_LOG.TXT", FILE_APPEND);
  if(fireFile) {
    fireFile.println("---- IGNITION ----");
  }

  unsigned long startTime = millis();
  unsigned long fireDuration = 3000; // 3 Δευτερόλεπτα Ανάφλεξης
  
  Serial.println("FIRING MOSFTET!");
  
  // ΑΝΑΜΜΑ MOSFET
  digitalWrite(MOSFET_PIN, HIGH); 
  
  //Καταγραφή
  while (millis() - startTime < fireDuration) {
    yield(); // ΚΡΙΣΙΜΟ: Αποτρέπει το Watchdog Timer του ESP32 από το να κάνει reset!
    
    if (scale.is_ready()) { 
      float reading = scale.get_units(1); 
      unsigned long now = millis() - startTime;
      
      if (fireFile) {
        fireFile.print(now);
        fireFile.print(",");
        fireFile.println(reading, 3);
      }
    }
  }

  // ΣΒΗΣΙΜΟ MOSFET - ΑΠΟΛΥΤΗ ΠΡΟΤΕΡΑΙΟΤΗΤΑ
  digitalWrite(MOSFET_PIN, LOW);
  Serial.println("MOSFET OFF");

  if(fireFile) {
    fireFile.close();
  }
}
