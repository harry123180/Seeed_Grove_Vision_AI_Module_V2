#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Grove Vision AI V2 上位機工具
基於 CustomTkinter 的串口通訊與影像顯示工具
支援 SenseCraft AI 協議

優化版本 v2：完全解耦 UI 與資料處理
"""

import customtkinter as ctk
from tkinter import messagebox
import serial
import serial.tools.list_ports
import threading
import json
import base64
import io
import time
from PIL import Image, ImageTk, ImageDraw
from typing import Optional, List, Dict, Any
from queue import Queue, Empty
import weakref


class SerialReader(threading.Thread):
    """串口讀取執行緒"""

    def __init__(self, port: str, baudrate: int, json_callback, raw_callback):
        super().__init__(daemon=True)
        self.port = port
        self.baudrate = baudrate
        self.json_callback = json_callback
        self.raw_callback = raw_callback
        self.serial: Optional[serial.Serial] = None
        self.running = False
        self.buffer = ""

    def connect(self) -> bool:
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.1
            )
            self.running = True
            return True
        except Exception as e:
            print(f"串口連接失敗: {e}")
            return False

    def disconnect(self):
        self.running = False
        if self.serial and self.serial.is_open:
            self.serial.close()

    def send_byte(self, value: int):
        if self.serial and self.serial.is_open:
            self.serial.write(bytes([value]))

    def send_string(self, text: str):
        if self.serial and self.serial.is_open:
            self.serial.write(text.encode('utf-8'))

    def run(self):
        while self.running:
            try:
                if self.serial and self.serial.is_open and self.serial.in_waiting > 0:
                    data = self.serial.read(self.serial.in_waiting)
                    try:
                        text = data.decode('utf-8', errors='ignore')
                        self.buffer += text

                        # 回調原始資料 (限制長度)
                        if text.strip() and self.raw_callback:
                            self.raw_callback(text[:200])

                        self._parse_buffer()
                    except Exception as e:
                        print(f"解碼錯誤: {e}")
                else:
                    time.sleep(0.002)
            except Exception as e:
                print(f"讀取錯誤: {e}")
                time.sleep(0.1)

    def _parse_buffer(self):
        while '\n' in self.buffer:
            line, self.buffer = self.buffer.split('\n', 1)
            line = line.strip()
            if line.startswith('\r'):
                line = line[1:]

            if line.startswith('{'):
                try:
                    data = json.loads(line)
                    if self.json_callback:
                        self.json_callback(data)
                except json.JSONDecodeError:
                    pass


class ImageProcessor(threading.Thread):
    """影像處理執行緒"""

    COCO_CLASSES = [
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck",
        "boat", "traffic light", "fire hydrant", "stop sign", "parking meter", "bench",
        "bird", "cat", "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra",
        "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
        "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove",
        "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup",
        "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
        "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
        "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse",
        "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
        "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier",
        "toothbrush"
    ]

    POSE_SKELETON = [
        (0, 1), (0, 2), (1, 3), (2, 4),
        (5, 6), (5, 7), (7, 9), (6, 8), (8, 10),
        (5, 11), (6, 12), (11, 12),
        (11, 13), (13, 15), (12, 14), (14, 16)
    ]

    def __init__(self, image_callback):
        super().__init__(daemon=True)
        self.image_callback = image_callback
        self.input_queue = Queue(maxsize=3)  # 限制佇列大小
        self.running = False
        self.frame_count = 0

    def submit(self, data: Dict):
        """提交資料處理（非阻塞，滿了就丟棄舊的）"""
        try:
            # 清空舊資料
            while not self.input_queue.empty():
                try:
                    self.input_queue.get_nowait()
                except Empty:
                    break
            self.input_queue.put_nowait(data)
        except:
            pass

    def start_processing(self):
        self.running = True
        self.start()

    def stop_processing(self):
        self.running = False

    def run(self):
        while self.running:
            try:
                data = self.input_queue.get(timeout=0.1)
                self._process_data(data)
            except Empty:
                continue
            except Exception as e:
                print(f"處理錯誤: {e}")

    def _process_data(self, data: Dict[str, Any]):
        msg_type = data.get("type", -1)
        payload = data.get("data", {})

        if msg_type == 1 and payload:
            result = self._process_inference_result(payload)
            if result and self.image_callback:
                self.image_callback(result)

    def _process_inference_result(self, data: Dict[str, Any]) -> Optional[Dict]:
        self.frame_count += 1
        image_b64 = data.get("image", "")
        boxes = data.get("boxes", [])
        keypoints = data.get("keypoints", [])

        if not image_b64:
            return None

        try:
            image_data = base64.b64decode(image_b64)
            image = Image.open(io.BytesIO(image_data))
            img_w, img_h = image.size

            if boxes:
                image = self._draw_boxes(image, boxes, img_w, img_h)
            if keypoints:
                image = self._draw_keypoints(image, keypoints, img_w, img_h)

            # 縮放
            display_image = self._resize_image(image, 800, 500)

            return {
                "image": display_image,
                "frame_count": self.frame_count,
                "size": (img_w, img_h)
            }
        except Exception as e:
            print(f"影像處理錯誤: {e}")
            return None

    def _draw_boxes(self, image: Image.Image, boxes: List, img_w: int, img_h: int) -> Image.Image:
        draw = ImageDraw.Draw(image)
        colors = ["#FF0000", "#00FF00", "#0000FF", "#FFFF00", "#FF00FF", "#00FFFF"]

        for box in boxes:
            if len(box) >= 6:
                x, y, w, h, score, target = box[:6]
                color = colors[int(target) % len(colors)]
                class_name = self.COCO_CLASSES[int(target)] if 0 <= int(target) < len(self.COCO_CLASSES) else f"class_{target}"
                draw.rectangle([int(x), int(y), int(x + w), int(y + h)], outline=color, width=3)
                draw.text((int(x), max(0, int(y) - 15)), f"{class_name}: {score}%", fill=color)
        return image

    def _draw_keypoints(self, image: Image.Image, keypoints_list: List, img_w: int, img_h: int) -> Image.Image:
        draw = ImageDraw.Draw(image)
        for person in keypoints_list:
            if len(person) < 2:
                continue
            bbox = person[0]
            kpts = person[1:]

            if len(bbox) >= 4:
                x, y, w, h = bbox[:4]
                draw.rectangle([int(x), int(y), int(x + w), int(y + h)], outline="#00FF00", width=2)

            points = []
            for kpt in kpts:
                if len(kpt) >= 2:
                    px, py = int(kpt[0]), int(kpt[1])
                    points.append((px, py))
                    draw.ellipse([px-4, py-4, px+4, py+4], fill="#FF0000", outline="#FFFFFF")

            for s, e in self.POSE_SKELETON:
                if s < len(points) and e < len(points):
                    draw.line([points[s], points[e]], fill="#00FFFF", width=2)
        return image

    def _resize_image(self, image: Image.Image, max_w: int, max_h: int) -> Image.Image:
        w, h = image.size
        ratio = min(max_w / w, max_h / h)
        if ratio < 1:
            return image.resize((int(w * ratio), int(h * ratio)), Image.Resampling.LANCZOS)
        return image


class StreamController(threading.Thread):
    """串流控制執行緒"""

    def __init__(self, send_func):
        super().__init__(daemon=True)
        self.send_func = send_func
        self.interval_ms = 100
        self.streaming = False
        self._lock = threading.Lock()
        self._stop_event = threading.Event()

    def set_interval(self, ms: int):
        with self._lock:
            self.interval_ms = max(30, ms)

    def start_streaming(self):
        self.streaming = True
        self._stop_event.clear()
        if not self.is_alive():
            self.start()

    def stop_streaming(self):
        self.streaming = False
        self._stop_event.set()

    def run(self):
        while True:
            if self.streaming and self.send_func:
                self.send_func("AT+INVOKE=1,0,0\r\n")
                with self._lock:
                    interval = self.interval_ms
                self._stop_event.wait(interval / 1000.0)
            else:
                self._stop_event.wait(0.1)
            self._stop_event.clear()


class GroveVisionAITool(ctk.CTk):
    """Grove Vision AI V2 上位機主視窗"""

    def __init__(self):
        super().__init__()

        self.title("Grove Vision AI V2 Tool")
        self.geometry("1400x900")

        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("blue")

        # 元件
        self.serial_reader: Optional[SerialReader] = None
        self.image_processor: Optional[ImageProcessor] = None
        self.stream_controller: Optional[StreamController] = None

        # 狀態
        self.is_connected = False
        self.is_streaming = False
        self.stream_interval = 100

        # 統計 (用 lock 保護)
        self._stats_lock = threading.Lock()
        self._frame_count = 0
        self._fps_count = 0
        self._last_fps_time = time.time()
        self._current_fps = 0

        # 待更新的影像 (用 lock 保護)
        self._image_lock = threading.Lock()
        self._pending_image: Optional[Image.Image] = None

        # 原始資料緩衝 (用 lock 保護)
        self._raw_lock = threading.Lock()
        self._raw_buffer = ""
        self._raw_dirty = False

        self._create_ui()
        self._start_display_loop()

    def _create_ui(self):
        self.main_frame = ctk.CTkFrame(self)
        self.main_frame.pack(fill="both", expand=True, padx=10, pady=10)

        # 左側控制面板
        self.control_frame = ctk.CTkFrame(self.main_frame, width=350)
        self.control_frame.pack(side="left", fill="y", padx=(0, 10))
        self.control_frame.pack_propagate(False)

        self._create_serial_controls()
        self._create_stream_controls()
        self._create_mode_controls()
        self._create_command_controls()
        self._create_info_panel()

        # 右側面板
        self.right_frame = ctk.CTkFrame(self.main_frame)
        self.right_frame.pack(side="right", fill="both", expand=True)

        # 影像顯示區
        self.image_frame = ctk.CTkFrame(self.right_frame)
        self.image_frame.pack(fill="both", expand=True, pady=(0, 10))

        self.image_label = ctk.CTkLabel(
            self.image_frame,
            text="等待連接...\n\n連接後點擊「開始串流」",
            font=("Arial", 16)
        )
        self.image_label.pack(fill="both", expand=True)

        # 原始資料 (可收合)
        self.raw_visible = ctk.BooleanVar(value=False)
        raw_header = ctk.CTkFrame(self.right_frame)
        raw_header.pack(fill="x")

        self.raw_toggle_btn = ctk.CTkButton(
            raw_header,
            text="▶ 原始資料",
            command=self._toggle_raw_panel,
            width=120,
            fg_color="transparent",
            text_color=("gray10", "gray90"),
            anchor="w"
        )
        self.raw_toggle_btn.pack(side="left")

        self.raw_frame = ctk.CTkFrame(self.right_frame)
        self.raw_text = ctk.CTkTextbox(self.raw_frame, height=120, font=("Consolas", 9))
        self.raw_text.pack(fill="both", expand=True)
        # 預設隱藏
        # self.raw_frame.pack(fill="x", pady=(0, 5))

        # 狀態列
        self.status_frame = ctk.CTkFrame(self)
        self.status_frame.pack(fill="x", padx=10, pady=(0, 10))

        self.status_label = ctk.CTkLabel(self.status_frame, text="未連接", anchor="w")
        self.status_label.pack(side="left", padx=10)

        self.fps_label = ctk.CTkLabel(self.status_frame, text="FPS: 0 | Frames: 0", anchor="e")
        self.fps_label.pack(side="right", padx=10)

    def _toggle_raw_panel(self):
        if self.raw_visible.get():
            self.raw_frame.pack_forget()
            self.raw_toggle_btn.configure(text="▶ 原始資料")
            self.raw_visible.set(False)
        else:
            self.raw_frame.pack(fill="x", pady=(0, 5))
            self.raw_toggle_btn.configure(text="▼ 原始資料")
            self.raw_visible.set(True)

    def _create_serial_controls(self):
        ctk.CTkLabel(self.control_frame, text="串口設定", font=("Arial", 16, "bold")).pack(pady=(10, 5))

        port_frame = ctk.CTkFrame(self.control_frame)
        port_frame.pack(fill="x", padx=10, pady=5)

        ctk.CTkLabel(port_frame, text="Port:").pack(side="left")
        self.port_combo = ctk.CTkComboBox(port_frame, values=[], width=140)
        self.port_combo.pack(side="left", padx=5)
        ctk.CTkButton(port_frame, text="🔄", width=30, command=self._refresh_ports).pack(side="left")

        baud_frame = ctk.CTkFrame(self.control_frame)
        baud_frame.pack(fill="x", padx=10, pady=5)

        ctk.CTkLabel(baud_frame, text="鮑率:").pack(side="left")
        self.baud_combo = ctk.CTkComboBox(baud_frame, values=["921600", "115200", "460800"], width=140)
        self.baud_combo.set("921600")
        self.baud_combo.pack(side="left", padx=5)

        self.connect_btn = ctk.CTkButton(
            self.control_frame, text="Connect", command=self._toggle_connection, fg_color="green"
        )
        self.connect_btn.pack(pady=10)

        self._refresh_ports()

    def _create_stream_controls(self):
        ctk.CTkLabel(self.control_frame, text="影像串流", font=("Arial", 16, "bold")).pack(pady=(15, 5))

        self.stream_btn = ctk.CTkButton(
            self.control_frame,
            text="▶ 開始串流",
            command=self._toggle_stream,
            fg_color="#FF5722",
            hover_color="#E64A19",
            height=40,
            font=("Arial", 14, "bold")
        )
        self.stream_btn.pack(pady=5, padx=10, fill="x")

        speed_frame = ctk.CTkFrame(self.control_frame)
        speed_frame.pack(fill="x", padx=10, pady=5)

        ctk.CTkLabel(speed_frame, text="間隔(ms):").pack(side="left")
        self.interval_slider = ctk.CTkSlider(
            speed_frame, from_=30, to=500, number_of_steps=47, command=self._on_interval_change
        )
        self.interval_slider.set(100)
        self.interval_slider.pack(side="left", fill="x", expand=True, padx=5)
        self.interval_label = ctk.CTkLabel(speed_frame, text="100", width=40)
        self.interval_label.pack(side="left")

    def _create_mode_controls(self):
        ctk.CTkLabel(self.control_frame, text="傳輸模式", font=("Arial", 14, "bold")).pack(pady=(15, 5))

        mode_frame = ctk.CTkFrame(self.control_frame)
        mode_frame.pack(fill="x", padx=10)

        ctk.CTkButton(mode_frame, text="UART", command=lambda: self._send_mode(0xFF), fg_color="#2E7D32", width=80).pack(side="left", padx=2)
        ctk.CTkButton(mode_frame, text="SPI", command=lambda: self._send_mode(0xFE), fg_color="#1565C0", width=80).pack(side="left", padx=2)
        ctk.CTkButton(mode_frame, text="UART+SPI", command=lambda: self._send_mode(0xFD), fg_color="#6A1B9A", width=80).pack(side="left", padx=2)

    def _create_command_controls(self):
        ctk.CTkLabel(self.control_frame, text="指令", font=("Arial", 14, "bold")).pack(pady=(15, 5))

        preset_frame = ctk.CTkFrame(self.control_frame)
        preset_frame.pack(fill="x", padx=10, pady=5)

        ctk.CTkButton(preset_frame, text="AT", width=50, command=lambda: self._send_command("AT\r\n")).pack(side="left", padx=2)
        ctk.CTkButton(preset_frame, text="INFO?", width=60, command=lambda: self._send_command("AT+INFO?\r\n")).pack(side="left", padx=2)
        ctk.CTkButton(preset_frame, text="INVOKE", width=70, command=lambda: self._send_command("AT+INVOKE=1,0,0\r\n"), fg_color="#FF5722").pack(side="left", padx=2)

        input_frame = ctk.CTkFrame(self.control_frame)
        input_frame.pack(fill="x", padx=10, pady=5)

        self.cmd_entry = ctk.CTkEntry(input_frame, placeholder_text="自訂指令...")
        self.cmd_entry.pack(side="left", fill="x", expand=True, padx=(0, 5))
        self.cmd_entry.bind("<Return>", lambda e: self._send_custom_command())
        ctk.CTkButton(input_frame, text="發送", width=50, command=self._send_custom_command).pack(side="left")

    def _create_info_panel(self):
        ctk.CTkLabel(self.control_frame, text="日誌", font=("Arial", 14, "bold")).pack(pady=(15, 5))

        self.info_text = ctk.CTkTextbox(self.control_frame, height=150, font=("Consolas", 10))
        self.info_text.pack(fill="both", expand=True, padx=10, pady=5)

        ctk.CTkButton(
            self.control_frame, text="清除", command=lambda: self.info_text.delete("1.0", "end"), width=80
        ).pack(pady=5)

    def _refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_combo.configure(values=ports)
        if ports:
            self.port_combo.set(ports[0])

    def _toggle_connection(self):
        if self.is_connected:
            self._disconnect()
        else:
            self._connect()

    def _connect(self):
        port = self.port_combo.get()
        baudrate = int(self.baud_combo.get())

        if not port:
            messagebox.showerror("錯誤", "請選擇串口")
            return

        # 建立影像處理器
        self.image_processor = ImageProcessor(self._on_image_ready)
        self.image_processor.start_processing()

        # 建立串口讀取器
        self.serial_reader = SerialReader(port, baudrate, self._on_json_data, self._on_raw_data)
        if self.serial_reader.connect():
            self.serial_reader.start()

            # 建立串流控制器
            self.stream_controller = StreamController(self._send_string_direct)

            self.is_connected = True
            self.connect_btn.configure(text="Disconnect", fg_color="red")
            self.status_label.configure(text=f"已連接: {port} @ {baudrate}")
            self._log(f"已連接到 {port}")
        else:
            self.image_processor.stop_processing()
            messagebox.showerror("錯誤", f"無法連接到 {port}")

    def _disconnect(self):
        if self.is_streaming:
            self._stop_stream()

        if self.stream_controller:
            self.stream_controller.stop_streaming()
            self.stream_controller = None

        if self.image_processor:
            self.image_processor.stop_processing()
            self.image_processor = None

        if self.serial_reader:
            self.serial_reader.disconnect()
            self.serial_reader = None

        self.is_connected = False
        self.connect_btn.configure(text="Connect", fg_color="green")
        self.status_label.configure(text="未連接")
        self.image_label.configure(image=None, text="等待連接...")
        self._log("已斷開連接")

    def _send_string_direct(self, text: str):
        """直接發送字串（供 StreamController 使用）"""
        if self.serial_reader:
            self.serial_reader.send_string(text)

    def _send_mode(self, mode: int):
        if self.serial_reader:
            self.serial_reader.send_byte(mode)
            self._log(f"模式切換: 0x{mode:02X}")
        else:
            messagebox.showwarning("警告", "請先連接")

    def _send_command(self, cmd: str):
        if self.serial_reader:
            self.serial_reader.send_string(cmd)
            self._log(f"發送: {cmd.strip()}")
        else:
            messagebox.showwarning("警告", "請先連接")

    def _send_custom_command(self):
        cmd = self.cmd_entry.get()
        if cmd:
            self._send_command(cmd + "\r\n")
            self.cmd_entry.delete(0, "end")

    def _toggle_stream(self):
        if not self.is_connected:
            messagebox.showwarning("警告", "請先連接")
            return
        if self.is_streaming:
            self._stop_stream()
        else:
            self._start_stream()

    def _start_stream(self):
        self.is_streaming = True
        self.stream_btn.configure(text="⏹ 停止串流", fg_color="#D32F2F", hover_color="#B71C1C")
        self._log("開始串流")
        if self.stream_controller:
            self.stream_controller.set_interval(self.stream_interval)
            self.stream_controller.start_streaming()

    def _stop_stream(self):
        self.is_streaming = False
        self.stream_btn.configure(text="▶ 開始串流", fg_color="#FF5722", hover_color="#E64A19")
        self._log("停止串流")
        if self.stream_controller:
            self.stream_controller.stop_streaming()

    def _on_interval_change(self, value):
        self.stream_interval = int(value)
        self.interval_label.configure(text=str(int(value)))
        if self.stream_controller:
            self.stream_controller.set_interval(int(value))

    # ===== 回調函數 (從其他執行緒呼叫) =====

    def _on_json_data(self, data: Dict):
        """JSON 資料回調 (從 SerialReader 執行緒)"""
        if self.image_processor:
            self.image_processor.submit(data)

    def _on_raw_data(self, text: str):
        """原始資料回調 (從 SerialReader 執行緒)"""
        if not self.raw_visible.get():
            return  # 面板隱藏時不處理
        with self._raw_lock:
            self._raw_buffer += text
            # 限制緩衝區大小
            if len(self._raw_buffer) > 5000:
                self._raw_buffer = self._raw_buffer[-3000:]
            self._raw_dirty = True

    def _on_image_ready(self, result: Dict):
        """影像處理完成回調 (從 ImageProcessor 執行緒)"""
        with self._image_lock:
            self._pending_image = result["image"]

        with self._stats_lock:
            self._frame_count = result["frame_count"]
            self._fps_count += 1

    # ===== UI 更新 =====

    def _log(self, message: str):
        timestamp = time.strftime("%H:%M:%S")
        self.info_text.insert("end", f"[{timestamp}] {message}\n")
        self.info_text.see("end")
        # 限制日誌大小
        content = self.info_text.get("1.0", "end")
        if len(content) > 10000:
            self.info_text.delete("1.0", "100.0")

    def _start_display_loop(self):
        self._update_display()

    def _update_display(self):
        """UI 更新迴圈 - 只做最少的工作"""
        try:
            # 更新影像
            with self._image_lock:
                pending = self._pending_image
                self._pending_image = None

            if pending:
                photo = ImageTk.PhotoImage(pending)
                self.image_label.configure(image=photo, text="")
                self.image_label.image = photo

            # 更新原始資料 (僅當面板可見且有新資料)
            if self.raw_visible.get():
                with self._raw_lock:
                    if self._raw_dirty:
                        # 只取最後一部分顯示
                        display_text = self._raw_buffer[-2000:]
                        self._raw_dirty = False
                        self.raw_text.delete("1.0", "end")
                        self.raw_text.insert("1.0", display_text)
                        self.raw_text.see("end")

            # 更新 FPS (每秒一次)
            current_time = time.time()
            with self._stats_lock:
                if current_time - self._last_fps_time >= 1.0:
                    self._current_fps = self._fps_count
                    self._fps_count = 0
                    self._last_fps_time = current_time
                fps = self._current_fps
                frames = self._frame_count

            self.fps_label.configure(text=f"FPS: {fps} | Frames: {frames}")

        except Exception as e:
            print(f"UI 錯誤: {e}")

        # 固定 30ms 更新一次 (~33 FPS)
        self.after(30, self._update_display)

    def on_closing(self):
        self._disconnect()
        self.destroy()


def main():
    app = GroveVisionAITool()
    app.protocol("WM_DELETE_WINDOW", app.on_closing)
    app.mainloop()


if __name__ == "__main__":
    main()
