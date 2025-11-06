# Schematics and Wiring Diagrams

This document provides detailed wiring diagrams for various configurations of the ESP32S3 SuperMini RGB control system.

## Built-in LED Configuration

Most ESP32S3 SuperMini boards have a built-in WS2812B RGB LED. No external wiring needed!

```
┌─────────────────────────────────┐
│   ESP32S3 SuperMini Board       │
│                                 │
│  ┌─────────┐                   │
│  │ ESP32S3 │        ┌───┐      │
│  │   MCU   │────────│RGB│      │
│  │         │ GPIO48 │LED│      │
│  └─────────┘        └───┘      │
│                                 │
│      [USB-C Port]               │
└─────────────────────────────────┘
```

**Configuration**:
```cpp
#define LED_PIN     48
#define NUM_LEDS    1
```

---

## Single External LED Wiring

Connect one external WS2812B LED to ESP32S3.

```
ESP32S3 SuperMini                WS2812B LED
┌──────────────┐                ┌──────────┐
│              │                │          │
│   GPIO 48 ───┼────────────────┼─ DIN    │
│              │   Data         │          │
│   GND    ────┼────────────────┼─ GND    │
│              │                │          │
│   3V3    ────┼────────────────┼─ VCC    │
│              │                │          │
└──────────────┘                └──────────┘
```

**Optional Components**:
```
ESP32S3                470Ω          WS2812B
GPIO 48 ────────────[Resistor]────────── DIN
```

**Configuration**:
```cpp
#define LED_PIN     48
#define NUM_LEDS    1
```

---

## LED Strip (Small - up to 10 LEDs)

For small LED strips powered by ESP32S3.

```
ESP32S3 SuperMini                WS2812B Strip
┌──────────────┐                ┌──────────────┐
│              │                │ LED LED LED  │
│   GPIO 48 ───┼────────────────┼─ DIN        │
│              │                │              │
│   GND    ────┼────────────────┼─ GND        │
│              │                │              │
│   5V     ────┼────────────────┼─ VCC        │
│              │                │              │
└──────────────┘                └──────────────┘
```

**With Protection** (Recommended):
```
ESP32S3        470Ω       WS2812B Strip
GPIO 48 ────[Resistor]──────── DIN


         ┌─────┐
5V   ────┤     ├───── VCC (strip)
         │1000µF
GND  ────┤     ├───── GND (strip)
         └─────┘
      Capacitor
```

**Configuration**:
```cpp
#define LED_PIN     48
#define NUM_LEDS    10  // Adjust to your LED count
```

---

## LED Strip (Large - 10+ LEDs)

For larger installations requiring external power.

```
                    External 5V Power Supply
                         ┌─────────┐
                         │   5V    │
                         │  Power  │
                         │ Supply  │
                         └─────────┘
                          +5V  GND
                           │    │
                           │    │
ESP32S3 SuperMini          │    │      WS2812B Strip
┌──────────────┐          │    │     ┌──────────────┐
│              │          │    │     │ LED LED LED  │
│   GPIO 48 ───┼──────────│────┼─────┼─ DIN        │
│              │          │    │     │              │
│   GND    ────┼──────────┴────┴─────┼─ GND        │
│              │          │          │              │
│   (5V NC)    │          └──────────┼─ VCC        │
│              │                     │              │
└──────────────┘                     └──────────────┘
                                      (60 LEDs max)
```

**Important Notes**:
- DO NOT connect external 5V to ESP32S3's 5V pin
- Always connect GND between all components
- Power supply must be adequate (60mA per LED at full brightness)

**With Full Protection** (Best Practice):
```
External Power                    ESP32S3           WS2812B Strip
┌──────────┐                    ┌─────────┐        ┌───────────┐
│   5V     │                    │         │ 470Ω   │           │
│  Power   │────┬───────────────│         ├────────┤ DIN       │
│  Supply  │    │               │ GPIO 48 │        │           │
└──────────┘    │               │         │        │           │
   GND          │               │   GND   ├────────┤ GND       │
    │           │               └─────────┘        │           │
    │      ┌────┴────┐                             │ VCC       │
    │      │ 1000µF  │             ┌───────────────┤           │
    │      │   Cap   │             │               └───────────┘
    └──────┴─────────┴─────────────┘
           GND rail (common ground)
```

**Power Supply Sizing**:
```
Number of LEDs × 60mA = Required Current

Examples:
- 30 LEDs:  30 × 60mA = 1.8A → Use 5V 2A supply
- 60 LEDs:  60 × 60mA = 3.6A → Use 5V 4A supply
- 100 LEDs: 100 × 60mA = 6A → Use 5V 10A supply
```

**Configuration**:
```cpp
#define LED_PIN     48
#define NUM_LEDS    60  // Adjust to your LED count
```

---

## Multiple LED Strips (Parallel)

Control multiple LED strips independently.

```
ESP32S3 SuperMini
┌──────────────┐                  Strip 1 (GPIO 48)
│              │                  ┌──────────────┐
│   GPIO 48 ───┼──────────────────┼─ DIN        │
│              │                  └──────────────┘
│              │
│   GPIO 47 ───┼──────────────────┐ Strip 2 (GPIO 47)
│              │                  └──────────────┐
│              │                  │ DIN          │
│   GPIO 21 ───┼──────────┐       └──────────────┘
│              │          │
│   GND    ────┼──────────┴────────── Common GND
│              │
└──────────────┘
              │
              │ Strip 3 (GPIO 21)
              └──────────────────┐
                                 │ DIN
                                 └──────────────┘

Power Supply ───── All VCC pins
```

**Code Configuration**:
```cpp
#define LED_PIN_1     48
#define LED_PIN_2     47
#define LED_PIN_3     21
#define NUM_LEDS      10

Adafruit_NeoPixel strip1(NUM_LEDS, LED_PIN_1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2(NUM_LEDS, LED_PIN_2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip3(NUM_LEDS, LED_PIN_3, NEO_GRB + NEO_KHZ800);
```

---

## Level Shifter Configuration

For long cable runs or many LEDs, use a level shifter.

```
ESP32S3          Level Shifter        WS2812B Strip
(3.3V)              (3.3V→5V)              (5V)
┌─────────┐        ┌──────────┐        ┌───────────┐
│         │        │   LV  HV │        │           │
│GPIO 48  ├────────┤ A1    B1 ├────────┤ DIN       │
│         │        │          │        │           │
│  GND    ├────────┤ GND  GND ├────────┤ GND       │
│         │        │          │        │           │
│  3V3    ├────────┤ LV       │        │           │
│         │        │      HV  ├───┐    │ VCC       │
└─────────┘        └──────────┘   │    └───────────┘
                                  │
                          5V Power Supply
```

**Recommended Level Shifters**:
- 74HCT245 (Octal Bus Transceiver)
- SN74AHCT125 (Quad Level Shifter)
- TXB0108 (Bi-directional)

---

## Pin Selection Guide

### Recommended Pins (Safe to Use)
```
┌──────────────────────────┐
│ ESP32S3 SuperMini        │
│                          │
│  GPIO 48 ────────── LED  │ ← Default, built-in LED
│  GPIO 47 ────────── OK   │ ← Alternative
│  GPIO 21 ────────── OK   │
│  GPIO 33 ────────── OK   │
│  GPIO 34 ────────── OK   │
│  GPIO 35 ────────── OK   │
│  GPIO 36 ────────── OK   │
│  GPIO 37 ────────── OK   │
└──────────────────────────┘
```

### Avoid These Pins
```
GPIO  0  - Boot mode selection
GPIO  3  - JTAG
GPIO 19  - USB D-
GPIO 20  - USB D+
GPIO 43  - UART TX
GPIO 44  - UART RX
GPIO 45  - VDD_SPI
GPIO 46  - BOOT button
```

---

## Component Specifications

### WS2812B RGB LED
```
Specifications:
- Voltage:        5V (works with 3.3V at low count)
- Current:        ~60mA at full white
- Protocol:       Single wire (800kHz)
- Color depth:    24-bit (8-bit per channel)
- Refresh rate:   400Hz+
- Daisy-chain:    Yes
```

### Power Requirements
```
Component          Current Draw
─────────────────────────────
ESP32S3 idle       ~80mA
ESP32S3 WiFi TX    ~200mA
1 LED (full)       ~60mA
1 LED (typical)    ~20mA
────────────────────────────
USB 2.0 max        500mA
USB 3.0 max        900mA
```

### Recommended Components

**Resistor** (Data line protection):
- Value: 470Ω (330-1000Ω acceptable)
- Power: 1/4W
- Purpose: Reduces signal ringing, protects GPIO

**Capacitor** (Power smoothing):
- Value: 1000µF (100-4700µF acceptable)
- Voltage: 10V or higher
- Purpose: Prevents voltage drops, smooths power

**Power Supply**:
- Voltage: 5V regulated
- Current: (Number of LEDs × 60mA) + 200mA
- Ripple: < 100mV
- Connector: Match your LED strip

---

## Safety Checklist

Before powering on:

- [ ] Double-check polarity (VCC, GND, Data)
- [ ] Verify no shorts between connections
- [ ] Confirm power supply voltage (5V)
- [ ] Check power supply current rating
- [ ] Ensure common ground between all components
- [ ] Add fuse to power supply if using high current
- [ ] Keep wires organized and insulated
- [ ] Ventilation for enclosed projects

---

## Troubleshooting Hardware

### LED doesn't work
1. Check power (measure 5V at LED VCC)
2. Verify GPIO pin number in code
3. Test with fewer LEDs (try NUM_LEDS = 1)
4. Swap data line to different pin

### Wrong colors
1. Try NEO_RGB instead of NEO_GRB
2. Check for poor connections
3. Measure voltage (should be 5V)

### Flickering
1. Add 470Ω resistor to data line
2. Add 1000µF capacitor to power
3. Shorten data wire
4. Use level shifter for long runs
5. Check power supply quality

### Random behavior
1. Reduce brightness in software
2. Improve grounding
3. Add capacitor
4. Check for interference
5. Use shielded cable for long runs

---

For more help, see:
- [HARDWARE.md](HARDWARE.md) - Detailed hardware guide
- [FAQ.md](FAQ.md) - Common questions
- [QUICKSTART.md](QUICKSTART.md) - Getting started

**Ready to build?** Start with the [Quick Start Guide](QUICKSTART.md)!
