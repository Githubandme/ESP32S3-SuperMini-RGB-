# ESP32S3 SuperMini 智能灯光控制系统

本项目为ESP32S3 SuperMini开发板设计的完整智能灯光控制系统，支持多种配网方式和丰富的控制功能，为安卓App开发提供完整的技术基础。

## 🚀 功能特性

### 1. **多模式配网系统**
- **Web热点配网** - 自动创建WiFi热点供用户配置
- **BLE蓝牙配网** - 支持蓝牙低功耗配网，无需连接WiFi
- **UDP广播发现** - 自动广播设备信息，便于局域网发现

### 2. **高级RGB灯光控制**
- **预设颜色模式** - 7种预设颜色 + 彩虹模式
- **HSV调光板** - 完整的HSV色彩模型控制
- **智能取色器** - 从色板中实时拾取任意颜色
- **亮度调节** - 0-100%无级亮度控制
- **状态同步** - 多界面实时同步灯光状态

### 3. **设备管理与发现**
- **设备识别** - 独特的设备ID和名称标识
- **状态监控** - 实时获取设备状态信息
- **远程控制** - 完整的HTTP API接口
- **自动重连** - WiFi断线自动重连机制

### 4. **用户交互界面**
- **响应式Web界面** - 适配手机和电脑浏览器
- **实时预览** - 颜色和亮度实时预览
- **多语言支持** - 中文界面和提示

## 📋 硬件要求

- **ESP32S3 SuperMini开发板**
- **USB数据线**（用于编程和供电）
- **WiFi网络**（2.4GHz频段）
- **安卓手机**（用于App开发和测试）

## 🔧 软件依赖

### Arduino库依赖
```cpp
#include <WiFi.h>          // WiFi连接
#include <WebServer.h>     // Web服务器
#include <Preferences.h>   // Flash存储
#include <Adafruit_NeoPixel.h> // RGB灯控制
#include <BLEDevice.h>     // BLE蓝牙
#include <BLEServer.h>     // BLE服务器
#include <BLEUtils.h>      // BLE工具
#include <BLE2902.h>       // BLE描述符
#include <WiFiUdp.h>       // UDP通信
```

### 安装步骤
1. 安装Arduino IDE（1.8.x或更高版本）
2. 配置ESP32开发板支持
3. 安装上述所有依赖库
4. 选择"ESP32S3 Dev Module"开发板
5. 编译并上传代码

## 🎯 使用说明

### 1. **初始配网流程**
```
设备上电 → 创建热点(ESP32_Config) → 连接热点 → 访问192.168.4.1 → 配置WiFi → 自动连接
```

### 2. **LED状态指示**
- **快速闪烁(200ms)**：等待WiFi配置
- **慢速闪烁(1000ms)**：正在连接WiFi  
- **常亮**：WiFi连接成功
- **蓝色闪烁**：BLE设备已连接

### 3. **按键操作**
- **短按GPIO0**：启用UDP广播（WiFi连接状态下）
- **长按3秒以上**：清除WiFi配置，重新进入配网模式

## 🌐 Web控制界面

### 可用页面
- `/` - 设备首页和状态信息
- `/control` - RGB灯光控制界面
- `/hsv` - HSV调光板和取色器
- `/scan` - WiFi网络扫描
- `/identify` - 设备识别
- `/broadcast` - UDP广播控制
- `/status` - 设备状态信息

### API接口
- `/api/info` - 获取设备信息（JSON格式）
- `/api/control` - 控制RGB灯光
- `/api/discover` - 设备发现接口
- `/api/broadcast` - UDP广播控制

## 📡 通信协议

### 1. **HTTP API接口**

#### 获取设备信息
```http
GET /api/info
Response: {
  "device_id": "ESP32_123456",
  "device_name": "ESP32灯光控制器",
  "ip_address": "192.168.1.100",
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "wifi_status": true,
  "rgb_enabled": true,
  "rgb_color": 1,
  "rgb_brightness": 80,
  "hsv_mode": false,
  "hsv_hue": 180.0,
  "hsv_saturation": 100.0,
  "hsv_value": 100.0,
  "broadcast_enabled": false
}
```

#### 控制RGB灯光
```http
# 开关控制
GET /api/control?power=on
GET /api/control?power=off

# 颜色控制（0=彩虹,1=红,2=橙,3=黄,4=绿,5=青,6=蓝,7=紫）
GET /api/control?color=1

# HSV模式控制
GET /api/control?hue=180&saturation=100&value=100

# 亮度控制
GET /api/control?brightness=80

# 组合控制
GET /api/control?power=on&color=1&brightness=80
```

#### UDP广播控制
```http
GET /api/broadcast?action=enable
GET /api/broadcast?action=disable
```

### 2. **UDP广播协议**
设备启用广播后，每5秒发送一次设备信息：
```json
{
  "device_id": "ESP32_123456",
  "device_name": "ESP32灯光控制器", 
  "ip_address": "192.168.1.100",
  "firmware_version": "1.0"
}
```

**广播地址**：`224.0.0.1:8888`
**超时时间**：10分钟自动关闭

### 3. **BLE蓝牙配网协议**

#### BLE服务UUID
- **服务UUID**: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- **WiFi配置特征**: `beb5483e-36e1-4688-b7f5-ea07361b26a8`（读写）
- **状态特征**: `86522b20-183f-4f43-985b-d00c750ed3ff`（读+通知）

#### 配网数据格式
```json
{
  "ssid": "你的WiFi名称",
  "password": "你的WiFi密码"
}
```

## 🎨 HSV调光板功能

### 核心特性
- **圆形色相选择器** - 360°色相环直观选择
- **实时颜色预览** - 当前颜色实时显示
- **取色器功能** - 从彩虹色板中拾取任意颜色
- **参数同步** - HSV值与RGB值实时转换

### 技术实现
- **HSV转RGB算法** - 精确的色彩空间转换
- **Canvas取色** - HTML5 Canvas实现取色功能
- **实时通信** - WebSocket风格的实时更新

## 🔌 安卓App开发指南

### 1. **设备发现流程**
```java
// 方法1: UDP广播发现
UDP监听224.0.0.1:8888 → 接收设备广播 → 解析设备信息

// 方法2: HTTP主动发现
访问http://[设备IP]/api/discover → 获取设备信息

// 方法3: BLE扫描
扫描BLE设备 → 过滤设备名称(ESP32-XXXX) → 连接配网
```

### 2. **控制流程设计**
```java
// 1. 设备发现和连接
// 2. 获取设备状态
// 3. 显示控制界面
// 4. 发送控制指令
// 5. 实时状态同步
```

### 3. **推荐架构**
- **MVP/MVVM架构** - 分离业务逻辑和UI
- **Retrofit2** - HTTP API调用
- **RxJava** - 异步操作处理
- **Room** - 本地设备存储
- **WorkManager** - 后台任务管理

### 4. **关键功能模块**

#### 设备管理模块
- 设备扫描和发现
- 设备连接状态管理
- 设备信息缓存
- 多设备支持

#### 灯光控制模块
- 颜色选择器（HSV/RGB）
- 亮度调节滑块
- 预设颜色快捷选择
- 场景模式管理

#### 网络通信模块
- HTTP请求封装
- UDP广播监听
- BLE连接管理
- 错误重试机制

## 🛠️ 技术细节

### IO分时复用技术
由于ESP32S3 SuperMini的RGB灯和LED共用IO48引脚，采用分时复用：
1. **RGB控制阶段**：NeoPixel协议发送颜色数据
2. **IO释放阶段**：RGB灯保持颜色
3. **LED控制阶段**：PWM控制LED亮度
4. **20ms周期切换**：利用视觉暂留实现"同时"控制

### 颜色空间转换
```cpp
// HSV转RGB算法
void hsvToRgb(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b) {
    h = fmod(h, 360.0);
    float c = v * s;
    float x = c * (1 - abs(fmod(h / 60.0, 2) - 1));
    float m = v - c;
    
    if (h < 60) { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }
    
    r = (r + m) * 255;
    g = (g + m) * 255;
    b = (b + m) * 255;
}
```

## 🐛 故障排除

### 常见问题
1. **无法连接WiFi**
   - 检查2.4GHz网络
   - 确认密码正确
   - 检查信号强度

2. **RGB灯不响应**
   - 确认RGB功能已开启
   - 检查亮度设置
   - 验证颜色模式

3. **设备无法发现**
   - 检查UDP广播状态
   - 确认在同一局域网
   - 验证防火墙设置

### 调试工具
- **串口监视器**：查看设备日志
- **网络扫描工具**：发现设备IP
- **BLE扫描工具**：调试蓝牙连接

## 🔧 定制化配置

### 可配置参数
```cpp
// 网络配置
const char* ssid_ap = "ESP32_Config";
const char* password_ap = "12345678";

// 设备标识
String deviceID = "ESP32_" + String((uint32_t)ESP.getEfuseMac(), HEX);
String deviceName = "ESP32灯光控制器";

// 控制参数
unsigned long buttonLongPressDuration = 3000; // 长按时间(ms)
unsigned long fastInterval = 200;             // 快闪间隔
unsigned long slowInterval = 1000;           // 慢闪间隔
uint8_t rgbBrightness = 50;                   // 默认亮度
```

## 📊 性能指标

- **响应时间**：<100ms（控制指令）
- **连接稳定性**：自动重连机制
- **内存使用**：优化后的内存管理
- **功耗控制**：低功耗待机模式

## 🔮 扩展功能建议

### 短期扩展
- OTA固件升级
- 定时任务功能
- 分组控制支持
- 情景模式存储

### 长期规划
- 语音控制集成
- AI颜色推荐
- 音乐律动模式
- 云端同步

## 📄 版权信息

本项目基于MIT许可证发布，欢迎二次开发和商业使用。

---

**文档版本**：v2.0  
**最后更新**：2025-11-06  
**适用设备**：ESP32S3 SuperMini  
**目标平台**：Android App开发