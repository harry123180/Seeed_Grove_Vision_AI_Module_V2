"""
從板子擷取一張照片並保存

用法：
    python capture_frame.py --port COM3
"""

import serial
import json
import base64
import argparse
import time
from datetime import datetime

def capture_frame(port, baudrate=921600, timeout=5, warmup=3, skip_frames=5):
    """從板子擷取一張照片"""
    print(f"連接 {port}...")

    ser = serial.Serial(port, baudrate, timeout=1)
    time.sleep(0.5)  # 等待連接穩定

    print(f"等待相機穩定 ({warmup} 秒)...")
    time.sleep(warmup)

    print(f"跳過前 {skip_frames} 張不穩定的照片...")
    frame_count = 0
    start_time = time.time()
    extended_timeout = timeout + skip_frames * 2  # 給更多時間

    while time.time() - start_time < extended_timeout:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if not line:
                continue

            # 尋找包含 INVOKE 的 JSON
            if '"name": "INVOKE"' in line or '"name":"INVOKE"' in line:
                # 嘗試解析 JSON
                try:
                    # 可能需要找到 JSON 的開始和結束
                    start_idx = line.find('{')
                    if start_idx >= 0:
                        json_str = line[start_idx:]
                        data = json.loads(json_str)

                        if 'data' in data and 'image' in data['data']:
                            frame_count += 1

                            # 跳過前幾張不穩定的
                            if frame_count <= skip_frames:
                                print(f"  跳過 frame {frame_count}/{skip_frames}")
                                continue

                            image_b64 = data['data']['image']

                            # 解碼 base64
                            image_data = base64.b64decode(image_b64)

                            # 保存圖片
                            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                            filename = f"capture_{timestamp}.jpg"

                            with open(filename, 'wb') as f:
                                f.write(image_data)

                            print(f"已保存: {filename} ({len(image_data)} bytes)")

                            # 同時顯示偵測結果
                            if 'hands' in data['data']:
                                hands = data['data']['hands']
                                print(f"偵測到 {len(hands)} 隻手")
                                for i, hand in enumerate(hands):
                                    bbox = hand[0][0]  # [[bbox], landmarks...]
                                    print(f"  Hand {i}: x={bbox[0]}, y={bbox[1]}, w={bbox[2]}, h={bbox[3]}, score={bbox[4]}%")

                            ser.close()
                            return filename

                except json.JSONDecodeError:
                    pass  # 繼續嘗試

        except Exception as e:
            print(f"錯誤: {e}")

    ser.close()
    print("超時，未收到圖片資料")
    return None

def main():
    parser = argparse.ArgumentParser(description='從板子擷取照片')
    parser.add_argument('--port', type=str, default='COM3', help='串口')
    parser.add_argument('--baudrate', type=int, default=921600, help='波特率')
    parser.add_argument('--warmup', type=int, default=3, help='相機預熱時間(秒)')
    parser.add_argument('--skip', type=int, default=5, help='跳過前N張不穩定的照片')
    args = parser.parse_args()

    filename = capture_frame(args.port, args.baudrate, warmup=args.warmup, skip_frames=args.skip)

    if filename:
        print(f"\n可以用這張圖片測試 PC 模型：")
        print(f"  python test_palm_model.py --image {filename}")

if __name__ == "__main__":
    main()
