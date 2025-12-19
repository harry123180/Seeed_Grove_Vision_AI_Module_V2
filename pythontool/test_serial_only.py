#!/usr/bin/env python3
"""
獨立測試腳本 - 只測試串口讀取，不使用 GUI
目的：驗證串口讀取和 JSON 解析是否正常
"""

import serial
import json
import time
import sys

# 配置
PORT = "COM3"
BAUDRATE = 921600
TIMEOUT = 0.1

def test_serial_read():
    """測試純串口讀取"""
    print(f"=== 測試串口讀取 ===")
    print(f"Port: {PORT}, Baudrate: {BAUDRATE}")

    try:
        ser = serial.Serial(
            port=PORT,
            baudrate=BAUDRATE,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=TIMEOUT
        )
        print(f"串口已連接: {ser.is_open}")
    except Exception as e:
        print(f"串口連接失敗: {e}")
        return

    buffer = ""
    json_count = 0
    error_count = 0
    start_time = time.time()

    print("\n開始讀取數據（按 Ctrl+C 停止）...\n")

    try:
        while True:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                try:
                    text = data.decode('utf-8', errors='ignore')
                    buffer += text

                    # 簡單的 JSON 解析（按行）
                    while '\n' in buffer:
                        line, buffer = buffer.split('\n', 1)
                        line = line.strip()

                        if line.startswith('{') and line.endswith('}'):
                            try:
                                obj = json.loads(line)
                                json_count += 1

                                # 顯示 JSON 類型
                                msg_type = obj.get("type", -1)
                                name = obj.get("name", "")

                                if msg_type == 1:  # INVOKE
                                    data_obj = obj.get("data", {})
                                    img_len = len(data_obj.get("image", "")) if isinstance(data_obj, dict) else 0
                                    fm_points = data_obj.get("fm_points", []) if isinstance(data_obj, dict) else []
                                    print(f"[JSON #{json_count}] type={msg_type}, name={name}, img_len={img_len}, fm_points_len={len(fm_points)}")
                                else:
                                    print(f"[JSON #{json_count}] type={msg_type}, name={name}")

                            except json.JSONDecodeError as e:
                                error_count += 1
                                print(f"[JSON Error #{error_count}] {e}")
                                print(f"  Line length: {len(line)}")
                                print(f"  First 100 chars: {line[:100]}")
                                print(f"  Last 100 chars: {line[-100:]}")
                        elif line.startswith('{'):
                            # 不完整的 JSON
                            error_count += 1
                            print(f"[Incomplete JSON #{error_count}] length={len(line)}, ends with: ...{line[-50:]}")

                except Exception as e:
                    print(f"[Decode Error] {e}")
            else:
                time.sleep(0.01)

            # 每 5 秒顯示統計
            elapsed = time.time() - start_time
            if elapsed > 5:
                print(f"\n--- 統計: JSON={json_count}, Errors={error_count}, Buffer={len(buffer)} ---\n")
                start_time = time.time()

    except KeyboardInterrupt:
        print("\n\n=== 測試結束 ===")
        print(f"總 JSON 數量: {json_count}")
        print(f"總錯誤數量: {error_count}")
        print(f"剩餘緩衝區: {len(buffer)} 字元")
    finally:
        ser.close()
        print("串口已關閉")


if __name__ == "__main__":
    if len(sys.argv) > 1:
        PORT = sys.argv[1]
    test_serial_read()
