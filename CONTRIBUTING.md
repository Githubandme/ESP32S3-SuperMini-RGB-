# Contributing to ESP32S3 SuperMini RGB Control System

Thank you for your interest in contributing! This document provides guidelines and instructions for contributing to this project.

## How to Contribute

### Reporting Bugs

If you find a bug, please open an issue with:
- Clear description of the problem
- Steps to reproduce
- Expected vs actual behavior
- Hardware setup details
- Code version/commit hash

### Suggesting Features

Feature requests are welcome! Please include:
- Use case description
- Proposed implementation approach
- Any relevant examples or references

### Submitting Code

1. **Fork the repository**
2. **Create a feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make your changes**
   - Follow the existing code style
   - Add comments for complex logic
   - Test your changes thoroughly

4. **Commit your changes**
   ```bash
   git commit -m "Add feature: description"
   ```

5. **Push to your fork**
   ```bash
   git push origin feature/your-feature-name
   ```

6. **Open a Pull Request**
   - Describe what your changes do
   - Reference any related issues
   - Include testing details

## Code Style Guidelines

### Arduino/C++ Code

- Use 2 spaces for indentation
- Use descriptive variable names
- Add comments for non-obvious code
- Keep functions focused and small
- Use const for constants

Example:
```cpp
// Good
const int LED_PIN = 48;
void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
  // Set the LED to specified RGB color
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

// Avoid
int p = 48;
void x(int a, int b, int c) {
  strip.setPixelColor(0, strip.Color(a, b, c));
  strip.show();
}
```

### HTML/CSS

- Use 2 spaces for indentation
- Keep styles organized
- Use semantic HTML
- Make responsive designs

### Documentation

- Use clear, concise language
- Include code examples
- Add diagrams where helpful
- Keep formatting consistent

## Development Setup

### Prerequisites

1. Arduino IDE 1.8.19 or later
2. ESP32 board support installed
3. Required libraries installed
4. ESP32S3 SuperMini hardware

### Testing Your Changes

Before submitting:

1. **Compile Check**
   - Verify code compiles without errors
   - Check for warnings

2. **Hardware Test**
   - Upload to actual hardware
   - Test all affected features
   - Verify existing features still work

3. **Documentation**
   - Update relevant docs
   - Add usage examples if needed

## Types of Contributions

### New Effects

Adding new LED effects is encouraged! Include:
- Effect function implementation
- Web UI integration
- Documentation
- Example usage

### Hardware Support

Support for new hardware configurations:
- Document pin configurations
- Test with actual hardware
- Update compatibility list

### Bug Fixes

- Reference the issue number
- Explain the root cause
- Describe the fix approach
- Add test cases if applicable

### Documentation

- Fix typos or unclear sections
- Add tutorials or guides
- Improve existing examples
- Translate to other languages

### Performance Improvements

- Benchmark before and after
- Explain the optimization
- Ensure no functionality loss

## Project Structure

```
ESP32S3-SuperMini-RGB-/
├── ESP32S3_RGB_Control/      # Main sketch
│   ├── ESP32S3_RGB_Control.ino
│   └── README.md
├── examples/                  # Example sketches
│   ├── Basic_Test/
│   ├── MQTT_Control/
│   └── HomeAssistant_Integration/
├── HARDWARE.md               # Hardware guide
├── CONTRIBUTING.md           # This file
└── README.md                 # Main readme
```

## Commit Message Guidelines

Use clear, descriptive commit messages:

```
Good examples:
- Add rainbow wave effect
- Fix brightness control bug
- Update WiFi connection docs
- Improve web UI responsiveness

Avoid:
- Fixed stuff
- Updates
- WIP
```

## Pull Request Process

1. **Ensure PR is focused**
   - One feature/fix per PR
   - Keep changes minimal

2. **Update documentation**
   - README if needed
   - Code comments
   - Example files

3. **Test thoroughly**
   - On real hardware
   - Different configurations
   - Edge cases

4. **Wait for review**
   - Address feedback promptly
   - Be open to suggestions
   - Discuss if you disagree

## Code Review

Reviewers will check for:
- Code quality and style
- Functionality correctness
- Documentation completeness
- Hardware compatibility
- Performance impact

## Community Guidelines

- Be respectful and constructive
- Help others learn
- Share knowledge
- Give credit where due
- Have fun building!

## Questions?

- Open an issue for questions
- Check existing issues first
- Be specific and clear

## License

By contributing, you agree that your contributions will be licensed under the same MIT License that covers this project.

---

Thank you for contributing to the ESP32S3 SuperMini RGB Control System! 🌈
