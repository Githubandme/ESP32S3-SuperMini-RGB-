/*
 * ESP32S3 SuperMini RGB Light Control System
 * 
 * This sketch provides a comprehensive RGB LED control system for ESP32S3 SuperMini
 * Features:
 * - WiFi web server for remote control
 * - Multiple lighting effects (solid, rainbow, fade, strobe, etc.)
 * - Adjustable brightness
 * - Color picker interface
 * 
 * Hardware: ESP32S3 SuperMini with WS2812B RGB LED (GPIO 48)
 * 
 * Author: ESP32S3 RGB Control System
 * Date: 2025
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>

// WiFi Configuration
const char* ssid = "ESP32S3-RGB";      // Change to your WiFi SSID or use AP mode
const char* password = "12345678";      // Change to your WiFi password

// LED Configuration
#define LED_PIN     48    // GPIO48 is typically used for the onboard RGB LED on ESP32S3 SuperMini
#define NUM_LEDS    1     // Number of RGB LEDs (SuperMini has 1 onboard)
#define BRIGHTNESS  50    // Initial brightness (0-255)

// Initialize NeoPixel
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Web Server
WebServer server(80);

// Current LED State
uint8_t currentRed = 255;
uint8_t currentGreen = 0;
uint8_t currentBlue = 0;
uint8_t currentBrightness = BRIGHTNESS;
String currentEffect = "solid";
bool isRunning = true;

// Effect Variables
unsigned long lastUpdate = 0;
uint16_t rainbowIndex = 0;
uint8_t fadeDirection = 1;
uint8_t fadeValue = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nESP32S3 SuperMini RGB Control System Starting...");
  
  // Initialize LED strip
  strip.begin();
  strip.setBrightness(currentBrightness);
  strip.show(); // Initialize all pixels to 'off'
  
  // Start WiFi in Access Point mode
  Serial.println("Setting up Access Point...");
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/setColor", handleSetColor);
  server.on("/setEffect", handleSetEffect);
  server.on("/setBrightness", handleSetBrightness);
  server.on("/toggle", handleToggle);
  server.on("/status", handleStatus);
  
  server.begin();
  Serial.println("HTTP server started");
  
  // Initial color display
  setColor(currentRed, currentGreen, currentBlue);
}

void loop() {
  server.handleClient();
  
  if (isRunning) {
    updateEffect();
  }
  
  delay(10); // Small delay to prevent watchdog issues
}

// Update current effect
void updateEffect() {
  unsigned long currentTime = millis();
  
  if (currentEffect == "solid") {
    // Solid color - no animation needed
    return;
  }
  else if (currentEffect == "rainbow") {
    if (currentTime - lastUpdate > 50) {
      rainbowCycle();
      lastUpdate = currentTime;
    }
  }
  else if (currentEffect == "fade") {
    if (currentTime - lastUpdate > 20) {
      fadeEffect();
      lastUpdate = currentTime;
    }
  }
  else if (currentEffect == "strobe") {
    if (currentTime - lastUpdate > 100) {
      strobeEffect();
      lastUpdate = currentTime;
    }
  }
  else if (currentEffect == "pulse") {
    if (currentTime - lastUpdate > 20) {
      pulseEffect();
      lastUpdate = currentTime;
    }
  }
}

// Set solid color
void setColor(uint8_t r, uint8_t g, uint8_t b) {
  for(int i=0; i<NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

// Rainbow cycle effect
void rainbowCycle() {
  for(int i=0; i<NUM_LEDS; i++) {
    strip.setPixelColor(i, Wheel((i * 256 / NUM_LEDS + rainbowIndex) & 255));
  }
  strip.show();
  rainbowIndex++;
  if (rainbowIndex >= 256) rainbowIndex = 0;
}

// Fade effect
void fadeEffect() {
  if (fadeDirection == 1) {
    fadeValue += 5;
    if (fadeValue >= 255) {
      fadeValue = 255;
      fadeDirection = 0;
    }
  } else {
    fadeValue -= 5;
    if (fadeValue <= 0) {
      fadeValue = 0;
      fadeDirection = 1;
    }
  }
  
  for(int i=0; i<NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(
      (currentRed * fadeValue) / 255,
      (currentGreen * fadeValue) / 255,
      (currentBlue * fadeValue) / 255
    ));
  }
  strip.show();
}

// Strobe effect
void strobeEffect() {
  static bool strobeState = false;
  strobeState = !strobeState;
  
  if (strobeState) {
    setColor(currentRed, currentGreen, currentBlue);
  } else {
    setColor(0, 0, 0);
  }
}

// Pulse effect
void pulseEffect() {
  static uint8_t pulseValue = 0;
  static int8_t pulseDir = 1;
  
  pulseValue += pulseDir * 3;
  if (pulseValue >= 255 || pulseValue <= 0) {
    pulseDir = -pulseDir;
  }
  
  for(int i=0; i<NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(
      (currentRed * pulseValue) / 255,
      (currentGreen * pulseValue) / 255,
      (currentBlue * pulseValue) / 255
    ));
  }
  strip.show();
}

// Wheel function for rainbow effect
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// Web server handlers
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32S3 RGB Control</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      max-width: 600px;
      margin: 0 auto;
      padding: 20px;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
    }
    .container {
      background: white;
      border-radius: 15px;
      padding: 30px;
      box-shadow: 0 10px 30px rgba(0,0,0,0.3);
    }
    h1 {
      color: #333;
      text-align: center;
      margin-bottom: 30px;
    }
    .control-group {
      margin-bottom: 25px;
    }
    label {
      display: block;
      margin-bottom: 8px;
      color: #555;
      font-weight: bold;
    }
    input[type="color"] {
      width: 100%;
      height: 60px;
      border: none;
      border-radius: 8px;
      cursor: pointer;
    }
    input[type="range"] {
      width: 100%;
      height: 8px;
      border-radius: 5px;
      background: #ddd;
      outline: none;
    }
    select, button {
      width: 100%;
      padding: 12px;
      border: 2px solid #667eea;
      border-radius: 8px;
      font-size: 16px;
      cursor: pointer;
      transition: all 0.3s;
    }
    select {
      background: white;
    }
    button {
      background: #667eea;
      color: white;
      font-weight: bold;
      border: none;
      margin-top: 10px;
    }
    button:hover {
      background: #5568d3;
      transform: translateY(-2px);
      box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
    }
    button:active {
      transform: translateY(0);
    }
    .status {
      background: #f0f0f0;
      padding: 15px;
      border-radius: 8px;
      margin-top: 20px;
      text-align: center;
    }
    .brightness-value {
      display: inline-block;
      margin-left: 10px;
      color: #667eea;
      font-weight: bold;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🌈 ESP32S3 RGB Control</h1>
    
    <div class="control-group">
      <label>Color Picker:</label>
      <input type="color" id="colorPicker" value="#ff0000">
      <button onclick="setColor()">Apply Color</button>
    </div>
    
    <div class="control-group">
      <label>Effect:</label>
      <select id="effectSelect" onchange="setEffect()">
        <option value="solid">Solid Color</option>
        <option value="rainbow">Rainbow Cycle</option>
        <option value="fade">Fade</option>
        <option value="strobe">Strobe</option>
        <option value="pulse">Pulse</option>
      </select>
    </div>
    
    <div class="control-group">
      <label>Brightness: <span class="brightness-value" id="brightnessValue">50</span>%</label>
      <input type="range" id="brightnessSlider" min="0" max="255" value="50" oninput="updateBrightness()">
    </div>
    
    <button onclick="toggleLED()">Toggle ON/OFF</button>
    
    <div class="status" id="status">
      Status: Ready
    </div>
  </div>
  
  <script>
    function setColor() {
      const color = document.getElementById('colorPicker').value;
      const r = parseInt(color.substr(1,2), 16);
      const g = parseInt(color.substr(3,2), 16);
      const b = parseInt(color.substr(5,2), 16);
      
      fetch(`/setColor?r=${r}&g=${g}&b=${b}`)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
        });
    }
    
    function setEffect() {
      const effect = document.getElementById('effectSelect').value;
      fetch(`/setEffect?effect=${effect}`)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
        });
    }
    
    function updateBrightness() {
      const brightness = document.getElementById('brightnessSlider').value;
      const percent = Math.round((brightness / 255) * 100);
      document.getElementById('brightnessValue').innerHTML = percent;
      
      fetch(`/setBrightness?value=${brightness}`)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
        });
    }
    
    function toggleLED() {
      fetch('/toggle')
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
        });
    }
    
    // Update status periodically
    setInterval(() => {
      fetch('/status')
        .then(response => response.json())
        .then(data => {
          document.getElementById('effectSelect').value = data.effect;
        });
    }, 2000);
  </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleSetColor() {
  if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
    currentRed = server.arg("r").toInt();
    currentGreen = server.arg("g").toInt();
    currentBlue = server.arg("b").toInt();
    
    currentEffect = "solid";
    setColor(currentRed, currentGreen, currentBlue);
    
    server.send(200, "text/plain", "Color updated");
    Serial.printf("Color set to R:%d G:%d B:%d\n", currentRed, currentGreen, currentBlue);
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void handleSetEffect() {
  if (server.hasArg("effect")) {
    currentEffect = server.arg("effect");
    rainbowIndex = 0;
    fadeValue = 0;
    fadeDirection = 1;
    
    server.send(200, "text/plain", "Effect set to " + currentEffect);
    Serial.println("Effect changed to: " + currentEffect);
  } else {
    server.send(400, "text/plain", "Missing effect parameter");
  }
}

void handleSetBrightness() {
  if (server.hasArg("value")) {
    currentBrightness = server.arg("value").toInt();
    strip.setBrightness(currentBrightness);
    strip.show();
    
    server.send(200, "text/plain", "Brightness updated");
    Serial.printf("Brightness set to: %d\n", currentBrightness);
  } else {
    server.send(400, "text/plain", "Missing value parameter");
  }
}

void handleToggle() {
  isRunning = !isRunning;
  
  if (!isRunning) {
    setColor(0, 0, 0);
    server.send(200, "text/plain", "LED OFF");
    Serial.println("LED turned OFF");
  } else {
    setColor(currentRed, currentGreen, currentBlue);
    server.send(200, "text/plain", "LED ON");
    Serial.println("LED turned ON");
  }
}

void handleStatus() {
  String json = "{";
  json += "\"effect\":\"" + currentEffect + "\",";
  json += "\"brightness\":" + String(currentBrightness) + ",";
  json += "\"r\":" + String(currentRed) + ",";
  json += "\"g\":" + String(currentGreen) + ",";
  json += "\"b\":" + String(currentBlue) + ",";
  json += "\"running\":" + String(isRunning ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
}
