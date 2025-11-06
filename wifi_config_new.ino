#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFiUdp.h>

// 函数声明
void setupWebServer();
void startAccessPoint();
void startBleService();
void updateLED();
void checkConfigButton();
void handleBleConnection();
void connectToWiFi();
void setRgbColor(int color);
void handleBroadcast();
void startReconfiguration();
void saveCredentials();
bool loadCredentials();
void enableBroadcast();
void handleBroadcastApi();
void handleBroadcastWeb();
void handleRoot();
void handleSetWifi();
void handleWifiScan();
void handleRgbControl();
void handleRgbApi();
void handleStatus();
void handleIdentify();
void handleApiControl();
void handleApiInfo();
void handleDiscover();
void handleHsvPicker();
void handleNotFound();

// LED引脚定义
const int ledPin = 2;  // 板载LED通常在GPIO2
// 配网按键引脚
const int configButtonPin = 0;  // 通常GPIO0可作为配网按键
// RGB和LED共用引脚
const int rgbLedPin = 48; // ESP32S3 SuperMini板载RGB和LED共用引脚

// 热点配置
const char* ssid_ap = "ESP32_Config";  // 热点名称
const char* password_ap = "12345678";  // 热点密码

// Web服务器端口
WebServer server(80);

// 存储WiFi连接信息
String ssid_sta = "";
String password_sta = "";

// 配网状态
bool wifi_configured = false;
bool wifi_connected = false;

// LED状态控制
unsigned long previousMillis = 0;
const long fastInterval = 200;  // 快闪间隔(ms)
const long slowInterval = 1000; // 慢闪间隔(ms)
int ledState = LOW;
int blinkMode = 0; // 0=快闪, 1=慢闪, 2=常亮

// LED控制状态
bool ledControlEnabled = false;  // 是否启用Web控制LED
bool ledWebState = false;        // Web界面控制的LED状态

// RGB控制状态
bool rgbEnabled = true;          // RGB是否启用
int currentRgbColor = 0;         // 当前RGB颜色 (0=彩虹模式, 1-7为固定颜色)
uint8_t rgbBrightness = 50;      // RGB亮度 (0-100)

// HSV色彩模型参数
float hsvHue = 0.0;              // 色相 (0-360度)
float hsvSaturation = 100.0;     // 饱和度 (0-100%)
float hsvValue = 100.0;          // 明度 (0-100%)
bool useHsvMode = false;         // 是否使用HSV模式

// 设备标识
String deviceName = "ESP32_RGB_Device";  // 设备名称
String deviceID = "";                    // 设备唯一ID

// 配置存储
Preferences preferences;

// 按键相关
unsigned long buttonPressTime = 0;
bool buttonPressed = false;
const long buttonLongPressDuration = 3000; // 长按3秒触发重新配网

// RGB灯控制
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, rgbLedPin, NEO_GRB + NEO_KHZ800);

// BLE配网相关
BLEServer *pServer = NULL;
BLECharacteristic *pWifiCharacteristic = NULL;
BLECharacteristic *pStatusCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t bleTxValue = 0;

// UDP广播相关
WiFiUDP udp;
const unsigned int udpPort = 8888;
unsigned long lastBroadcast = 0;
const unsigned long broadcastInterval = 5000; // 每5秒广播一次
const unsigned long broadcastTimeout = 600000; // 10分钟超时
unsigned long broadcastStartTime = 0;
bool broadcastEnabled = false;

// BLE服务和特征UUID
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define WIFI_CHAR_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define STATUS_CHAR_UUID    "86522b20-183f-4f43-985b-d00c750ed3ff"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("BLE设备已连接");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("BLE设备已断开");
    }
};

class WifiCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue();
      if (value.length() > 0) {
        Serial.print("收到BLE配网数据: ");
        Serial.println(value);
        
        // 解析WiFi配置信息 (格式: "ssid|password")
        int separatorPos = value.indexOf('|');
        if (separatorPos != -1) {
          // 确保不包含空字符串
          String receivedSsid = value.substring(0, separatorPos);
          String receivedPassword = value.substring(separatorPos + 1);
          
          if (receivedSsid.length() > 0) {
            // 存储WiFi配置
            ssid_sta = receivedSsid;
            password_sta = receivedPassword;
            
            Serial.print("SSID: ");
            Serial.println(ssid_sta);
            Serial.print("Password: ");
            Serial.println(password_sta.length() > 0 ? "******" : "(无密码)");
            
            // 保存WiFi配置
            saveCredentials();
            
            // 发送确认消息
            pStatusCharacteristic->setValue("WiFi配置已接收");
            pStatusCharacteristic->notify();
            
            // 设置配网状态
            wifi_configured = true;
            wifi_connected = false;
            
            // 通知主循环进行WiFi连接
            Serial.println("BLE配网完成，准备连接WiFi");
          } else {
            pStatusCharacteristic->setValue("SSID不能为空");
            pStatusCharacteristic->notify();
            Serial.println("错误：SSID不能为空");
          }
        } else {
          pStatusCharacteristic->setValue("配网数据格式错误");
          pStatusCharacteristic->notify();
          Serial.println("错误：配网数据格式错误，缺少分隔符'|'");
        }
      }
    }
};

void setup() {
  // 初始化串口
  Serial.begin(115200);
  
  // 生成设备唯一ID（使用芯片ID）
  uint64_t chipid = ESP.getEfuseMac();
  deviceID = String((uint32_t)(chipid >> 32), HEX) + String((uint32_t)chipid, HEX);
  deviceID.toUpperCase();
  
  // 初始化RGB灯
  strip.begin();
  strip.show(); // 初始化为全灭
  strip.setPixelColor(0, strip.Color(255, 0, 0)); // 设置为红色进行测试
  strip.show(); // 显示红色
  delay(1000);  // 持续1秒
  strip.setPixelColor(0, 0, 0, 0); // 关闭
  strip.show(); // 更新显示
  
  // 初始化LED引脚
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // 初始化按键引脚
  pinMode(configButtonPin, INPUT_PULLUP);
  
  // 初始化Preferences
  preferences.begin("wifi-config", false);
  
  // 尝试从Flash读取WiFi配置
  if (loadCredentials()) {
    Serial.println("从Flash加载WiFi配置:");
    Serial.print("SSID: ");
    Serial.println(ssid_sta);
    wifi_configured = true;
    wifi_connected = false;
    // LED慢闪表示正在连接WiFi
    blinkMode = 1;
    
    // 启动时显示蓝色表示正在连接
    setRgbColor(6);
  } else {
    // 启动热点模式
    startAccessPoint();
    // 启动BLE配网服务
    startBleService();
    // 设置Web服务器路由
    setupWebServer();
    // 启动Web服务器
    server.begin();
    
    Serial.println("ESP32热点已启动");
    Serial.print("热点名称: ");
    Serial.println(ssid_ap);
    Serial.print("热点密码: ");
    Serial.println(password_ap);
    Serial.print("访问 http://");
    Serial.print(WiFi.softAPIP());
    Serial.println(" 进行配网");
    Serial.print("BLE配网服务已启动，设备名称: ");
    Serial.println(deviceID);
    
    // LED快闪表示等待配网
    blinkMode = 0;
    
    // 显示彩虹模式表示等待配网
    setRgbColor(0);
  }
}

void loop() {
  // 处理Web服务器请求
  server.handleClient();
  
  // 处理BLE连接状态
  handleBleConnection();
  
  // 检查按键状态
  checkConfigButton();
  
  // 更新LED状态
  updateLED();
  
  // 如果已经配置了WiFi但尚未连接，则尝试连接
  if (wifi_configured && !wifi_connected) {
    connectToWiFi();
  }
  
  // 如果已经连接WiFi，检查网络状态
  if (wifi_connected) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi连接断开，尝试重连...");
      wifi_connected = false;
      wifi_configured = true;
      blinkMode = 1; // 慢闪表示正在重连
      
      // 显示红色表示连接断开
      setRgbColor(1);
    } else {
      // WiFi连接正常，处理UDP广播
      handleBroadcast();
    }
  }
}

// 处理UDP广播
void handleBroadcast() {
  // 检查是否需要启用广播
  if (broadcastEnabled) {
    // 检查是否超时
    if (millis() - broadcastStartTime > broadcastTimeout) {
      // 广播超时，关闭广播
      broadcastEnabled = false;
      Serial.println("UDP广播超时，已关闭");
    } else {
      // 定期广播IP地址
      if (millis() - lastBroadcast > broadcastInterval) {
        // 创建JSON格式的设备信息
        String message = "{";
        message += "\"device_id\":\"" + deviceID + "\",";
        message += "\"device_name\":\"" + deviceName + "\",";
        message += "\"ip_address\":\"" + WiFi.localIP().toString() + "\",";
        message += "\"mac_address\":\"" + WiFi.macAddress() + "\"";
        message += "}";
        
        // 修复：使用正确的UDP方法
        udp.beginMulticast(IPAddress(224, 0, 0, 1), udpPort);
        udp.beginPacket(IPAddress(224, 0, 0, 1), udpPort);
        udp.write((uint8_t*)message.c_str(), message.length());
        udp.endPacket();
        
        Serial.println("广播设备信息: " + message);
        lastBroadcast = millis();
      }
    }
  }
}

// 启用UDP广播
void enableBroadcast() {
  if (!broadcastEnabled) {
    broadcastEnabled = true;
    broadcastStartTime = millis();
    lastBroadcast = 0; // 立即发送第一次广播
    
    // 启动UDP用于广播IP地址
    udp.begin(udpPort);
    
    Serial.println("UDP广播已启用，将在10分钟内广播设备信息");
  }
}

// 广播IP地址
void broadcastIP() {
  if (millis() - lastBroadcast > broadcastInterval) {
    // 创建JSON格式的设备信息
    String message = "{";
    message += "\"device_id\":\"" + deviceID + "\",";
    message += "\"device_name\":\"" + deviceName + "\",";
    message += "\"ip_address\":\"" + WiFi.localIP().toString() + "\",";
    message += "\"mac_address\":\"" + WiFi.macAddress() + "\"";
    message += "}";
    
    // 修复：使用正确的UDP方法
    udp.beginMulticast(IPAddress(224, 0, 0, 1), udpPort);
    udp.beginPacket(IPAddress(224, 0, 0, 1), udpPort);
    udp.write((uint8_t*)message.c_str(), message.length());
    udp.endPacket();
    
    Serial.println("广播设备信息: " + message);
    lastBroadcast = millis();
  }
}

// 处理BLE连接状态
void handleBleConnection() {
  // 断开连接后重新广播
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // 等待蓝牙栈准备好
    pServer->startAdvertising(); // 重新开始广播
    Serial.println("重新开始BLE广播");
    oldDeviceConnected = deviceConnected;
  }
  
  // 连接建立
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}

// 启动BLE服务
void startBleService() {
  // 创建BLE设备
  String bleName = "ESP32-" + deviceID.substring(0, 4);
  BLEDevice::init(bleName.c_str());
  
  // 创建BLE服务器
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // 创建BLE服务
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // 创建WiFi配置特征
  pWifiCharacteristic = pService->createCharacteristic(
                      WIFI_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );
  pWifiCharacteristic->setCallbacks(new WifiCallbacks());
  
  // 创建状态特征
  pStatusCharacteristic = pService->createCharacteristic(
                      STATUS_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pStatusCharacteristic->setValue("等待配网");
  
  // 添加描述符
  pStatusCharacteristic->addDescriptor(new BLE2902());
  
  // 启动服务
  pService->start();
  
  // 启动广播
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE配网服务已启动");
  Serial.print("BLE设备名称: ");
  Serial.println(bleName);
}

// 检查配网按键状态
void checkConfigButton() {
  // 检测按键按下
  if (digitalRead(configButtonPin) == LOW) {
    if (!buttonPressed) {
      buttonPressed = true;
      buttonPressTime = millis();
    } else {
      // 检查是否长按
      if ((millis() - buttonPressTime) >= buttonLongPressDuration) {
        // 长按触发重新配网
        if (wifi_connected) {
          Serial.println("长按按键，触发重新配网");
          startReconfiguration();
          buttonPressTime = millis(); // 防止重复触发
        }
      }
    }
  } else {
    if (buttonPressed) {
      // 短按处理 - 启用UDP广播
      if ((millis() - buttonPressTime) < buttonLongPressDuration) {
        if (wifi_connected) {
          Serial.println("短按按键，启用UDP广播");
          enableBroadcast();
        }
      }
      buttonPressed = false;
    }
  }
}

// 启动重新配网
void startReconfiguration() {
  // 断开现有WiFi连接
  WiFi.disconnect(true);
  wifi_connected = false;
  
  // 启动热点模式
  WiFi.mode(WIFI_AP_STA); // 设置为AP+STA模式
  delay(100);
  startAccessPoint();
  
  // 启动BLE配网服务
  startBleService();
  
  // 设置Web服务器路由
  setupWebServer();
  
  // 启动Web服务器
  server.begin();
  
  // 删除保存的WiFi配置
  preferences.remove("ssid");
  preferences.remove("password");
  ssid_sta = "";
  password_sta = "";
  
  Serial.println("重新进入配网模式");
  Serial.print("热点名称: ");
  Serial.println(ssid_ap);
  Serial.print("热点密码: ");
  Serial.println(password_ap);
  Serial.print("访问 http://");
  Serial.print(WiFi.softAPIP());
  Serial.println(" 进行配网");
  Serial.print("BLE配网服务已启动，设备名称: ");
  Serial.println(deviceID);
  
  // LED快闪表示等待配网
  blinkMode = 0;
  
  // 恢复RGB彩虹模式
  setRgbColor(0);
}

// 更新LED状态
void updateLED() {
  unsigned long currentMillis = millis();
  long interval = (blinkMode == 0) ? fastInterval : slowInterval;
  
  if (ledControlEnabled) {
    // 如果启用了Web控制LED，则使用Web设置的状态
    digitalWrite(ledPin, ledWebState ? HIGH : LOW);
  } else {
    if (blinkMode == 2) {
      // 常亮模式
      digitalWrite(ledPin, HIGH);
    } else if (currentMillis - previousMillis >= interval) {
      // 闪烁模式
      previousMillis = currentMillis;
      ledState = (ledState == LOW) ? HIGH : LOW;
      digitalWrite(ledPin, ledState);
    }
  }
}

// 启动热点
void startAccessPoint() {
  Serial.println("正在启动热点...");
  
  // 明确设置WiFi模式为接入点
  WiFi.mode(WIFI_AP);
  delay(100);
  
  // 启动热点
  boolean result = WiFi.softAP(ssid_ap, password_ap, 1, 0, 4);
  
  if (result) {
    IPAddress IP = WiFi.softAPIP();
    Serial.print("热点IP地址: ");
    Serial.println(IP);
  } else {
    Serial.println("热点启动失败");
  }
}

// 设置Web服务器
void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/setwifi", handleSetWifi);
  server.on("/scan", handleWifiScan);
  server.on("/control", handleRgbControl);     // RGB控制页面
  server.on("/rgb", handleRgbApi);             // RGB控制API
  server.on("/status", handleStatus);          // 状态接口
  server.on("/identify", handleIdentify);      // 设备识别接口
  server.on("/api/control", handleApiControl); // API控制接口
  server.on("/api/info", handleApiInfo);       // API设备信息接口
  server.on("/api/discover", handleDiscover);  // 设备发现接口
  server.on("/api/broadcast", handleBroadcastApi); // 广播控制API
  server.on("/broadcast", handleBroadcastWeb); // 广播控制Web界面
  server.on("/hsv", handleHsvPicker);          // HSV调光板界面
  server.onNotFound(handleNotFound);
}

// 主页处理函数
void handleRoot() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta charset='utf-8'><title>ESP32 WiFi配置</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; text-align: center; }";
  html += "input[type='text'], input[type='password'] { width: 100%; padding: 12px; margin: 8px 0; box-sizing: border-box; border: 1px solid #ccc; border-radius: 4px; }";
  html += "input[type='submit'] { width: 100%; background-color: #4CAF50; color: white; padding: 14px 20px; margin: 8px 0; border: none; border-radius: 4px; cursor: pointer; }";
  html += "input[type='submit']:hover { background-color: #45a049; }";
  html += ".button { display: inline-block; padding: 10px 20px; margin: 5px; background-color: #008CBA; color: white; text-decoration: none; border-radius: 4px; }";
  html += ".button:hover { background-color: #007B9A; }";
  html += ".control-button { display: inline-block; padding: 15px 30px; margin: 10px; font-size: 18px; background-color: #4CAF50; color: white; text-decoration: none; border-radius: 4px; }";
  html += ".control-button.off { background-color: #f44336; }";
  html += ".info { background-color: #e7f3ff; padding: 15px; border-left: 6px solid #2196F3; margin: 20px 0; }";
  html += "</style></head>";
  html += "<body><div class='container'>";
  html += "<h1>ESP32 WiFi配置</h1>";
  
  html += "<div class='info'>";
  html += "<p><strong>配网方式1 (推荐):</strong> 使用手机APP通过蓝牙配网，无需断开当前网络</p>";
  html += "<p><strong>配网方式2:</strong> 连接热点 '" + String(ssid_ap) + "' (密码: " + String(password_ap) + ")</p>";
  html += "</div>";
  
  html += "<form action='/setwifi' method='POST'>";
  html += "<label for='ssid'>WiFi名称:</label>";
  html += "<input type='text' id='ssid' name='ssid' required><br>";
  html += "<label for='password'>WiFi密码:</label>";
  html += "<input type='password' id='password' name='password'><br>";
  html += "<input type='submit' value='连接WiFi'>";
  html += "</form>";
  html += "<p><a class='button' href='/scan'>扫描WiFi网络</a></p>";
  html += "<p><a class='button' href='/control'>RGB灯控制</a></p>";
  html += "<p><a class='button' href='/hsv'>HSV调光板</a></p>";
  html += "<p><a class='button' href='/identify'>识别设备</a></p>";
  html += "<p><a class='button' href='/broadcast'>UDP广播控制</a></p>";
  html += "<p>当前设备IP: " + WiFi.softAPIP().toString() + "</p>";
  html += "<p>设备ID: " + deviceID + "</p>";
  html += "<p>BLE设备名称: ESP32-" + deviceID.substring(0, 4) + "</p>";
  html += "<p>UDP广播状态: " + String(broadcastEnabled ? "启用中" : "已关闭") + "</p>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

// RGB控制页面
void handleRgbControl() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta charset='utf-8'><title>RGB灯控制</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }";
  html += ".container { max-width: 800px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); text-align: center; }";
  html += "h1 { color: #333; }";
  html += ".power-btn { display: inline-block; padding: 15px 30px; margin: 10px; font-size: 18px; background-color: #4CAF50; color: white; text-decoration: none; border-radius: 4px; cursor: pointer; }";
  html += ".power-btn.off { background-color: #f44336; }";
  html += ".color-btn { display: inline-block; width: 80px; height: 40px; margin: 5px; border: 2px solid #333; border-radius: 5px; cursor: pointer; }";
  html += ".rainbow-btn { background: linear-gradient(90deg, red, orange, yellow, green, blue, indigo, violet); }";
  html += ".hsv-control { width: 100%; margin: 10px 0; }";
  html += ".hsv-slider { width: 100%; height: 30px; margin: 5px 0; }";
  html += ".hue-slider { background: linear-gradient(90deg, #ff0000, #ffff00, #00ff00, #00ffff, #0000ff, #ff00ff, #ff0000); }";
  html += ".saturation-slider { background: linear-gradient(90deg, #808080, currentColor); }";
  html += ".value-slider { background: linear-gradient(90deg, #000000, currentColor, #ffffff); }";
  html += ".brightness-control { width: 100%; margin: 10px 0; }";
  html += ".status { font-size: 18px; margin: 20px; }";
  html += ".button { display: inline-block; padding: 10px 20px; margin: 5px; background-color: #008CBA; color: white; text-decoration: none; border-radius: 4px; }";
  html += ".button:hover { background-color: #007B9A; }";
  html += ".tab { overflow: hidden; border: 1px solid #ccc; background-color: #f1f1f1; }";
  html += ".tab button { background-color: inherit; float: left; border: none; outline: none; cursor: pointer; padding: 14px 16px; transition: 0.3s; }";
  html += ".tab button:hover { background-color: #ddd; }";
  html += ".tab button.active { background-color: #ccc; }";
  html += ".tabcontent { display: none; padding: 6px 12px; border: 1px solid #ccc; border-top: none; }";
  html += "</style>";
  html += "<script>";
  html += "function openTab(evt, tabName) {";
  html += "  var i, tabcontent, tablinks;";
  html += "  tabcontent = document.getElementsByClassName('tabcontent');";
  html += "  for (i = 0; i < tabcontent.length; i++) {";
  html += "    tabcontent[i].style.display = 'none';";
  html += "  }";
  html += "  tablinks = document.getElementsByClassName('tablinks');";
  html += "  for (i = 0; i < tablinks.length; i++) {";
  html += "    tablinks[i].className = tablinks[i].className.replace(' active', '');";
  html += "  }";
  html += "  document.getElementById(tabName).style.display = 'block';";
  html += "  evt.currentTarget.className += ' active';";
  html += "}";
  html += "function toggleRgb() {";
  html += "  var button = document.getElementById('powerButton');";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  if (button.classList.contains('off')) {";
  html += "    xhr.open('GET', '/api/control?power=on', true);";
  html += "    xhr.send();";
  html += "    button.classList.remove('off');";
  html += "    button.textContent = '关闭RGB';";
  html += "  } else {";
  html += "    xhr.open('GET', '/api/control?power=off', true);";
  html += "    xhr.send();";
  html += "    button.classList.add('off');";
  html += "    button.textContent = '开启RGB';";
  html += "  }";
  html += "}";
  html += "function setColor(color) {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/api/control?color=' + color, true);";
  html += "  xhr.send();";
  html += "}";
  html += "function setHsv() {";
  html += "  var hue = document.getElementById('hue').value;";
  html += "  var saturation = document.getElementById('saturation').value;";
  html += "  var value = document.getElementById('value').value;";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/api/control?hue=' + hue + '&saturation=' + saturation + '&value=' + value, true);";
  html += "  xhr.send();";
  html += "}";
  html += "function setBrightness() {";
  html += "  var brightness = document.getElementById('brightness').value;";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/api/control?brightness=' + brightness, true);";
  html += "  xhr.send();";
  html += "}";
  html += "function updateStatus() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/api/info', true);";
  html += "  xhr.onreadystatechange = function() {";
  html += "    if (xhr.readyState === 4 && xhr.status === 200) {";
  html += "      var data = JSON.parse(xhr.responseText);";
  html += "      var statusHtml = '<p><strong>RGB状态:</strong> ' + (data.rgb_enabled ? '开启' : '关闭') + '</p>';";
  html += "      if (data.hsv_mode) {";
  html += "        statusHtml += '<p><strong>HSV模式:</strong> 色相 ' + data.hsv_hue + '度, 饱和度 ' + data.hsv_saturation + '%, 明度 ' + data.hsv_value + '%</p>';";
  html += "      } else {";
  html += "        var colorNames = ['彩虹', '红色', '橙色', '黄色', '绿色', '青色', '蓝色', '紫色'];";
  html += "        statusHtml += '<p><strong>颜色:</strong> ' + colorNames[data.rgb_color + 1] + '</p>';";
  html += "      }";
  html += "      statusHtml += '<p><strong>亮度:</strong> ' + data.rgb_brightness + '%</p>';";
  html += "      document.getElementById('status').innerHTML = statusHtml;";
  html += "    }";
  html += "  };";
  html += "  xhr.send();";
  html += "}";
  html += "setInterval(updateStatus, 1000);";
  html += "</script>";
  html += "</head>";
  html += "<body onload='updateStatus(); openTab(event, \"PresetTab\")'>";
  html += "<div class='container'>";
  html += "<h1>RGB灯控制</h1>";
  html += "<div class='status' id='status'>加载中...</div>";
  
  html += "<button id='powerButton' class='power-btn " + String(rgbEnabled ? "" : "off") + "' onclick='toggleRgb()'>" + String(rgbEnabled ? "关闭RGB" : "开启RGB") + "</button>";
  
  html += "<div class='tab'>";
  html += "<button class='tablinks' onclick=\"openTab(event, 'PresetTab')\">预设颜色</button>";
  html += "<button class='tablinks' onclick=\"openTab(event, 'HsvTab')\">HSV调色</button>";
  html += "</div>";
  
  html += "<div id='PresetTab' class='tabcontent'>";
  html += "<h2>预设颜色</h2>";
  html += "<div>";
  html += "<div class='color-btn' style='background-color: red;' onclick='setColor(1)'></div>";
  html += "<div class='color-btn' style='background-color: orange;' onclick='setColor(2)'></div>";
  html += "<div class='color-btn' style='background-color: yellow;' onclick='setColor(3)'></div>";
  html += "<div class='color-btn' style='background-color: green;' onclick='setColor(4)'></div>";
  html += "<div class='color-btn' style='background-color: cyan;' onclick='setColor(5)'></div>";
  html += "<div class='color-btn' style='background-color: blue;' onclick='setColor(6)'></div>";
  html += "<div class='color-btn' style='background-color: purple;' onclick='setColor(7)'></div>";
  html += "<div class='color-btn rainbow-btn' onclick='setColor(0)'>彩虹</div>";
  html += "</div>";
  html += "</div>";
  
  html += "<div id='HsvTab' class='tabcontent'>";
  html += "<h2>HSV调色板</h2>";
  html += "<div class='hsv-control'>";
  html += "<p><strong>色相 (0-360度):</strong> <span id='hueValue'>" + String(hsvHue) + "度</span></p>";
  html += "<input type='range' id='hue' class='hsv-slider hue-slider' min='0' max='360' value='" + String(hsvHue) + "' oninput='document.getElementById(\"hueValue\").textContent = this.value + \"度\"; setHsv()'>";
  html += "</div>";
  html += "<div class='hsv-control'>";
  html += "<p><strong>饱和度 (0-100%):</strong> <span id='saturationValue'>" + String(hsvSaturation) + "%</span></p>";
  html += "<input type='range' id='saturation' class='hsv-slider saturation-slider' min='0' max='100' value='" + String(hsvSaturation) + "' oninput='document.getElementById(\"saturationValue\").textContent = this.value + \"%\"; setHsv()'>";
  html += "</div>";
  html += "<div class='hsv-control'>";
  html += "<p><strong>明度 (0-100%):</strong> <span id='valueValue'>" + String(hsvValue) + "%</span></p>";
  html += "<input type='range' id='value' class='hsv-slider value-slider' min='0' max='100' value='" + String(hsvValue) + "' oninput='document.getElementById(\"valueValue\").textContent = this.value + \"%\"; setHsv()'>";
  html += "</div>";
  html += "</div>";
  
  html += "<h2>亮度调节</h2>";
  html += "<input type='range' id='brightness' class='brightness-control' min='0' max='100' value='" + String(rgbBrightness) + "' onchange='setBrightness()'>";
  
  html += "<p><a class='button' href='/'>返回首页</a></p>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

// 设备识别接口
void handleIdentify() {
  // 闪烁白色光5次，每次500ms，用于识别设备
  server.send(200, "text/plain", "设备识别中...请观察RGB灯闪烁");
  
  // 保存当前状态
  bool oldRgbEnabled = rgbEnabled;
  int oldColor = currentRgbColor;
  uint8_t oldBrightness = rgbBrightness;
  
  // 临时开启RGB
  rgbEnabled = true;
  
  // 闪烁5次白色光
  for (int i = 0; i < 5; i++) {
    setRgbColor(0); // 白色
    delay(500);
    setRgbColor(-1); // 关闭
    delay(500);
  }
  
  // 恢复原来的状态
  rgbEnabled = oldRgbEnabled;
  currentRgbColor = oldColor;
  rgbBrightness = oldBrightness;
  if (rgbEnabled) {
    setRgbColor(currentRgbColor);
  }
}

// API控制接口
void handleApiControl() {
  bool updated = false;
  
  // HSV模式控制
  if (server.hasArg("hue")) {
    float hue = server.arg("hue").toFloat();
    if (hue >= 0 && hue <= 360) {
      hsvHue = hue;
      useHsvMode = true;
      if (rgbEnabled) {
        setRgbColor(currentRgbColor);
      }
      updated = true;
      Serial.println("HSV色相已设置为: " + String(hue) + "度");
    }
  }
  
  if (server.hasArg("saturation")) {
    float saturation = server.arg("saturation").toFloat();
    if (saturation >= 0 && saturation <= 100) {
      hsvSaturation = saturation;
      useHsvMode = true;
      if (rgbEnabled) {
        setRgbColor(currentRgbColor);
      }
      updated = true;
      Serial.println("HSV饱和度已设置为: " + String(saturation) + "%");
    }
  }
  
  if (server.hasArg("value")) {
    float value = server.arg("value").toFloat();
    if (value >= 0 && value <= 100) {
      hsvValue = value;
      useHsvMode = true;
      if (rgbEnabled) {
        setRgbColor(currentRgbColor);
      }
      updated = true;
      Serial.println("HSV明度已设置为: " + String(value) + "%");
    }
  }
  
  // 预设颜色控制
  if (server.hasArg("color")) {
    int color = server.arg("color").toInt();
    if (color >= -1 && color <= 7) {
      currentRgbColor = color;
      useHsvMode = false; // 切换到预设颜色模式
      if (rgbEnabled) {
        setRgbColor(color);
      }
      updated = true;
      Serial.println("RGB颜色已设置为: " + String(color));
    }
  }
  
  // 亮度控制
  if (server.hasArg("brightness")) {
    int brightness = server.arg("brightness").toInt();
    if (brightness >= 0 && brightness <= 100) {
      rgbBrightness = brightness;
      // 重新设置颜色以应用亮度
      if (rgbEnabled) {
        setRgbColor(currentRgbColor);
      }
      updated = true;
      Serial.println("RGB亮度已设置为: " + String(brightness));
    }
  }
  
  // 点亮时间控制
  if (server.hasArg("duration")) {
    long duration = server.arg("duration").toInt();
    if (duration > 0) {
      // 如果同时设置了颜色，则点亮指定时间后关闭
      if (server.hasArg("color") || server.hasArg("brightness") || server.hasArg("hue")) {
        // 保持当前设置的持续时间
        delay(duration);
        
        // 时间到后关闭RGB
        bool oldRgbEnabled = rgbEnabled;
        rgbEnabled = false;
        setRgbColor(-1);
        rgbEnabled = oldRgbEnabled;
      }
      updated = true;
      Serial.println("RGB点亮时间设置为: " + String(duration) + "ms");
    }
  }
  
  // 开关控制
  if (server.hasArg("power")) {
    String power = server.arg("power");
    if (power == "on") {
      rgbEnabled = true;
      // 确保使用正确的颜色模式
      if (useHsvMode) {
        setRgbColor(0); // 使用HSV模式
      } else {
        setRgbColor(currentRgbColor); // 使用预设颜色模式
      }
      updated = true;
      Serial.println("RGB灯已开启");
    } else if (power == "off") {
      rgbEnabled = false;
      setRgbColor(-1); // 关闭RGB
      updated = true;
      Serial.println("RGB灯已关闭");
    }
  }
  
  if (updated) {
    server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"RGB设置已更新\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"缺少有效参数\"}");
  }
}

// API设备信息接口
void handleApiInfo() {
  String json = "{";
  json += "\"device_id\":\"" + deviceID + "\",";
  json += "\"device_name\":\"" + deviceName + "\",";
  json += "\"ip_address\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"mac_address\":\"" + WiFi.macAddress() + "\",";
  json += "\"wifi_status\":" + String(wifi_connected ? "true" : "false") + ",";
  json += "\"rgb_enabled\":" + String(rgbEnabled ? "true" : "false") + ",";
  json += "\"rgb_color\":" + String(currentRgbColor) + ",";
  json += "\"rgb_brightness\":" + String(rgbBrightness) + ",";
  json += "\"hsv_mode\":" + String(useHsvMode ? "true" : "false") + ",";
  json += "\"hsv_hue\":" + String(hsvHue) + ",";
  json += "\"hsv_saturation\":" + String(hsvSaturation) + ",";
  json += "\"hsv_value\":" + String(hsvValue) + ",";
  json += "\"broadcast_enabled\":" + String(broadcastEnabled ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
}

// 设备发现接口
void handleDiscover() {
  String json = "{";
  json += "\"device_id\":\"" + deviceID + "\",";
  json += "\"device_name\":\"" + deviceName + "\",";
  json += "\"ip_address\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"mac_address\":\"" + WiFi.macAddress() + "\",";
  json += "\"broadcast_enabled\":" + String(broadcastEnabled ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
}

// RGB控制API
void handleRgbApi() {
  bool updated = false;
  
  if (server.hasArg("power")) {
    String power = server.arg("power");
    if (power == "on") {
      rgbEnabled = true;
      setRgbColor(currentRgbColor);
      updated = true;
      Serial.println("RGB灯已开启");
    } else if (power == "off") {
      rgbEnabled = false;
      setRgbColor(-1); // 关闭RGB
      updated = true;
      Serial.println("RGB灯已关闭");
    }
  }
  
  if (server.hasArg("color")) {
    int color = server.arg("color").toInt();
    if (color >= -1 && color <= 7) {
      currentRgbColor = color;
      if (rgbEnabled) {
        setRgbColor(color);
      }
      updated = true;
      Serial.println("RGB颜色已设置为: " + String(color));
    }
  }
  
  if (server.hasArg("brightness")) {
    int brightness = server.arg("brightness").toInt();
    if (brightness >= 0 && brightness <= 100) {
      rgbBrightness = brightness;
      // 重新设置颜色以应用亮度
      if (rgbEnabled) {
        setRgbColor(currentRgbColor);
      }
      updated = true;
      Serial.println("RGB亮度已设置为: " + String(brightness));
    }
  }
  
  if (updated) {
    server.send(200, "text/plain", "RGB设置已更新");
  } else {
    server.send(400, "text/plain", "缺少有效参数");
  }
}

// HSV转RGB转换函数
void hsvToRgb(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b) {
  h = fmod(h, 360.0);
  if (h < 0) h += 360.0;
  
  s = constrain(s, 0.0, 100.0);
  v = constrain(v, 0.0, 100.0);
  
  float c = v * s / 10000.0;
  float x = c * (1 - abs(fmod(h / 60.0, 2) - 1));
  float m = v / 100.0 - c;
  
  float r1, g1, b1;
  
  if (h < 60) {
    r1 = c; g1 = x; b1 = 0;
  } else if (h < 120) {
    r1 = x; g1 = c; b1 = 0;
  } else if (h < 180) {
    r1 = 0; g1 = c; b1 = x;
  } else if (h < 240) {
    r1 = 0; g1 = x; b1 = c;
  } else if (h < 300) {
    r1 = x; g1 = 0; b1 = c;
  } else {
    r1 = c; g1 = 0; b1 = x;
  }
  
  r = (uint8_t)((r1 + m) * 255);
  g = (uint8_t)((g1 + m) * 255);
  b = (uint8_t)((b1 + m) * 255);
}

// 设置RGB颜色
void setRgbColor(int color) {
  // 首先检查RGB是否启用
  if (!rgbEnabled) {
    strip.setPixelColor(0, 0, 0, 0); // 关闭RGB灯
    strip.show();
    Serial.println("RGB灯已关闭");
    return;
  }
  
  if (useHsvMode) {
    // 使用HSV模式
    uint8_t r, g, b;
    hsvToRgb(hsvHue, hsvSaturation, hsvValue, r, g, b);
    
    // 应用亮度调节
    r = (r * rgbBrightness) / 100;
    g = (g * rgbBrightness) / 100;
    b = (b * rgbBrightness) / 100;
    
    strip.setPixelColor(0, r, g, b);
  } else {
    // 使用预设颜色模式
    switch (color) {
      case -1: // 关闭
        strip.setPixelColor(0, 0, 0, 0);
        break;
      case 0: // 彩虹模式，暂时显示白色
        strip.setPixelColor(0, strip.Color(255, 255, 255));
        break;
      case 1: // 红色
        strip.setPixelColor(0, strip.Color(255, 0, 0));
        break;
      case 2: // 橙色
        strip.setPixelColor(0, strip.Color(255, 140, 0));
        break;
      case 3: // 黄色
        strip.setPixelColor(0, strip.Color(255, 255, 0));
        break;
      case 4: // 绿色
        strip.setPixelColor(0, strip.Color(0, 255, 0));
        break;
      case 5: // 青色
        strip.setPixelColor(0, strip.Color(0, 255, 255));
        break;
      case 6: // 蓝色
        strip.setPixelColor(0, strip.Color(0, 0, 255));
        break;
      case 7: // 紫色
        strip.setPixelColor(0, strip.Color(128, 0, 128));
        break;
    }
    
    // 应用亮度
    if (color != -1) {
      uint32_t currentColor = strip.getPixelColor(0);
      uint8_t r = (currentColor >> 16) & 0xFF;
      uint8_t g = (currentColor >> 8) & 0xFF;
      uint8_t b = currentColor & 0xFF;
      
      // 应用亮度调节
      r = (r * rgbBrightness) / 100;
      g = (g * rgbBrightness) / 100;
      b = (b * rgbBrightness) / 100;
      
      strip.setPixelColor(0, r, g, b);
    }
  }
  
  strip.show();
  if (useHsvMode) {
    Serial.println("HSV颜色已更新 - 色相: " + String(hsvHue) + "度, 饱和度: " + String(hsvSaturation) + "%, 明度: " + String(hsvValue) + "%, 亮度: " + String(rgbBrightness) + "%");
  } else {
    Serial.println("RGB颜色已更新，亮度: " + String(rgbBrightness) + "%");
  }
}

// 状态查询处理函数
void handleStatus() {
  String status = "<div>";
  status += "<p><strong>WiFi状态:</strong> " + String(wifi_connected ? "已连接" : "未连接") + "</p>";
  if (wifi_connected) {
    status += "<p><strong>SSID:</strong> " + ssid_sta + "</p>";
    status += "<p><strong>IP地址:</strong> " + WiFi.localIP().toString() + "</p>";
  }
  status += "<p><strong>LED控制:</strong> " + String(ledControlEnabled ? "Web控制模式" : "系统状态模式") + "</p>";
  if (ledControlEnabled) {
    status += "<p><strong>LED状态:</strong> " + String(ledWebState ? "开启" : "关闭") + "</p>";
  } else {
    String mode = "未知";
    if (blinkMode == 0) mode = "快速闪烁（等待配网）";
    else if (blinkMode == 1) mode = "慢速闪烁（正在连接）";
    else if (blinkMode == 2) mode = "常亮（连接成功）";
    status += "<p><strong>LED模式:</strong> " + mode + "</p>";
  }
  
  status += "<p><strong>RGB状态:</strong> " + String(rgbEnabled ? "开启" : "关闭") + "</p>";
  if (rgbEnabled) {
    String color = "未知";
    switch (currentRgbColor) {
      case 0: color = "彩虹模式"; break;
      case 1: color = "红色"; break;
      case 2: color = "橙色"; break;
      case 3: color = "黄色"; break;
      case 4: color = "绿色"; break;
      case 5: color = "青色"; break;
      case 6: color = "蓝色"; break;
      case 7: color = "紫色"; break;
    }
    status += "<p><strong>RGB颜色:</strong> " + color + "</p>";
  }
  status += "<p><strong>RGB亮度:</strong> " + String(rgbBrightness) + "%</p>";
  status += "<p><strong>设备ID:</strong> " + deviceID + "</p>";
  status += "<p><strong>设备名称:</strong> " + deviceName + "</p>";
  status += "<p><strong>BLE状态:</strong> " + String(deviceConnected ? "已连接" : "等待连接") + "</p>";
  status += "<p><strong>UDP广播状态:</strong> " + String(broadcastEnabled ? "启用中" : "已关闭") + "</p>";
  if (broadcastEnabled) {
    unsigned long remaining = broadcastTimeout - (millis() - broadcastStartTime);
    status += "<p><strong>广播剩余时间:</strong> " + String(remaining / 1000) + "秒</p>";
  }
  status += "</div>";
  
  server.send(200, "text/html", status);
}

// 扫描WiFi网络
void handleWifiScan() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta charset='utf-8'><title>WiFi扫描结果</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; text-align: center; }";
  html += "table { width: 100%; border-collapse: collapse; margin: 20px 0; }";
  html += "th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }";
  html += "tr:hover { background-color: #f5f5f5; }";
  html += "th { background-color: #4CAF50; color: white; }";
  html += ".button { display: inline-block; padding: 10px 20px; margin: 5px; background-color: #008CBA; color: white; text-decoration: none; border-radius: 4px; }";
  html += ".button:hover { background-color: #007B9A; }";
  html += "</style></head>";
  html += "<body><div class='container'>";
  html += "<h1>WiFi扫描结果</h1>";
  
  int n = WiFi.scanNetworks();
  html += "<p>找到 " + String(n) + " 个网络:</p>";
  html += "<table><tr><th>名称</th><th>信号强度</th><th>加密方式</th></tr>";
  
  if (n == 0) {
    html += "<tr><td colspan='3'>未找到WiFi网络</td></tr>";
  } else {
    for (int i = 0; i < n; ++i) {
      html += "<tr>";
      html += "<td>" + WiFi.SSID(i) + "</td>";
      html += "<td>" + String(WiFi.RSSI(i)) + " dBm</td>";
      html += "<td>" + String((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "开放" : "加密") + "</td>";
      html += "</tr>";
    }
  }
  
  html += "</table>";
  html += "<p><a class='button' href='/'>返回配置页面</a></p>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

// 处理WiFi设置
void handleSetWifi() {
  if (server.hasArg("ssid")) {
    ssid_sta = server.arg("ssid");
    password_sta = server.arg("password");
    wifi_configured = true;
    wifi_connected = false;
    
    // 保存WiFi配置到Flash
    saveCredentials();
    
    String html = "<!DOCTYPE html><html>";
    html += "<head><meta charset='utf-8'><title>WiFi配置</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }";
    html += ".container { max-width: 600px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }";
    html += "h1 { color: #333; text-align: center; }";
    html += ".success { color: #4CAF50; }";
    html += ".button { display: inline-block; padding: 10px 20px; margin: 5px; background-color: #008CBA; color: white; text-decoration: none; border-radius: 4px; }";
    html += ".button:hover { background-color: #007B9A; }";
    html += "</style></head>";
    html += "<body><div class='container'>";
    html += "<h1 class='success'>WiFi配置成功!</h1>";
    html += "<p>正在尝试连接到 '" + ssid_sta + "'</p>";
    html += "<p>请断开此设备连接并检查串口监视器获取IP地址</p>";
    html += "<p><a class='button' href='/'>返回首页</a></p>";
    html += "</div></body></html>";
    
    server.send(200, "text/html", html);
  } else {
    server.send(400, "text/plain", "参数错误");
  }
}

// 处理未找到的页面
void handleNotFound() {
  server.send(404, "text/plain", "页面未找到");
}

// 连接到WiFi
void connectToWiFi() {
  Serial.println("正在连接到WiFi: " + ssid_sta);
  
  // 关闭热点
  WiFi.softAPdisconnect(true);
  
  // 关闭BLE服务
  if (pServer != NULL) {
    pServer->removeService(pServer->getServiceByUUID(SERVICE_UUID));
  }
  
  // 切换到STA模式
  WiFi.mode(WIFI_STA);
  delay(100);
  WiFi.begin(ssid_sta.c_str(), password_sta.c_str());
  
  // 等待连接结果
  int retryCount = 0;
  const int maxRetries = 20;
  
  // LED慢闪表示正在连接WiFi
  blinkMode = 1;
  
  while (WiFi.status() != WL_CONNECTED && retryCount < maxRetries) {
    delay(500);
    Serial.print(".");
    retryCount++;
    updateLED(); // 更新LED状态
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    // 连接成功
    Serial.println("");
    Serial.println("WiFi连接成功!");
    Serial.print("IP地址: ");
    Serial.println(WiFi.localIP());
    
    // LED常亮表示连接成功
    blinkMode = 2;
    wifi_connected = true;
    
    // 清除标志防止重复连接
    wifi_configured = false;
    
    // 重新设置Web服务器路由（WiFi连接后需要提供控制功能）
    setupWebServer();
    server.begin();
    
    // 设置RGB为蓝色表示连接成功
    if (rgbEnabled) {
      setRgbColor(6); // 蓝色
    }
    
    // 启用UDP广播
    enableBroadcast();
  } else {
    // 连接失败，重新打开热点
    Serial.println("");
    Serial.println("WiFi连接失败!");
    
    // LED快闪表示连接失败
    blinkMode = 0;
    
    WiFi.mode(WIFI_AP);
    delay(100);
    startAccessPoint();
    startBleService(); // 重新启动BLE服务
    setupWebServer();
    server.begin();
    
    // 清除WiFi信息
    ssid_sta = "";
    password_sta = "";
    wifi_configured = false;
    wifi_connected = false;
    
    // 恢复RGB彩虹模式
    setRgbColor(0);
  }
}

// 保存WiFi配置到Flash
void saveCredentials() {
  preferences.putString("ssid", ssid_sta);
  preferences.putString("password", password_sta);
  Serial.println("WiFi配置已保存到Flash");
}

// 从Flash加载WiFi配置
bool loadCredentials() {
  ssid_sta = preferences.getString("ssid", "");
  password_sta = preferences.getString("password", "");
  
  if (ssid_sta.length() > 0) {
    Serial.println("从Flash成功加载WiFi配置");
    return true;
  }
  
  Serial.println("Flash中没有保存的WiFi配置");
  return false;
}

// 广播控制API
void handleBroadcastApi() {
  if (server.hasArg("action")) {
    String action = server.arg("action");
    if (action == "enable") {
      enableBroadcast();
      server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"UDP广播已启用\"}");
    } else if (action == "disable") {
      broadcastEnabled = false;
      server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"UDP广播已禁用\"}");
    } else {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"无效的操作参数\"}");
    }
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"缺少action参数\"}");
  }
}

// 广播控制Web界面
void handleBroadcastWeb() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta charset='utf-8'><title>UDP广播控制</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); text-align: center; }";
  html += "h1 { color: #333; }";
  html += ".button { display: inline-block; padding: 15px 30px; margin: 10px; font-size: 18px; background-color: #4CAF50; color: white; text-decoration: none; border-radius: 4px; cursor: pointer; }";
  html += ".button.off { background-color: #f44336; }";
  html += ".status { font-size: 18px; margin: 20px; }";
  html += "</style>";
  html += "<script>";
  html += "function toggleBroadcast() {";
  html += "  var button = document.getElementById('broadcastButton');";
  html += "  var status = document.getElementById('status');";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  if (button.classList.contains('off')) {";
  html += "    xhr.open('GET', '/api/broadcast?action=enable', true);";
  html += "    xhr.onreadystatechange = function() {";
  html += "      if (xhr.readyState === 4 && xhr.status === 200) {";
  html += "        button.classList.remove('off');";
  html += "        button.textContent = '禁用UDP广播';";
  html += "        status.textContent = 'UDP广播状态: 启用中';";
  html += "      }";
  html += "    };";
  html += "    xhr.send();";
  html += "  } else {";
  html += "    xhr.open('GET', '/api/broadcast?action=disable', true);";
  html += "    xhr.onreadystatechange = function() {";
  html += "      if (xhr.readyState === 4 && xhr.status === 200) {";
  html += "        button.classList.add('off');";
  html += "        button.textContent = '启用UDP广播';";
  html += "        status.textContent = 'UDP广播状态: 已关闭';";
  html += "      }";
  html += "    };";
  html += "    xhr.send();";
  html += "  }";
  html += "}";
  html += "</script>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>UDP广播控制</h1>";
  html += "<div class='status' id='status'>UDP广播状态: " + String(broadcastEnabled ? "启用中" : "已关闭") + "</div>";
  html += "<button id='broadcastButton' class='button " + String(broadcastEnabled ? "" : "off") + "' onclick='toggleBroadcast()'>" + String(broadcastEnabled ? "禁用UDP广播" : "启用UDP广播") + "</button>";
  html += "<p><a class='button' href='/'>返回首页</a></p>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

// HSV调光板界面
void handleHsvPicker() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta charset='utf-8'><title>HSV调光板</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f0f0f0; }";
  html += ".container { max-width: 800px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); text-align: center; }";
  html += "h1 { color: #333; margin-bottom: 20px; }";
  html += ".hsv-circle { position: relative; width: 300px; height: 300px; margin: 0 auto; border-radius: 50%; background: conic-gradient(red, yellow, lime, aqua, blue, magenta, red); cursor: pointer; }";
  html += ".hsv-circle::before { content: ''; position: absolute; top: 50%; left: 50%; width: 280px; height: 280px; background: radial-gradient(circle, transparent 0%, #333 100%); border-radius: 50%; transform: translate(-50%, -50%); pointer-events: none; }";
  html += ".hsv-selector { position: absolute; width: 20px; height: 20px; border: 2px solid white; border-radius: 50%; background: rgba(255,255,255,0.5); pointer-events: none; transform: translate(-50%, -50%); box-shadow: 0 0 5px rgba(0,0,0,0.5); }";
  html += ".hsv-controls { margin: 20px 0; }";
  html += ".hsv-slider { width: 100%; max-width: 400px; margin: 10px auto; }";
  html += ".hue-slider { background: linear-gradient(90deg, #ff0000, #ffff00, #00ff00, #00ffff, #0000ff, #ff00ff, #ff0000); }";
  html += ".saturation-slider { background: linear-gradient(90deg, #808080, currentColor); }";
  html += ".value-slider { background: linear-gradient(90deg, #000000, currentColor, #ffffff); }";
  html += ".brightness-slider { background: linear-gradient(90deg, #000000, #ffffff); }";
  html += ".color-preview { width: 100px; height: 100px; margin: 20px auto; border: 2px solid #333; border-radius: 10px; }";
  html += ".button { display: inline-block; padding: 10px 20px; margin: 5px; background-color: #008CBA; color: white; text-decoration: none; border-radius: 4px; }";
  html += ".button:hover { background-color: #007B9A; }";
  html += ".power-btn { display: inline-block; padding: 15px 30px; margin: 10px; font-size: 18px; background-color: #4CAF50; color: white; text-decoration: none; border-radius: 4px; cursor: pointer; }";
  html += ".power-btn.off { background-color: #f44336; }";
  html += ".status { font-size: 16px; margin: 10px; padding: 10px; background-color: #f5f5f5; border-radius: 5px; }";
  html += ".color-picker-section { margin: 20px 0; padding: 15px; background-color: #f9f9f9; border-radius: 10px; }";
  html += ".color-picker-btn { display: inline-block; padding: 10px 20px; margin: 10px; background-color: #2196F3; color: white; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }";
  html += ".color-picker-btn:hover { background-color: #1976D2; }";
  html += ".color-picker-canvas { width: 100%; max-width: 400px; height: 200px; margin: 10px auto; border: 2px solid #ccc; border-radius: 5px; cursor: crosshair; }";
  html += ".color-picker-info { margin: 10px; font-size: 14px; color: #666; }";
  html += "</style>";
  html += "<script>";
  html += "let currentHue = " + String(hsvHue) + ";";
  html += "let currentSaturation = " + String(hsvSaturation) + ";";
  html += "let currentValue = " + String(hsvValue) + ";";
  html += "let currentBrightness = " + String(rgbBrightness) + ";";
  html += "let rgbEnabled = " + String(rgbEnabled ? "true" : "false") + ";";
  html += "let colorPickerActive = false;";
  html += "let colorPickerCanvas = null;";
  html += "let colorPickerCtx = null;";
  html += "";
  html += "function updateColorPreview() {";
  html += "  const preview = document.getElementById('colorPreview');";
  html += "  const h = currentHue;";
  html += "  const s = currentSaturation / 100;";
  html += "  const v = currentValue / 100;";
  html += "  const rgb = hsvToRgb(h, s, v);";
  html += "  preview.style.backgroundColor = `rgb(${rgb.r}, ${rgb.g}, ${rgb.b})`;";
  html += "}";
  html += "";
  html += "function hsvToRgb(h, s, v) {";
  html += "  h = h % 360;";
  html += "  if (h < 0) h += 360;";
  html += "  ";
  html += "  const c = v * s;";
  html += "  const x = c * (1 - Math.abs((h / 60) % 2 - 1));";
  html += "  const m = v - c;";
  html += "  ";
  html += "  let r1, g1, b1;";
  html += "  if (h < 60) {";
  html += "    r1 = c; g1 = x; b1 = 0;";
  html += "  } else if (h < 120) {";
  html += "    r1 = x; g1 = c; b1 = 0;";
  html += "  } else if (h < 180) {";
  html += "    r1 = 0; g1 = c; b1 = x;";
  html += "  } else if (h < 240) {";
  html += "    r1 = 0; g1 = x; b1 = c;";
  html += "  } else if (h < 300) {";
  html += "    r1 = x; g1 = 0; b1 = c;";
  html += "  } else {";
  html += "    r1 = c; g1 = 0; b1 = x;";
  html += "  }";
  html += "  ";
  html += "  return {";
  html += "    r: Math.round((r1 + m) * 255),";
  html += "    g: Math.round((g1 + m) * 255),";
  html += "    b: Math.round((b1 + m) * 255)";
  html += "  };";
  html += "}";
  html += "";
  html += "function updateSelectorPosition() {";
  html += "  const selector = document.getElementById('hsvSelector');";
  html += "  const circle = document.getElementById('hsvCircle');";
  html += "  const radius = circle.offsetWidth / 2;";
  html += "  const angle = (currentHue - 90) * Math.PI / 180;";
  html += "  const distance = (currentSaturation / 100) * radius;";
  html += "  const x = radius + Math.cos(angle) * distance;";
  html += "  const y = radius + Math.sin(angle) * distance;";
  html += "  selector.style.left = x + 'px';";
  html += "  selector.style.top = y + 'px';";
  html += "}";
  html += "";
  html += "function handleCircleClick(event) {";
  html += "  const circle = document.getElementById('hsvCircle');";
  html += "  const rect = circle.getBoundingClientRect();";
  html += "  const centerX = rect.left + circle.offsetWidth / 2;";
  html += "  const centerY = rect.top + circle.offsetHeight / 2;";
  html += "  const radius = circle.offsetWidth / 2;";
  html += "  ";
  html += "  const x = event.clientX - centerX;";
  html += "  const y = event.clientY - centerY;";
  html += "  const distance = Math.sqrt(x * x + y * y);";
  html += "  ";
  html += "  if (distance <= radius) {";
  html += "    const angle = Math.atan2(y, x) * 180 / Math.PI;";
  html += "    currentHue = (angle + 90 + 360) % 360;";
  html += "    currentSaturation = Math.min(100, (distance / radius) * 100);";
  html += "    ";
  html += "    document.getElementById('hue').value = currentHue;";
  html += "    document.getElementById('hueValue').textContent = Math.round(currentHue) + '°';";
  html += "    document.getElementById('saturation').value = currentSaturation;";
  html += "    document.getElementById('saturationValue').textContent = Math.round(currentSaturation) + '%';";
  html += "    ";
  html += "    updateSelectorPosition();";
  html += "    updateColorPreview();";
  html += "    sendColorUpdate();";
  html += "  }";
  html += "}";
  html += "";
  html += "function updateHue() {";
  html += "  currentHue = parseFloat(document.getElementById('hue').value);";
  html += "  document.getElementById('hueValue').textContent = Math.round(currentHue) + '°';";
  html += "  updateSelectorPosition();";
  html += "  updateColorPreview();";
  html += "  sendColorUpdate();";
  html += "}";
  html += "";
  html += "function updateSaturation() {";
  html += "  currentSaturation = parseFloat(document.getElementById('saturation').value);";
  html += "  document.getElementById('saturationValue').textContent = Math.round(currentSaturation) + '%';";
  html += "  updateSelectorPosition();";
  html += "  updateColorPreview();";
  html += "  sendColorUpdate();";
  html += "}";
  html += "";
  html += "function updateValue() {";
  html += "  currentValue = parseFloat(document.getElementById('value').value);";
  html += "  document.getElementById('valueValue').textContent = Math.round(currentValue) + '%';";
  html += "  updateColorPreview();";
  html += "  sendColorUpdate();";
  html += "}";
  html += "";
  html += "function updateBrightness() {";
  html += "  currentBrightness = parseFloat(document.getElementById('brightness').value);";
  html += "  document.getElementById('brightnessValue').textContent = Math.round(currentBrightness) + '%';";
  html += "  sendColorUpdate();";
  html += "}";
  html += "";
  html += "function sendColorUpdate() {";
  html += "  const xhr = new XMLHttpRequest();";
  html += "  const url = '/api/control?hue=' + currentHue + '&saturation=' + currentSaturation + '&value=' + currentValue + '&brightness=' + currentBrightness;";
  html += "  xhr.open('GET', url, true);";
  html += "  xhr.send();";
  html += "  updateStatus();";
  html += "}";
  html += "";
  html += "function toggleRgb() {";
  html += "  const button = document.getElementById('powerButton');";
  html += "  const xhr = new XMLHttpRequest();";
  html += "  if (rgbEnabled) {";
  html += "    xhr.open('GET', '/api/control?power=off', true);";
  html += "    xhr.send();";
  html += "    button.classList.add('off');";
  html += "    button.textContent = '开启RGB';";
  html += "    rgbEnabled = false;";
  html += "  } else {";
  html += "    xhr.open('GET', '/api/control?power=on', true);";
  html += "    xhr.send();";
  html += "    button.classList.remove('off');";
  html += "    button.textContent = '关闭RGB';";
  html += "    rgbEnabled = true;";
  html += "  }";
  html += "}";
  html += "";
  html += "// 取色器功能";
  html += "function initColorPicker() {";
  html += "  colorPickerCanvas = document.getElementById('colorPickerCanvas');";
  html += "  colorPickerCtx = colorPickerCanvas.getContext('2d');";
  html += "  ";
  html += "  // 创建彩虹渐变背景";
  html += "  const gradient = colorPickerCtx.createLinearGradient(0, 0, colorPickerCanvas.width, 0);";
  html += "  gradient.addColorStop(0, 'red');";
  html += "  gradient.addColorStop(0.17, 'orange');";
  html += "  gradient.addColorStop(0.33, 'yellow');";
  html += "  gradient.addColorStop(0.5, 'green');";
  html += "  gradient.addColorStop(0.67, 'blue');";
  html += "  gradient.addColorStop(0.83, 'indigo');";
  html += "  gradient.addColorStop(1, 'violet');";
  html += "  ";
  html += "  colorPickerCtx.fillStyle = gradient;";
  html += "  colorPickerCtx.fillRect(0, 0, colorPickerCanvas.width, colorPickerCanvas.height);";
  html += "  ";
  html += "  // 添加亮度渐变";
  html += "  const brightnessGradient = colorPickerCtx.createLinearGradient(0, 0, 0, colorPickerCanvas.height);";
  html += "  brightnessGradient.addColorStop(0, 'rgba(255,255,255,0)');";
  html += "  brightnessGradient.addColorStop(0.5, 'rgba(128,128,128,0.5)');";
  html += "  brightnessGradient.addColorStop(1, 'rgba(0,0,0,1)');";
  html += "  ";
  html += "  colorPickerCtx.fillStyle = brightnessGradient;";
  html += "  colorPickerCtx.fillRect(0, 0, colorPickerCanvas.width, colorPickerCanvas.height);";
  html += "}";
  html += "";
  html += "function toggleColorPicker() {";
  html += "  const pickerSection = document.getElementById('colorPickerSection');";
  html += "  const pickerBtn = document.getElementById('colorPickerBtn');";
  html += "  ";
  html += "  if (colorPickerActive) {";
  html += "    pickerSection.style.display = 'none';";
  html += "    pickerBtn.textContent = '开启取色器';";
  html += "    colorPickerActive = false;";
  html += "  } else {";
  html += "    pickerSection.style.display = 'block';";
  html += "    pickerBtn.textContent = '关闭取色器';";
  html += "    colorPickerActive = true;";
  html += "    initColorPicker();";
  html += "  }";
  html += "}";
  html += "";
  html += "function handleCanvasClick(event) {";
  html += "  if (!colorPickerActive) return;";
  html += "  ";
  html += "  const canvas = event.target;";
  html += "  const rect = canvas.getBoundingClientRect();";
  html += "  const x = event.clientX - rect.left;";
  html += "  const y = event.clientY - rect.top;";
  html += "  ";
  html += "  // 获取点击位置的颜色";
  html += "  const pixel = colorPickerCtx.getImageData(x, y, 1, 1).data;";
  html += "  const r = pixel[0];";
  html += "  const g = pixel[1];";
  html += "  const b = pixel[2];";
  html += "  ";
  html += "  // 将RGB转换为HSV";
  html += "  const hsv = rgbToHsv(r, g, b);";
  html += "  ";
  html += "  // 更新当前颜色设置";
  html += "  currentHue = hsv.h;";
  html += "  currentSaturation = hsv.s * 100;";
  html += "  currentValue = hsv.v * 100;";
  html += "  ";
  html += "  // 更新UI";
  html += "  document.getElementById('hue').value = currentHue;";
  html += "  document.getElementById('hueValue').textContent = Math.round(currentHue) + '°';";
  html += "  document.getElementById('saturation').value = currentSaturation;";
  html += "  document.getElementById('saturationValue').textContent = Math.round(currentSaturation) + '%';";
  html += "  document.getElementById('value').value = currentValue;";
  html += "  document.getElementById('valueValue').textContent = Math.round(currentValue) + '%';";
  html += "  ";
  html += "  // 更新选择器位置和预览";
  html += "  updateSelectorPosition();";
  html += "  updateColorPreview();";
  html += "  ";
  html += "  // 发送颜色更新到设备";
  html += "  sendColorUpdate();";
  html += "  ";
  html += "  // 显示取色信息";
  html += "  const info = document.getElementById('colorPickerInfo');";
  html += "  info.textContent = `取色成功! RGB: ${r}, ${g}, ${b} | HSV: ${Math.round(hsv.h)}°, ${Math.round(hsv.s * 100)}%, ${Math.round(hsv.v * 100)}%`;";
  html += "}";
  html += "";
  html += "function rgbToHsv(r, g, b) {";
  html += "  r /= 255;";
  html += "  g /= 255;";
  html += "  b /= 255;";
  html += "  ";
  html += "  const max = Math.max(r, g, b);";
  html += "  const min = Math.min(r, g, b);";
  html += "  const delta = max - min;";
  html += "  ";
  html += "  let h = 0;";
  html += "  let s = 0;";
  html += "  let v = max;";
  html += "  ";
  html += "  if (delta !== 0) {";
  html += "    s = delta / max;";
  html += "    ";
  html += "    if (max === r) {";
  html += "      h = ((g - b) / delta) % 6;";
  html += "    } else if (max === g) {";
  html += "      h = (b - r) / delta + 2;";
  html += "    } else {";
  html += "      h = (r - g) / delta + 4;";
  html += "    }";
  html += "    ";
  html += "    h *= 60;";
  html += "    if (h < 0) h += 360;";
  html += "  }";
  html += "  ";
  html += "  return { h: h, s: s, v: v };";
  html += "}";
  html += "";
  html += "function updateStatus() {";
  html += "  const xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/api/info', true);";
  html += "  xhr.onreadystatechange = function() {";
  html += "    if (xhr.readyState === 4 && xhr.status === 200) {";
  html += "      const data = JSON.parse(xhr.responseText);";
  html += "      let statusHtml = '<p><strong>RGB状态:</strong> ' + (data.rgb_enabled ? '开启' : '关闭') + '</p>';";
  html += "      statusHtml += '<p><strong>HSV模式:</strong> 色相 ' + Math.round(data.hsv_hue) + '度, 饱和度 ' + Math.round(data.hsv_saturation) + '%, 明度 ' + Math.round(data.hsv_value) + '%</p>';";
  html += "      statusHtml += '<p><strong>亮度:</strong> ' + data.rgb_brightness + '%</p>';";
  html += "      document.getElementById('status').innerHTML = statusHtml;";
  html += "      ";
  html += "      // 更新本地变量";
  html += "      currentHue = data.hsv_hue;";
  html += "      currentSaturation = data.hsv_saturation;";
  html += "      currentValue = data.hsv_value;";
  html += "      currentBrightness = data.rgb_brightness;";
  html += "      rgbEnabled = data.rgb_enabled;";
  html += "      ";
  html += "      // 更新UI";
  html += "      document.getElementById('hue').value = currentHue;";
  html += "      document.getElementById('hueValue').textContent = Math.round(currentHue) + '°';";
  html += "      document.getElementById('saturation').value = currentSaturation;";
  html += "      document.getElementById('saturationValue').textContent = Math.round(currentSaturation) + '%';";
  html += "      document.getElementById('value').value = currentValue;";
  html += "      document.getElementById('valueValue').textContent = Math.round(currentValue) + '%';";
  html += "      document.getElementById('brightness').value = currentBrightness;";
  html += "      document.getElementById('brightnessValue').textContent = Math.round(currentBrightness) + '%';";
  html += "      ";
  html += "      const powerButton = document.getElementById('powerButton');";
  html += "      if (rgbEnabled) {";
  html += "        powerButton.classList.remove('off');";
  html += "        powerButton.textContent = '关闭RGB';";
  html += "      } else {";
  html += "        powerButton.classList.add('off');";
  html += "        powerButton.textContent = '开启RGB';";
  html += "      }";
  html += "      ";
  html += "      updateSelectorPosition();";
  html += "      updateColorPreview();";
  html += "    }";
  html += "  };";
  html += "  xhr.send();";
  html += "}";
  html += "";
  html += "window.onload = function() {";
  html += "  updateSelectorPosition();";
  html += "  updateColorPreview();";
  html += "  updateStatus();";
  html += "  setInterval(updateStatus, 2000);";
  html += "};";
  html += "</script>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>HSV调光板</h1>";
  html += "<div class='status' id='status'>加载中...</div>";
  html += "";
  html += "<button id='powerButton' class='power-btn " + String(rgbEnabled ? "" : "off") + "' onclick='toggleRgb()'>" + String(rgbEnabled ? "关闭RGB" : "开启RGB") + "</button>";
  html += "";
  html += "<div class='hsv-circle' id='hsvCircle' onclick='handleCircleClick(event)'>";
  html += "  <div class='hsv-selector' id='hsvSelector'></div>";
  html += "</div>";
  html += "";
  html += "<div class='color-preview' id='colorPreview'></div>";
  html += "";
  html += "<div style='margin: 20px 0;'>";
  html += "  <button id='colorPickerBtn' class='color-picker-btn' onclick='toggleColorPicker()'>开启取色器</button>";
  html += "</div>";
  html += "";
  html += "<div id='colorPickerSection' class='color-picker-section' style='display: none;'>";
  html += "  <h3>取色器</h3>";
  html += "  <p class='color-picker-info'>点击下方色板中的任意位置来拾取颜色</p>";
  html += "  <canvas id='colorPickerCanvas' class='color-picker-canvas' onclick='handleCanvasClick(event)'></canvas>";
  html += "  <p id='colorPickerInfo' class='color-picker-info'>等待取色...</p>";
  html += "</div>";
  html += "";
  html += "<div class='hsv-controls'>";
  html += "  <div>";
  html += "    <p><strong>色相 (0-360°):</strong> <span id='hueValue'>" + String(hsvHue) + "°</span></p>";
  html += "    <input type='range' id='hue' class='hsv-slider hue-slider' min='0' max='360' value='" + String(hsvHue) + "' oninput='updateHue()'>";
  html += "  </div>";
  html += "  <div>";
  html += "    <p><strong>饱和度 (0-100%):</strong> <span id='saturationValue'>" + String(hsvSaturation) + "%</span></p>";
  html += "    <input type='range' id='saturation' class='hsv-slider saturation-slider' min='0' max='100' value='" + String(hsvSaturation) + "' oninput='updateSaturation()'>";
  html += "  </div>";
  html += "  <div>";
  html += "    <p><strong>明度 (0-100%):</strong> <span id='valueValue'>" + String(hsvValue) + "%</span></p>";
  html += "    <input type='range' id='value' class='hsv-slider value-slider' min='0' max='100' value='" + String(hsvValue) + "' oninput='updateValue()'>";
  html += "  </div>";
  html += "  <div>";
  html += "    <p><strong>亮度 (0-100%):</strong> <span id='brightnessValue'>" + String(rgbBrightness) + "%</span></p>";
  html += "    <input type='range' id='brightness' class='hsv-slider brightness-slider' min='0' max='100' value='" + String(rgbBrightness) + "' oninput='updateBrightness()'>";
  html += "  </div>";
  html += "</div>";
  html += "";
  html += "<p><a class='button' href='/control'>返回RGB控制</a></p>";
  html += "<p><a class='button' href='/'>返回首页</a></p>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

