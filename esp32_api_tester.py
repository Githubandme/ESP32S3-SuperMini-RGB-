#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ESP32S3 SuperMini API功能测试程序
基于ttkbootstrap的现代化GUI测试工具

功能特性：
1. 设备发现和连接管理
2. RGB灯光控制测试
3. HSV调光板测试
4. UDP广播测试
5. 设备状态监控
6. 测试报告生成

作者: ESP32开发团队
版本: 1.0.0
日期: 2025-11-06
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import ttkbootstrap as ttkb
from ttkbootstrap.constants import *
import requests
import socket
import threading
import json
import time
from datetime import datetime
import os
import math
import random

class ESP32APITester:
    def __init__(self):
        self.root = ttkb.Window(
            title="ESP32S3 SuperMini API测试工具",
            themename="darkly",
            size=(1200, 800),
            resizable=(True, True)
        )
        self.root.iconbitmap(default="")
        
        # 设备连接状态
        self.device_ip = ""
        self.connected = False
        self.device_info = {}
        
        # UDP广播监听
        self.udp_listening = False
        self.udp_thread = None
        
        # 测试结果记录
        self.test_results = []
        
        self.setup_ui()
        
    def setup_ui(self):
        """设置用户界面"""
        # 创建主框架
        main_frame = ttkb.Frame(self.root, padding=10)
        main_frame.pack(fill=BOTH, expand=True)
        
        # 创建标签页
        notebook = ttkb.Notebook(main_frame)
        notebook.pack(fill=BOTH, expand=True)
        
        # 设备连接标签页
        self.setup_connection_tab(notebook)
        
        # RGB控制标签页
        self.setup_rgb_tab(notebook)
        
        # HSV调光板标签页
        self.setup_hsv_tab(notebook)
        
        # UDP广播标签页
        self.setup_udp_tab(notebook)
        
        # 测试报告标签页
        self.setup_report_tab(notebook)
        
    def setup_connection_tab(self, notebook):
        """设备连接标签页"""
        connection_frame = ttkb.Frame(notebook, padding=10)
        notebook.add(connection_frame, text="设备连接")
        
        # 设备发现区域
        discovery_frame = ttkb.Labelframe(connection_frame, text="设备发现", padding=10)
        discovery_frame.pack(fill=X, pady=5)
        
        ttkb.Button(discovery_frame, text="扫描局域网设备", 
                    command=self.scan_network, bootstyle=PRIMARY).pack(side=LEFT, padx=5)
        ttkb.Button(discovery_frame, text="开始UDP监听", 
                    command=self.start_udp_listener, bootstyle=SUCCESS).pack(side=LEFT, padx=5)
        ttkb.Button(discovery_frame, text="停止UDP监听", 
                    command=self.stop_udp_listener, bootstyle=DANGER).pack(side=LEFT, padx=5)
        
        # 设备列表
        devices_frame = ttkb.Frame(connection_frame)
        devices_frame.pack(fill=BOTH, expand=True, pady=5)
        
        # 设备列表树状视图
        columns = ("IP地址", "设备ID", "设备名称", "状态")
        self.device_tree = ttkb.Treeview(devices_frame, columns=columns, show="headings", height=8)
        
        for col in columns:
            self.device_tree.heading(col, text=col)
            self.device_tree.column(col, width=150)
        
        self.device_tree.pack(fill=BOTH, expand=True, side=LEFT)
        
        # 滚动条
        scrollbar = ttkb.Scrollbar(devices_frame, orient=VERTICAL, command=self.device_tree.yview)
        scrollbar.pack(side=RIGHT, fill=Y)
        self.device_tree.configure(yscrollcommand=scrollbar.set)
        
        # 设备连接区域
        connect_frame = ttkb.Labelframe(connection_frame, text="设备连接", padding=10)
        connect_frame.pack(fill=X, pady=5)
        
        ttkb.Label(connect_frame, text="设备IP地址:").grid(row=0, column=0, sticky=W, padx=5)
        self.ip_entry = ttkb.Entry(connect_frame, width=20)
        self.ip_entry.grid(row=0, column=1, padx=5)
        
        ttkb.Button(connect_frame, text="连接设备", 
                   command=self.connect_device, bootstyle=PRIMARY).grid(row=0, column=2, padx=5)
        ttkb.Button(connect_frame, text="断开连接", 
                   command=self.disconnect_device, bootstyle=DANGER).grid(row=0, column=3, padx=5)
        
        # 设备信息显示
        info_frame = ttkb.Labelframe(connection_frame, text="设备信息", padding=10)
        info_frame.pack(fill=X, pady=5)
        
        self.info_text = scrolledtext.ScrolledText(info_frame, height=8, width=80)
        self.info_text.pack(fill=BOTH, expand=True)
        self.info_text.config(state=DISABLED)
        
        # 绑定设备选择事件
        self.device_tree.bind("<<TreeviewSelect>>", self.on_device_select)
        
    def setup_rgb_tab(self, notebook):
        """RGB控制标签页"""
        rgb_frame = ttkb.Frame(notebook, padding=10)
        notebook.add(rgb_frame, text="RGB控制")
        
        # 电源控制
        power_frame = ttkb.Labelframe(rgb_frame, text="电源控制", padding=10)
        power_frame.pack(fill=X, pady=5)
        
        self.power_var = tk.BooleanVar()
        ttkb.Checkbutton(power_frame, text="开启RGB灯", variable=self.power_var,
                        command=self.toggle_power, bootstyle="round-toggle").pack(side=LEFT, padx=5)
        
        # 预设颜色
        colors_frame = ttkb.Labelframe(rgb_frame, text="预设颜色", padding=10)
        colors_frame.pack(fill=X, pady=5)
        
        colors = [
            ("彩虹", 0, "primary"),
            ("红色", 1, "danger"),
            ("橙色", 2, "warning"),
            ("黄色", 3, "warning"),
            ("绿色", 4, "success"),
            ("青色", 5, "info"),
            ("蓝色", 6, "primary"),
            ("紫色", 7, "secondary")
        ]
        
        for i, (name, value, style) in enumerate(colors):
            ttkb.Button(colors_frame, text=name, bootstyle=style,
                       command=lambda v=value: self.set_color(v)).grid(row=i//4, column=i%4, padx=5, pady=2)
        
        # 亮度控制
        brightness_frame = ttkb.Labelframe(rgb_frame, text="亮度控制", padding=10)
        brightness_frame.pack(fill=X, pady=5)
        
        ttkb.Label(brightness_frame, text="亮度:").pack(side=LEFT, padx=5)
        self.brightness_var = tk.IntVar(value=50)
        brightness_scale = ttkb.Scale(brightness_frame, from_=0, to=100, 
                                     variable=self.brightness_var, orient=HORIZONTAL,
                                     command=self.update_brightness)
        brightness_scale.pack(side=LEFT, fill=X, expand=True, padx=5)
        
        self.brightness_label = ttkb.Label(brightness_frame, text="50%")
        self.brightness_label.pack(side=LEFT, padx=5)
        
        # 颜色预览
        preview_frame = ttkb.Labelframe(rgb_frame, text="颜色预览", padding=10)
        preview_frame.pack(fill=BOTH, expand=True, pady=5)
        
        self.color_preview = tk.Canvas(preview_frame, width=200, height=200, bg="#808080")
        self.color_preview.pack(pady=10)
        
        # 测试按钮
        test_frame = ttkb.Frame(rgb_frame)
        test_frame.pack(fill=X, pady=5)
        
        ttkb.Button(test_frame, text="测试所有颜色", 
                   command=self.test_all_colors, bootstyle=INFO).pack(side=LEFT, padx=5)
        ttkb.Button(test_frame, text="彩虹渐变测试", 
                   command=self.rainbow_test, bootstyle=PRIMARY).pack(side=LEFT, padx=5)
        
    def setup_hsv_tab(self, notebook):
        """HSV调光板标签页 - 优化版"""
        hsv_frame = ttkb.Frame(notebook, padding=15)
        notebook.add(hsv_frame, text="🎨 HSV调光板")
        
        # 创建主容器 - 使用更现代的布局
        main_container = ttkb.Frame(hsv_frame)
        main_container.pack(fill=BOTH, expand=True)
        
        # 顶部控制栏
        top_bar = ttkb.Frame(main_container)
        top_bar.pack(fill=X, pady=(0, 10))
        
        # 电源控制
        self.hsv_power_var = tk.BooleanVar(value=True)
        power_switch = ttkb.Checkbutton(top_bar, text="💡 开启RGB灯", variable=self.hsv_power_var,
                                       command=self.toggle_hsv_power, bootstyle="round-toggle")
        power_switch.pack(side=LEFT, padx=(0, 20))
        
        # 快速操作按钮
        ttkb.Button(top_bar, text="🌈 彩虹渐变", 
                   command=self.hsv_gradient_test, bootstyle="outline-primary").pack(side=LEFT, padx=5)
        ttkb.Button(top_bar, text="🎲 随机颜色", 
                   command=self.random_color_test, bootstyle="outline-success").pack(side=LEFT, padx=5)
        ttkb.Button(top_bar, text="🔄 重置", 
                   command=self.reset_hsv_params, bootstyle="outline-warning").pack(side=LEFT, padx=5)
        
        # 主内容区域
        content_frame = ttkb.Frame(main_container)
        content_frame.pack(fill=BOTH, expand=True)
        
        # 左侧：HSV圆形调光板（更大更美观）
        left_frame = ttkb.Frame(content_frame)
        left_frame.pack(side=LEFT, fill=BOTH, expand=True, padx=(0, 15))
        
        # HSV圆形调光板容器
        hsv_circle_container = ttkb.Frame(left_frame)
        hsv_circle_container.pack(fill=BOTH, expand=True)
        
        # 创建更大的HSV圆形调光板画布
        self.hsv_canvas = tk.Canvas(hsv_circle_container, width=400, height=400, bg="#2c3e50", 
                                   highlightthickness=0, relief="flat")
        self.hsv_canvas.pack(expand=True)
        
        # 创建更美观的选择器
        self.hsv_selector = self.hsv_canvas.create_oval(195, 195, 205, 205, 
                                                       fill="#ffffff", outline="#34495e", 
                                                       width=3, tags="selector")
        
        # 绑定鼠标事件
        self.hsv_canvas.bind("<Button-1>", self.on_hsv_circle_click)
        self.hsv_canvas.bind("<B1-Motion>", self.on_hsv_circle_drag)
        
        # 绘制HSV圆形调光板
        self.draw_hsv_circle()
        
        # 颜色预览区域
        preview_frame = ttkb.Labelframe(left_frame, text="🎯 当前颜色", padding=10)
        preview_frame.pack(fill=X, pady=(10, 0))
        
        preview_content = ttkb.Frame(preview_frame)
        preview_content.pack(fill=X)
        
        ttkb.Label(preview_content, text="预览:").pack(side=LEFT, padx=(0, 10))
        self.hsv_preview = tk.Canvas(preview_content, width=120, height=60, bg="#3498db", 
                                    highlightthickness=1, highlightbackground="#bdc3c7")
        self.hsv_preview.pack(side=LEFT, padx=(0, 20))
        
        # 颜色值显示
        self.color_value_label = ttkb.Label(preview_content, text="RGB(52, 152, 219)", 
                                           font=("Consolas", 10))
        self.color_value_label.pack(side=LEFT)
        
        # 右侧：参数控制面板
        right_frame = ttkb.Frame(content_frame)
        right_frame.pack(side=RIGHT, fill=Y, padx=(15, 0))
        
        # HSV参数控制面板
        hsv_controls_frame = ttkb.Labelframe(right_frame, text="⚙️ 参数调节", padding=15)
        hsv_controls_frame.pack(fill=X, pady=(0, 15))
        
        # 色相控制 - 使用更直观的滑块
        hue_frame = ttkb.Frame(hsv_controls_frame)
        hue_frame.pack(fill=X, pady=8)
        
        hue_header = ttkb.Frame(hue_frame)
        hue_header.pack(fill=X)
        ttkb.Label(hue_header, text="🎨 色相", font=("", 10, "bold")).pack(side=LEFT)
        self.hue_label = ttkb.Label(hue_header, text="0°", font=("Consolas", 10, "bold"), 
                                  foreground="#e74c3c")
        self.hue_label.pack(side=RIGHT)
        
        self.hue_var = tk.IntVar(value=0)
        hue_scale = ttkb.Scale(hue_frame, from_=0, to=360, variable=self.hue_var,
                              orient=HORIZONTAL, command=self.update_hsv_from_slider,
                              bootstyle="primary")
        hue_scale.pack(fill=X, pady=5)
        
        # 饱和度控制
        saturation_frame = ttkb.Frame(hsv_controls_frame)
        saturation_frame.pack(fill=X, pady=8)
        
        sat_header = ttkb.Frame(saturation_frame)
        sat_header.pack(fill=X)
        ttkb.Label(sat_header, text="🔴 饱和度", font=("", 10, "bold")).pack(side=LEFT)
        self.saturation_label = ttkb.Label(sat_header, text="100%", font=("Consolas", 10, "bold"),
                                          foreground="#e74c3c")
        self.saturation_label.pack(side=RIGHT)
        
        self.saturation_var = tk.IntVar(value=100)
        saturation_scale = ttkb.Scale(saturation_frame, from_=0, to=100, 
                                     variable=self.saturation_var, orient=HORIZONTAL,
                                     command=self.update_hsv_from_slider, bootstyle="danger")
        saturation_scale.pack(fill=X, pady=5)
        
        # 明度控制
        value_frame = ttkb.Frame(hsv_controls_frame)
        value_frame.pack(fill=X, pady=8)
        
        val_header = ttkb.Frame(value_frame)
        val_header.pack(fill=X)
        ttkb.Label(val_header, text="💡 明度", font=("", 10, "bold")).pack(side=LEFT)
        self.value_label = ttkb.Label(val_header, text="100%", font=("Consolas", 10, "bold"),
                                      foreground="#f39c12")
        self.value_label.pack(side=RIGHT)
        
        self.value_var = tk.IntVar(value=100)
        value_scale = ttkb.Scale(value_frame, from_=0, to=100, 
                               variable=self.value_var, orient=HORIZONTAL,
                               command=self.update_hsv_from_slider, bootstyle="warning")
        value_scale.pack(fill=X, pady=5)
        
        # 亮度控制
        brightness_frame = ttkb.Frame(hsv_controls_frame)
        brightness_frame.pack(fill=X, pady=8)
        
        bright_header = ttkb.Frame(brightness_frame)
        bright_header.pack(fill=X)
        ttkb.Label(bright_header, text="☀️ 亮度", font=("", 10, "bold")).pack(side=LEFT)
        self.hsv_brightness_label = ttkb.Label(bright_header, text="50%", font=("Consolas", 10, "bold"),
                                              foreground="#f39c12")
        self.hsv_brightness_label.pack(side=RIGHT)
        
        self.hsv_brightness_var = tk.IntVar(value=50)
        brightness_scale = ttkb.Scale(brightness_frame, from_=0, to=100, 
                                    variable=self.hsv_brightness_var, orient=HORIZONTAL,
                                    command=self.update_hsv_from_slider, bootstyle="warning")
        brightness_scale.pack(fill=X, pady=5)
        
        # 取色器功能
        color_picker_frame = ttkb.Labelframe(right_frame, text="🎯 快速取色", padding=15)
        color_picker_frame.pack(fill=X)
        
        # 取色器画布
        self.color_picker_canvas = tk.Canvas(color_picker_frame, width=250, height=80, 
                                            bg="#ecf0f1", highlightthickness=1, 
                                            highlightbackground="#bdc3c7")
        self.color_picker_canvas.pack(pady=10)
        
        # 绑定取色器点击事件
        self.color_picker_canvas.bind("<Button-1>", self.on_color_picker_click)
        
        # 绘制取色器
        self.draw_color_picker()
        
        ttkb.Label(color_picker_frame, text="点击色板快速选择颜色", 
                  font=("", 9), foreground="#7f8c8d").pack()
        
    def setup_udp_tab(self, notebook):
        """UDP广播标签页"""
        udp_frame = ttkb.Frame(notebook, padding=10)
        notebook.add(udp_frame, text="UDP广播")
        
        # UDP控制
        control_frame = ttkb.Labelframe(udp_frame, text="UDP广播控制", padding=10)
        control_frame.pack(fill=X, pady=5)
        
        ttkb.Button(control_frame, text="启用广播", 
                   command=lambda: self.control_broadcast("enable"), bootstyle=SUCCESS).pack(side=LEFT, padx=5)
        ttkb.Button(control_frame, text="禁用广播", 
                   command=lambda: self.control_broadcast("disable"), bootstyle=DANGER).pack(side=LEFT, padx=5)
        
        # UDP消息显示
        messages_frame = ttkb.Labelframe(udp_frame, text="UDP消息", padding=10)
        messages_frame.pack(fill=BOTH, expand=True, pady=5)
        
        self.udp_text = scrolledtext.ScrolledText(messages_frame, height=15, width=80)
        self.udp_text.pack(fill=BOTH, expand=True)
        
        # 清空按钮
        clear_frame = ttkb.Frame(udp_frame)
        clear_frame.pack(fill=X, pady=5)
        
        ttkb.Button(clear_frame, text="清空消息", 
                   command=self.clear_udp_messages, bootstyle=WARNING).pack(side=LEFT, padx=5)
        
    def setup_report_tab(self, notebook):
        """测试报告标签页"""
        report_frame = ttkb.Frame(notebook, padding=10)
        notebook.add(report_frame, text="测试报告")
        
        # 测试控制
        control_frame = ttkb.Labelframe(report_frame, text="测试控制", padding=10)
        control_frame.pack(fill=X, pady=5)
        
        ttkb.Button(control_frame, text="运行完整测试", 
                   command=self.run_full_test, bootstyle=PRIMARY).pack(side=LEFT, padx=5)
        ttkb.Button(control_frame, text="生成测试报告", 
                   command=self.generate_report, bootstyle=SUCCESS).pack(side=LEFT, padx=5)
        ttkb.Button(control_frame, text="清空测试记录", 
                   command=self.clear_test_results, bootstyle=DANGER).pack(side=LEFT, padx=5)
        
        # 测试结果
        results_frame = ttkb.Labelframe(report_frame, text="测试结果", padding=10)
        results_frame.pack(fill=BOTH, expand=True, pady=5)
        
        self.results_text = scrolledtext.ScrolledText(results_frame, height=20, width=80)
        self.results_text.pack(fill=BOTH, expand=True)
        
    # ========== 设备连接相关方法 ==========
    
    def scan_network(self):
        """扫描局域网设备"""
        self.add_test_result("开始扫描局域网设备", "信息")
        
        # 清空设备列表
        for item in self.device_tree.get_children():
            self.device_tree.delete(item)
        
        # 使用ARP扫描当前网段设备
        def arp_scan():
            import subprocess
            import re
            
            try:
                # 获取本机IP和子网掩码
                result = subprocess.run(['ipconfig'], capture_output=True, text=True)
                
                # 解析IP地址和子网掩码
                ip_match = re.search(r'IPv4 Address[^\d]*(\d+\.\d+\.\d+\.\d+)', result.stdout)
                subnet_match = re.search(r'Subnet Mask[^\d]*(\d+\.\d+\.\d+\.\d+)', result.stdout)
                
                if ip_match and subnet_match:
                    ip = ip_match.group(1)
                    subnet = subnet_match.group(1)
                    
                    # 计算网段
                    ip_parts = ip.split('.')
                    subnet_parts = subnet.split('.')
                    
                    # 确定网段范围
                    network_parts = []
                    for i in range(4):
                        if subnet_parts[i] == '255':
                            network_parts.append(ip_parts[i])
                        else:
                            network_parts.append('0')
                    
                    network_base = '.'.join(network_parts)
                    
                    # 扫描当前网段
                    devices_found = []
                    
                    # 扫描当前网段的前20个IP地址
                    for i in range(1, 21):
                        target_ip = f"{network_parts[0]}.{network_parts[1]}.{network_parts[2]}.{i}"
                        
                        # 跳过本机IP
                        if target_ip == ip:
                            continue
                        
                        try:
                            # 使用ping检测设备是否在线
                            result = subprocess.run(['ping', '-n', '1', '-w', '1000', target_ip], 
                                                   capture_output=True, text=True)
                            
                            if result.returncode == 0:
                                # 尝试获取设备信息
                                try:
                                    response = requests.get(f"http://{target_ip}/api/info", timeout=2)
                                    if response.status_code == 200:
                                        device_info = response.json()
                                        devices_found.append((
                                            target_ip,
                                            device_info.get('device_id', '未知'),
                                            device_info.get('device_name', '未知设备'),
                                            "在线"
                                        ))
                                except:
                                    # 不是ESP32设备，但显示为普通设备
                                    devices_found.append((target_ip, "未知", "网络设备", "在线"))
                                    
                        except:
                            pass
                    
                    # 在主线程中更新设备列表
                    self.root.after(0, lambda d=devices_found: self.update_device_list(d))
                    
            except Exception as e:
                self.root.after(0, lambda: self.add_test_result(f"网络扫描失败: {str(e)}", "失败"))
        
        # 在后台线程中执行扫描
        threading.Thread(target=arp_scan, daemon=True).start()
    
    def update_device_list(self, devices):
        """更新设备列表"""
        for item in self.device_tree.get_children():
            self.device_tree.delete(item)
        
        for device in devices:
            self.device_tree.insert("", "end", values=device)
        
        self.add_test_result(f"发现 {len(devices)} 个设备", "成功")
    
    def start_udp_listener(self):
        """启动UDP监听"""
        if self.udp_listening:
            messagebox.showwarning("警告", "UDP监听器已在运行中")
            return
        
        self.udp_listening = True
        self.udp_thread = threading.Thread(target=self.udp_listener, daemon=True)
        self.udp_thread.start()
        
        self.add_udp_message("UDP监听器已启动")
        self.add_test_result("启动UDP监听器", "成功")
    
    def stop_udp_listener(self):
        """停止UDP监听"""
        self.udp_listening = False
        self.add_udp_message("UDP监听器已停止")
        self.add_test_result("停止UDP监听器", "成功")
    
    def udp_listener(self):
        """UDP监听线程"""
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        try:
            # 绑定到组播地址和端口
            sock.bind(('0.0.0.0', 8888))
            
            # 加入组播组
            mreq = socket.inet_aton("224.0.0.1") + socket.inet_aton("0.0.0.0")
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
            
            sock.settimeout(1.0)  # 设置超时以便检查停止标志
            
            while self.udp_listening:
                try:
                    data, addr = sock.recvfrom(1024)
                    message = data.decode('utf-8')
                    
                    # 在主线程中更新UI
                    self.root.after(0, lambda a=addr[0], m=message: self.add_udp_message(f"来自 {a}: {m}"))
                    
                    # 解析设备信息并添加到设备列表
                    try:
                        device_info = json.loads(message)
                        if 'device_id' in device_info:
                            self.root.after(0, lambda d=device_info, a=addr[0]: self.add_discovered_device(d, a))
                    except:
                        pass
                        
                except socket.timeout:
                    continue
                    
        except Exception as e:
            self.root.after(0, lambda: self.add_udp_message(f"UDP监听错误: {str(e)}"))
        finally:
            sock.close()
    
    def add_discovered_device(self, device_info, ip):
        """添加发现的设备到列表"""
        device_id = device_info.get('device_id', '未知')
        device_name = device_info.get('device_name', '未知设备')
        
        # 检查是否已存在
        for item in self.device_tree.get_children():
            if self.device_tree.item(item, 'values')[0] == ip:
                return
        
        self.device_tree.insert("", "end", values=(ip, device_id, device_name, "在线"))
    
    def on_device_select(self, event):
        """设备选择事件"""
        selection = self.device_tree.selection()
        if selection:
            item = selection[0]
            ip = self.device_tree.item(item, 'values')[0]
            self.ip_entry.delete(0, tk.END)
            self.ip_entry.insert(0, ip)
    
    def connect_device(self):
        """连接设备"""
        ip = self.ip_entry.get().strip()
        if not ip:
            messagebox.showerror("错误", "请输入设备IP地址")
            return
        
        try:
            # 测试连接
            response = requests.get(f"http://{ip}/api/info", timeout=5)
            if response.status_code == 200:
                self.device_info = response.json()
                self.device_ip = ip
                self.connected = True
                
                # 更新设备信息显示
                self.update_device_info()
                
                self.add_test_result(f"连接设备 {ip}", "成功")
                messagebox.showinfo("成功", f"已成功连接到设备 {ip}")
            else:
                raise Exception(f"HTTP {response.status_code}")
                
        except Exception as e:
            self.add_test_result(f"连接设备 {ip}", f"失败: {str(e)}")
            messagebox.showerror("错误", f"连接设备失败: {str(e)}")
    
    def disconnect_device(self):
        """断开设备连接"""
        self.connected = False
        self.device_ip = ""
        self.device_info = {}
        
        self.info_text.config(state=NORMAL)
        self.info_text.delete(1.0, tk.END)
        self.info_text.insert(tk.END, "设备已断开连接")
        self.info_text.config(state=DISABLED)
        
        self.add_test_result("断开设备连接", "成功")
    
    def update_device_info(self):
        """更新设备信息显示"""
        if not self.connected:
            return
        
        info_text = f"设备信息 - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n"
        info_text += "="*50 + "\n"
        
        for key, value in self.device_info.items():
            info_text += f"{key}: {value}\n"
        
        self.info_text.config(state=NORMAL)
        self.info_text.delete(1.0, tk.END)
        self.info_text.insert(tk.END, info_text)
        self.info_text.config(state=DISABLED)
    
    # ========== RGB控制相关方法 ==========
    
    def toggle_power(self):
        """切换电源状态"""
        if not self.connected:
            messagebox.showerror("错误", "请先连接设备")
            self.power_var.set(False)
            return
        
        power_state = "on" if self.power_var.get() else "off"
        
        try:
            response = requests.get(f"http://{self.device_ip}/api/control?power={power_state}", timeout=5)
            if response.status_code == 200:
                self.add_test_result(f"电源{power_state.upper()}", "成功")
                self.update_device_info()
            else:
                raise Exception(f"HTTP {response.status_code}")
        except Exception as e:
            self.add_test_result(f"电源{power_state.upper()}", f"失败: {str(e)}")
            messagebox.showerror("错误", f"控制失败: {str(e)}")
    
    def set_color(self, color):
        """设置颜色"""
        if not self.connected:
            messagebox.showerror("错误", "请先连接设备")
            return
        
        try:
            response = requests.get(f"http://{self.device_ip}/api/control?color={color}", timeout=5)
            if response.status_code == 200:
                # 更新颜色预览
                self.update_color_preview(color)
                self.add_test_result(f"设置颜色 {color}", "成功")
                self.update_device_info()
            else:
                raise Exception(f"HTTP {response.status_code}")
        except Exception as e:
            self.add_test_result(f"设置颜色 {color}", f"失败: {str(e)}")
    
    def update_brightness(self, value):
        """更新亮度"""
        brightness = int(float(value))
        self.brightness_label.config(text=f"{brightness}%")
        
        if not self.connected:
            return
        
        try:
            response = requests.get(f"http://{self.device_ip}/api/control?brightness={brightness}", timeout=5)
            if response.status_code == 200:
                self.add_test_result(f"设置亮度 {brightness}%", "成功")
                self.update_device_info()
            else:
                raise Exception(f"HTTP {response.status_code}")
        except Exception as e:
            self.add_test_result(f"设置亮度 {brightness}%", f"失败: {str(e)}")
    
    def update_color_preview(self, color):
        """更新颜色预览"""
        color_map = {
            0: "#FFFFFF",  # 彩虹 - 白色
            1: "#FF0000",  # 红色
            2: "#FFA500",  # 橙色
            3: "#FFFF00",  # 黄色
            4: "#00FF00",  # 绿色
            5: "#00FFFF",  # 青色
            6: "#0000FF",  # 蓝色
            7: "#800080"   # 紫色
        }
        
        color_hex = color_map.get(color, "#808080")
        self.color_preview.config(bg=color_hex)
    
    def test_all_colors(self):
        """测试所有颜色"""
        if not self.connected:
            messagebox.showerror("错误", "请先连接设备")
            return
        
        def test_sequence():
            colors = [1, 2, 3, 4, 5, 6, 7, 0]  # 红橙黄绿青蓝紫彩虹
            
            for color in colors:
                try:
                    response = requests.get(f"http://{self.device_ip}/api/control?color={color}", timeout=5)
                    self.add_test_result(f"测试颜色 {color}", "成功")
                    self.root.after(0, lambda c=color: self.update_color_preview(c))
                    time.sleep(1)  # 每个颜色显示1秒
                except Exception as e:
                    self.add_test_result(f"测试颜色 {color}", f"失败: {str(e)}")
        
        threading.Thread(target=test_sequence, daemon=True).start()
    
    def rainbow_test(self):
        """彩虹渐变测试"""
        if not self.connected:
            messagebox.showerror("错误", "请先连接设备")
            return
        
        def rainbow_sequence():
            for hue in range(0, 360, 10):  # 每10度一个变化
                try:
                    response = requests.get(f"http://{self.device_ip}/api/control?hue={hue}&saturation=100&value=100", timeout=5)
                    self.add_test_result(f"彩虹测试 色相{hue}°", "成功")
                    time.sleep(0.1)  # 快速变化
                except Exception as e:
                    self.add_test_result(f"彩虹测试 色相{hue}°", f"失败: {str(e)}")
        
        threading.Thread(target=rainbow_sequence, daemon=True).start()
    
    # ========== HSV控制相关方法 ==========
    
    def draw_hsv_circle(self):
        """绘制HSV圆形调光板 - 优化版"""
        self.hsv_canvas.delete("hsv_circle")
        
        center_x, center_y = 200, 200
        radius = 180
        inner_radius = 50  # 内圆半径，用于创建环形效果
        
        # 绘制背景圆环
        self.hsv_canvas.create_oval(center_x - radius, center_y - radius,
                                   center_x + radius, center_y + radius,
                                   fill="#34495e", outline="#2c3e50", width=2,
                                   tags="hsv_circle")
        
        # 绘制HSV色环 - 使用更精细的渐变
        for angle in range(0, 360, 2):  # 更细的分辨率
            rad = angle * 3.14159 / 180

            # 计算内外圆上的点（canvas 的 y 向下为正，使用 center_y - sin 来匹配选择器的计算）
            x1 = center_x + inner_radius * math.cos(rad)
            y1 = center_y - inner_radius * math.sin(rad)
            x2 = center_x + radius * math.cos(rad)
            y2 = center_y - radius * math.sin(rad)
            
            # 计算HSV颜色
            rgb = self.hsv_to_rgb(angle, 100, 100)
            color = f"#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}"
            
            # 创建渐变扇形而不是简单的线条
            self.hsv_canvas.create_line(x1, y1, x2, y2, width=12, fill=color, 
                                       capstyle=tk.ROUND, tags="hsv_circle")
        
        # 绘制中心区域
        self.hsv_canvas.create_oval(center_x - inner_radius, center_y - inner_radius,
                                   center_x + inner_radius, center_y + inner_radius,
                                   fill="#2c3e50", outline="#34495e", width=2,
                                   tags="hsv_circle")
        
        # 添加刻度标记
        for major_angle in range(0, 360, 30):
            rad = major_angle * 3.14159 / 180
            x1 = center_x + (radius + 5) * math.cos(rad)
            y1 = center_y - (radius + 5) * math.sin(rad)
            x2 = center_x + (radius + 15) * math.cos(rad)
            y2 = center_y - (radius + 15) * math.sin(rad)

            self.hsv_canvas.create_line(x1, y1, x2, y2, width=2, fill="#ecf0f1",
                                       tags="hsv_circle")

            # 添加角度标签
            if major_angle % 90 == 0:  # 只在主要方向添加标签
                label_x = center_x + (radius + 25) * math.cos(rad)
                label_y = center_y - (radius + 25) * math.sin(rad)
                self.hsv_canvas.create_text(label_x, label_y, text=f"{major_angle}°",
                                          fill="#ecf0f1", font=("Arial", 8, "bold"),
                                          tags="hsv_circle")
    
    def draw_color_picker(self):
        """绘制取色器 - 修复版"""
        self.color_picker_canvas.delete("color_picker")
        
        width, height = 250, 80
        
        # 绘制背景
        self.color_picker_canvas.create_rectangle(0, 0, width, height, 
                                                 fill="#ecf0f1", outline="", 
                                                 tags="color_picker")
        
        # 创建彩虹渐变 - 使用像素级渐变
        for x in range(0, width):
            # 计算色相（水平方向）
            hue = (x / width) * 360
            
            for y in range(0, height):
                # 计算明度（垂直方向）- 从顶部100%到底部50%
                value = 100 - (y / height) * 50
                
                # 计算RGB颜色
                rgb = self.hsv_to_rgb(hue, 100, value)
                color = f"#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}"
                
                # 绘制单个像素点
                self.color_picker_canvas.create_rectangle(x, y, x+1, y+1, 
                                                       fill=color, outline="", 
                                                       tags="color_picker")
        
        # 添加边框
        self.color_picker_canvas.create_rectangle(2, 2, width-2, height-2, 
                                                 outline="#bdc3c7", width=1,
                                                 tags="color_picker")
        
        # 添加指示器
        self.color_picker_canvas.create_text(width//2, height + 15, 
                                            text="← 色相 | 明度 ↓", 
                                            fill="#7f8c8d", font=("Arial", 8),
                                            tags="color_picker")
    
    def hsv_to_rgb(self, h, s, v):
        """HSV转RGB转换"""
        h = h % 360
        if h < 0:
            h += 360
        
        s = max(0, min(100, s)) / 100.0
        v = max(0, min(100, v)) / 100.0
        
        c = v * s
        x = c * (1 - abs((h / 60) % 2 - 1))
        m = v - c
        
        if h < 60:
            r, g, b = c, x, 0
        elif h < 120:
            r, g, b = x, c, 0
        elif h < 180:
            r, g, b = 0, c, x
        elif h < 240:
            r, g, b = 0, x, c
        elif h < 300:
            r, g, b = x, 0, c
        else:
            r, g, b = c, 0, x
        
        r = int((r + m) * 255)
        g = int((g + m) * 255)
        b = int((b + m) * 255)
        
        return (r, g, b)
    
    def rgb_to_hsv(self, r, g, b):
        """RGB转HSV转换"""
        r, g, b = r/255.0, g/255.0, b/255.0
        max_val = max(r, g, b)
        min_val = min(r, g, b)
        delta = max_val - min_val
        
        if delta == 0:
            h = 0
        elif max_val == r:
            h = 60 * (((g - b) / delta) % 6)
        elif max_val == g:
            h = 60 * (((b - r) / delta) + 2)
        else:
            h = 60 * (((r - g) / delta) + 4)
        
        if max_val == 0:
            s = 0
        else:
            s = delta / max_val
        
        v = max_val
        
        return (h, s * 100, v * 100)
    
    def on_hsv_circle_click(self, event):
        """HSV圆形调光板点击事件"""
        self.update_hsv_from_circle(event.x, event.y)
    
    def on_hsv_circle_drag(self, event):
        """HSV圆形调光板拖拽事件"""
        self.update_hsv_from_circle(event.x, event.y)
    
    def update_hsv_from_circle(self, x, y):
        """根据圆形调光板位置更新HSV参数 - 优化版"""
        center_x, center_y = 200, 200
        radius = 180
        inner_radius = 50
        
        # 计算相对位置
        dx = x - center_x
        dy = y - center_y
        distance = math.sqrt(dx*dx + dy*dy)
        
        # 限制距离在有效范围内
        if distance < inner_radius:
            distance = inner_radius
        elif distance > radius:
            distance = radius
        
        # 计算角度（0°在右侧，逆时针增加，与绘图一致）
        angle = math.atan2(-dy, dx) * 180 / math.pi
        if angle < 0:
            angle += 360
        
        # 计算饱和度（基于距离）
        saturation = min(100, max(0, ((distance - inner_radius) / (radius - inner_radius)) * 100))
        
        # 更新HSV参数
        self.hue_var.set(int(angle))
        self.saturation_var.set(int(saturation))
        
        # 更新UI
        self.update_hsv_ui()
        
        # 发送到设备
        self.send_hsv_to_device()
        
        # 更新选择器位置
        self.update_selector_position()
    
    def on_color_picker_click(self, event):
        """取色器点击事件 - 修复版"""
        x, y = event.x, event.y
        
        # 获取点击位置的颜色
        try:
            # 获取画布实际尺寸
            canvas_width = 250
            canvas_height = 80
            
            # 确保坐标在有效范围内
            x = max(0, min(x, canvas_width - 1))
            y = max(0, min(y, canvas_height - 1))
            
            # 计算色相（水平方向）
            hue = (x / canvas_width) * 360
            
            # 计算明度（垂直方向）- 从顶部到底部，明度从100%到50%
            value = 100 - (y / canvas_height) * 50
            
            # 直接使用HSV值，不需要复杂的转换
            saturation = 100  # 取色器中的颜色都是100%饱和度
            
            # 更新HSV参数
            self.hue_var.set(int(hue))
            self.saturation_var.set(int(saturation))
            self.value_var.set(int(value))
            
            # 更新UI
            self.update_hsv_ui()
            
            # 发送到设备
            self.send_hsv_to_device()
            
            # 显示取色信息
            rgb = self.hsv_to_rgb(hue, saturation, value)
            self.add_test_result(f"取色成功: RGB({rgb[0]},{rgb[1]},{rgb[2]}) HSV({int(hue)}°,{int(saturation)}%,{int(value)}%)", "成功")
            
        except Exception as e:
            self.add_test_result(f"取色失败: {str(e)}", "失败")
    
    def update_hsv_from_slider(self, value=None):
        """根据滑块更新HSV参数"""
        self.update_hsv_ui()
        self.send_hsv_to_device()
    
    def update_hsv_ui(self):
        """更新HSV界面显示 - 优化版"""
        hue = self.hue_var.get()
        saturation = self.saturation_var.get()
        value_val = self.value_var.get()
        brightness = self.hsv_brightness_var.get()
        
        # 更新标签（带颜色高亮）
        self.hue_label.config(text=f"{hue}°")
        self.saturation_label.config(text=f"{saturation}%")
        self.value_label.config(text=f"{value_val}%")
        self.hsv_brightness_label.config(text=f"{brightness}%")
        
        # 更新选择器位置
        self.update_selector_position()
        
        # 更新颜色预览
        self.update_hsv_preview(hue, saturation, value_val)
        
        # 更新颜色值显示
        rgb = self.hsv_to_rgb(hue, saturation, value_val)
        self.color_value_label.config(text=f"RGB({rgb[0]}, {rgb[1]}, {rgb[2]})")
        
        # 根据亮度调整标签颜色
        brightness_level = value_val / 100.0
        if brightness_level < 0.3:
            text_color = "#ffffff"
        else:
            text_color = "#2c3e50"
        
        self.hue_label.config(foreground=text_color)
        self.saturation_label.config(foreground=text_color)
        self.value_label.config(foreground=text_color)
        self.hsv_brightness_label.config(foreground=text_color)
    
    def update_selector_position(self):
        """更新HSV选择器位置 - 优化版"""
        hue = self.hue_var.get()
        saturation = self.saturation_var.get()
        
        center_x, center_y = 200, 200
        radius = 180
        inner_radius = 50
        
        # 计算选择器位置（角度与绘图一致，0°在右侧，逆时针增加）
        angle = hue * math.pi / 180  # 转换为弧度
        
        # 计算实际距离（考虑内圆半径）
        actual_distance = inner_radius + (saturation / 100) * (radius - inner_radius)
        
        x = center_x + math.cos(angle) * actual_distance
        y = center_y - math.sin(angle) * actual_distance  # 注意y轴方向
        
        # 移动选择器
        self.hsv_canvas.coords(self.hsv_selector, 
                              x-5, y-5, x+5, y+5)
        
        # 更新选择器颜色（根据当前颜色调整轮廓）
        current_rgb = self.hsv_to_rgb(hue, saturation, 100)
        brightness = sum(current_rgb) / 3
        outline_color = "#ffffff" if brightness < 128 else "#000000"
        
        self.hsv_canvas.itemconfig(self.hsv_selector, outline=outline_color)
    
    def update_hsv_preview(self, hue, saturation, value):
        """更新HSV预览"""
        rgb = self.hsv_to_rgb(hue, saturation, value)
        color = f"#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}"
        self.hsv_preview.config(bg=color)
    
    def send_hsv_to_device(self):
        """发送HSV参数到设备"""
        if not self.connected:
            return
        
        hue = self.hue_var.get()
        saturation = self.saturation_var.get()
        value_val = self.value_var.get()
        brightness = self.hsv_brightness_var.get()
        
        try:
            response = requests.get(
                f"http://{self.device_ip}/api/control?hue={hue}&saturation={saturation}&value={value_val}&brightness={brightness}", 
                timeout=5
            )
            if response.status_code == 200:
                self.add_test_result(f"HSV设置 H{hue}° S{saturation}% V{value_val}% B{brightness}%", "成功")
                self.update_device_info()
            else:
                raise Exception(f"HTTP {response.status_code}")
        except Exception as e:
            self.add_test_result(f"HSV设置 H{hue}° S{saturation}% V{value_val}%", f"失败: {str(e)}")
    
    def toggle_hsv_power(self):
        """切换HSV电源状态"""
        if not self.connected:
            messagebox.showerror("错误", "请先连接设备")
            self.hsv_power_var.set(False)
            return
        
        power_state = "on" if self.hsv_power_var.get() else "off"
        
        try:
            response = requests.get(f"http://{self.device_ip}/api/control?power={power_state}", timeout=5)
            if response.status_code == 200:
                self.add_test_result(f"HSV电源{power_state.upper()}", "成功")
                self.update_device_info()
                
                # 如果开启电源，发送当前HSV设置
                if self.hsv_power_var.get():
                    self.send_hsv_to_device()
            else:
                raise Exception(f"HTTP {response.status_code}")
        except Exception as e:
            self.add_test_result(f"HSV电源{power_state.upper()}", f"失败: {str(e)}")
            messagebox.showerror("错误", f"控制失败: {str(e)}")
    
    def reset_hsv_params(self):
        """重置HSV参数"""
        self.hue_var.set(0)
        self.saturation_var.set(100)
        self.value_var.set(100)
        self.hsv_brightness_var.set(50)
        
        self.update_hsv_ui()
        self.send_hsv_to_device()
        
        self.add_test_result("重置HSV参数", "成功")
    
    
    def hsv_gradient_test(self):
        """HSV渐变测试"""
        if not self.connected:
            messagebox.showerror("错误", "请先连接设备")
            return
        
        def hsv_sequence():
            # 色相渐变
            for hue in range(0, 360, 5):
                try:
                    response = requests.get(
                        f"http://{self.device_ip}/api/control?hue={hue}&saturation=100&value=100", 
                        timeout=5
                    )
                    self.root.after(0, lambda h=hue: [
                        self.hue_var.set(h),
                        self.update_hsv_ui()
                    ])
                    time.sleep(0.05)
                except:
                    pass
            
            # 饱和度渐变
            for sat in range(100, 0, -5):
                try:
                    response = requests.get(
                        f"http://{self.device_ip}/api/control?hue=180&saturation={sat}&value=100", 
                        timeout=5
                    )
                    self.root.after(0, lambda s=sat: [
                        self.saturation_var.set(s),
                        self.update_hsv_ui()
                    ])
                    time.sleep(0.05)
                except:
                    pass
            
            # 明度渐变
            for val in range(100, 0, -5):
                try:
                    response = requests.get(
                        f"http://{self.device_ip}/api/control?hue=180&saturation=100&value={val}", 
                        timeout=5
                    )
                    self.root.after(0, lambda v=val: [
                        self.value_var.set(v),
                        self.update_hsv_ui()
                    ])
                    time.sleep(0.05)
                except:
                    pass
        
        threading.Thread(target=hsv_sequence, daemon=True).start()
    
    def random_color_test(self):
        """随机颜色测试"""
        if not self.connected:
            messagebox.showerror("错误", "请先连接设备")
            return
        
        def random_sequence():
            for _ in range(20):  # 测试20个随机颜色
                hue = random.randint(0, 360)
                saturation = random.randint(50, 100)
                value = random.randint(50, 100)
                
                try:
                    response = requests.get(
                        f"http://{self.device_ip}/api/control?hue={hue}&saturation={saturation}&value={value}", 
                        timeout=5
                    )
                    self.root.after(0, lambda h=hue, s=saturation, v=value: [
                        self.hue_var.set(h), 
                        self.saturation_var.set(s), 
                        self.value_var.set(v),
                        self.update_hsv_ui()
                    ])
                    time.sleep(0.5)
                except:
                    pass
        
        threading.Thread(target=random_sequence, daemon=True).start()
    
    # ========== UDP广播相关方法 ==========
    
    def control_broadcast(self, action):
        """控制UDP广播"""
        if not self.connected:
            messagebox.showerror("错误", "请先连接设备")
            return
        
        try:
            response = requests.get(f"http://{self.device_ip}/api/broadcast?action={action}", timeout=5)
            if response.status_code == 200:
                self.add_test_result(f"UDP广播{action.upper()}", "成功")
                self.add_udp_message(f"设备广播已{action}")
                self.update_device_info()
            else:
                raise Exception(f"HTTP {response.status_code}")
        except Exception as e:
            self.add_test_result(f"UDP广播{action.upper()}", f"失败: {str(e)}")
            messagebox.showerror("错误", f"控制失败: {str(e)}")
    
    def add_udp_message(self, message):
        """添加UDP消息"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        formatted_message = f"[{timestamp}] {message}\n"
        
        self.udp_text.insert(tk.END, formatted_message)
        self.udp_text.see(tk.END)
    
    def clear_udp_messages(self):
        """清空UDP消息"""
        self.udp_text.delete(1.0, tk.END)
    
    # ========== 测试报告相关方法 ==========
    
    def add_test_result(self, test_name, result):
        """添加测试结果"""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self.test_results.append({
            "timestamp": timestamp,
            "test_name": test_name,
            "result": result
        })
        
        # 更新结果显示
        self.update_results_display()
    
    def update_results_display(self):
        """更新测试结果显示"""
        self.results_text.delete(1.0, tk.END)
        
        # 统计结果
        total_tests = len(self.test_results)
        success_tests = len([r for r in self.test_results if "成功" in r["result"]])
        failed_tests = total_tests - success_tests
        
        header = f"测试报告 - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n"
        header += "="*60 + "\n"
        header += f"总测试数: {total_tests} | 成功: {success_tests} | 失败: {failed_tests}\n"
        header += "="*60 + "\n\n"
        
        self.results_text.insert(tk.END, header)
        
        # 显示详细结果
        for result in self.test_results[-50:]:  # 显示最近50条记录
            status = "✅" if "成功" in result["result"] else "❌"
            line = f"[{result['timestamp']}] {status} {result['test_name']} - {result['result']}\n"
            self.results_text.insert(tk.END, line)
        
        self.results_text.see(tk.END)
    
    def run_full_test(self):
        """运行完整测试"""
        if not self.connected:
            messagebox.showerror("错误", "请先连接设备")
            return
        
        def full_test_sequence():
            # 1. 基础连接测试
            self.add_test_result("基础连接测试", "开始")
            
            # 2. RGB功能测试
            self.add_test_result("RGB功能测试", "开始")
            
            # 测试所有颜色
            colors = [1, 2, 3, 4, 5, 6, 7, 0]
            for color in colors:
                try:
                    response = requests.get(f"http://{self.device_ip}/api/control?color={color}", timeout=5)
                    self.add_test_result(f"颜色{color}测试", "成功")
                    time.sleep(0.5)
                except:
                    self.add_test_result(f"颜色{color}测试", "失败")
            
            # 3. HSV功能测试
            self.add_test_result("HSV功能测试", "开始")
            
            # 测试HSV参数
            test_params = [
                (0, 100, 100),   # 红色
                (120, 100, 100), # 绿色
                (240, 100, 100), # 蓝色
            ]
            
            for hue, sat, val in test_params:
                try:
                    response = requests.get(
                        f"http://{self.device_ip}/api/control?hue={hue}&saturation={sat}&value={val}", 
                        timeout=5
                    )
                    self.add_test_result(f"HSV测试 H{hue}° S{sat}% V{val}%", "成功")
                    time.sleep(0.5)
                except:
                    self.add_test_result(f"HSV测试 H{hue}° S{sat}% V{val}%", "失败")
            
            # 4. UDP广播测试
            self.add_test_result("UDP广播测试", "开始")
            
            try:
                response = requests.get(f"http://{self.device_ip}/api/broadcast?action=enable", timeout=5)
                self.add_test_result("启用广播", "成功")
                time.sleep(2)
                
                response = requests.get(f"http://{self.device_ip}/api/broadcast?action=disable", timeout=5)
                self.add_test_result("禁用广播", "成功")
            except:
                self.add_test_result("UDP广播测试", "失败")
            
            self.add_test_result("完整测试", "完成")
            messagebox.showinfo("完成", "完整测试已完成")
        
        threading.Thread(target=full_test_sequence, daemon=True).start()
    
    def generate_report(self):
        """生成测试报告"""
        if not self.test_results:
            messagebox.showwarning("警告", "没有测试结果可生成报告")
            return
        
        # 创建报告文件
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"esp32_test_report_{timestamp}.txt"
        
        try:
            with open(filename, 'w', encoding='utf-8') as f:
                f.write("ESP32S3 SuperMini API测试报告\n")
                f.write("="*60 + "\n")
                f.write(f"生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
                f.write(f"设备IP: {self.device_ip if self.connected else '未连接'}\n")
                f.write("="*60 + "\n\n")
                
                # 统计信息
                total_tests = len(self.test_results)
                success_tests = len([r for r in self.test_results if "成功" in r["result"]])
                failed_tests = total_tests - success_tests
                success_rate = (success_tests / total_tests * 100) if total_tests > 0 else 0
                
                f.write("测试统计:\n")
                f.write(f"总测试数: {total_tests}\n")
                f.write(f"成功测试: {success_tests}\n")
                f.write(f"失败测试: {failed_tests}\n")
                f.write(f"成功率: {success_rate:.1f}%\n\n")
                
                # 详细结果
                f.write("详细测试结果:\n")
                f.write("-"*60 + "\n")
                
                for result in self.test_results:
                    status = "成功" if "成功" in result["result"] else "失败"
                    f.write(f"[{result['timestamp']}] {result['test_name']} - {status}\n")
            
            messagebox.showinfo("成功", f"测试报告已生成: {filename}")
            self.add_test_result("生成测试报告", "成功")
            
        except Exception as e:
            messagebox.showerror("错误", f"生成报告失败: {str(e)}")
            self.add_test_result("生成测试报告", f"失败: {str(e)}")
    
    def clear_test_results(self):
        """清空测试记录"""
        self.test_results.clear()
        self.results_text.delete(1.0, tk.END)
        self.add_test_result("清空测试记录", "完成")
    
    def run(self):
        """运行应用程序"""
        self.root.mainloop()

if __name__ == "__main__":
    # 检查依赖
    try:
        import ttkbootstrap
        import requests
    except ImportError as e:
        print(f"缺少依赖库: {e}")
        print("请安装所需依赖:")
        print("pip install ttkbootstrap requests")
        exit(1)
    
    app = ESP32APITester()
    app.run()