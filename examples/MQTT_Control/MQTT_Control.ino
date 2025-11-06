/*
 * ESP32S3 SuperMini RGB - MQTT Control
 * 
 * Control RGB LED via MQTT protocol for IoT integration.
 * Compatible with Home Assistant, Node-RED, and other MQTT systems.
 * 
 * Required Library: PubSubClient (by Nick O'Leary)
 * 
 * MQTT Topics:
 * - esp32s3/rgb/set - Set color (format: "r,g,b")
 * - esp32s3/rgb/brightness - Set brightness (0-255)
 * - esp32s3/rgb/effect - Set effect (solid/rainbow/fade/strobe/pulse)
 * - esp32s3/rgb/power - Turn on/off (ON/OFF)
 * - esp32s3/rgb/state - Status updates (published)
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// MQTT Configuration
const char* mqtt_server = "YOUR_MQTT_BROKER_IP";
const int mqtt_port = 1883;
const char* mqtt_user = "YOUR_MQTT_USER";      // Leave empty if no auth
const char* mqtt_password = "YOUR_MQTT_PASS";  // Leave empty if no auth
const char* mqtt_client_id = "ESP32S3_RGB";

// MQTT Topics
const char* topic_set = "esp32s3/rgb/set";
const char* topic_brightness = "esp32s3/rgb/brightness";
const char* topic_effect = "esp32s3/rgb/effect";
const char* topic_power = "esp32s3/rgb/power";
const char* topic_state = "esp32s3/rgb/state";

// LED Configuration
#define LED_PIN     48
#define NUM_LEDS    1
#define BRIGHTNESS  50

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
WiFiClient espClient;
PubSubClient client(espClient);

// LED State
uint8_t currentRed = 255;
uint8_t currentGreen = 0;
uint8_t currentBlue = 0;
uint8_t currentBrightness = BRIGHTNESS;
String currentEffect = "solid";
bool isPoweredOn = true;

// Effect variables
unsigned long lastUpdate = 0;
uint16_t rainbowIndex = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32S3 RGB MQTT Control");
  
  // Initialize LED
  strip.begin();
  strip.setBrightness(currentBrightness);
  strip.show();
  
  // Connect to WiFi
  setupWiFi();
  
  // Setup MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
  
  // Initial color
  setColor(currentRed, currentGreen, currentBlue);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
  if (isPoweredOn) {
    updateEffect();
  }
  
  delay(10);
}

void setupWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      
      // Subscribe to topics
      client.subscribe(topic_set);
      client.subscribe(topic_brightness);
      client.subscribe(topic_effect);
      client.subscribe(topic_power);
      
      // Publish initial state
      publishState();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("MQTT message on ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);
  
  if (strcmp(topic, topic_set) == 0) {
    // Parse "r,g,b" format
    int r, g, b;
    if (sscanf(message.c_str(), "%d,%d,%d", &r, &g, &b) == 3) {
      currentRed = constrain(r, 0, 255);
      currentGreen = constrain(g, 0, 255);
      currentBlue = constrain(b, 0, 255);
      currentEffect = "solid";
      setColor(currentRed, currentGreen, currentBlue);
      publishState();
    }
  }
  else if (strcmp(topic, topic_brightness) == 0) {
    currentBrightness = constrain(message.toInt(), 0, 255);
    strip.setBrightness(currentBrightness);
    strip.show();
    publishState();
  }
  else if (strcmp(topic, topic_effect) == 0) {
    currentEffect = message;
    rainbowIndex = 0;
    publishState();
  }
  else if (strcmp(topic, topic_power) == 0) {
    if (message == "ON") {
      isPoweredOn = true;
      setColor(currentRed, currentGreen, currentBlue);
    } else if (message == "OFF") {
      isPoweredOn = false;
      setColor(0, 0, 0);
    }
    publishState();
  }
}

void publishState() {
  String state = "{";
  state += "\"power\":\"" + String(isPoweredOn ? "ON" : "OFF") + "\",";
  state += "\"effect\":\"" + currentEffect + "\",";
  state += "\"brightness\":" + String(currentBrightness) + ",";
  state += "\"color\":{";
  state += "\"r\":" + String(currentRed) + ",";
  state += "\"g\":" + String(currentGreen) + ",";
  state += "\"b\":" + String(currentBlue);
  state += "}}";
  
  client.publish(topic_state, state.c_str());
}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
  for(int i=0; i<NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

void updateEffect() {
  unsigned long currentTime = millis();
  
  if (currentEffect == "rainbow" && currentTime - lastUpdate > 50) {
    rainbowCycle();
    lastUpdate = currentTime;
  }
  else if (currentEffect == "fade" && currentTime - lastUpdate > 20) {
    fadeEffect();
    lastUpdate = currentTime;
  }
  else if (currentEffect == "strobe" && currentTime - lastUpdate > 100) {
    strobeEffect();
    lastUpdate = currentTime;
  }
  else if (currentEffect == "pulse" && currentTime - lastUpdate > 20) {
    pulseEffect();
    lastUpdate = currentTime;
  }
}

void rainbowCycle() {
  for(int i=0; i<NUM_LEDS; i++) {
    strip.setPixelColor(i, Wheel((i * 256 / NUM_LEDS + rainbowIndex) & 255));
  }
  strip.show();
  rainbowIndex++;
  if (rainbowIndex >= 256) rainbowIndex = 0;
}

void fadeEffect() {
  static uint8_t fadeValue = 0;
  static int8_t fadeDir = 1;
  
  fadeValue += fadeDir * 5;
  if (fadeValue >= 255 || fadeValue <= 0) {
    fadeDir = -fadeDir;
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

void strobeEffect() {
  static bool strobeState = false;
  strobeState = !strobeState;
  
  if (strobeState) {
    setColor(currentRed, currentGreen, currentBlue);
  } else {
    setColor(0, 0, 0);
  }
}

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
