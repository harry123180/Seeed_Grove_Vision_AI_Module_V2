#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""主視窗模組"""

import customtkinter as ctk
from tkinter import messagebox, filedialog
import base64
import io
import time
import threading
from PIL import Image, ImageTk
from typing import Optional, Dict, Any
from queue import Queue, Empty

from .serial_reader import SerialReader, list_serial_ports
from .drawing import draw_boxes, draw_keypoints, draw_face_mesh, resize_image, get_class_name
from .xmodem_flasher import XmodemFlasher, FlashProgress
from .image_processor import ImageProcessor, ProcessedFrame


class GroveVisionAITool(ctk.CTk):
    """Grove Vision AI V2 上位機主視窗"""

    def __init__(self):
        super().__init__()

        self.title("Grove Vision AI V2 Tool")
        self.geometry("1400x900")

        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("blue")

        self.serial_reader: Optional[SerialReader] = None
        self.image_processor: Optional[ImageProcessor] = None
        self.data_queue = Queue()  # 原始 JSON 資料
        self.raw_queue = Queue()   # 原始串口文字
        self.processed_queue = Queue()  # 處理完成的幀
        self.is_connected = False
        self.frame_count = 0
        self.fps = 0
        self.last_fps_time = time.time()
        self.fps_frame_count = 0

        # 性能模式
        self.high_performance_mode = True  # 使用新的並行處理器

        # Face Mesh 偏移調整（校準後的預設值）
        self.fm_offset_x = ctk.DoubleVar(value=-1)
        self.fm_offset_y = ctk.DoubleVar(value=30)
        self.fm_scale = ctk.DoubleVar(value=0.74)

        # 燒錄相關
        self.flasher: Optional[XmodemFlasher] = None
        self.firmware_path = ctk.StringVar()
        self.model_paths = []

        self._create_ui()
        self._update_loop()

    def _create_ui(self):
        """建立使用者介面"""
        # 建立 TabView
        self.tabview = ctk.CTkTabview(self)
        self.tabview.pack(fill="both", expand=True, padx=10, pady=10)

        self.tabview.add("串流監控")
        self.tabview.add("韌體燒錄")

        self._create_monitor_tab()
        self._create_flash_tab()

        # 底部狀態列
        self.status_frame = ctk.CTkFrame(self)
        self.status_frame.pack(fill="x", padx=10, pady=(0, 10))

        self.status_label = ctk.CTkLabel(self.status_frame, text="未連接", anchor="w")
        self.status_label.pack(side="left", padx=10)

        self.fps_label = ctk.CTkLabel(self.status_frame, text="FPS: 0 | Frames: 0", anchor="e")
        self.fps_label.pack(side="right", padx=10)

    def _create_monitor_tab(self):
        """建立串流監控頁籤"""
        tab = self.tabview.tab("串流監控")

        main_frame = ctk.CTkFrame(tab)
        main_frame.pack(fill="both", expand=True)

        # 左側控制面板
        control_frame = ctk.CTkFrame(main_frame, width=350)
        control_frame.pack(side="left", fill="y", padx=(0, 10))
        control_frame.pack_propagate(False)

        self._create_serial_controls(control_frame)
        self._create_mode_controls(control_frame)
        self._create_command_controls(control_frame)
        self._create_offset_controls(control_frame)
        self._create_info_panel(control_frame)

        # 右側面板
        right_frame = ctk.CTkFrame(main_frame)
        right_frame.pack(side="right", fill="both", expand=True)

        # 影像顯示區
        image_frame = ctk.CTkFrame(right_frame, height=500)
        image_frame.pack(fill="both", expand=True, pady=(0, 10))

        self.image_label = ctk.CTkLabel(image_frame, text="等待連接...", font=("Arial", 16))
        self.image_label.pack(fill="both", expand=True)

        # 原始資料顯示區
        raw_label = ctk.CTkLabel(right_frame, text="原始串口資料", font=("Arial", 14, "bold"))
        raw_label.pack(anchor="w")

        self.raw_text = ctk.CTkTextbox(right_frame, height=150, font=("Consolas", 10))
        self.raw_text.pack(fill="x", pady=(0, 5))

    def _create_flash_tab(self):
        """建立韌體燒錄頁籤"""
        tab = self.tabview.tab("韌體燒錄")

        # 左右分割
        left_frame = ctk.CTkFrame(tab)
        left_frame.pack(side="left", fill="both", expand=True, padx=10, pady=10)

        right_frame = ctk.CTkFrame(tab, width=400)
        right_frame.pack(side="right", fill="y", padx=10, pady=10)
        right_frame.pack_propagate(False)

        # === 左側：設定區 ===

        # 串口選擇
        port_frame = ctk.CTkFrame(left_frame)
        port_frame.pack(fill="x", pady=5)

        ctk.CTkLabel(port_frame, text="串口:", font=("Arial", 14)).pack(side="left", padx=5)

        self.flash_port_combo = ctk.CTkComboBox(port_frame, values=list_serial_ports(), width=150)
        self.flash_port_combo.pack(side="left", padx=5)

        ctk.CTkButton(port_frame, text="刷新", width=60,
                      command=self._refresh_flash_ports).pack(side="left", padx=5)

        ctk.CTkLabel(port_frame, text="(燒錄使用 921600 baud)", font=("Arial", 10)).pack(side="left", padx=10)

        # 韌體選擇
        fw_frame = ctk.CTkFrame(left_frame)
        fw_frame.pack(fill="x", pady=5)

        ctk.CTkLabel(fw_frame, text="韌體:", font=("Arial", 14)).pack(side="left", padx=5)

        fw_entry = ctk.CTkEntry(fw_frame, textvariable=self.firmware_path, width=350)
        fw_entry.pack(side="left", padx=5)

        ctk.CTkButton(fw_frame, text="瀏覽", width=60,
                      command=self._browse_firmware).pack(side="left", padx=5)

        # 模型選擇
        model_frame = ctk.CTkFrame(left_frame)
        model_frame.pack(fill="x", pady=5)

        ctk.CTkLabel(model_frame, text="模型:", font=("Arial", 14)).pack(anchor="w", padx=5)

        self.model_listbox = ctk.CTkTextbox(model_frame, height=80, font=("Consolas", 10))
        self.model_listbox.pack(fill="x", padx=5, pady=5)

        model_btn_frame = ctk.CTkFrame(model_frame)
        model_btn_frame.pack(fill="x", padx=5)

        ctk.CTkButton(model_btn_frame, text="添加模型", width=100,
                      command=self._add_model).pack(side="left", padx=5)

        ctk.CTkButton(model_btn_frame, text="清除列表", width=100,
                      command=self._clear_models).pack(side="left", padx=5)

        # 預設配置
        preset_frame = ctk.CTkFrame(left_frame)
        preset_frame.pack(fill="x", pady=5)

        ctk.CTkLabel(preset_frame, text="快速配置:", font=("Arial", 14)).pack(side="left", padx=5)

        ctk.CTkButton(preset_frame, text="Face Mesh", width=100,
                      command=self._preset_face_mesh).pack(side="left", padx=5)

        ctk.CTkButton(preset_frame, text="YOLOv8 OD", width=100,
                      command=self._preset_yolov8_od).pack(side="left", padx=5)

        ctk.CTkButton(preset_frame, text="YOLOv8 Pose", width=100,
                      command=self._preset_yolov8_pose).pack(side="left", padx=5)

        # 進度條
        progress_frame = ctk.CTkFrame(left_frame)
        progress_frame.pack(fill="x", pady=10)

        self.flash_progress = ctk.CTkProgressBar(progress_frame)
        self.flash_progress.pack(fill="x", pady=5, padx=10)
        self.flash_progress.set(0)

        self.flash_status_label = ctk.CTkLabel(progress_frame, text="就緒 - 選擇配置後按「開始燒錄」", font=("Arial", 12))
        self.flash_status_label.pack(pady=5)

        # 燒錄按鈕
        btn_frame = ctk.CTkFrame(left_frame)
        btn_frame.pack(fill="x", pady=10)

        self.flash_btn = ctk.CTkButton(btn_frame, text="開始燒錄", width=200, height=50,
                                        font=("Arial", 16, "bold"), fg_color="green",
                                        command=self._start_flash)
        self.flash_btn.pack(side="left", padx=20)

        self.stop_flash_btn = ctk.CTkButton(btn_frame, text="停止", width=100, height=50,
                                             font=("Arial", 14), fg_color="red",
                                             command=self._stop_flash, state="disabled")
        self.stop_flash_btn.pack(side="left", padx=10)

        # === 右側：日誌區 ===
        ctk.CTkLabel(right_frame, text="燒錄日誌", font=("Arial", 16, "bold")).pack(pady=10)

        self.flash_log = ctk.CTkTextbox(right_frame, font=("Consolas", 10))
        self.flash_log.pack(fill="both", expand=True, padx=10, pady=5)

        ctk.CTkButton(right_frame, text="清除日誌", width=100,
                      command=lambda: self.flash_log.delete("1.0", "end")).pack(pady=10)

    def _create_serial_controls(self, parent):
        """建立串口控制區"""
        ctk.CTkLabel(parent, text="串口設定", font=("Arial", 16, "bold")).pack(pady=(10, 5))

        port_frame = ctk.CTkFrame(parent)
        port_frame.pack(fill="x", padx=10, pady=5)

        ctk.CTkLabel(port_frame, text="COM:").pack(side="left")

        self.port_combo = ctk.CTkComboBox(port_frame, values=list_serial_ports(), width=120)
        self.port_combo.pack(side="left", padx=5)

        ctk.CTkButton(port_frame, text="刷新", width=50,
                      command=self._refresh_ports).pack(side="left")

        baud_frame = ctk.CTkFrame(parent)
        baud_frame.pack(fill="x", padx=10, pady=5)

        ctk.CTkLabel(baud_frame, text="鮑率:").pack(side="left")

        self.baud_combo = ctk.CTkComboBox(baud_frame, values=["921600", "115200"], width=120)
        self.baud_combo.set("921600")
        self.baud_combo.pack(side="left", padx=5)

        self.connect_btn = ctk.CTkButton(parent, text="Connect",
                                          command=self._toggle_connection, fg_color="green")
        self.connect_btn.pack(pady=10)

    def _create_mode_controls(self, parent):
        """建立模式控制區"""
        ctk.CTkLabel(parent, text="傳輸模式", font=("Arial", 16, "bold")).pack(pady=(20, 5))

        ctk.CTkButton(parent, text="UART Mode (0xFF)",
                      command=lambda: self._send_mode(0xFF),
                      fg_color="#2E7D32").pack(pady=3, padx=10, fill="x")

        ctk.CTkButton(parent, text="SPI Mode (0xFE)",
                      command=lambda: self._send_mode(0xFE),
                      fg_color="#1565C0").pack(pady=3, padx=10, fill="x")

        ctk.CTkButton(parent, text="UART+SPI (0xFD)",
                      command=lambda: self._send_mode(0xFD),
                      fg_color="#6A1B9A").pack(pady=3, padx=10, fill="x")

    def _create_command_controls(self, parent):
        """建立指令控制區"""
        ctk.CTkLabel(parent, text="指令", font=("Arial", 16, "bold")).pack(pady=(20, 5))

        # 第一排按鈕
        btn_frame = ctk.CTkFrame(parent)
        btn_frame.pack(fill="x", padx=10, pady=5)

        ctk.CTkButton(btn_frame, text="AT", width=50,
                      command=lambda: self._send_command("AT\r\n")).pack(side="left", padx=2)

        ctk.CTkButton(btn_frame, text="INFO?", width=60,
                      command=lambda: self._send_command("AT+INFO?\r\n")).pack(side="left", padx=2)

        ctk.CTkButton(btn_frame, text="ID?", width=50,
                      command=lambda: self._send_command("AT+ID?\r\n")).pack(side="left", padx=2)

        # 第二排按鈕
        btn_frame2 = ctk.CTkFrame(parent)
        btn_frame2.pack(fill="x", padx=10, pady=5)

        ctk.CTkButton(btn_frame2, text="AT+INVOKE", width=90, fg_color="#FF5722",
                      command=lambda: self._send_command("AT+INVOKE=1,0,0\r\n")).pack(side="left", padx=2)

        ctk.CTkButton(btn_frame2, text="AT+SAMPLE", width=90,
                      command=lambda: self._send_command("AT+SAMPLE=1\r\n")).pack(side="left", padx=2)

        ctk.CTkButton(btn_frame2, text="AT+STREAM", width=90,
                      command=lambda: self._send_command("AT+STREAMSTART\r\n")).pack(side="left", padx=2)

        # 自訂指令輸入
        input_frame = ctk.CTkFrame(parent)
        input_frame.pack(fill="x", padx=10, pady=5)

        self.cmd_entry = ctk.CTkEntry(input_frame, placeholder_text="自訂指令...")
        self.cmd_entry.pack(side="left", fill="x", expand=True, padx=(0, 5))
        self.cmd_entry.bind("<Return>", lambda e: self._send_custom_command())

        ctk.CTkButton(input_frame, text="發送", width=50,
                      command=self._send_custom_command).pack(side="left")

        # Hex 發送
        hex_frame = ctk.CTkFrame(parent)
        hex_frame.pack(fill="x", padx=10, pady=5)

        self.hex_entry = ctk.CTkEntry(hex_frame, placeholder_text="Hex (例: FF FE 01)")
        self.hex_entry.pack(side="left", fill="x", expand=True, padx=(0, 5))

        ctk.CTkButton(hex_frame, text="Hex", width=50,
                      command=self._send_hex).pack(side="left")

    def _create_offset_controls(self, parent):
        """建立 Face Mesh 偏移調整控制區"""
        ctk.CTkLabel(parent, text="Mesh 偏移調整", font=("Arial", 16, "bold")).pack(pady=(20, 5))

        # X 偏移
        x_frame = ctk.CTkFrame(parent)
        x_frame.pack(fill="x", padx=10, pady=2)

        ctk.CTkLabel(x_frame, text="X:", width=30).pack(side="left")
        self.offset_x_slider = ctk.CTkSlider(
            x_frame, from_=-200, to=200, variable=self.fm_offset_x,
            command=self._on_offset_change
        )
        self.offset_x_slider.pack(side="left", fill="x", expand=True, padx=5)
        self.offset_x_label = ctk.CTkLabel(x_frame, text="-1", width=40)
        self.offset_x_label.pack(side="left")

        # Y 偏移
        y_frame = ctk.CTkFrame(parent)
        y_frame.pack(fill="x", padx=10, pady=2)

        ctk.CTkLabel(y_frame, text="Y:", width=30).pack(side="left")
        self.offset_y_slider = ctk.CTkSlider(
            y_frame, from_=-200, to=200, variable=self.fm_offset_y,
            command=self._on_offset_change
        )
        self.offset_y_slider.pack(side="left", fill="x", expand=True, padx=5)
        self.offset_y_label = ctk.CTkLabel(y_frame, text="30", width=40)
        self.offset_y_label.pack(side="left")

        # 縮放
        scale_frame = ctk.CTkFrame(parent)
        scale_frame.pack(fill="x", padx=10, pady=2)

        ctk.CTkLabel(scale_frame, text="Scale:", width=30).pack(side="left")
        self.scale_slider = ctk.CTkSlider(
            scale_frame, from_=0.5, to=2.0, variable=self.fm_scale,
            command=self._on_offset_change
        )
        self.scale_slider.pack(side="left", fill="x", expand=True, padx=5)
        self.scale_label = ctk.CTkLabel(scale_frame, text="0.74", width=40)
        self.scale_label.pack(side="left")

        # 重置按鈕
        ctk.CTkButton(parent, text="重置偏移", width=80,
                      command=self._reset_offset).pack(pady=5)

    def _on_offset_change(self, value=None):
        """偏移值變化時更新顯示和處理器"""
        x = int(self.fm_offset_x.get())
        y = int(self.fm_offset_y.get())
        s = self.fm_scale.get()

        self.offset_x_label.configure(text=str(x))
        self.offset_y_label.configure(text=str(y))
        self.scale_label.configure(text=f"{s:.2f}")

        # 更新 ImageProcessor 的偏移設定
        if self.image_processor:
            self.image_processor.set_offset(x, y, s)

    def _reset_offset(self):
        """重置偏移值"""
        self.fm_offset_x.set(0)
        self.fm_offset_y.set(0)
        self.fm_scale.set(1.0)
        self._on_offset_change()

    def _create_info_panel(self, parent):
        """建立資訊顯示區"""
        ctk.CTkLabel(parent, text="日誌", font=("Arial", 16, "bold")).pack(pady=(20, 5))

        self.info_text = ctk.CTkTextbox(parent, height=150, font=("Consolas", 10))
        self.info_text.pack(fill="both", expand=True, padx=10, pady=5)

        ctk.CTkButton(parent, text="清除", width=80,
                      command=lambda: self.info_text.delete("1.0", "end")).pack(pady=5)

    # ========== 串流監控功能 ==========

    def _refresh_ports(self):
        """刷新串口列表"""
        ports = list_serial_ports()
        self.port_combo.configure(values=ports)
        if ports:
            self.port_combo.set(ports[0])

    def _refresh_flash_ports(self):
        """刷新燒錄頁面串口列表"""
        ports = list_serial_ports()
        self.flash_port_combo.configure(values=ports)
        if ports:
            self.flash_port_combo.set(ports[0])

    def _toggle_connection(self):
        """切換連接狀態"""
        if self.is_connected:
            self._disconnect()
        else:
            self._connect()

    def _connect(self):
        """連接串口"""
        port = self.port_combo.get()
        baudrate = int(self.baud_combo.get())

        if not port:
            messagebox.showerror("錯誤", "請選擇串口")
            return

        self.serial_reader = SerialReader(port, baudrate, self.data_queue, self.raw_queue)

        if self.serial_reader.connect():
            self.serial_reader.start()

            # 啟動圖像處理器（高性能模式）
            if self.high_performance_mode:
                self.image_processor = ImageProcessor(
                    input_queue=self.data_queue,
                    output_queue=self.processed_queue,
                    max_output_size=2,  # 最多保留 2 幀
                    target_size=(800, 500),  # 目標顯示尺寸
                )
                # 設定校準後的偏移值
                self.image_processor.set_offset(
                    int(self.fm_offset_x.get()),
                    int(self.fm_offset_y.get()),
                    self.fm_scale.get()
                )
                self.image_processor.start()
                self._log("高性能模式：圖像處理器已啟動")

            self.is_connected = True
            self.connect_btn.configure(text="Disconnect", fg_color="red")
            self.status_label.configure(text=f"已連接: {port} @ {baudrate}")
            self._log(f"已連接到 {port}")
        else:
            messagebox.showerror("錯誤", f"無法連接到 {port}")

    def _disconnect(self):
        """斷開串口"""
        # 停止圖像處理器
        if self.image_processor:
            self.image_processor.stop()
            # 輸出性能統計
            stats = self.image_processor.get_stats()
            self._log(f"性能統計: 處理={stats['processed']}, 丟棄={stats['dropped']}, 平均={stats['avg_time_ms']:.1f}ms")
            self.image_processor = None

        if self.serial_reader:
            self.serial_reader.disconnect()
            self.serial_reader = None

        self.is_connected = False
        self.connect_btn.configure(text="Connect", fg_color="green")
        self.status_label.configure(text="未連接")
        self._log("已斷開連接")

    def _send_mode(self, mode: int):
        """發送模式切換"""
        if self.serial_reader:
            self.serial_reader.send_byte(mode)
            self._log(f"已發送: 0x{mode:02X}")
        else:
            messagebox.showwarning("警告", "請先連接串口")

    def _send_command(self, cmd: str):
        """發送指令"""
        if self.serial_reader and self.is_connected:
            self.serial_reader.send_string(cmd)
            self._log(f"已發送: {cmd.strip()}")
        else:
            messagebox.showwarning("警告", "請先連接串口")

    def _send_custom_command(self):
        """發送自訂指令"""
        cmd = self.cmd_entry.get()
        if cmd:
            self._send_command(cmd + "\r\n")
            self.cmd_entry.delete(0, "end")

    def _send_hex(self):
        """發送 Hex 資料"""
        hex_str = self.hex_entry.get().strip()
        if hex_str and self.serial_reader:
            try:
                hex_str = hex_str.replace(" ", "")
                data = bytes.fromhex(hex_str)
                self.serial_reader.serial.write(data)
                self._log(f"已發送 Hex: {' '.join(f'{b:02X}' for b in data)}")
                self.hex_entry.delete(0, "end")
            except ValueError as e:
                messagebox.showerror("錯誤", f"無效的 Hex 格式: {e}")

    def _log(self, message: str):
        """寫入日誌"""
        timestamp = time.strftime("%H:%M:%S")
        self.info_text.insert("end", f"[{timestamp}] {message}\n")
        self.info_text.see("end")

    def _update_loop(self):
        """UI 更新迴圈"""
        try:
            # === 處理原始串口資料 ===
            raw_count = 0
            raw_text = ""
            while raw_count < 20:
                try:
                    raw = self.raw_queue.get_nowait()
                    raw_count += 1
                    if len(raw) < 200 and not raw.strip().startswith('{'):
                        raw_text += raw
                except Empty:
                    break

            if raw_text:
                self.raw_text.insert("end", raw_text[:500])
                self.raw_text.see("end")
                content = self.raw_text.get("1.0", "end")
                if len(content) > 10000:
                    self.raw_text.delete("1.0", "end-5000c")

            # === 高性能模式：從 processed_queue 取得處理完成的幀 ===
            if self.high_performance_mode and self.image_processor:
                try:
                    # 取得最新的處理完成幀
                    frame: ProcessedFrame = self.processed_queue.get_nowait()

                    # 直接更新 GUI（圖像已經縮放好了）
                    photo = ImageTk.PhotoImage(frame.image)
                    self.image_label.configure(image=photo, text="")
                    self.image_label.image = photo

                    self.frame_count = frame.frame_id
                    self.fps_frame_count += 1

                    # 每 30 幀輸出一次性能資訊
                    if self.frame_count % 30 == 0:
                        stats = self.image_processor.get_stats()
                        print(f"[Perf] Frame {self.frame_count}: proc_time={frame.processing_time:.1f}ms, "
                              f"avg={stats['avg_time_ms']:.1f}ms, dropped={stats['dropped']}")

                except Empty:
                    pass
                except Exception as e:
                    print(f"[Display Error] {e}")

            # === 低性能模式（備用）：直接處理 JSON ===
            elif not self.high_performance_mode:
                json_count = 0
                while json_count < 5:
                    try:
                        data = self.data_queue.get_nowait()
                        json_count += 1
                        self._process_data(data)
                    except Empty:
                        break
                    except Exception as e:
                        print(f"[JSON Process Error] {e}")

            # === 更新 FPS 顯示 ===
            current_time = time.time()
            if current_time - self.last_fps_time >= 1.0:
                self.fps = self.fps_frame_count
                self.fps_frame_count = 0
                self.last_fps_time = current_time

                # 顯示更詳細的性能資訊
                if self.image_processor:
                    stats = self.image_processor.get_stats()
                    self.fps_label.configure(
                        text=f"FPS: {self.fps} | Frames: {self.frame_count} | Avg: {stats['avg_time_ms']:.0f}ms"
                    )
                else:
                    self.fps_label.configure(text=f"FPS: {self.fps} | Frames: {self.frame_count}")

        except Exception as e:
            print(f"[Update Loop Error] {e}")

        self.after(33, self._update_loop)  # ~30 FPS GUI 更新率

    def _process_data(self, data: Dict[str, Any]):
        """處理接收到的資料"""
        try:
            # 安全取得欄位
            if not isinstance(data, dict):
                print(f"[Data Error] Expected dict, got {type(data)}: {data}")
                return

            msg_type = data.get("type", -1)
            name = data.get("name", "")
            payload = data.get("data")

            # 調試日誌 - 輸出到終端機
            print(f"[RX] type={msg_type}, name={name}")

            # GUI 日誌 - 只顯示簡要資訊
            if payload and isinstance(payload, dict):
                keys = list(payload.keys())
                self._log(f"JSON type={msg_type}, name={name}, keys={keys}")
            elif payload is not None:
                self._log(f"JSON type={msg_type}, name={name}")

            if msg_type == 1:  # 推論結果
                self._process_inference_result(payload)
            elif msg_type == 0:  # 查詢回應
                self._log(f"回應: {name}")

        except Exception as e:
            # 錯誤輸出到終端機，不讓 GUI 崩潰
            import traceback
            print(f"[Process Error] {e}")
            traceback.print_exc()
            self._log(f"處理錯誤: {e}")

    def _process_inference_result(self, data):
        """處理推論結果"""
        try:
            self.frame_count += 1
            self.fps_frame_count += 1

            # 安全檢查 data 類型
            if not isinstance(data, dict):
                return

            # 取得資料
            resolution = data.get("resolution", [240, 240])
            image_b64 = data.get("image", "")
            boxes = data.get("boxes", [])
            keypoints = data.get("keypoints", [])
            fm_points = data.get("fm_points", [])

            if not image_b64:
                return

            # 解碼影像
            try:
                image_data = base64.b64decode(image_b64)
                image = Image.open(io.BytesIO(image_data))
            except Exception as e:
                print(f"[Image Error] {e}")
                return

            # 取得實際圖像尺寸
            actual_w, actual_h = image.size

            # 座標參考尺寸：優先使用 resolution，否則使用實際圖像尺寸
            if isinstance(resolution, list) and len(resolution) >= 2:
                ref_w, ref_h = resolution[0], resolution[1]
            else:
                ref_w, ref_h = actual_w, actual_h

            # 調試輸出（只每 30 幀輸出一次）
            if self.frame_count % 30 == 1:
                print(f"[Frame {self.frame_count}] img={actual_w}x{actual_h}, ref={ref_w}x{ref_h}, fm={len(fm_points)}")

            # 繪製 boxes
            if boxes and isinstance(boxes, list):
                try:
                    image = draw_boxes(image, boxes, ref_w, ref_h)
                except Exception as e:
                    print(f"[Draw Boxes Error] {e}")

            # 繪製 keypoints
            if keypoints and isinstance(keypoints, list):
                try:
                    image = draw_keypoints(image, keypoints, ref_w, ref_h)
                except Exception as e:
                    print(f"[Draw Keypoints Error] {e}")

            # 繪製 Face Mesh（啟用調試模式以查看座標）
            if fm_points and isinstance(fm_points, list):
                try:
                    # 每 60 幀啟用一次調試輸出
                    debug_mode = (self.frame_count % 60 == 1)
                    image = draw_face_mesh(image, fm_points, ref_w, ref_h, debug=debug_mode)
                except Exception as e:
                    print(f"[Draw Face Mesh Error] {e}")

            # 更新 GUI
            try:
                display_image = resize_image(image, 800, 500)
                photo = ImageTk.PhotoImage(display_image)
                self.image_label.configure(image=photo, text="")
                self.image_label.image = photo
            except Exception as e:
                print(f"[GUI Error] {e}")

        except Exception as e:
            # 最外層保護，確保不會崩潰
            import traceback
            print(f"[Inference Fatal Error] {e}")
            traceback.print_exc()

    # ========== 燒錄功能 ==========

    def _browse_firmware(self):
        """瀏覽韌體檔案"""
        path = filedialog.askopenfilename(
            title="選擇韌體檔案",
            filetypes=[("Image files", "*.img"), ("All files", "*.*")]
        )
        if path:
            self.firmware_path.set(path)

    def _add_model(self):
        """添加模型"""
        path = filedialog.askopenfilename(
            title="選擇模型檔案",
            filetypes=[("TFLite files", "*.tflite"), ("All files", "*.*")]
        )
        if path:
            # 從檔名解析地址
            addr = self._parse_flash_addr(path)
            self.model_paths.append((path, addr, 0))
            self.model_listbox.insert("end", f"{path} @ 0x{addr:X}\n")

    def _parse_flash_addr(self, path: str) -> int:
        """從檔名解析 flash 地址"""
        import re
        import os
        filename = os.path.basename(path)
        match = re.search(r'0x([0-9A-Fa-f]+)', filename)
        if match:
            return int(match.group(1), 16)
        return 0x200000  # 預設

    def _clear_models(self):
        """清除模型列表"""
        self.model_paths = []
        self.model_listbox.delete("1.0", "end")

    def _preset_face_mesh(self):
        """預設 Face Mesh 配置"""
        import os
        base = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
        model_dir = os.path.join(base, "model_zoo", "tflm_fd_fm")
        fw_dir = os.path.join(base, "we2_image_gen_local", "output_case1_sec_wlcsp")

        self.firmware_path.set(os.path.join(fw_dir, "output.img"))
        self._clear_models()

        models = [
            ("0_fd_0x200000.tflite", 0x200000),
            ("1_fm_0x280000.tflite", 0x280000),
            ("2_il_0x32A000.tflite", 0x32A000),
        ]
        for name, addr in models:
            path = os.path.join(model_dir, name)
            if os.path.exists(path):
                self.model_paths.append((path, addr, 0))
                self.model_listbox.insert("end", f"{name} @ 0x{addr:X}\n")

    def _preset_yolov8_od(self):
        """預設 YOLOv8 OD 配置"""
        import os
        base = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
        model_dir = os.path.join(base, "model_zoo", "tflm_yolov8_od")
        fw_dir = os.path.join(base, "we2_image_gen_local", "output_case1_sec_wlcsp")

        self.firmware_path.set(os.path.join(fw_dir, "output.img"))
        self._clear_models()

        for f in os.listdir(model_dir):
            if f.endswith('.tflite'):
                addr = self._parse_flash_addr(f)
                path = os.path.join(model_dir, f)
                self.model_paths.append((path, addr, 0))
                self.model_listbox.insert("end", f"{f} @ 0x{addr:X}\n")

    def _preset_yolov8_pose(self):
        """預設 YOLOv8 Pose 配置"""
        import os
        base = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
        model_dir = os.path.join(base, "model_zoo", "tflm_yolov8_pose")
        fw_dir = os.path.join(base, "we2_image_gen_local", "output_case1_sec_wlcsp")

        self.firmware_path.set(os.path.join(fw_dir, "output.img"))
        self._clear_models()

        for f in os.listdir(model_dir):
            if f.endswith('.tflite'):
                addr = self._parse_flash_addr(f)
                path = os.path.join(model_dir, f)
                self.model_paths.append((path, addr, 0))
                self.model_listbox.insert("end", f"{f} @ 0x{addr:X}\n")

    def _update_flash_progress(self, progress: FlashProgress):
        """更新燒錄進度"""
        # 寫入日誌訊息
        if progress.log_message:
            timestamp = time.strftime("%H:%M:%S")
            self.flash_log.insert("end", f"[{timestamp}] {progress.log_message}\n")
            self.flash_log.see("end")

        # 更新進度條
        if progress.total_packets > 0:
            pct = progress.sent_packets / progress.total_packets
            self.flash_progress.set(pct)

        # 更新狀態文字
        self.flash_status_label.configure(
            text=f"{progress.status}: {progress.current_file} ({progress.sent_packets}/{progress.total_packets})"
        )

        if progress.status == "done":
            self.flash_btn.configure(state="normal")
            self.stop_flash_btn.configure(state="disabled")
            messagebox.showinfo("完成", "燒錄完成！")
        elif progress.status == "error":
            self.flash_btn.configure(state="normal")
            self.stop_flash_btn.configure(state="disabled")
            messagebox.showerror("錯誤", progress.log_message or progress.current_file)

    def _start_flash(self):
        """開始燒錄"""
        port = self.flash_port_combo.get()
        if not port:
            messagebox.showerror("錯誤", "請選擇串口")
            return

        fw_path = self.firmware_path.get()
        if not fw_path:
            messagebox.showerror("錯誤", "請選擇韌體檔案")
            return

        self.flash_btn.configure(state="disabled")
        self.stop_flash_btn.configure(state="normal")

        def flash_thread():
            self.flasher = XmodemFlasher()
            self.flasher.set_progress_callback(
                lambda p: self.after(0, lambda: self._update_flash_progress(p))
            )

            if not self.flasher.connect(port, baudrate=921600):
                return

            self.flasher.flash_all(fw_path, self.model_paths, protocol='xmodem')
            self.flasher.disconnect()

        threading.Thread(target=flash_thread, daemon=True).start()

    def _stop_flash(self):
        """停止燒錄"""
        if self.flasher:
            self.flasher.stop()
        self.flash_btn.configure(state="normal")
        self.stop_flash_btn.configure(state="disabled")

    def on_closing(self):
        """關閉視窗"""
        self._disconnect()
        self.destroy()
