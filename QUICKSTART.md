# Quick Start Guide

Get your ESP32S3 SuperMini RGB LED working in 5 minutes! ⚡

## What You Need

- ✅ ESP32S3 SuperMini board (with built-in RGB LED)
- ✅ USB-C cable
- ✅ Computer with Arduino IDE installed

## Step-by-Step Setup

### 1️⃣ Install Arduino IDE

If you don't have it yet:
1. Download from: https://www.arduino.cc/en/software
2. Install and open Arduino IDE

### 2️⃣ Add ESP32 Board Support

1. Click `File` → `Preferences`
2. In "Additional Board Manager URLs", paste:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Click OK
4. Go to `Tools` → `Board` → `Boards Manager`
5. Search "ESP32"
6. Install "esp32" by Espressif Systems

### 3️⃣ Install Library

1. Click `Sketch` → `Include Library` → `Manage Libraries`
2. Search "Adafruit NeoPixel"
3. Click Install

### 4️⃣ Configure Board

1. Go to `Tools` → `Board` → `ESP32 Arduino` → `ESP32S3 Dev Module`
2. Set these options:
   - USB CDC On Boot: **Enabled**
   - Upload Speed: **921600**
3. Connect your board via USB-C
4. Select the correct Port in `Tools` → `Port`

### 5️⃣ Upload the Code

1. Download or clone this repository
2. Open `ESP32S3_RGB_Control/ESP32S3_RGB_Control.ino`
3. Click the Upload button (→)
4. Wait for "Done uploading" message

### 6️⃣ Connect to WiFi

1. On your phone or computer, open WiFi settings
2. Connect to:
   - **Network Name**: `ESP32S3-RGB`
   - **Password**: `12345678`

### 7️⃣ Open the Control Panel

1. Open a web browser
2. Go to: **http://192.168.4.1**
3. 🎉 Start controlling your RGB LED!

## Quick Test

Want to test your hardware first? Try this simple sketch:

```cpp
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel led(1, 48, NEO_GRB + NEO_KHZ800);

void setup() {
  led.begin();
  led.setBrightness(50);
  led.show();
}

void loop() {
  // Red
  led.setPixelColor(0, led.Color(255, 0, 0));
  led.show();
  delay(1000);
  
  // Green
  led.setPixelColor(0, led.Color(0, 255, 0));
  led.show();
  delay(1000);
  
  // Blue
  led.setPixelColor(0, led.Color(0, 0, 255));
  led.show();
  delay(1000);
}
```

Your LED should cycle through Red → Green → Blue every second.

## Troubleshooting

### ❌ Upload Failed
- **Solution**: Hold the BOOT button while connecting USB, then upload

### ❌ Can't Connect to WiFi
- **Solution**: Check the network name is "ESP32S3-RGB" and password is "12345678"

### ❌ LED Not Working
- **Solution**: Most boards use GPIO 48. Check your board's documentation if this doesn't work.

### ❌ Compilation Error
- **Solution**: Make sure you installed the Adafruit NeoPixel library and selected ESP32S3 Dev Module as the board

## What's Next?

### Customize It
- Change WiFi name and password in the code
- Adjust default brightness
- Modify available effects

### Add More LEDs
- Connect WS2812B LED strips
- See [HARDWARE.md](HARDWARE.md) for wiring

### Advanced Features
- Try the [MQTT Control example](examples/MQTT_Control/)
- Set up [Home Assistant integration](examples/HomeAssistant_Integration/)
- Create custom effects

## Web Interface Features

Once connected, you can:

🎨 **Pick Any Color**
- Click the color picker
- Choose your desired color
- Click "Apply Color"

✨ **Select Effects**
- Solid Color
- Rainbow Cycle
- Fade
- Strobe
- Pulse

💡 **Adjust Brightness**
- Drag the slider
- Changes apply instantly

🔘 **Toggle On/Off**
- Turn LED on/off
- Keeps your settings

## Need More Help?

📖 **Detailed Documentation**: [README.md](README.md)  
🔧 **Hardware Guide**: [HARDWARE.md](HARDWARE.md)  
❓ **FAQ**: [FAQ.md](FAQ.md)  
🤝 **Contributing**: [CONTRIBUTING.md](CONTRIBUTING.md)

## Support This Project

⭐ **Star this repo** if you find it useful!  
🐛 **Report bugs** by opening an issue  
💡 **Suggest features** we should add  
🔧 **Contribute** code improvements

---

**Enjoy your ESP32S3 RGB Light!** 🌈✨

Made with ❤️ by the maker community
