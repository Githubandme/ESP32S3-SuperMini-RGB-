#ifndef RGB_LED_H
#define RGB_LED_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class RGB_LED
{
public:
    // 构造函数
    RGB_LED(uint16_t numPixels, uint16_t pin);

    // 初始化LED灯带
    void begin();

    // 启动独立任务
    void startTask();

    // 停止任务
    void stopTask();

    // 设置颜色（带亮度参数） io,r,g,b,亮度
    void setColor(uint8_t id, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness = 100);

    // RGB设置完成，打包发送数据
    void rgb_show();

    // 直达任务通知
    void set_rgb(uint32_t value); // 直达任务通知，传入新数据
    
    // LED控制函数
    void set_led(bool state);
    void set_led_brightness(uint8_t brightness);

private:
    Adafruit_NeoPixel strip;
    uint16_t _numPixels;     // 灯珠个数
    TaskHandle_t taskHandle; // 每个实例独立的任务句柄
    bool led_state;
    uint8_t led_brightness;

    // 颜色轮计算
    uint32_t Wheel(byte WheelPos);
    bool rgb_rainbow;

    // 提取通知处理为独立函数
    bool handleNotification(uint32_t value);

    // 提取通知检查为独立函数
    bool checkForNotification(uint32_t *value);

    void LED_vTaskDelay(int d);

    static void LED_RGB_Function(void *param); // 静态任务函数
};

#endif