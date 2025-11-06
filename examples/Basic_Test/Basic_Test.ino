/*
 * ESP32S3 SuperMini RGB - Basic Test
 * 
 * Simple sketch to test if your RGB LED is working correctly.
 * Cycles through Red, Green, Blue colors.
 * 
 * Hardware: ESP32S3 SuperMini with WS2812B RGB LED on GPIO 48
 */

#include <Adafruit_NeoPixel.h>

// Configuration
#define LED_PIN     48
#define NUM_LEDS    1
#define BRIGHTNESS  50

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32S3 RGB Basic Test");
  
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  
  Serial.println("LED initialized");
  Serial.println("Cycling through colors...");
}

void loop() {
  // Red
  Serial.println("Red");
  strip.setPixelColor(0, strip.Color(255, 0, 0));
  strip.show();
  delay(1000);
  
  // Green
  Serial.println("Green");
  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();
  delay(1000);
  
  // Blue
  Serial.println("Blue");
  strip.setPixelColor(0, strip.Color(0, 0, 255));
  strip.show();
  delay(1000);
  
  // Yellow
  Serial.println("Yellow");
  strip.setPixelColor(0, strip.Color(255, 255, 0));
  strip.show();
  delay(1000);
  
  // Cyan
  Serial.println("Cyan");
  strip.setPixelColor(0, strip.Color(0, 255, 255));
  strip.show();
  delay(1000);
  
  // Magenta
  Serial.println("Magenta");
  strip.setPixelColor(0, strip.Color(255, 0, 255));
  strip.show();
  delay(1000);
  
  // White
  Serial.println("White");
  strip.setPixelColor(0, strip.Color(255, 255, 255));
  strip.show();
  delay(1000);
  
  // Off
  Serial.println("Off");
  strip.setPixelColor(0, strip.Color(0, 0, 0));
  strip.show();
  delay(1000);
}
