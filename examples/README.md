# Examples

This directory contains example sketches demonstrating different use cases and integrations for the ESP32S3 SuperMini RGB control system.

## Available Examples

### 1. Basic_Test
📁 `Basic_Test/Basic_Test.ino`

**Purpose**: Simple hardware test sketch  
**Complexity**: ⭐ Beginner  
**Requirements**: Just the ESP32S3 SuperMini board

**What it does**:
- Cycles through basic colors (Red, Green, Blue, Yellow, Cyan, Magenta, White)
- Tests if your RGB LED is working correctly
- Provides serial output for debugging

**Use this when**:
- First time setting up your board
- Troubleshooting hardware issues
- Verifying LED functionality
- Learning the basics

**How to use**:
1. Open `Basic_Test.ino` in Arduino IDE
2. Upload to your ESP32S3
3. Open Serial Monitor (115200 baud)
4. Watch the LED cycle through colors

---

### 2. MQTT_Control
📁 `MQTT_Control/MQTT_Control.ino`

**Purpose**: Control RGB LED via MQTT protocol  
**Complexity**: ⭐⭐ Intermediate  
**Requirements**: 
- ESP32S3 SuperMini board
- WiFi network
- MQTT broker (Mosquitto, HiveMQ, etc.)
- PubSubClient library

**What it does**:
- Connects to your WiFi network
- Communicates with an MQTT broker
- Accepts color, brightness, and effect commands via MQTT
- Publishes status updates
- Works with any MQTT-compatible system

**Use this when**:
- Building IoT projects
- Integrating with Node-RED
- Creating automation systems
- Need remote control without direct web access

**MQTT Topics**:
```
Subscribe (commands):
- esp32s3/rgb/set           - Set color (format: "r,g,b")
- esp32s3/rgb/brightness    - Set brightness (0-255)
- esp32s3/rgb/effect        - Set effect name
- esp32s3/rgb/power         - Turn on/off ("ON"/"OFF")

Publish (status):
- esp32s3/rgb/state         - Current state (JSON)
```

**How to use**:
1. Install PubSubClient library
2. Edit WiFi credentials in the sketch
3. Edit MQTT broker settings
4. Upload to ESP32S3
5. Use any MQTT client to control

**Example MQTT commands**:
```bash
# Using mosquitto_pub
mosquitto_pub -t "esp32s3/rgb/set" -m "255,0,0"        # Red
mosquitto_pub -t "esp32s3/rgb/brightness" -m "128"     # 50% brightness
mosquitto_pub -t "esp32s3/rgb/effect" -m "rainbow"     # Rainbow effect
mosquitto_pub -t "esp32s3/rgb/power" -m "OFF"          # Turn off
```

---

### 3. HomeAssistant_Integration
📁 `HomeAssistant_Integration/HomeAssistant_Integration.ino`

**Purpose**: Full Home Assistant integration with auto-discovery  
**Complexity**: ⭐⭐⭐ Advanced  
**Requirements**:
- ESP32S3 SuperMini board
- Home Assistant installation
- MQTT broker (configured in Home Assistant)
- PubSubClient library
- ArduinoJson library

**What it does**:
- Connects to your WiFi and MQTT broker
- Automatically discovers itself in Home Assistant
- Appears as a native Light entity
- Supports color control, brightness, and effects
- Updates state in real-time
- No manual configuration needed in Home Assistant!

**Use this when**:
- You have Home Assistant set up
- Want seamless smart home integration
- Need Google Home/Alexa control (via Home Assistant)
- Building a complete smart lighting system

**Features**:
- ✅ Auto-discovery (no YAML configuration needed)
- ✅ Color picker in Home Assistant UI
- ✅ Brightness slider
- ✅ Effect selection dropdown
- ✅ On/Off control
- ✅ Real-time state updates
- ✅ Persistent device information

**How to use**:
1. Install PubSubClient and ArduinoJson libraries
2. Configure WiFi credentials
3. Configure MQTT broker (same as Home Assistant)
4. Upload to ESP32S3
5. Open Home Assistant
6. Device appears automatically under Configuration → Devices
7. Add to dashboard and control!

**Home Assistant Configuration**:
```yaml
# No configuration needed! Auto-discovery does it all.
# But ensure MQTT is configured in configuration.yaml:
mqtt:
  broker: YOUR_BROKER_IP
  username: YOUR_USERNAME
  password: YOUR_PASSWORD
```

**Troubleshooting**:
- If device doesn't appear, check MQTT broker connection
- Verify Home Assistant MQTT integration is working
- Check Serial Monitor for connection status
- Look for discovery message on topic: `homeassistant/light/esp32s3_rgb/config`

---

## Choosing the Right Example

| Need | Use This Example |
|------|-----------------|
| Just testing hardware | Basic_Test |
| Simple LED control | Main project (ESP32S3_RGB_Control) |
| IoT integration | MQTT_Control |
| Home Assistant | HomeAssistant_Integration |
| Custom automation | MQTT_Control |
| Learning/Education | Basic_Test → Main → MQTT → HomeAssistant |

## Library Installation

### For Arduino IDE

**Required for all examples**:
- Adafruit NeoPixel

**For MQTT examples**:
- PubSubClient by Nick O'Leary

**For Home Assistant example**:
- PubSubClient by Nick O'Leary
- ArduinoJson by Benoit Blanchon

**How to install**:
1. Open Arduino IDE
2. Go to `Sketch` → `Include Library` → `Manage Libraries`
3. Search for library name
4. Click Install

### For PlatformIO

Libraries are automatically installed based on `platformio.ini` configuration.

## Modifying Examples

All examples follow similar structure:

```cpp
// 1. Configuration
#define LED_PIN 48
const char* ssid = "...";

// 2. Setup
void setup() {
  // Initialize hardware
  // Connect to network
  // Setup services
}

// 3. Main loop
void loop() {
  // Handle network
  // Update LED
  // Process effects
}

// 4. Helper functions
void setColor(...) { }
void updateEffect() { }
```

Feel free to:
- Change pin numbers
- Modify effects
- Add new features
- Combine examples

## Testing Examples

### Basic_Test
1. Upload sketch
2. Open Serial Monitor
3. Watch LED cycle
4. Should see: Red → Green → Blue → Yellow → Cyan → Magenta → White → Off

### MQTT_Control
1. Start MQTT broker
2. Upload sketch
3. Use MQTT client to send commands
4. Verify LED responds
5. Check state topic for updates

### HomeAssistant_Integration
1. Ensure Home Assistant + MQTT working
2. Upload sketch
3. Wait 30 seconds
4. Check Configuration → Devices in Home Assistant
5. Add device card to dashboard
6. Test controls

## Common Issues

### Compilation Errors
- ✅ Install all required libraries
- ✅ Select correct board (ESP32S3 Dev Module)
- ✅ Update ESP32 board package

### Upload Fails
- ✅ Hold BOOT button during upload
- ✅ Check COM port selection
- ✅ Try lower upload speed (115200)

### WiFi Connection Issues
- ✅ Verify SSID and password
- ✅ Use 2.4GHz network (not 5GHz)
- ✅ Check WiFi signal strength

### MQTT Problems
- ✅ Verify broker IP address
- ✅ Check username/password
- ✅ Ensure broker is running
- ✅ Test with MQTT Explorer tool

### Home Assistant Not Discovering
- ✅ Check MQTT integration in HA
- ✅ Verify broker settings match
- ✅ Look for discovery message
- ✅ Restart Home Assistant

## Contributing New Examples

Have a cool use case? Contribute an example!

See [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.

Ideas for new examples:
- Bluetooth control
- Web API integration
- Voice control
- Music reactive lighting
- Temperature-based colors
- Motion sensor triggers
- Multi-strip synchronization

---

**Need help?** Check the [FAQ](../FAQ.md) or open an issue!
