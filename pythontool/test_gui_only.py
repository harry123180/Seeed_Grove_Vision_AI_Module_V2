#!/usr/bin/env python3
"""
獨立測試腳本 - 只測試 GUI，使用模擬數據
目的：驗證 GUI 和圖像處理是否正常
"""

import customtkinter as ctk
from PIL import Image, ImageTk
import base64
import io
import time
import threading
from queue import Queue, Empty

# 一個簡單的 JPEG base64（灰色 10x10 圖像）
SIMPLE_IMAGE_B64 = "/9j/4AAQSkZJRgABAQEASABIAAD/2wBDAAgGBgcGBQgHBwcJCQgKDBQNDAsLDBkSEw8UHRofHh0aHBwgJC4nICIsIxwcKDcpLDAxNDQ0Hyc5PTgyPC4zNDL/2wBDAQkJCQwLDBgNDRgyIRwhMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjL/wAARCAAKAAoDASIAAhEBAxEB/8QAFgABAQEAAAAAAAAAAAAAAAAABgcI/8QAIhAAAgICAgICAwAAAAAAAAAAAQIDBAURBgASITEHE0FR/8QAFQEBAQAAAAAAAAAAAAAAAAAAAAX/xAAZEQADAQEBAAAAAAAAAAAAAAABAgMAERL/2gAMAwEAAhEDEEEBAP8AFtM5LlnKuS1slk8rYt2oIjXilsyGR1jLKxUFiSBsKP8AACggOBwPXSorLqGmZ//Z"


class TestApp(ctk.CTk):
    """測試用 GUI"""

    def __init__(self):
        super().__init__()
        self.title("GUI 測試")
        self.geometry("800x600")

        self.data_queue = Queue()
        self.frame_count = 0
        self.error_count = 0

        # 狀態標籤
        self.status_label = ctk.CTkLabel(self, text="等待數據...", font=("Arial", 14))
        self.status_label.pack(pady=10)

        # 圖像顯示
        self.image_label = ctk.CTkLabel(self, text="No Image")
        self.image_label.pack(fill="both", expand=True, padx=20, pady=20)

        # 按鈕
        btn_frame = ctk.CTkFrame(self)
        btn_frame.pack(pady=10)

        ctk.CTkButton(btn_frame, text="發送模擬數據", command=self._send_test_data).pack(side="left", padx=5)
        ctk.CTkButton(btn_frame, text="啟動自動發送", command=self._start_auto_send).pack(side="left", padx=5)
        ctk.CTkButton(btn_frame, text="停止", command=self._stop_auto_send).pack(side="left", padx=5)

        self.auto_sending = False
        self._update_loop()

    def _send_test_data(self):
        """發送模擬數據到 Queue"""
        test_data = {
            "type": 1,
            "name": "INVOKE",
            "code": 0,
            "data": {
                "count": self.frame_count,
                "image": SIMPLE_IMAGE_B64,
                "boxes": [],
                "fm_points": []
            }
        }
        self.data_queue.put(test_data)
        print(f"[Test] 發送模擬數據 #{self.frame_count}")

    def _start_auto_send(self):
        """啟動自動發送線程"""
        if not self.auto_sending:
            self.auto_sending = True
            threading.Thread(target=self._auto_send_thread, daemon=True).start()
            print("[Test] 自動發送已啟動")

    def _stop_auto_send(self):
        """停止自動發送"""
        self.auto_sending = False
        print("[Test] 自動發送已停止")

    def _auto_send_thread(self):
        """自動發送線程"""
        while self.auto_sending:
            self._send_test_data()
            time.sleep(0.1)  # 10 FPS

    def _update_loop(self):
        """UI 更新迴圈"""
        try:
            # 處理 Queue 中的數據
            count = 0
            while count < 5:
                try:
                    data = self.data_queue.get_nowait()
                    count += 1
                    self._process_data(data)
                except Empty:
                    break

        except Exception as e:
            self.error_count += 1
            print(f"[Update Error #{self.error_count}] {e}")

        self.after(50, self._update_loop)

    def _process_data(self, data):
        """處理數據"""
        try:
            if not isinstance(data, dict):
                print(f"[Error] Expected dict, got {type(data)}")
                return

            msg_type = data.get("type", -1)
            if msg_type != 1:
                return

            payload = data.get("data", {})
            if not isinstance(payload, dict):
                print(f"[Error] Payload not dict: {type(payload)}")
                return

            image_b64 = payload.get("image", "")
            if not image_b64:
                print("[Warning] No image data")
                return

            # 解碼圖像
            image_data = base64.b64decode(image_b64)
            image = Image.open(io.BytesIO(image_data))

            # 放大顯示
            image = image.resize((400, 400), Image.Resampling.NEAREST)

            # 更新 GUI
            photo = ImageTk.PhotoImage(image)
            self.image_label.configure(image=photo, text="")
            self.image_label.image = photo

            self.frame_count += 1
            self.status_label.configure(text=f"Frame: {self.frame_count}, Errors: {self.error_count}")

            print(f"[Frame #{self.frame_count}] 圖像更新成功")

        except Exception as e:
            self.error_count += 1
            print(f"[Process Error #{self.error_count}] {e}")
            import traceback
            traceback.print_exc()


if __name__ == "__main__":
    print("=== GUI 測試 ===")
    print("點擊 '發送模擬數據' 測試單次更新")
    print("點擊 '啟動自動發送' 測試連續更新")
    print()

    ctk.set_appearance_mode("dark")
    app = TestApp()
    app.mainloop()
