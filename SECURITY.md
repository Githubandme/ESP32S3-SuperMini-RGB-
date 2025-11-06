# Security Guidelines

## Important Security Notes

This project is designed for educational and hobby use. Please follow these security guidelines when deploying your ESP32S3 RGB control system.

---

## 🔒 WiFi Security

### Default Credentials

The default WiFi credentials in the code are:
```cpp
const char* ssid = "ESP32S3-RGB";
const char* password = "12345678";
```

⚠️ **CRITICAL**: These are weak default credentials for demonstration purposes only.

### Recommendations

**For Home Use:**
1. Change the default password to a strong password (12+ characters)
2. Use a mix of uppercase, lowercase, numbers, and symbols
3. Example: `const char* password = "MyStr0ng!P@ssw0rd2025";`

**For Production/Commercial Use:**
1. Implement device-specific passwords (e.g., based on MAC address)
2. Consider WPA2-Enterprise for business environments
3. Implement password change functionality via web interface
4. Store passwords in encrypted form

**Example of Device-Specific Password:**
```cpp
void generateUniquePassword(char* password, size_t len) {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(password, len, "RGB-%02X%02X%02X", mac[3], mac[4], mac[5]);
}
```

---

## 🌐 Web Interface Security

### Current Security Level

The web interface is **NOT password-protected** by default. Anyone who can connect to the WiFi can control the LEDs.

### Acceptable Use Cases

This is acceptable for:
- Home networks (where you control WiFi access)
- Isolated networks
- Educational purposes
- Development/testing

### Adding Authentication

If you need authentication, add HTTP Basic Auth:

```cpp
// Add at top of sketch
const char* www_username = "admin";
const char* www_password = "your_secure_password";

// In handleRoot() and other handlers
if (!server.authenticate(www_username, www_password)) {
  return server.requestAuthentication();
}
```

---

## 🔐 MQTT Security

### Default Configuration

The MQTT examples use plaintext credentials:
```cpp
const char* mqtt_user = "YOUR_MQTT_USER";
const char* mqtt_password = "YOUR_MQTT_PASS";
```

### Security Best Practices

1. **Use Strong Passwords**
   - Minimum 16 characters
   - Unique per device
   - Randomly generated

2. **Use TLS/SSL**
   ```cpp
   WiFiClientSecure espClient;  // Instead of WiFiClient
   espClient.setCACert(ca_cert);
   ```

3. **Use Client Certificates**
   - More secure than passwords
   - Supported by most MQTT brokers

4. **Limit Topic Permissions**
   - Configure broker ACLs
   - Each device should only access its own topics

5. **Network Isolation**
   - Keep MQTT on private network
   - Use VPN for remote access
   - Avoid exposing to internet

---

## 📡 Network Security

### Access Point Mode

When using AP mode (default):
- ✅ Device creates its own network
- ✅ No exposure to internet
- ✅ Limited range (~30m)
- ⚠️ Anyone in range can see the network
- ⚠️ Weak password allows unauthorized access

**Recommendations:**
- Use strong password
- Change default SSID
- Disable when not needed

### Station Mode

When connecting to existing WiFi:
- ✅ Protected by your router's security
- ⚠️ Exposed to all devices on same network
- ⚠️ Can be accessed from anywhere on network

**Recommendations:**
- Use guest network if available
- Implement web authentication
- Use firewall rules if possible
- Monitor connected devices

---

## 💾 Credentials Storage

### Current Implementation

Credentials are stored in source code as plaintext.

⚠️ **WARNING**: Never commit real credentials to version control!

### Better Approaches

1. **Separate Configuration File**
   ```cpp
   // credentials.h (add to .gitignore)
   #define WIFI_SSID "your_ssid"
   #define WIFI_PASS "your_password"
   ```

2. **SPIFFS Configuration**
   ```cpp
   File config = SPIFFS.open("/config.json", "r");
   // Parse JSON config
   ```

3. **Web-Based Setup**
   - First boot: Create AP with setup page
   - User enters credentials via form
   - Store in EEPROM/SPIFFS
   - Reboot and connect

4. **Environment Variables** (PlatformIO)
   ```ini
   build_flags = 
     -DWIFI_SSID=\"${sysenv.WIFI_SSID}\"
     -DWIFI_PASS=\"${sysenv.WIFI_PASS}\"
   ```

---

## 🛡️ General Security Practices

### For Developers

1. **Never commit secrets**
   - Use .gitignore
   - Review commits before pushing
   - Use git-secrets or similar tools

2. **Validate all inputs**
   - Sanitize web form data
   - Check parameter ranges
   - Prevent buffer overflows

3. **Keep firmware updated**
   - Monitor for security updates
   - Update ESP32 Arduino Core
   - Update libraries regularly

4. **Use HTTPS when possible**
   - For web interface
   - For firmware updates
   - For API calls

### For Users

1. **Change default passwords immediately**
2. **Keep device firmware updated**
3. **Use isolated/guest networks when possible**
4. **Monitor for unusual activity**
5. **Disable when not in use (if critical)**

---

## 🚨 Vulnerability Reporting

If you discover a security vulnerability:

1. **Do NOT open a public issue**
2. Email the maintainers privately
3. Include:
   - Description of vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fix (if any)

We will:
- Acknowledge within 48 hours
- Provide fix timeline
- Credit you in security advisory (if desired)

---

## 📋 Security Checklist

Before deploying, check:

- [ ] Default passwords changed
- [ ] WiFi password is strong (12+ chars)
- [ ] MQTT credentials are strong
- [ ] TLS/SSL enabled for MQTT (if over internet)
- [ ] Web authentication added (if needed)
- [ ] Credentials not in version control
- [ ] Using latest firmware version
- [ ] Device on isolated network (if possible)
- [ ] Unnecessary features disabled
- [ ] Firewall rules configured (if applicable)

---

## 🎯 Security by Use Case

### Home Network (Low Risk)
- ✅ Default configuration OK with strong password
- ✅ Can use HTTP without authentication
- ✅ WiFi AP mode or station mode both fine
- ⚠️ Still change default password!

### Guest Network (Medium Risk)
- ⚠️ Add web authentication
- ⚠️ Use strong passwords
- ⚠️ Limit network access if possible
- ✅ Consider this for parties/events

### Public Space (High Risk)
- ❌ Not recommended without major modifications
- ⚠️ Must add authentication
- ⚠️ Must use HTTPS
- ⚠️ Must use strong encryption
- ⚠️ Consider VPN access only

### Commercial (Critical)
- ❌ Needs professional security audit
- ⚠️ Implement enterprise WiFi
- ⚠️ Use certificate-based authentication
- ⚠️ Implement access logging
- ⚠️ Use device management system

---

## 🔗 Additional Resources

### Securing ESP32
- [ESP32 Security Features](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/index.html)
- [ESP32 Secure Boot](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/secure-boot-v2.html)
- [ESP32 Flash Encryption](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/flash-encryption.html)

### MQTT Security
- [MQTT Security Fundamentals](https://www.hivemq.com/blog/mqtt-security-fundamentals/)
- [Securing MQTT Brokers](https://mosquitto.org/documentation/authentication-methods/)

### Web Security
- [OWASP IoT Security Guide](https://owasp.org/www-project-internet-of-things/)

---

## ⚖️ Legal Disclaimer

This software is provided "as is" without warranty of any kind. The developers are not responsible for any security breaches, data loss, or damages resulting from the use of this software.

**Users are solely responsible for:**
- Securing their devices
- Protecting their credentials
- Complying with local regulations
- Implementing appropriate security measures

---

## 📞 Questions?

Security questions? See:
- [FAQ.md](FAQ.md) for common questions
- [CONTRIBUTING.md](CONTRIBUTING.md) for reporting issues
- GitHub Issues for non-sensitive questions

**Remember: Security is a journey, not a destination. Stay vigilant!** 🔒
