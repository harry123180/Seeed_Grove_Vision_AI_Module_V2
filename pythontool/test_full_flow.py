#!/usr/bin/env python3
"""
獨立測試腳本 - 完整流程測試（串口 + 線程 + Queue）
不使用 GUI，只用終端輸出
目的：驗證多線程 + Queue 的數據流是否正常
"""

import serial
import json
import time
import threading
from queue import Queue, Empty
import sys

# 配置
PORT = "COM3"
BAUDRATE = 921600


class SerialReaderTest(threading.Thread):
    """測試用串口讀取器"""

    def __init__(self, port, baudrate, data_queue):
        super().__init__(daemon=True)
        self.port = port
        self.baudrate = baudrate
        self.data_queue = data_queue
        self.serial = None
        self.running = False
        self.buffer = ""
        self.json_count = 0
        self.error_count = 0

    def connect(self):
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=0.1
            )
            self.running = True
            print(f"[Serial] 已連接: {self.port}")
            return True
        except Exception as e:
            print(f"[Serial] 連接失敗: {e}")
            return False

    def disconnect(self):
        self.running = False
        if self.serial and self.serial.is_open:
            self.serial.close()
        print("[Serial] 已斷開")

    def run(self):
        """執行緒主迴圈"""
        print("[Serial] 線程已啟動")
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
                print(f"[Serial Error] {e}")
                time.sleep(0.1)

    def _parse_buffer(self):
        """解析 JSON"""
        # 限制緩衝區大小
        if len(self.buffer) > 100000:
            print(f"[Buffer] 太大，清空: {len(self.buffer)}")
            self.buffer = ""
            return

        # 按行解析
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
                    self.json_count += 1
                    self.data_queue.put(obj)
                except json.JSONDecodeError as e:
                    self.error_count += 1
                    print(f"[JSON Error #{self.error_count}] {e}")


def process_data(data, frame_count):
    """處理數據"""
    if not isinstance(data, dict):
        print(f"[Process] 非 dict: {type(data)}")
        return

    msg_type = data.get("type", -1)
    name = data.get("name", "")

    if msg_type == 1:  # INVOKE
        payload = data.get("data", {})
        if isinstance(payload, dict):
            img_len = len(payload.get("image", ""))
            fm_points = payload.get("fm_points", [])
            print(f"[Frame #{frame_count}] INVOKE: img={img_len}, fm_points={len(fm_points)}")
        else:
            print(f"[Frame #{frame_count}] INVOKE: payload not dict")
    else:
        print(f"[RX] type={msg_type}, name={name}")


def main():
    global PORT
    if len(sys.argv) > 1:
        PORT = sys.argv[1]

    print("=== 完整流程測試（無 GUI）===")
    print(f"Port: {PORT}, Baudrate: {BAUDRATE}")
    print()

    data_queue = Queue()
    reader = SerialReaderTest(PORT, BAUDRATE, data_queue)

    if not reader.connect():
        return

    reader.start()

    frame_count = 0
    start_time = time.time()

    print("\n開始處理數據（按 Ctrl+C 停止）...\n")

    try:
        while True:
            # 處理 Queue（模擬主線程）
            try:
                data = data_queue.get(timeout=0.1)
                frame_count += 1
                process_data(data, frame_count)
            except Empty:
                pass

            # 每 5 秒統計
            elapsed = time.time() - start_time
            if elapsed > 5:
                qsize = data_queue.qsize()
                print(f"\n--- 統計: Frames={frame_count}, JSON={reader.json_count}, Errors={reader.error_count}, Queue={qsize} ---\n")
                start_time = time.time()

    except KeyboardInterrupt:
        print("\n\n=== 測試結束 ===")
        print(f"總 Frames: {frame_count}")
        print(f"總 JSON: {reader.json_count}")
        print(f"總 Errors: {reader.error_count}")
        print(f"Queue 剩餘: {data_queue.qsize()}")
    finally:
        reader.disconnect()


if __name__ == "__main__":
    main()
