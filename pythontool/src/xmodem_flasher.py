#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""XMODEM 燒錄模組"""

import serial
import time
import os
import math
import threading
from typing import Optional, Callable, List
from dataclasses import dataclass

try:
    import xmodem
except ImportError:
    xmodem = None


@dataclass
class FlashProgress:
    """燒錄進度"""
    current_file: str = ""
    total_packets: int = 0
    sent_packets: int = 0
    error_count: int = 0
    status: str = "idle"  # idle, waiting, flashing, done, error
    log_message: str = ""  # 日誌訊息


class XmodemFlasher:
    """XMODEM 燒錄器"""

    DEF_TIMEOUT = 60
    DEF_BAUDRATE = 921600

    def __init__(self):
        self.serial: Optional[serial.Serial] = None
        self.progress = FlashProgress()
        self.progress_callback: Optional[Callable[[FlashProgress], None]] = None
        self._stop_flag = False
        self._stop_sending = False

    def set_progress_callback(self, callback: Callable[[FlashProgress], None]):
        """設定進度回調"""
        self.progress_callback = callback

    def _update_progress(self, log_msg: str = ""):
        """更新進度"""
        if log_msg:
            self.progress.log_message = log_msg
        if self.progress_callback:
            self.progress_callback(self.progress)

    def _log(self, msg: str):
        """發送日誌"""
        self._update_progress(msg)

    def connect(self, port: str, baudrate: int = DEF_BAUDRATE, timeout: int = DEF_TIMEOUT) -> bool:
        """連接串口"""
        try:
            self._log(f"正在連接 {port} @ {baudrate}...")
            self.serial = serial.Serial(
                port=port,
                baudrate=baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=timeout
            )
            self.serial.flushInput()
            self.serial.flushOutput()
            self._log(f"串口連接成功: {port}")
            return True
        except Exception as e:
            self.progress.status = "error"
            self._log(f"連接失敗: {e}")
            return False

    def disconnect(self):
        """斷開連接"""
        if self.serial and self.serial.is_open:
            self.serial.close()
            self._log("串口已斷開")
        self.serial = None

    def stop(self):
        """停止燒錄"""
        self._stop_flag = True
        self._log("使用者停止燒錄")

    def _send_command(self, command: str):
        """發送指令"""
        if self.serial:
            self.serial.write(bytes(command + "\r", encoding='ascii'))

    def _getc(self, size, timeout=1):
        """XMODEM getc"""
        return self.serial.read(size) if self.serial else None

    def _putc(self, data, timeout=1):
        """XMODEM putc"""
        return self.serial.write(data) if self.serial else None

    def _xmodem_callback(self, total_packets, success_count, error_count):
        """XMODEM 進度回調"""
        self.progress.sent_packets = total_packets
        self.progress.error_count = error_count
        pct = (total_packets / self.progress.total_packets * 100) if self.progress.total_packets > 0 else 0
        self._log(f"傳輸中: {total_packets}/{self.progress.total_packets} ({pct:.1f}%)")

    def _send_keys_continuously(self):
        """Background thread to continuously send '1' to catch bootloader"""
        while not self._stop_sending:
            try:
                if self.serial and self.serial.is_open:
                    self.serial.write(b'1')
            except:
                pass
            time.sleep(0.05)  # Send every 50ms

    def wait_for_bootloader(self, timeout: int = 60) -> bool:
        """等待 Bootloader 就緒 - 持續發送按鍵以捕捉 bootloader"""
        self.progress.status = "waiting"
        self._log("=" * 40)
        self._log("請按下開發板的 RESET 按鈕!")
        self._log("(程式會持續發送按鍵以捕捉 bootloader)")
        self._log("=" * 40)

        # 設置較短的 timeout
        if self.serial:
            self.serial.timeout = 0.2

        # Start background thread to continuously send keys
        self._stop_sending = False
        sender_thread = threading.Thread(target=self._send_keys_continuously, daemon=True)
        sender_thread.start()

        start_time = time.time()
        bootloader_detected = False

        while time.time() - start_time < timeout:
            if self._stop_flag:
                self._stop_sending = True
                self._log("使用者取消")
                return False

            try:
                # 讀取串口數據
                if self.serial and self.serial.in_waiting > 0:
                    raw_data = self.serial.readline()
                else:
                    raw_data = b''

                if raw_data:
                    # 解碼顯示
                    try:
                        display_msg = raw_data.decode('utf-8', errors='ignore').strip()
                    except:
                        display_msg = str(raw_data)

                    if display_msg:
                        self._log(f"[RX] {display_msg}")

                    response_str = str(raw_data).lower()

                    # 檢測 Bootloader 啟動訊息
                    if 'input any key' in response_str or 'x-modem' in response_str or '1. flash image update' in response_str:
                        self._log("檢測到 Bootloader!")
                        bootloader_detected = True

                    # 檢查是否收到 xmodem 就緒訊息
                    if 'send data using the xmodem protocol' in response_str:
                        self._stop_sending = True
                        self._log("Bootloader 就緒! XMODEM 模式已啟動")
                        time.sleep(1.0)  # 等待 1 秒讓設備完全準備好
                        if self.serial:
                            self.serial.flushInput()
                        self._send_command('1')  # 確認進入 xmodem 模式
                        time.sleep(0.5)
                        if self.serial:
                            self.serial.flushInput()
                        return True

            except Exception as e:
                self._log(f"讀取錯誤: {e}")

            time.sleep(0.05)

        self._stop_sending = True
        self.progress.status = "error"
        self._log(f"等待 Bootloader 超時 ({timeout}秒)")
        self._log("提示: 請在看到提示後立即按下 RESET 按鈕")
        return False

    def flash_firmware(self, firmware_path: str, protocol: str = 'xmodem') -> bool:
        """燒錄韌體"""
        if not xmodem:
            self._log("錯誤: 缺少 xmodem 模組，請執行: pip install xmodem")
            self.progress.status = "error"
            return False

        if not os.path.exists(firmware_path):
            self._log(f"錯誤: 檔案不存在: {firmware_path}")
            self.progress.status = "error"
            return False

        packet_size = 128 if protocol == 'xmodem' else 1024
        file_size = os.path.getsize(firmware_path)

        self.progress.status = "flashing"
        self.progress.current_file = os.path.basename(firmware_path)
        self.progress.total_packets = math.ceil(file_size / packet_size)
        self.progress.sent_packets = 0

        self._log(f"開始燒錄韌體: {self.progress.current_file}")
        self._log(f"檔案大小: {file_size} bytes, 封包數: {self.progress.total_packets}")

        modem = xmodem.XMODEM(getc=self._getc, putc=self._putc, mode=protocol)

        with open(firmware_path, 'rb') as f:
            result = modem.send(f, callback=self._xmodem_callback)

        if result:
            self._log("韌體燒錄成功!")
        else:
            self._log("韌體燒錄失敗!")

        return result

    def flash_model(self, model_path: str, flash_addr: int, offset: int = 0, protocol: str = 'xmodem') -> bool:
        """燒錄模型"""
        if not xmodem:
            self._log("錯誤: 缺少 xmodem 模組")
            self.progress.status = "error"
            return False

        if not os.path.exists(model_path):
            self._log(f"錯誤: 檔案不存在: {model_path}")
            self.progress.status = "error"
            return False

        packet_size = 128 if protocol == 'xmodem' else 1024
        modem = xmodem.XMODEM(getc=self._getc, putc=self._putc, mode=protocol)

        # 建立 preamble
        self._log(f"準備模型: {os.path.basename(model_path)} @ 0x{flash_addr:X}")
        preamble = self._create_preamble(flash_addr, offset, packet_size)

        # 發送 preamble
        self.progress.current_file = f"Preamble"
        self.progress.total_packets = 1
        self.progress.sent_packets = 0

        import io
        preamble_stream = io.BytesIO(preamble)
        self._log("發送 Preamble...")
        if not modem.send(preamble_stream, callback=self._xmodem_callback):
            self._log("Preamble 發送失敗!")
            return False
        self._log("Preamble 發送成功")

        # 等待確認
        self._wait_for_reboot_prompt()
        self._send_command('n')
        self._log("繼續傳輸模型...")
        time.sleep(1)
        if self.serial:
            self.serial.flushInput()

        # 發送模型
        file_size = os.path.getsize(model_path)
        self.progress.current_file = os.path.basename(model_path)
        self.progress.total_packets = math.ceil(file_size / packet_size)
        self.progress.sent_packets = 0

        self._log(f"開始燒錄模型: {self.progress.current_file}")
        self._log(f"檔案大小: {file_size} bytes")

        with open(model_path, 'rb') as f:
            result = modem.send(f, callback=self._xmodem_callback)

        if result:
            self._log(f"模型燒錄成功: {os.path.basename(model_path)}")
        else:
            self._log(f"模型燒錄失敗: {os.path.basename(model_path)}")

        return result

    def _create_preamble(self, flash_addr: int, offset: int, packet_size: int) -> bytes:
        """建立 model preamble"""
        header = [0xC0, 0x5A]
        header += list(flash_addr.to_bytes(4, 'little'))
        header += list(offset.to_bytes(4, 'little'))
        header += [0x5A, 0xC0]
        header += [0xFF] * (packet_size - 12)
        return bytes(header)

    def _wait_for_reboot_prompt(self, timeout: int = 30) -> bool:
        """等待重啟提示"""
        self._log("等待裝置回應...")
        start_time = time.time()
        while time.time() - start_time < timeout:
            if self._stop_flag:
                return False
            try:
                if self.serial and self.serial.in_waiting > 0:
                    raw_data = self.serial.readline()
                    try:
                        response = raw_data.decode('utf-8', errors='ignore').strip()
                    except:
                        response = str(raw_data)

                    if response:
                        self._log(f"[RX] {response}")

                    if 'reboot' in response.lower() or '(y)' in response:
                        return True
            except Exception:
                pass
            time.sleep(0.1)
        return False

    def reboot_device(self):
        """重啟裝置"""
        self._log("等待重啟確認...")
        self._wait_for_reboot_prompt()
        self._send_command('y')
        self.progress.status = "done"
        self._log("=" * 40)
        self._log("燒錄完成！裝置正在重啟...")
        self._log("=" * 40)

    def flash_all(self, firmware_path: Optional[str], models: List[tuple],
                  protocol: str = 'xmodem') -> bool:
        """燒錄韌體和所有模型"""
        self._stop_flag = False
        self._log("開始燒錄流程...")

        # 等待 Bootloader
        if not self.wait_for_bootloader():
            return False

        # 燒錄韌體
        if firmware_path:
            if not self.flash_firmware(firmware_path, protocol):
                self.progress.status = "error"
                return False

        # 燒錄模型
        for i, (model_path, flash_addr, offset) in enumerate(models):
            if self._stop_flag:
                return False

            self._log(f"--- 模型 {i+1}/{len(models)} ---")
            self._wait_for_reboot_prompt()
            self._send_command('n')
            time.sleep(1)
            if self.serial:
                self.serial.flushInput()

            if not self.flash_model(model_path, flash_addr, offset, protocol):
                self.progress.status = "error"
                return False

        # 重啟
        self.reboot_device()
        return True
