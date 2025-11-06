# Hardware Setup Guide

## ESP32S3 SuperMini Specifications

### Board Features
- **MCU**: ESP32-S3FN8 (Dual-core Xtensa LX7)
- **Flash**: 8MB
- **PSRAM**: 8MB
- **USB**: Native USB (Type-C)
- **WiFi**: 2.4GHz 802.11 b/g/n
- **Bluetooth**: BLE 5.0
- **Size**: 22.52mm × 18mm (Ultra compact!)

### Built-in RGB LED
Most ESP32S3 SuperMini boards come with a built-in WS2812B RGB LED:
- **Location**: On-board (usually near the USB port)
- **GPIO Pin**: GPIO 48 (default)
- **Type**: WS2812B NeoPixel compatible
- **Voltage**: 3.3V

## Pin Configuration

### Default Setup (Built-in LED)
```
ESP32S3 SuperMini
┌─────────────────┐
│                 │
│    [USB-C]      │
│                 │
│   GPIO 48 ──────┼──> Built-in RGB LED (WS2812B)
│                 │
│                 │
└─────────────────┘
```

### External LED Strip Connection

If you want to connect an external WS2812B LED strip:

```
ESP32S3 SuperMini          WS2812B LED Strip
┌─────────────────┐       ┌──────────────┐
│                 │       │              │
│   GPIO 48  ─────┼───────┼──> DIN       │
│                 │       │              │
│   GND      ─────┼───────┼──> GND       │
│                 │       │              │
│   5V or 3V3 ────┼───────┼──> VCC       │
│                 │       │              │
└─────────────────┘       └──────────────┘
```

**Important Notes:**
- For strips with 10+ LEDs, use external 5V power supply
- Add a 470Ω resistor between GPIO and LED DIN (optional but recommended)
- Add a 1000µF capacitor across LED strip power (optional but recommended)
- Keep data wire short (< 1 meter) for best signal integrity

## Wiring for Different LED Counts

### Single LED (Built-in)
- No additional wiring required
- Default configuration works out of the box

### LED Strip (1-10 LEDs)
- Can be powered from ESP32S3's 5V pin
- Connect as shown in diagram above

### LED Strip (10-60 LEDs)
- Requires external 5V power supply
- Connect power supply GND to ESP32S3 GND
- Power supply 5V to LED strip VCC (NOT to ESP32S3)
- ESP32S3 GPIO 48 to LED strip DIN
- ESP32S3 GND to LED strip GND

### Large LED Strip (60+ LEDs)
- Use external 5V power supply (5A or more)
- Consider level shifter (3.3V to 5V) for data line
- Add data line resistor (470Ω)
- Add power capacitor (1000µF)
- Keep data wire as short as possible

## GPIO Pin Options

You can use different GPIO pins if needed. Recommended pins:

| GPIO | Notes |
|------|-------|
| 48   | Default, usually connected to built-in LED |
| 21   | Safe to use, no special function |
| 47   | Safe to use, no special function |
| 33   | Safe to use, no special function |
| 34   | Safe to use, no special function |

**Avoid using these pins:**
- GPIO 0: Boot mode selection
- GPIO 3: JTAG
- GPIO 43: UART TX
- GPIO 44: UART RX
- GPIO 19-20: USB

## Power Considerations

### USB Power
- USB-C provides up to 500mA (USB 2.0) or 900mA (USB 3.0)
- Sufficient for:
  - Built-in LED (single)
  - Small LED strip (1-10 LEDs at low brightness)

### External Power
Required for larger installations:
- **LED Current**: ~60mA per LED at full white brightness
- **Example**: 30 LEDs × 60mA = 1.8A (need 5V 2A+ supply)

### Power Supply Recommendations
- **1-10 LEDs**: USB power OK
- **10-30 LEDs**: 5V 2A power supply
- **30-60 LEDs**: 5V 3A power supply
- **60-100 LEDs**: 5V 5A power supply
- **100+ LEDs**: 5V 10A+ power supply

## Safety Notes

⚠️ **Important Safety Guidelines:**

1. **Polarity**: Double-check VCC, GND, and DIN connections
2. **Voltage**: WS2812B LEDs need 5V (3.3V from ESP32 usually works for short strips)
3. **Current**: Don't exceed your power supply's rating
4. **Heat**: Large LED installations can get hot - ensure ventilation
5. **Insulation**: Keep exposed connections away from conductive surfaces
6. **Fuses**: Consider adding a fuse to your power supply

## Testing Your Setup

### Step 1: Visual Inspection
- Check all connections are secure
- Verify no shorts between pins
- Ensure correct polarity

### Step 2: Power Test
- Connect only power (no data line)
- LEDs should light up dimly (random colors)
- If not, check power connections

### Step 3: Upload Basic Test
Upload a simple sketch to test the LED:
```cpp
#include <Adafruit_NeoPixel.h>

#define LED_PIN 48
#define NUM_LEDS 1

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(50);
  strip.show();
}

void loop() {
  strip.setPixelColor(0, strip.Color(255, 0, 0));  // Red
  strip.show();
  delay(1000);
  
  strip.setPixelColor(0, strip.Color(0, 255, 0));  // Green
  strip.show();
  delay(1000);
  
  strip.setPixelColor(0, strip.Color(0, 0, 255));  // Blue
  strip.show();
  delay(1000);
}
```

If LED cycles through Red, Green, Blue - your hardware is working correctly!

## Troubleshooting Hardware Issues

### LED Doesn't Light Up
- ✓ Check power supply
- ✓ Verify GPIO pin number
- ✓ Test with known-good LED
- ✓ Check wiring polarity
- ✓ Try reducing NUM_LEDS to 1

### LED Shows Wrong Colors
- ✓ Try changing NEO_GRB to NEO_RGB in code
- ✓ Check for poor connections
- ✓ Verify data line isn't too long

### LED Flickers or Glitches
- ✓ Add 470Ω resistor to data line
- ✓ Add capacitor to power supply
- ✓ Reduce brightness
- ✓ Use shorter data wire
- ✓ Check power supply capacity

### Random Colors at Boot
- This is normal for WS2812B LEDs
- They show random colors until initialized
- Will be fixed once sketch runs

## Recommended Components

### For Beginners
- ESP32S3 SuperMini board (with built-in LED)
- USB-C cable
- Nothing else needed!

### For Advanced Projects
- WS2812B LED strip (5V, 60 LEDs/m)
- 5V power supply (appropriate amperage)
- 470Ω resistor
- 1000µF capacitor
- Breadboard and jumper wires
- Heat shrink tubing
- Enclosure

## Where to Buy

### ESP32S3 SuperMini Boards
- AliExpress
- Amazon
- Local electronics stores

### LED Strips
- Look for "WS2812B" or "NeoPixel"
- Available in various densities (30, 60, 144 LEDs/m)
- Individual LEDs also available

---

**Ready to build?** Head back to the [main documentation](ESP32S3_RGB_Control/README.md) for software setup!
