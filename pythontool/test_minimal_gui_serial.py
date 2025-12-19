#!/usr/bin/env python3
"""
最小化 GUI + 串口測試
逐步啟用功能以定位崩潰原因
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

# 配置
PORT = "COM3"
BAUDRATE = 921600


class MinimalGUITest(ctk.CTk):
    """最小化 GUI 測試"""

    def __init__(self, port):
        super().__init__()
        self.title("Minimal GUI + Serial Test")
        self.geometry("800x600")

        self.port = port
        self.serial = None
        self.data_queue = Queue()
        self.running = False
        self.frame_count = 0
        self.error_count = 0
        self.buffer = ""

        # === GUI 組件 ===
        self.status_label = ctk.CTkLabel(self, text="Status: Not connected", font=("Arial", 14))
        self.status_label.pack(pady=10)

        self.frame_label = ctk.CTkLabel(self, text="Frames: 0 | Errors: 0", font=("Arial", 12))
        self.frame_label.pack(pady=5)

        # 圖像顯示
        self.image_label = ctk.CTkLabel(self, text="No Image", width=640, height=400)
        self.image_label.pack(pady=10)

        # 控制按鈕
        btn_frame = ctk.CTkFrame(self)
        btn_frame.pack(pady=10)

        self.connect_btn = ctk.CTkButton(btn_frame, text="Connect", command=self._toggle_connect)
        self.connect_btn.pack(side="left", padx=5)

        ctk.CTkButton(btn_frame, text="Send AT", command=self._send_at).pack(side="left", padx=5)
        ctk.CTkButton(btn_frame, text="Send INVOKE", command=self._send_invoke).pack(side="left", padx=5)

        # 功能開關
        switch_frame = ctk.CTkFrame(self)
        switch_frame.pack(pady=10)

        self.decode_image_var = ctk.BooleanVar(value=True)
        ctk.CTkSwitch(switch_frame, text="Decode Image", variable=self.decode_image_var).pack(side="left", padx=10)

        self.display_image_var = ctk.BooleanVar(value=True)
        ctk.CTkSwitch(switch_frame, text="Display Image", variable=self.display_image_var).pack(side="left", padx=10)

        # 日誌
        self.log_text = ctk.CTkTextbox(self, height=150, font=("Consolas", 10))
        self.log_text.pack(fill="x", padx=10, pady=5)

        # 啟動更新循環
        self._update_loop()

    def _log(self, msg):
        """寫入日誌"""
        timestamp = time.strftime("%H:%M:%S")
        text = f"[{timestamp}] {msg}\n"
        self.log_text.insert("end", text)
        self.log_text.see("end")
        print(text.strip())  # 同時輸出到終端機

    def _toggle_connect(self):
        """切換連接"""
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

            self.status_label.configure(text=f"Status: Connected to {self.port}")
            self.connect_btn.configure(text="Disconnect", fg_color="red")
            self._log("Connected!")

        except Exception as e:
            self._log(f"Connect failed: {e}")

    def _disconnect(self):
        """斷開連接"""
        self.running = False
        if self.serial and self.serial.is_open:
            self.serial.close()
        self.status_label.configure(text="Status: Disconnected")
        self.connect_btn.configure(text="Connect", fg_color="green")
        self._log("Disconnected")

    def _send_at(self):
        """發送 AT 指令"""
        if self.serial and self.serial.is_open:
            self.serial.write(b"AT\r\n")
            self._log("Sent: AT")

    def _send_invoke(self):
        """發送 INVOKE 指令"""
        if self.serial and self.serial.is_open:
            self.serial.write(b"AT+INVOKE=1,0,0\r\n")
            self._log("Sent: AT+INVOKE=1,0,0")

    def _read_thread(self):
        """串口讀取線程"""
        self._log("Read thread started")
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
        """解析 JSON"""
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
                except json.JSONDecodeError as e:
                    print(f"[JSON Error] {e}")

    def _update_loop(self):
        """GUI 更新循環"""
        try:
            # 處理最多 5 個 JSON
            count = 0
            while count < 5:
                try:
                    data = self.data_queue.get_nowait()
                    count += 1
                    self._process_json(data)
                except Empty:
                    break

            # 更新 frame 計數
            self.frame_label.configure(text=f"Frames: {self.frame_count} | Errors: {self.error_count}")

        except Exception as e:
            self.error_count += 1
            print(f"[Update Loop Error] {e}")

        self.after(50, self._update_loop)

    def _process_json(self, data):
        """處理 JSON 資料"""
        try:
            msg_type = data.get("type", -1)
            name = data.get("name", "")

            if msg_type == 1:  # INVOKE result
                self.frame_count += 1
                payload = data.get("data", {})

                if not isinstance(payload, dict):
                    self._log(f"Frame #{self.frame_count}: payload not dict")
                    return

                image_b64 = payload.get("image", "")
                img_len = len(image_b64) if image_b64 else 0
                fm_len = len(payload.get("fm_points", []))

                self._log(f"Frame #{self.frame_count}: img={img_len}, fm={fm_len}")

                # 測試圖像解碼
                if self.decode_image_var.get() and image_b64:
                    try:
                        image_data = base64.b64decode(image_b64)
                        image = Image.open(io.BytesIO(image_data))
                        self._log(f"  -> Image decoded: {image.size}")

                        # 測試圖像顯示
                        if self.display_image_var.get():
                            try:
                                # 縮放
                                display_image = image.resize((640, 400), Image.Resampling.LANCZOS)
                                photo = ImageTk.PhotoImage(display_image)
                                self.image_label.configure(image=photo, text="")
                                self.image_label.image = photo  # 保持引用
                                self._log(f"  -> Image displayed")
                            except Exception as e:
                                self.error_count += 1
                                self._log(f"  -> Display error: {e}")
                    except Exception as e:
                        self.error_count += 1
                        self._log(f"  -> Decode error: {e}")
            else:
                self._log(f"RX: type={msg_type}, name={name}")

        except Exception as e:
            self.error_count += 1
            self._log(f"Process error: {e}")
            import traceback
            traceback.print_exc()

    def on_closing(self):
        """關閉視窗"""
        self._disconnect()
        self.destroy()


if __name__ == "__main__":
    port = sys.argv[1] if len(sys.argv) > 1 else PORT

    print("=== Minimal GUI + Serial Test ===")
    print(f"Port: {port}, Baudrate: {BAUDRATE}")
    print()
    print("Steps to test:")
    print("1. Click 'Connect' - should not crash")
    print("2. Click 'Send AT' - should show response")
    print("3. Click 'Send INVOKE' - should receive frames")
    print()
    print("If crash occurs, note which step!")
    print()

    ctk.set_appearance_mode("dark")
    app = MinimalGUITest(port)
    app.protocol("WM_DELETE_WINDOW", app.on_closing)
    app.mainloop()
