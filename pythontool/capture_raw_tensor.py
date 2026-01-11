#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
擷取韌體 Raw Tensor 輸出

用法:
    python capture_raw_tensor.py --port COM3

會自動記錄到 logs/raw_tensor_*.log
"""

import serial
import argparse
import time
import os
from datetime import datetime

LOG_DIR = "logs"
os.makedirs(LOG_DIR, exist_ok=True)

def main():
    parser = argparse.ArgumentParser(description='擷取 Raw Tensor 輸出')
    parser.add_argument('--port', type=str, default='COM3', help='串口')
    parser.add_argument('--baudrate', type=int, default=921600, help='波特率')
    parser.add_argument('--duration', type=int, default=30, help='擷取時間(秒)')
    args = parser.parse_args()

    log_file = os.path.join(LOG_DIR, f"raw_tensor_{datetime.now().strftime('%Y%m%d_%H%M%S')}.log")

    print(f"連接 {args.port} @ {args.baudrate}")
    print(f"Log 檔案: {log_file}")
    print(f"擷取時間: {args.duration} 秒")
    print("=" * 50)

    try:
        ser = serial.Serial(
            port=args.port,
            baudrate=args.baudrate,
            timeout=0.1
        )
    except Exception as e:
        print(f"無法連接: {e}")
        return

    # 發送 AT 指令啟動
    print("發送 AT 指令...")
    time.sleep(0.3)
    ser.write(b"AT\r\n")
    time.sleep(0.2)
    ser.write(b"AT+INVOKE=1,0,0\r\n")
    time.sleep(0.3)
    print("開始擷取...\n")

    start_time = time.time()
    frame_count = 0

    with open(log_file, 'w', encoding='utf-8') as f:
        f.write(f"Raw Tensor Capture - {datetime.now()}\n")
        f.write(f"Port: {args.port}, Baudrate: {args.baudrate}\n")
        f.write("=" * 60 + "\n\n")

        buffer = ""

        try:
            while time.time() - start_time < args.duration:
                if ser.in_waiting > 0:
                    data = ser.read(ser.in_waiting)
                    try:
                        text = data.decode('utf-8', errors='ignore')
                        buffer += text

                        # 逐行處理
                        while '\n' in buffer:
                            line, buffer = buffer.split('\n', 1)
                            line = line.strip()

                            # 只記錄 [RAW_TENSOR] 行
                            if '[RAW_TENSOR]' in line:
                                print(line)
                                f.write(line + "\n")
                                f.flush()

                                if 'Frame' in line:
                                    frame_count += 1

                    except Exception as e:
                        pass
                else:
                    time.sleep(0.01)

        except KeyboardInterrupt:
            print("\n中斷")

    ser.close()

    print("\n" + "=" * 50)
    print(f"完成! 擷取了 {frame_count} 幀")
    print(f"Log 存於: {log_file}")

if __name__ == "__main__":
    main()
