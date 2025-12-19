#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Hand Tracking GUI 測試程式

用於測試 Grove Vision AI V2 的手部追蹤功能。
支援串口連接、圖像顯示、手部關鍵點繪製。

Usage:
    python test_hand_tracking.py [COM_PORT]
    python test_hand_tracking.py COM3

Author: Grove Vision AI Team
Date: 2025/12/19
"""

import serial
import json
import time
import threading
import base64
import io
import sys
from queue import Queue, Empty

import customtkinter as ctk
from PIL import Image, ImageTk

# 導入手部繪製模組
sys.path.insert(0, 'src')
from hand_drawing import draw_hands, draw_hand_landmarks, draw_hand_bbox

# 配置
DEFAULT_PORT = "COM3"
BAUDRATE = 921600
UPDATE_INTERVAL_MS = 50


class HandTrackingTestApp(ctk.CTk):
    """Hand Tracking 測試 GUI"""

    def __init__(self, port: str):
        super().__init__()
        self.title("Hand Tracking Test - Grove Vision AI V2")
        self.geometry("900x700")

        self.port = port
        self.serial = None
        self.data_queue = Queue()
        self.running = False
        self.buffer = ""

        # 統計
        self.frame_count = 0
        self.hand_count = 0
        self.fps = 0.0
        self.last_fps_time = time.time()
        self.fps_frame_count = 0

        # === GUI 佈局 ===
        self._create_gui()

        # 啟動更新循環
        self._update_loop()

    def _create_gui(self):
        """建立 GUI 介面"""
        # 頂部狀態列
        status_frame = ctk.CTkFrame(self)
        status_frame.pack(fill="x", padx=10, pady=5)

        self.status_label = ctk.CTkLabel(
            status_frame,
            text=f"Status: Not connected | Port: {self.port}",
            font=("Arial", 14)
        )
        self.status_label.pack(side="left", padx=10)

        self.fps_label = ctk.CTkLabel(
            status_frame,
            text="FPS: --",
            font=("Arial", 14, "bold")
        )
        self.fps_label.pack(side="right", padx=10)

        # 統計資訊
        self.stats_label = ctk.CTkLabel(
            status_frame,
            text="Frames: 0 | Hands: 0",
            font=("Arial", 12)
        )
        self.stats_label.pack(side="right", padx=20)

        # 圖像顯示區
        self.image_frame = ctk.CTkFrame(self)
        self.image_frame.pack(fill="both", expand=True, padx=10, pady=5)

        self.image_label = ctk.CTkLabel(
            self.image_frame,
            text="No Image\n\nConnect to device and send INVOKE command",
            font=("Arial", 16),
            width=640,
            height=480
        )
        self.image_label.pack(expand=True)

        # 控制按鈕
        btn_frame = ctk.CTkFrame(self)
        btn_frame.pack(fill="x", padx=10, pady=5)

        self.connect_btn = ctk.CTkButton(
            btn_frame,
            text="Connect",
            command=self._toggle_connect,
            width=120,
            fg_color="green"
        )
        self.connect_btn.pack(side="left", padx=5)

        ctk.CTkButton(
            btn_frame,
            text="Send AT",
            command=self._send_at,
            width=100
        ).pack(side="left", padx=5)

        ctk.CTkButton(
            btn_frame,
            text="Start INVOKE",
            command=self._send_invoke_start,
            width=120
        ).pack(side="left", padx=5)

        ctk.CTkButton(
            btn_frame,
            text="Stop INVOKE",
            command=self._send_invoke_stop,
            width=120
        ).pack(side="left", padx=5)

        # 顯示選項
        options_frame = ctk.CTkFrame(btn_frame)
        options_frame.pack(side="right", padx=10)

        self.show_bbox_var = ctk.BooleanVar(value=True)
        ctk.CTkCheckBox(
            options_frame,
            text="Show BBox",
            variable=self.show_bbox_var
        ).pack(side="left", padx=5)

        self.show_landmarks_var = ctk.BooleanVar(value=True)
        ctk.CTkCheckBox(
            options_frame,
            text="Show Landmarks",
            variable=self.show_landmarks_var
        ).pack(side="left", padx=5)

        self.show_labels_var = ctk.BooleanVar(value=False)
        ctk.CTkCheckBox(
            options_frame,
            text="Show Labels",
            variable=self.show_labels_var
        ).pack(side="left", padx=5)

        # 日誌區
        self.log_text = ctk.CTkTextbox(self, height=120, font=("Consolas", 10))
        self.log_text.pack(fill="x", padx=10, pady=5)

    def _log(self, msg: str):
        """寫入日誌"""
        timestamp = time.strftime("%H:%M:%S")
        text = f"[{timestamp}] {msg}\n"
        self.log_text.insert("end", text)
        self.log_text.see("end")
        # 限制日誌長度
        if int(self.log_text.index('end-1c').split('.')[0]) > 100:
            self.log_text.delete("1.0", "10.0")

    def _toggle_connect(self):
        """切換連接狀態"""
        if self.running:
            self._disconnect()
        else:
            self._connect()

    def _connect(self):
        """連接串口"""
        try:
            self._log(f"Connecting to {self.port}...")
            self.serial = serial.Serial(
                port=self.port,
                baudrate=BAUDRATE,
                timeout=0.1
            )
            self.running = True
            self.buffer = ""

            # 啟動讀取線程
            threading.Thread(target=self._read_thread, daemon=True).start()

            self.status_label.configure(text=f"Status: Connected | Port: {self.port}")
            self.connect_btn.configure(text="Disconnect", fg_color="red")
            self._log("Connected!")

        except Exception as e:
            self._log(f"Connect failed: {e}")

    def _disconnect(self):
        """斷開連接"""
        self.running = False
        if self.serial and self.serial.is_open:
            self.serial.close()
        self.status_label.configure(text=f"Status: Disconnected | Port: {self.port}")
        self.connect_btn.configure(text="Connect", fg_color="green")
        self._log("Disconnected")

    def _send_at(self):
        """發送 AT 測試"""
        if self.serial and self.serial.is_open:
            self.serial.write(b"AT\r\n")
            self._log("Sent: AT")

    def _send_invoke_start(self):
        """開始推論"""
        if self.serial and self.serial.is_open:
            self.serial.write(b"AT+INVOKE=1,0,0\r\n")
            self._log("Sent: AT+INVOKE=1,0,0 (Start)")

    def _send_invoke_stop(self):
        """停止推論"""
        if self.serial and self.serial.is_open:
            self.serial.write(b"AT+INVOKE=-1,0,0\r\n")
            self._log("Sent: AT+INVOKE=-1,0,0 (Stop)")

    def _read_thread(self):
        """串口讀取線程"""
        while self.running:
            try:
                if self.serial and self.serial.is_open and self.serial.in_waiting > 0:
                    data = self.serial.read(self.serial.in_waiting)
                    text = data.decode('utf-8', errors='ignore')
                    self.buffer += text
                    self._parse_buffer()
                else:
                    time.sleep(0.01)
            except Exception as e:
                print(f"[Read Error] {e}")
                time.sleep(0.1)

    def _parse_buffer(self):
        """解析 JSON 資料"""
        max_iter = 10
        iter_count = 0

        while iter_count < max_iter and '\n' in self.buffer:
            iter_count += 1
            newline_idx = self.buffer.find('\n')
            line = self.buffer[:newline_idx].strip()
            self.buffer = self.buffer[newline_idx + 1:]

            if line.startswith('{') and line.endswith('}'):
                try:
                    obj = json.loads(line)
                    self.data_queue.put(obj)
                except json.JSONDecodeError:
                    pass

    def _update_loop(self):
        """GUI 更新循環"""
        try:
            # 處理 JSON 資料
            count = 0
            while count < 5:
                try:
                    data = self.data_queue.get_nowait()
                    count += 1
                    self._process_data(data)
                except Empty:
                    break

            # 更新 FPS
            current_time = time.time()
            elapsed = current_time - self.last_fps_time
            if elapsed >= 1.0:
                self.fps = self.fps_frame_count / elapsed
                self.fps_frame_count = 0
                self.last_fps_time = current_time
                self.fps_label.configure(text=f"FPS: {self.fps:.1f}")

            # 更新統計
            self.stats_label.configure(
                text=f"Frames: {self.frame_count} | Hands: {self.hand_count}"
            )

        except Exception as e:
            print(f"[Update Error] {e}")

        self.after(UPDATE_INTERVAL_MS, self._update_loop)

    def _process_data(self, data: dict):
        """處理接收到的資料"""
        try:
            msg_type = data.get("type", -1)

            if msg_type == 1:  # INVOKE 結果
                self.frame_count += 1
                self.fps_frame_count += 1

                payload = data.get("data", {})
                if not isinstance(payload, dict):
                    return

                # 取得圖像
                image_b64 = payload.get("image", "")
                if not image_b64:
                    return

                # 解碼圖像
                image_data = base64.b64decode(image_b64)
                image = Image.open(io.BytesIO(image_data))

                if image.mode != 'RGB':
                    image = image.convert('RGB')

                # 取得解析度
                resolution = payload.get("resolution", [image.size[0], image.size[1]])
                img_w, img_h = resolution[0], resolution[1]

                # 取得手部資料
                hands = payload.get("hands", [])  # [[bbox, landmarks], ...]

                if hands:
                    self.hand_count = len(hands)

                    # 繪製手部
                    if self.show_landmarks_var.get() or self.show_bbox_var.get():
                        for hand_idx, hand in enumerate(hands):
                            if len(hand) >= 2:
                                bbox = hand[0]
                                landmarks = hand[1]

                                if self.show_bbox_var.get() and bbox:
                                    image = draw_hand_bbox(image, bbox, img_w, img_h)

                                if self.show_landmarks_var.get() and landmarks:
                                    image = draw_hand_landmarks(
                                        image, landmarks, img_w, img_h,
                                        hand_idx=hand_idx,
                                        draw_labels=self.show_labels_var.get()
                                    )
                else:
                    self.hand_count = 0

                # 縮放並顯示
                display_w, display_h = 640, 480
                img_w_orig, img_h_orig = image.size
                ratio = min(display_w / img_w_orig, display_h / img_h_orig)
                new_size = (int(img_w_orig * ratio), int(img_h_orig * ratio))
                image = image.resize(new_size, Image.Resampling.BILINEAR)

                photo = ImageTk.PhotoImage(image)
                self.image_label.configure(image=photo, text="")
                self.image_label.image = photo

            elif msg_type == 0:  # AT 回應
                name = data.get("name", "")
                self._log(f"AT Response: {name}")

        except Exception as e:
            self._log(f"Process error: {e}")
            import traceback
            traceback.print_exc()

    def on_closing(self):
        """關閉視窗"""
        self._disconnect()
        self.destroy()


def main():
    """主程式"""
    port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT

    print("=" * 50)
    print("  Hand Tracking Test - Grove Vision AI V2")
    print("=" * 50)
    print(f"  Port: {port}")
    print(f"  Baudrate: {BAUDRATE}")
    print()
    print("  Steps:")
    print("  1. Click 'Connect' to connect to device")
    print("  2. Click 'Send AT' to test connection")
    print("  3. Click 'Start INVOKE' to start hand tracking")
    print()
    print("=" * 50)

    ctk.set_appearance_mode("dark")
    app = HandTrackingTestApp(port)
    app.protocol("WM_DELETE_WINDOW", app.on_closing)
    app.mainloop()


if __name__ == "__main__":
    main()
