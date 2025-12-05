#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Grove Vision AI V2 上位機工具
基於 CustomTkinter 的串口通訊與影像顯示工具
支援 SenseCraft AI 協議
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
import re
from PIL import Image, ImageTk, ImageDraw
from typing import Optional, List, Dict, Any
from dataclasses import dataclass
from queue import Queue, Empty


class SerialReader(threading.Thread):
    """串口讀取執行緒"""

    def __init__(self, port: str, baudrate: int, data_queue: Queue, raw_queue: Queue):
        super().__init__(daemon=True)
        self.port = port
        self.baudrate = baudrate
        self.data_queue = data_queue
        self.raw_queue = raw_queue
        self.serial: Optional[serial.Serial] = None
        self.running = False
        self.buffer = ""

    def connect(self) -> bool:
        """連接串口"""
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
        """斷開串口"""
        self.running = False
        if self.serial and self.serial.is_open:
            self.serial.close()

    def send_byte(self, value: int):
        """發送單一 byte"""
        if self.serial and self.serial.is_open:
            self.serial.write(bytes([value]))

    def send_string(self, text: str):
        """發送字串"""
        if self.serial and self.serial.is_open:
            self.serial.write(text.encode('utf-8'))

    def run(self):
        """執行緒主迴圈"""
        while self.running:
            try:
                if self.serial and self.serial.is_open and self.serial.in_waiting > 0:
                    data = self.serial.read(self.serial.in_waiting)
                    try:
                        text = data.decode('utf-8', errors='ignore')
                        self.buffer += text

                        # 送出原始資料供顯示
                        if text.strip():
                            self.raw_queue.put(text)

                        self._parse_buffer()
                    except Exception as e:
                        print(f"解碼錯誤: {e}")
                else:
                    time.sleep(0.01)
            except Exception as e:
                print(f"讀取錯誤: {e}")
                time.sleep(0.1)

    def _parse_buffer(self):
        """解析緩衝區中的 JSON 資料"""
        while '\n' in self.buffer:
            line, self.buffer = self.buffer.split('\n', 1)
            line = line.strip()

            # 移除開頭的 \r
            if line.startswith('\r'):
                line = line[1:]

            # 嘗試解析 JSON
            if line.startswith('{'):
                try:
                    data = json.loads(line)
                    self.data_queue.put(data)
                except json.JSONDecodeError as e:
                    pass


class GroveVisionAITool(ctk.CTk):
    """Grove Vision AI V2 上位機主視窗"""

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

    def __init__(self):
        super().__init__()

        self.title("Grove Vision AI V2 Tool")
        self.geometry("1400x900")

        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("blue")

        self.serial_reader: Optional[SerialReader] = None
        self.data_queue = Queue()
        self.raw_queue = Queue()
        self.is_connected = False
        self.frame_count = 0
        self.fps = 0
        self.last_fps_time = time.time()
        self.fps_frame_count = 0
        self.show_raw_data = ctk.BooleanVar(value=True)

        self._create_ui()
        self._update_loop()

    def _create_ui(self):
        """建立使用者介面"""
        self.main_frame = ctk.CTkFrame(self)
        self.main_frame.pack(fill="both", expand=True, padx=10, pady=10)

        # 左側控制面板
        self.control_frame = ctk.CTkFrame(self.main_frame, width=350)
        self.control_frame.pack(side="left", fill="y", padx=(0, 10))
        self.control_frame.pack_propagate(False)

        self._create_serial_controls()
        self._create_mode_controls()
        self._create_command_controls()
        self._create_info_panel()

        # 右側面板
        self.right_frame = ctk.CTkFrame(self.main_frame)
        self.right_frame.pack(side="right", fill="both", expand=True)

        # 影像顯示區
        self.image_frame = ctk.CTkFrame(self.right_frame, height=500)
        self.image_frame.pack(fill="both", expand=True, pady=(0, 10))

        self.image_label = ctk.CTkLabel(self.image_frame, text="等待連接...\n\n連接後如果沒有畫面，請嘗試:\n1. 點擊 'UART Mode' 按鈕\n2. 發送 AT+INVOKE 指令\n3. 確認韌體版本支援 UART 輸出", font=("Arial", 16))
        self.image_label.pack(fill="both", expand=True)

        # 原始資料顯示區
        raw_label = ctk.CTkLabel(self.right_frame, text="原始串口資料", font=("Arial", 14, "bold"))
        raw_label.pack(anchor="w")

        self.raw_text = ctk.CTkTextbox(self.right_frame, height=200, font=("Consolas", 10))
        self.raw_text.pack(fill="x", pady=(0, 5))

        # 底部狀態列
        self.status_frame = ctk.CTkFrame(self)
        self.status_frame.pack(fill="x", padx=10, pady=(0, 10))

        self.status_label = ctk.CTkLabel(self.status_frame, text="未連接", anchor="w")
        self.status_label.pack(side="left", padx=10)

        self.fps_label = ctk.CTkLabel(self.status_frame, text="FPS: 0 | Frames: 0", anchor="e")
        self.fps_label.pack(side="right", padx=10)

    def _create_serial_controls(self):
        """建立串口控制區"""
        serial_label = ctk.CTkLabel(self.control_frame, text="串口設定", font=("Arial", 16, "bold"))
        serial_label.pack(pady=(10, 5))

        port_frame = ctk.CTkFrame(self.control_frame)
        port_frame.pack(fill="x", padx=10, pady=5)

        ctk.CTkLabel(port_frame, text="COM Port:").pack(side="left")

        self.port_combo = ctk.CTkComboBox(port_frame, values=[], width=150)
        self.port_combo.pack(side="left", padx=5)

        refresh_btn = ctk.CTkButton(port_frame, text="🔄", width=30, command=self._refresh_ports)
        refresh_btn.pack(side="left")

        baud_frame = ctk.CTkFrame(self.control_frame)
        baud_frame.pack(fill="x", padx=10, pady=5)

        ctk.CTkLabel(baud_frame, text="鮑率:").pack(side="left")

        self.baud_combo = ctk.CTkComboBox(
            baud_frame,
            values=["921600", "115200", "460800", "230400"],
            width=150
        )
        self.baud_combo.set("921600")
        self.baud_combo.pack(side="left", padx=5)

        self.connect_btn = ctk.CTkButton(
            self.control_frame,
            text="Connect",
            command=self._toggle_connection,
            fg_color="green"
        )
        self.connect_btn.pack(pady=10)

        self._refresh_ports()

    def _create_mode_controls(self):
        """建立模式控制區"""
        mode_label = ctk.CTkLabel(self.control_frame, text="傳輸模式切換", font=("Arial", 16, "bold"))
        mode_label.pack(pady=(20, 5))

        uart_btn = ctk.CTkButton(
            self.control_frame,
            text="UART Mode (0xFF)",
            command=lambda: self._send_mode(0xFF),
            fg_color="#2E7D32"
        )
        uart_btn.pack(pady=3, padx=10, fill="x")

        spi_btn = ctk.CTkButton(
            self.control_frame,
            text="SPI Mode (0xFE)",
            command=lambda: self._send_mode(0xFE),
            fg_color="#1565C0"
        )
        spi_btn.pack(pady=3, padx=10, fill="x")

        both_btn = ctk.CTkButton(
            self.control_frame,
            text="UART+SPI Mode (0xFD)",
            command=lambda: self._send_mode(0xFD),
            fg_color="#6A1B9A"
        )
        both_btn.pack(pady=3, padx=10, fill="x")

    def _create_command_controls(self):
        """建立指令控制區"""
        cmd_label = ctk.CTkLabel(self.control_frame, text="自訂指令", font=("Arial", 16, "bold"))
        cmd_label.pack(pady=(20, 5))

        # 預設指令按鈕
        preset_frame = ctk.CTkFrame(self.control_frame)
        preset_frame.pack(fill="x", padx=10, pady=5)

        ctk.CTkButton(
            preset_frame,
            text="AT",
            width=60,
            command=lambda: self._send_command("AT\r\n")
        ).pack(side="left", padx=2)

        ctk.CTkButton(
            preset_frame,
            text="AT+INFO?",
            width=80,
            command=lambda: self._send_command("AT+INFO?\r\n")
        ).pack(side="left", padx=2)

        ctk.CTkButton(
            preset_frame,
            text="AT+ID?",
            width=70,
            command=lambda: self._send_command("AT+ID?\r\n")
        ).pack(side="left", padx=2)

        preset_frame2 = ctk.CTkFrame(self.control_frame)
        preset_frame2.pack(fill="x", padx=10, pady=5)

        ctk.CTkButton(
            preset_frame2,
            text="AT+INVOKE",
            width=90,
            command=lambda: self._send_command("AT+INVOKE=1,0,0\r\n"),
            fg_color="#FF5722"
        ).pack(side="left", padx=2)

        ctk.CTkButton(
            preset_frame2,
            text="AT+SAMPLE",
            width=90,
            command=lambda: self._send_command("AT+SAMPLE=1\r\n")
        ).pack(side="left", padx=2)

        ctk.CTkButton(
            preset_frame2,
            text="AT+STREAM",
            width=90,
            command=lambda: self._send_command("AT+STREAMSTART\r\n")
        ).pack(side="left", padx=2)

        # 自訂指令輸入
        input_frame = ctk.CTkFrame(self.control_frame)
        input_frame.pack(fill="x", padx=10, pady=5)

        self.cmd_entry = ctk.CTkEntry(input_frame, placeholder_text="輸入指令...")
        self.cmd_entry.pack(side="left", fill="x", expand=True, padx=(0, 5))

        ctk.CTkButton(
            input_frame,
            text="發送",
            width=50,
            command=self._send_custom_command
        ).pack(side="left")

        # 綁定 Enter 鍵
        self.cmd_entry.bind("<Return>", lambda e: self._send_custom_command())

        # Hex 發送
        hex_frame = ctk.CTkFrame(self.control_frame)
        hex_frame.pack(fill="x", padx=10, pady=5)

        self.hex_entry = ctk.CTkEntry(hex_frame, placeholder_text="Hex (例: FF FE 01)")
        self.hex_entry.pack(side="left", fill="x", expand=True, padx=(0, 5))

        ctk.CTkButton(
            hex_frame,
            text="Hex",
            width=50,
            command=self._send_hex
        ).pack(side="left")

    def _create_info_panel(self):
        """建立資訊顯示區"""
        info_label = ctk.CTkLabel(self.control_frame, text="訊息日誌", font=("Arial", 16, "bold"))
        info_label.pack(pady=(20, 5))

        self.info_text = ctk.CTkTextbox(self.control_frame, height=200, font=("Consolas", 10))
        self.info_text.pack(fill="both", expand=True, padx=10, pady=5)

        btn_frame = ctk.CTkFrame(self.control_frame)
        btn_frame.pack(fill="x", padx=10, pady=5)

        ctk.CTkButton(
            btn_frame,
            text="清除日誌",
            command=lambda: self.info_text.delete("1.0", "end"),
            width=100
        ).pack(side="left", padx=5)

        ctk.CTkButton(
            btn_frame,
            text="清除原始",
            command=lambda: self.raw_text.delete("1.0", "end"),
            width=100
        ).pack(side="left", padx=5)

    def _refresh_ports(self):
        """重新整理串口列表"""
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.port_combo.configure(values=ports)
        if ports:
            self.port_combo.set(ports[0])
        else:
            self.port_combo.set("")

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
            self.is_connected = True
            self.connect_btn.configure(text="Disconnect", fg_color="red")
            self.status_label.configure(text=f"已連接: {port} @ {baudrate}")
            self._log(f"已連接到 {port}")
        else:
            messagebox.showerror("錯誤", f"無法連接到 {port}")

    def _disconnect(self):
        """斷開串口"""
        if self.serial_reader:
            self.serial_reader.disconnect()
            self.serial_reader = None

        self.is_connected = False
        self.connect_btn.configure(text="Connect", fg_color="green")
        self.status_label.configure(text="未連接")
        self.image_label.configure(image=None, text="等待連接...")
        self._log("已斷開連接")

    def _send_mode(self, mode: int):
        """發送模式切換指令"""
        if self.serial_reader:
            self.serial_reader.send_byte(mode)
            self._log(f"已發送模式切換: 0x{mode:02X}")
        else:
            messagebox.showwarning("警告", "請先連接串口")

    def _send_command(self, cmd: str):
        """發送字串指令"""
        if self.serial_reader:
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
                # 移除空格並轉換
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
            # 處理原始資料
            while True:
                try:
                    raw = self.raw_queue.get_nowait()
                    # 只顯示前 500 字元避免卡頓
                    display = raw[:500] if len(raw) > 500 else raw
                    self.raw_text.insert("end", display)
                    self.raw_text.see("end")

                    # 限制文字框大小
                    content = self.raw_text.get("1.0", "end")
                    if len(content) > 50000:
                        self.raw_text.delete("1.0", "500.0")
                except Empty:
                    break

            # 處理 JSON 資料
            while True:
                try:
                    data = self.data_queue.get_nowait()
                    self._process_data(data)
                except Empty:
                    break

            # 更新 FPS
            current_time = time.time()
            if current_time - self.last_fps_time >= 1.0:
                self.fps = self.fps_frame_count
                self.fps_frame_count = 0
                self.last_fps_time = current_time
                self.fps_label.configure(text=f"FPS: {self.fps} | Frames: {self.frame_count}")

        except Exception as e:
            print(f"更新錯誤: {e}")

        self.after(10, self._update_loop)

    def _process_data(self, data: Dict[str, Any]):
        """處理接收到的資料"""
        try:
            msg_type = data.get("type", -1)
            name = data.get("name", "")
            code = data.get("code", -1)
            payload = data.get("data", {})

            self._log(f"JSON: type={msg_type}, name={name}, code={code}")

            if msg_type == 1:  # 推論結果
                # 顯示 payload 中有哪些 key
                if payload:
                    keys = list(payload.keys())
                    self._log(f"  data keys: {keys}")

                    # 檢查 resolution
                    if "resolution" in payload:
                        self._log(f"  resolution: {payload.get('resolution')}")

                    # 檢查是否有 image
                    if "image" in payload:
                        img_len = len(payload.get("image", ""))
                        self._log(f"  image length: {img_len}")

                    # 檢查 boxes
                    if "boxes" in payload:
                        boxes = payload.get('boxes', [])
                        self._log(f"  boxes ({len(boxes)}): {boxes}")

                    # 檢查 keypoints
                    if "keypoints" in payload:
                        self._log(f"  keypoints count: {len(payload.get('keypoints', []))}")

                    # 顯示 perf (效能資訊)
                    if "perf" in payload:
                        self._log(f"  perf: {payload.get('perf')}")

                    # 顯示 rotate
                    if "rotate" in payload:
                        self._log(f"  rotate: {payload.get('rotate')}")

                self._process_inference_result(payload)
            elif msg_type == 0:  # 查詢回應
                self._log(f"  回應: {json.dumps(payload, ensure_ascii=False)[:300]}")

        except Exception as e:
            self._log(f"處理錯誤: {e}")

    def _process_inference_result(self, data: Dict[str, Any]):
        """處理推論結果"""
        self.frame_count += 1
        self.fps_frame_count += 1

        # 解析 resolution - 可能是 [w, h] 或其他格式
        resolution = data.get("resolution", [240, 240])
        if isinstance(resolution, list) and len(resolution) >= 2:
            img_width, img_height = resolution[0], resolution[1]
        else:
            img_width, img_height = 240, 240

        image_b64 = data.get("image", "")
        boxes = data.get("boxes", [])
        keypoints = data.get("keypoints", [])

        if image_b64:
            try:
                # 嘗試解碼 Base64
                image_data = base64.b64decode(image_b64)
                self._log(f"  解碼後大小: {len(image_data)} bytes")

                # 嘗試開啟圖片
                image = Image.open(io.BytesIO(image_data))
                self._log(f"  圖片尺寸: {image.size}, 格式: {image.format}")

                # 繪製偵測框
                if boxes:
                    image = self._draw_boxes(image, boxes, image.width, image.height)

                # 繪製姿態
                if keypoints:
                    image = self._draw_keypoints(image, keypoints, image.width, image.height)

                # 調整顯示大小
                display_image = self._resize_image(image, 800, 500)

                # 更新顯示
                photo = ImageTk.PhotoImage(display_image)
                self.image_label.configure(image=photo, text="")
                self.image_label.image = photo

            except base64.binascii.Error as e:
                self._log(f"  Base64 解碼錯誤: {e}")
            except Exception as e:
                self._log(f"  影像處理錯誤: {e}")
                import traceback
                traceback.print_exc()
        else:
            self._log("  無影像資料")

        # 顯示偵測結果
        if boxes:
            for i, box in enumerate(boxes):
                if len(box) >= 6:
                    x, y, w, h, score, target = box[:6]
                    class_name = self._get_class_name(target)
                    self._log(f"  [{i}] {class_name}: ({x},{y},{w},{h}) score={score}")

    def _draw_boxes(self, image: Image.Image, boxes: List, img_w: int, img_h: int) -> Image.Image:
        """繪製偵測框"""
        draw = ImageDraw.Draw(image)
        actual_w, actual_h = image.size

        scale_x = actual_w / img_w if img_w > 0 else 1
        scale_y = actual_h / img_h if img_h > 0 else 1

        colors = ["#FF0000", "#00FF00", "#0000FF", "#FFFF00", "#FF00FF", "#00FFFF",
                  "#FFA500", "#800080", "#008000", "#000080"]

        for i, box in enumerate(boxes):
            if len(box) >= 6:
                x, y, w, h, score, target = box[:6]

                x1 = int(x * scale_x)
                y1 = int(y * scale_y)
                x2 = int((x + w) * scale_x)
                y2 = int((y + h) * scale_y)

                color = colors[target % len(colors)]
                class_name = self._get_class_name(target)

                draw.rectangle([x1, y1, x2, y2], outline=color, width=3)
                label = f"{class_name}: {score}%"
                draw.text((x1, max(0, y1 - 15)), label, fill=color)

        return image

    def _draw_keypoints(self, image: Image.Image, keypoints_list: List, img_w: int, img_h: int) -> Image.Image:
        """繪製姿態關鍵點"""
        draw = ImageDraw.Draw(image)
        actual_w, actual_h = image.size

        scale_x = actual_w / img_w if img_w > 0 else 1
        scale_y = actual_h / img_h if img_h > 0 else 1

        for person in keypoints_list:
            if len(person) < 2:
                continue

            bbox = person[0]
            kpts = person[1:] if len(person) > 1 else []

            if len(bbox) >= 4:
                x, y, w, h = bbox[:4]
                x1, y1 = int(x * scale_x), int(y * scale_y)
                x2, y2 = int((x + w) * scale_x), int((y + h) * scale_y)
                draw.rectangle([x1, y1, x2, y2], outline="#00FF00", width=2)

            points = []
            for kpt in kpts:
                if len(kpt) >= 2:
                    px = int(kpt[0] * scale_x)
                    py = int(kpt[1] * scale_y)
                    points.append((px, py))
                    r = 4
                    draw.ellipse([px-r, py-r, px+r, py+r], fill="#FF0000", outline="#FFFFFF")

            for start_idx, end_idx in self.POSE_SKELETON:
                if start_idx < len(points) and end_idx < len(points):
                    draw.line([points[start_idx], points[end_idx]], fill="#00FFFF", width=2)

        return image

    def _resize_image(self, image: Image.Image, max_width: int, max_height: int) -> Image.Image:
        """調整影像大小"""
        width, height = image.size
        ratio = min(max_width / width, max_height / height)

        if ratio < 1:
            new_size = (int(width * ratio), int(height * ratio))
            return image.resize(new_size, Image.Resampling.LANCZOS)
        return image

    def _get_class_name(self, class_id: int) -> str:
        """取得類別名稱"""
        if 0 <= class_id < len(self.COCO_CLASSES):
            return self.COCO_CLASSES[class_id]
        return f"class_{class_id}"

    def on_closing(self):
        """關閉視窗時的處理"""
        self._disconnect()
        self.destroy()


def main():
    app = GroveVisionAITool()
    app.protocol("WM_DELETE_WINDOW", app.on_closing)
    app.mainloop()


if __name__ == "__main__":
    main()
