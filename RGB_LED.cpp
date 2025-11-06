#include "RGB_LED.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>

// 定义颜色常量 (R, G, B)
#define COLOR_RED {255, 0, 0}      // 红色
#define COLOR_ORANGE {255, 140, 0} // 橙色
#define COLOR_YELLOW {255, 255, 0} // 黄色
#define COLOR_GREEN {0, 255, 0}    // 绿色
#define COLOR_CYAN {0, 128, 128}   // 青色
#define COLOR_BLUE {0, 0, 255}     // 蓝色
#define COLOR_PURPLE {128, 0, 128} // 紫色

// 储存颜色的变量
uint8_t colors[7][3] = {
    COLOR_RED,
    COLOR_ORANGE,
    COLOR_YELLOW,
    COLOR_GREEN,
    COLOR_CYAN,
    COLOR_BLUE,
    COLOR_PURPLE};

// 构造函数
RGB_LED::RGB_LED(uint16_t numPixels, uint16_t pin)
    : strip(numPixels, pin, NEO_GRB + NEO_KHZ800),
      _numPixels(numPixels),
      taskHandle(nullptr),
      led_state(false),
      led_brightness(50) {}

// 初始化LED灯带
void RGB_LED::begin()
{
    strip.begin();
    strip.show(); // 初始化为全灭
}

// 设置单个灯珠红绿蓝（带亮度参数） 灯珠编号,r,g,b,亮度
void RGB_LED::setColor(uint8_t id, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    // 参数校验
    if (id < 0 || id >= _numPixels)
    {
        // id不正确
        return;
    }
    brightness = constrain(brightness, 0, 100);
    r = constrain(r, 0, 255);
    g = constrain(g, 0, 255);
    b = constrain(b, 0, 255);

    // 优化亮度计算（减少整数运算精度损失）
    uint16_t scaledBrightness = brightness * 255 / 100;
    r = (r * scaledBrightness) / 255;
    g = (g * scaledBrightness) / 255;
    b = (b * scaledBrightness) / 255;
    strip.setPixelColor(id, strip.Color(r, g, b));
}

void RGB_LED::rgb_show() // 发送亮度参数
{
    strip.show();
}

// 颜色轮计算（私有方法）
uint32_t RGB_LED::Wheel(byte WheelPos)
{
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85)
    {
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    else if (WheelPos < 170)
    {
        WheelPos -= 85;
        return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    else
    {
        WheelPos -= 170;
        return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    }
}

// 启动自动变换 + 直达任务通知
void RGB_LED::startTask()
{
    if (taskHandle == nullptr)
    {
        xTaskCreatePinnedToCore(
            LED_RGB_Function, // 任务函数
            "LED_Task",       // 任务名称
            3 * 1024,         // 堆栈大小
            this,             // 传递this指针作为参数
            3,                // 优先级
            &taskHandle,      // 存储任务句柄
            1  // 核心
        );
    }
}

// 停止任务
void RGB_LED::stopTask()
{
    if (taskHandle != nullptr)
    {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
}

void RGB_LED::LED_RGB_Function(void *param)
{
    RGB_LED *instance = static_cast<RGB_LED *>(param); // 传入 类 里面的参数给 instance 引用

    // 等待任务完全初始化（Arduino 中任务创建有延迟）
    vTaskDelay(pdMS_TO_TICKS(10));

    // 直达任务通知
    uint32_t value = 0;
    // 传入直达任务初始值
    xTaskNotify(instance->taskHandle, value, eSetValueWithoutOverwrite);
    instance->rgb_rainbow = true; // 开启七彩

    while (1)
    {

        if (instance->checkForNotification(&value))
        {
            instance->handleNotification(value);
        }

        // 彩虹动画（优化循环结构并添加适当延时）
        if (instance->rgb_rainbow)
        {
            for (uint16_t j = 0; j < 256; j++)
            {
                for (uint16_t i = 0; i < instance->_numPixels; i++)
                {
                    instance->strip.setPixelColor(i, instance->Wheel((i + j) & 255));
                }
                instance->strip.show();

                // 检查通知并处理
                if (instance->checkForNotification(&value))
                {
                    if (instance->handleNotification(value))
                    {
                        break; // 退出彩虹动画
                    }
                }
                instance->LED_vTaskDelay(20); // 延时
            }
        }
        else
        {
            instance->LED_vTaskDelay(20); // 延时
        }

        vTaskDelay(pdMS_TO_TICKS(1)); // 预防卡死
    }
}

// 直达任务通知，传入新数据
void RGB_LED::set_rgb(uint32_t value)
{
    xTaskNotify(taskHandle, value, eSetValueWithoutOverwrite); // 任务句柄，值，直达任务
}

// 提取通知检查为独立函数
bool RGB_LED::checkForNotification(uint32_t *value)
{
    BaseType_t result = xTaskNotifyWait(
        0x00,      // 不清除位
        ULONG_MAX, // 退出时清除所有位
        value,     // 存储通知值
        0          // 不等待，立即返回
    );
    return (result == pdTRUE);
}

// 提取通知处理为独立函数
bool RGB_LED::handleNotification(uint32_t value)
{
    if (value >= 1 && value <= 7)
    {
        setColor(
            0,
            colors[value - 1][0],
            colors[value - 1][1],
            colors[value - 1][2]);
        rgb_show();
        rgb_rainbow = false;
        return true; // 通知导致动画中断
    }
    else
    {
        rgb_rainbow = true;
        return false; // 通知不影响当前动画
    }
}

void RGB_LED::LED_vTaskDelay(int d)
{
    // 共用的板载LED灯初始化

    pinMode(strip.getPin(), INPUT); // 释放引脚
    
    // 使用Arduino标准的analogWrite函数实现LED亮度控制
    int fc = 2;                        // 细分时间

    for (size_t i = 0; i < fc; i++)
    {
        static int jb = 255 / 100; // 100步长
        static int minjb = jb * 2; // 最低亮度
        static int maxjb = 200;    // 最高亮度
        static int direction = 1;  // 1表示递增，-1表示递减
        static int led_v = 0;

        // 根据方向调整值
        led_v += direction * jb;

        // 检查是否需要改变方向
        if (led_v >= maxjb)
        {
            led_v = maxjb; // 确保不会超过最大亮度
            direction = -1;
        }
        else if (led_v <= minjb) // 不要完全熄灭
        {
            led_v = minjb; // 不要完全熄灭
            direction = 1;
        }
        
        // 使用analogWrite控制LED亮度
        analogWrite(strip.getPin(), led_v);
        
        // 延迟
        vTaskDelay(pdMS_TO_TICKS(d / fc));
    }

    // 恢复NeoPixel控制
    analogWrite(strip.getPin(), 0); // 关闭PWM输出
    pinMode(strip.getPin(), OUTPUT);
}

// 设置LED状态
void RGB_LED::set_led(bool state) {
    led_state = state;
    if (!state) {
        setColor(0, 0, 0, 0, 0); // 关闭LED
        rgb_show();
    }
}

// 设置LED亮度
void RGB_LED::set_led_brightness(uint8_t brightness) {
    led_brightness = brightness;
    if (led_state) {
        setColor(0, 255, 255, 255, brightness); // 白光
        rgb_show();
    }
}