# ESP32S3 SuperMini RGB Light Control System

A comprehensive RGB LED control system for the ESP32S3 SuperMini board with WiFi web interface.

## Features

- 🌈 **Multiple Light Effects**
  - Solid Color
  - Rainbow Cycle
  - Fade Effect
  - Strobe Effect
  - Pulse Effect

- 🎨 **Color Control**
  - Full RGB color picker
  - 16.7 million color combinations

- 💡 **Brightness Control**
  - Adjustable brightness (0-255)
  - Real-time brightness adjustment

- 📱 **Web Interface**
  - Responsive design for mobile and desktop
  - Real-time control
  - Modern, intuitive UI

- 🔌 **WiFi Access Point**
  - No router required
  - Direct connection to ESP32S3
  - Easy setup

## Hardware Requirements

- **ESP32S3 SuperMini** board
- **WS2812B RGB LED** (typically built-in on GPIO 48)
- **USB-C cable** for programming and power

## Software Requirements

- **Arduino IDE** (1.8.19 or later) or **PlatformIO**
- **ESP32 Board Support** (ESP32 Arduino Core)
- **Adafruit NeoPixel Library**

## Installation

### 1. Install Arduino IDE

Download and install from: https://www.arduino.cc/en/software

### 2. Install ESP32 Board Support

1. Open Arduino IDE
2. Go to `File` → `Preferences`
3. Add this URL to "Additional Board Manager URLs":
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
4. Go to `Tools` → `Board` → `Boards Manager`
5. Search for "ESP32" and install "esp32" by Espressif Systems

### 3. Install Required Libraries

1. Go to `Sketch` → `Include Library` → `Manage Libraries`
2. Search and install:
   - **Adafruit NeoPixel** (by Adafruit)

### 4. Configure Board Settings

1. Select `Tools` → `Board` → `ESP32S3 Dev Module`
2. Configure settings:
   - **USB CDC On Boot**: Enabled
   - **CPU Frequency**: 240MHz (WiFi)
   - **Flash Mode**: QIO 80MHz
   - **Flash Size**: 4MB (32Mb)
   - **Partition Scheme**: Default 4MB with spiffs
   - **PSRAM**: QSPI PSRAM
   - **Upload Mode**: UART0 / Hardware CDC
   - **Upload Speed**: 921600

## Usage

### 1. Upload the Sketch

1. Open `ESP32S3_RGB_Control.ino` in Arduino IDE
2. Connect your ESP32S3 SuperMini via USB-C
3. Select the correct COM port in `Tools` → `Port`
4. Click the Upload button (→)

### 2. Connect to the WiFi Access Point

1. After uploading, open Serial Monitor (115200 baud)
2. Note the IP address (typically 192.168.4.1)
3. On your phone or computer, connect to WiFi:
   - **SSID**: `ESP32S3-RGB`
   - **Password**: `12345678`

### 3. Access the Web Interface

1. Open a web browser
2. Navigate to: `http://192.168.4.1`
3. Control your RGB LED!

## Web Interface Controls

### Color Picker
- Click on the color picker to select any color
- Click "Apply Color" to set the LED color

### Effects Dropdown
- **Solid Color**: Display selected color
- **Rainbow Cycle**: Continuous rainbow animation
- **Fade**: Smooth fade in/out effect
- **Strobe**: Fast blinking effect
- **Pulse**: Breathing effect

### Brightness Slider
- Adjust from 0% to 100%
- Changes apply in real-time

### Toggle Button
- Turn LED on/off while preserving settings

## Customization

### Change WiFi Credentials

Edit these lines in the sketch:
```cpp
const char* ssid = "ESP32S3-RGB";      // Your WiFi SSID
const char* password = "12345678";      // Your WiFi Password
```

### Change LED Pin

Edit this line if using a different GPIO:
```cpp
#define LED_PIN     48    // Change to your LED pin
```

### Change Number of LEDs

If using an LED strip with multiple LEDs:
```cpp
#define NUM_LEDS    1     // Change to your LED count
```

### Adjust Initial Brightness

```cpp
#define BRIGHTNESS  50    // 0-255
```

## API Endpoints

You can also control the LED programmatically:

- **Set Color**: `/setColor?r=255&g=0&b=0`
- **Set Effect**: `/setEffect?effect=rainbow`
- **Set Brightness**: `/setBrightness?value=128`
- **Toggle On/Off**: `/toggle`
- **Get Status**: `/status` (returns JSON)

Example using curl:
```bash
curl "http://192.168.4.1/setColor?r=0&g=255&b=0"
curl "http://192.168.4.1/setEffect?effect=rainbow"
curl "http://192.168.4.1/setBrightness?value=200"
```

## Troubleshooting

### LED Not Working

1. Check GPIO pin number (GPIO 48 for most SuperMini boards)
2. Verify LED is WS2812B compatible
3. Check power supply (some LEDs need external power)

### Cannot Connect to WiFi

1. Check WiFi credentials
2. Make sure your device supports 2.4GHz WiFi
3. Move closer to the ESP32S3

### Upload Failed

1. Hold BOOT button while connecting USB
2. Try different USB cable
3. Reduce upload speed to 115200
4. Check driver installation

### Serial Monitor Shows Nothing

1. Enable "USB CDC On Boot" in board settings
2. Set baud rate to 115200
3. Press the RST button on the board

## Technical Details

### Pin Configuration
- **LED Data Pin**: GPIO 48 (default for ESP32S3 SuperMini)
- **LED Type**: WS2812B (NeoPixel compatible)
- **LED Protocol**: 800KHz

### Memory Usage
- **Flash**: ~300KB
- **RAM**: ~40KB

### Performance
- **WiFi**: 2.4GHz 802.11 b/g/n
- **CPU**: Dual-core 240MHz
- **Update Rate**: 20-50ms (depending on effect)

## License

This project is open source and available under the MIT License.

## Contributing

Feel free to submit issues and pull requests!

## Credits

- Built with Arduino framework
- Uses Adafruit NeoPixel library
- Designed for ESP32S3 SuperMini

## Version History

- **1.0.0** (2025) - Initial release
  - WiFi AP mode
  - 5 lighting effects
  - Web interface
  - Brightness control
  - Color picker

## Support

For issues and questions, please open an issue on GitHub.

---

Enjoy your ESP32S3 RGB Light Control System! 🌈✨
