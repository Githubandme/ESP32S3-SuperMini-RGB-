/*
 * ESP32S3 SuperMini RGB - Home Assistant Integration
 * 
 * Full Home Assistant integration with MQTT Auto-Discovery.
 * Automatically appears in Home Assistant as a light entity.
 * 
 * Required Library: 
 * - PubSubClient (by Nick O'Leary)
 * - ArduinoJson (by Benoit Blanchon)
 * 
 * Configuration:
 * 1. Update WiFi credentials
 * 2. Update MQTT broker settings
 * 3. Upload to ESP32S3
 * 4. Device will auto-discover in Home Assistant
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// MQTT Configuration
const char* mqtt_server = "YOUR_MQTT_BROKER_IP";
const int mqtt_port = 1883;
const char* mqtt_user = "YOUR_MQTT_USER";
const char* mqtt_password = "YOUR_MQTT_PASS";

// Device Configuration
const char* device_name = "ESP32S3 RGB Light";
const char* device_id = "esp32s3_rgb_light";

// MQTT Topics (Home Assistant compatible)
const char* ha_discovery_topic = "homeassistant/light/esp32s3_rgb/config";
const char* state_topic = "homeassistant/light/esp32s3_rgb/state";
const char* command_topic = "homeassistant/light/esp32s3_rgb/set";

// LED Configuration
#define LED_PIN     48
#define NUM_LEDS    1

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
WiFiClient espClient;
PubSubClient client(espClient);

// LED State
struct LightState {
  bool power = true;
  uint8_t brightness = 255;
  uint8_t red = 255;
  uint8_t green = 255;
  uint8_t blue = 255;
  String effect = "solid";
} light;

// Effect variables
unsigned long lastUpdate = 0;
uint16_t effectIndex = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32S3 RGB - Home Assistant Integration");
  
  // Initialize LED
  strip.begin();
  strip.setBrightness(light.brightness);
  strip.show();
  
  // Connect to WiFi
  setupWiFi();
  
  // Setup MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
  client.setBufferSize(512); // Increase buffer for discovery
  
  // Initial color
  updateLED();
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
  if (light.power) {
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
    
    if (client.connect(device_id, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      
      // Send Home Assistant discovery message
      sendDiscoveryMessage();
      
      // Subscribe to command topic
      client.subscribe(command_topic);
      
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

void sendDiscoveryMessage() {
  StaticJsonDocument<512> doc;
  
  doc["name"] = device_name;
  doc["unique_id"] = device_id;
  doc["command_topic"] = command_topic;
  doc["state_topic"] = state_topic;
  doc["schema"] = "json";
  doc["brightness"] = true;
  doc["rgb"] = true;
  doc["effect"] = true;
  
  JsonArray effects = doc.createNestedArray("effect_list");
  effects.add("solid");
  effects.add("rainbow");
  effects.add("fade");
  effects.add("strobe");
  effects.add("pulse");
  
  JsonObject device = doc.createNestedObject("device");
  device["identifiers"][0] = device_id;
  device["name"] = device_name;
  device["model"] = "ESP32S3 SuperMini RGB";
  device["manufacturer"] = "Espressif";
  device["sw_version"] = "1.0.0";
  
  String output;
  serializeJson(doc, output);
  
  client.publish(ha_discovery_topic, output.c_str(), true);
  Serial.println("Discovery message sent");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("MQTT message: ");
  Serial.println(message);
  
  // Parse JSON command
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.println("Failed to parse JSON");
    return;
  }
  
  // Update state based on command
  if (doc.containsKey("state")) {
    light.power = (doc["state"] == "ON");
  }
  
  if (doc.containsKey("brightness")) {
    light.brightness = doc["brightness"];
  }
  
  if (doc.containsKey("color")) {
    light.red = doc["color"]["r"];
    light.green = doc["color"]["g"];
    light.blue = doc["color"]["b"];
    light.effect = "solid"; // Reset to solid when color changes
  }
  
  if (doc.containsKey("effect")) {
    light.effect = doc["effect"].as<String>();
    effectIndex = 0; // Reset effect
  }
  
  updateLED();
  publishState();
}

void publishState() {
  StaticJsonDocument<256> doc;
  
  doc["state"] = light.power ? "ON" : "OFF";
  doc["brightness"] = light.brightness;
  doc["color"]["r"] = light.red;
  doc["color"]["g"] = light.green;
  doc["color"]["b"] = light.blue;
  doc["effect"] = light.effect;
  
  String output;
  serializeJson(doc, output);
  
  client.publish(state_topic, output.c_str(), true);
}

void updateLED() {
  if (!light.power) {
    for(int i=0; i<NUM_LEDS; i++) {
      strip.setPixelColor(i, 0);
    }
    strip.show();
    return;
  }
  
  strip.setBrightness(light.brightness);
  
  if (light.effect == "solid") {
    for(int i=0; i<NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(light.red, light.green, light.blue));
    }
    strip.show();
  }
}

void updateEffect() {
  unsigned long currentTime = millis();
  
  if (light.effect == "solid") {
    return;
  }
  else if (light.effect == "rainbow" && currentTime - lastUpdate > 50) {
    rainbowEffect();
    lastUpdate = currentTime;
  }
  else if (light.effect == "fade" && currentTime - lastUpdate > 20) {
    fadeEffect();
    lastUpdate = currentTime;
  }
  else if (light.effect == "strobe" && currentTime - lastUpdate > 100) {
    strobeEffect();
    lastUpdate = currentTime;
  }
  else if (light.effect == "pulse" && currentTime - lastUpdate > 20) {
    pulseEffect();
    lastUpdate = currentTime;
  }
}

void rainbowEffect() {
  for(int i=0; i<NUM_LEDS; i++) {
    strip.setPixelColor(i, Wheel((i * 256 / NUM_LEDS + effectIndex) & 255));
  }
  strip.show();
  effectIndex++;
  if (effectIndex >= 256) effectIndex = 0;
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
      (light.red * fadeValue) / 255,
      (light.green * fadeValue) / 255,
      (light.blue * fadeValue) / 255
    ));
  }
  strip.show();
}

void strobeEffect() {
  static bool strobeState = false;
  strobeState = !strobeState;
  
  if (strobeState) {
    for(int i=0; i<NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(light.red, light.green, light.blue));
    }
  } else {
    for(int i=0; i<NUM_LEDS; i++) {
      strip.setPixelColor(i, 0);
    }
  }
  strip.show();
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
      (light.red * pulseValue) / 255,
      (light.green * pulseValue) / 255,
      (light.blue * pulseValue) / 255
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
