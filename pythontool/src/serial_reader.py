#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""串口讀取模組"""

import serial
import serial.tools.list_ports
import threading
import json
import time
from typing import Optional
from queue import Queue


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
        """解析緩衝區中的 JSON 資料 - 處理交錯的 debug 輸出"""
        try:
            # 限制緩衝區大小
            if len(self.buffer) > 500000:
                print(f"[Buffer Warning] Too large ({len(self.buffer)}), clearing...")
                self.buffer = ""
                return

            max_iterations = 10
            iterations = 0

            while iterations < max_iterations:
                iterations += 1

                # 找到 JSON 開始 '{"'（更精確的匹配）
                start_idx = self.buffer.find('{"')
                if start_idx == -1:
                    # 沒有 JSON，保留最後 1000 字元（可能是不完整的 JSON）
                    if len(self.buffer) > 1000:
                        self.buffer = self.buffer[-1000:]
                    break

                # 丟棄 JSON 之前的內容
                if start_idx > 0:
                    self.buffer = self.buffer[start_idx:]

                # 找到 JSON 結尾 '}}\n' 或 '"}}\n'（完整的 JSON 行）
                # 支援多種結尾模式
                end_patterns = ['}}\n', '"}\n', ']}\n']
                end_idx = -1

                for pattern in end_patterns:
                    idx = self.buffer.find(pattern)
                    if idx != -1:
                        if end_idx == -1 or idx < end_idx:
                            end_idx = idx + len(pattern) - 1  # 指向 '\n' 前的位置

                if end_idx == -1:
                    # 還沒收到完整 JSON，等待更多資料
                    break

                # 提取 JSON 行
                line = self.buffer[:end_idx].strip()
                self.buffer = self.buffer[end_idx + 1:]

                # 驗證並解析 JSON
                if line.startswith('{') and line.endswith('}'):
                    try:
                        data = json.loads(line)
                        self.data_queue.put(data)
                    except json.JSONDecodeError as e:
                        # 可能是交錯的資料，嘗試清理
                        cleaned = self._try_clean_json(line)
                        if cleaned:
                            try:
                                data = json.loads(cleaned)
                                self.data_queue.put(data)
                            except:
                                pass  # 放棄這個損壞的 JSON

        except Exception as e:
            print(f"[Parse Buffer Error] {e}")
            self.buffer = ""

    def _try_clean_json(self, line: str) -> str:
        """嘗試清理交錯的 JSON（移除嵌入的 debug 輸出）"""
        import re
        # 移除 SENSORDPLIB_STATUS_... 等 debug 輸出
        cleaned = re.sub(r'SENSORDPLIB_[A-Z_]+\s+\d+\s*', '', line)
        cleaned = re.sub(r'\n+', '', cleaned)
        return cleaned.strip()


def list_serial_ports():
    """列出可用串口"""
    return [port.device for port in serial.tools.list_ports.comports()]
