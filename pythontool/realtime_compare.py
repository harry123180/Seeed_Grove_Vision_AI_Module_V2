#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
即時比較工具：PC 模型 vs 板子韌體

功能：
- 接收板子的圖像串流
- 同時顯示 PC 模型結果和板子韌體結果
- 可切換顯示模式：PC / 板子 / 疊圖

操作：
- 1: 只顯示 PC 結果 (綠框)
- 2: 只顯示板子結果 (紅框)
- 3: 疊圖顯示兩者
- S: 儲存當前畫面
- Q: 退出
"""

import serial
import serial.tools.list_ports
import json
import base64
import numpy as np
import cv2
import tensorflow as tf
import argparse
import time
import threading
import re
import os
from datetime import datetime
from queue import Queue, Empty
from io import BytesIO
from PIL import Image

# ============================================
# Logging
# ============================================
LOG_DIR = "logs"
os.makedirs(LOG_DIR, exist_ok=True)
LOG_FILE = os.path.join(LOG_DIR, f"compare_{datetime.now().strftime('%Y%m%d_%H%M%S')}.log")

def log_result(frame_id, pc_dets, board_dets, log_file):
    """記錄偵測結果到 log 檔案"""
    with open(log_file, 'a', encoding='utf-8') as f:
        f.write(f"\n{'='*60}\n")
        f.write(f"Frame {frame_id} @ {datetime.now().strftime('%H:%M:%S.%f')[:-3]}\n")
        f.write(f"{'='*60}\n")

        f.write(f"\n[PC Model] {len(pc_dets)} detections:\n")
        for i, det in enumerate(pc_dets):
            x, y, w, h = det['bbox']
            score = det['score']
            f.write(f"  PC[{i}]: x={x}, y={y}, w={w}, h={h}, score={score:.2%}\n")

        f.write(f"\n[Board FW] {len(board_dets)} detections:\n")
        for i, det in enumerate(board_dets):
            x, y, w, h = det['bbox']
            score = det['score']
            f.write(f"  FW[{i}]: x={x}, y={y}, w={w}, h={h}, score={score:.2%}\n")

        # 比較分析
        if pc_dets and board_dets:
            pc_box = pc_dets[0]['bbox']
            fw_box = board_dets[0]['bbox']
            f.write(f"\n[Compare] Top detection:\n")
            f.write(f"  PC: ({pc_box[0]}, {pc_box[1]}) size=({pc_box[2]}x{pc_box[3]})\n")
            f.write(f"  FW: ({fw_box[0]}, {fw_box[1]}) size=({fw_box[2]}x{fw_box[3]})\n")

            # 計算差異 (PC 座標需要先轉換到相同空間才能比較)
            f.write(f"  Delta: dx={fw_box[0]-pc_box[0]}, dy={fw_box[1]-pc_box[1]}\n")

# ============================================
# Model Configuration
# ============================================
MODEL_PATH = "../model_zoo/tflm_hand_tracking/0_palm_det_0x400000.tflite"
INPUT_SIZE = 256

# ============================================
# Anchor Generation (與板子同步)
# ============================================
def generate_anchors():
    anchors = []
    layer_configs = [
        (8, 2),   # stride=8, 2 anchors per cell -> 32x32x2 = 2048
        (16, 2),  # stride=16, 2 anchors per cell -> 16x16x2 = 512
        (32, 6),  # stride=32, 6 anchors per cell -> 8x8x6 = 384
    ]
    for stride, num_anchors in layer_configs:
        grid_size = INPUT_SIZE // stride
        for y in range(grid_size):
            for x in range(grid_size):
                cx = (x + 0.5) / grid_size
                cy = (y + 0.5) / grid_size
                for _ in range(num_anchors):
                    anchors.append((cx, cy))
    return np.array(anchors)

def sigmoid(x):
    return 1 / (1 + np.exp(-np.clip(x, -500, 500)))

# ============================================
# PC Model Inference
# ============================================
class PCPalmDetector:
    def __init__(self, model_path):
        print(f"[PC] 載入模型: {model_path}")
        self.interpreter = tf.lite.Interpreter(model_path=model_path)
        self.interpreter.allocate_tensors()
        self.input_details = self.interpreter.get_input_details()
        self.output_details = self.interpreter.get_output_details()
        self.anchors = generate_anchors()
        print(f"[PC] Anchors: {len(self.anchors)}")

    def detect(self, img_bgr, threshold=0.5):
        """對圖片做 palm detection，返回 bbox list"""
        # Resize to model input size
        img_256 = cv2.resize(img_bgr, (INPUT_SIZE, INPUT_SIZE))
        img_rgb = cv2.cvtColor(img_256, cv2.COLOR_BGR2RGB)

        # Preprocess
        if self.input_details[0]['dtype'] == np.int8:
            img_input = (img_rgb.astype(np.float32) - 128).astype(np.int8)
        else:
            img_input = img_rgb.astype(np.float32) / 127.5 - 1.0
        img_input = np.expand_dims(img_input, axis=0)

        # Inference
        self.interpreter.set_tensor(self.input_details[0]['index'], img_input)
        self.interpreter.invoke()

        # Get outputs
        boxes = None
        scores = None
        for out in self.output_details:
            output = self.interpreter.get_tensor(out['index'])
            if output.shape[-1] == 1:
                scores = output
            elif output.shape[-1] == 18:
                boxes = output

        if boxes is None or scores is None:
            return []

        # Process detections
        score_sigmoid = sigmoid(scores.flatten())
        detections = []

        for idx in range(len(score_sigmoid)):
            if idx >= len(self.anchors):
                break
            if score_sigmoid[idx] > threshold:
                box = boxes[0, idx, :4]
                dx, dy, dw, dh = box
                anchor_cx, anchor_cy = self.anchors[idx]

                # Decode (標準方式)
                cx = anchor_cx + dx / float(INPUT_SIZE)
                cy = anchor_cy + dy / float(INPUT_SIZE)
                w = dw / float(INPUT_SIZE)
                h = dh / float(INPUT_SIZE)

                # Convert to pixel coordinates (in 256x256 space)
                x1 = int((cx - w/2) * INPUT_SIZE)
                y1 = int((cy - h/2) * INPUT_SIZE)
                x2 = int((cx + w/2) * INPUT_SIZE)
                y2 = int((cy + h/2) * INPUT_SIZE)

                detections.append({
                    'bbox': (x1, y1, x2 - x1, y2 - y1),  # x, y, w, h
                    'score': score_sigmoid[idx],
                    'source': 'PC'
                })

        # Sort by score and take top 3
        detections.sort(key=lambda x: x['score'], reverse=True)
        return detections[:3]

# ============================================
# Serial Stream Reader (參照 serial_reader.py)
# ============================================
class BoardStreamReader(threading.Thread):
    def __init__(self, port, baudrate, data_queue):
        super().__init__(daemon=True)
        self.port = port
        self.baudrate = baudrate
        self.data_queue = data_queue
        self.serial = None
        self.running = False
        self.buffer = ""

    def connect(self):
        """連接串口"""
        try:
            print(f"[Serial] 連接 {self.port} @ {self.baudrate}")
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
            print(f"[Serial] 連接失敗: {e}")
            return False

    def start_stream(self):
        """發送 AT 指令啟動串流"""
        if not self.serial or not self.serial.is_open:
            return

        print("[Serial] 發送 AT 指令...")
        time.sleep(0.3)

        # 先發送 AT 測試
        self.serial.write(b"AT\r\n")
        time.sleep(0.2)

        # 啟動 invoke (帶圖片: AT+INVOKE=1,0,0)
        # 參數: invoke_with_image=1, invoke_continuous=0, invoke_count=0(無限)
        self.serial.write(b"AT+INVOKE=1,0,0\r\n")
        time.sleep(0.3)

        print("[Serial] AT 指令已發送")

    def run(self):
        """執行緒主迴圈"""
        while self.running:
            try:
                if self.serial and self.serial.is_open and self.serial.in_waiting > 0:
                    data = self.serial.read(self.serial.in_waiting)
                    try:
                        text = data.decode('utf-8', errors='ignore')
                        self.buffer += text
                        self._parse_buffer()
                    except Exception as e:
                        print(f"[Serial] 解碼錯誤: {e}")
                else:
                    time.sleep(0.01)
            except Exception as e:
                print(f"[Serial] 讀取錯誤: {e}")
                time.sleep(0.1)

    def _parse_buffer(self):
        """解析緩衝區中的 JSON 資料"""
        try:
            # 限制緩衝區大小
            if len(self.buffer) > 500000:
                print(f"[Serial] Buffer 太大，清除...")
                self.buffer = ""
                return

            max_iterations = 10
            iterations = 0

            while iterations < max_iterations:
                iterations += 1

                # 找到 JSON 開始
                start_idx = self.buffer.find('{"')
                if start_idx == -1:
                    if len(self.buffer) > 1000:
                        self.buffer = self.buffer[-1000:]
                    break

                # 丟棄 JSON 之前的內容
                if start_idx > 0:
                    self.buffer = self.buffer[start_idx:]

                # 找到 JSON 結尾
                end_patterns = ['}}\n', '"}\n', ']}\n']
                end_idx = -1

                for pattern in end_patterns:
                    idx = self.buffer.find(pattern)
                    if idx != -1:
                        if end_idx == -1 or idx < end_idx:
                            end_idx = idx + len(pattern) - 1

                if end_idx == -1:
                    break

                # 提取 JSON 行
                line = self.buffer[:end_idx].strip()
                self.buffer = self.buffer[end_idx + 1:]

                # 解析 JSON
                if line.startswith('{') and line.endswith('}'):
                    try:
                        data = json.loads(line)
                        self.data_queue.put(data)
                    except json.JSONDecodeError:
                        # 嘗試清理
                        cleaned = self._try_clean_json(line)
                        if cleaned:
                            try:
                                data = json.loads(cleaned)
                                self.data_queue.put(data)
                            except:
                                pass

        except Exception as e:
            print(f"[Serial] 解析錯誤: {e}")
            self.buffer = ""

    def _try_clean_json(self, line):
        """清理交錯的 JSON"""
        cleaned = re.sub(r'SENSORDPLIB_[A-Z_]+\s+\d+\s*', '', line)
        cleaned = re.sub(r'\n+', '', cleaned)
        return cleaned.strip()

    def disconnect(self):
        """斷開連接"""
        self.running = False
        if self.serial and self.serial.is_open:
            self.serial.close()
        print("[Serial] 已斷開")

# ============================================
# Frame Processor
# ============================================
def process_frame_data(data):
    """
    處理韌體回傳的 JSON 資料

    格式: {"type": 1, "name": "INVOKE", "data": {...}}
    data 內容:
    - resolution: [w, h]
    - image: base64 encoded JPEG
    - hands: [[[bbox], [lm0],...[lm20]], handedness], ...]
    - boxes: [[x, y, w, h, score, target], ...]
    """
    if not isinstance(data, dict):
        return None, []

    msg_type = data.get("type", -1)
    if msg_type != 1:  # 只處理推論結果
        return None, []

    payload = data.get("data", {})
    if not isinstance(payload, dict):
        return None, []

    # 取得圖像
    image_b64 = payload.get("image", "")
    if not image_b64:
        return None, []

    try:
        image_data = base64.b64decode(image_b64)
        img_array = np.frombuffer(image_data, dtype=np.uint8)
        img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
    except Exception as e:
        print(f"[Process] 圖像解碼失敗: {e}")
        return None, []

    # 解析板子的偵測結果
    board_dets = []

    # 處理 hands 資料
    hands = payload.get("hands", [])
    if hands and isinstance(hands, list):
        for hand in hands:
            if isinstance(hand, list) and len(hand) >= 1:
                hand_points = hand[0]  # [[bbox], [lm0], [lm1],...[lm20]]
                if isinstance(hand_points, list) and len(hand_points) >= 1:
                    bbox = hand_points[0]  # [x, y, w, h, score]
                    if isinstance(bbox, list) and len(bbox) >= 5:
                        x, y, w, h, score = bbox[0], bbox[1], bbox[2], bbox[3], bbox[4]
                        board_dets.append({
                            'bbox': (x, y, w, h),
                            'score': score / 100.0,
                            'source': 'Board'
                        })

    # 處理 boxes 資料 (通用格式)
    boxes = payload.get("boxes", [])
    if boxes and isinstance(boxes, list):
        for box in boxes:
            if isinstance(box, list) and len(box) >= 5:
                x, y, w, h, score = box[0], box[1], box[2], box[3], box[4]
                board_dets.append({
                    'bbox': (x, y, w, h),
                    'score': score / 100.0,
                    'source': 'Board'
                })

    return img, board_dets

# ============================================
# Visualization
# ============================================
def draw_detections(img, detections, color, label_prefix="", ref_size=None):
    """繪製偵測框"""
    img_h, img_w = img.shape[:2]

    for i, det in enumerate(detections):
        x, y, w, h = det['bbox']
        score = det['score']

        # 如果是 PC 結果，需要從 256x256 空間轉換到圖像空間
        if det['source'] == 'PC' and ref_size:
            scale_x = img_w / ref_size[0]
            scale_y = img_h / ref_size[1]
            x = int(x * scale_x)
            y = int(y * scale_y)
            w = int(w * scale_x)
            h = int(h * scale_y)

        # Clamp to image bounds
        x = max(0, min(x, img_w))
        y = max(0, min(y, img_h))
        w = max(1, min(w, img_w - x))
        h = max(1, min(h, img_h - y))

        cv2.rectangle(img, (x, y), (x + w, y + h), color, 2)
        label = f"{label_prefix}{score:.0%}"
        cv2.putText(img, label, (x, y - 5),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 1)

    return img

# ============================================
# Main
# ============================================
def main():
    parser = argparse.ArgumentParser(description='即時比較 PC vs 板子')
    parser.add_argument('--port', type=str, default='COM3', help='串口')
    parser.add_argument('--baudrate', type=int, default=921600, help='波特率')
    parser.add_argument('--threshold', type=float, default=0.5, help='PC 模型閾值')
    parser.add_argument('--no-pc', action='store_true', help='不執行 PC 模型 (只顯示板子結果)')
    args = parser.parse_args()

    # Initialize PC detector
    pc_detector = None
    if not args.no_pc:
        try:
            pc_detector = PCPalmDetector(MODEL_PATH)
        except Exception as e:
            print(f"[警告] 無法載入 PC 模型: {e}")
            print("[警告] 將只顯示板子結果")

    # Initialize serial reader
    data_queue = Queue()
    reader = BoardStreamReader(args.port, args.baudrate, data_queue)

    if not reader.connect():
        print("[錯誤] 無法連接串口")
        return

    reader.start()
    reader.start_stream()

    display_mode = 3  # 1=PC, 2=Board, 3=Both
    frame_count = 0
    last_fps_time = time.time()
    fps_count = 0
    fps = 0

    print("\n" + "="*50)
    print("即時比較工具")
    print("="*50)
    print("  1: 只顯示 PC 結果 (綠框)")
    print("  2: 只顯示板子結果 (紅框)")
    print("  3: 疊圖顯示兩者")
    print("  S: 儲存當前畫面")
    print("  Q: 退出")
    print("="*50)
    print(f"\nLog 檔案: {LOG_FILE}")
    print("\n等待圖像串流...")

    try:
        while True:
            # 從佇列取得資料
            try:
                data = data_queue.get(timeout=0.1)
            except Empty:
                # 檢查鍵盤 (非阻塞)
                key = cv2.waitKey(1) & 0xFF
                if key == ord('q') or key == ord('Q'):
                    break
                continue

            # 處理資料
            img, board_dets = process_frame_data(data)

            if img is None:
                continue

            frame_count += 1
            fps_count += 1

            # 計算 FPS
            current_time = time.time()
            if current_time - last_fps_time >= 1.0:
                fps = fps_count
                fps_count = 0
                last_fps_time = current_time

            # PC 偵測
            pc_dets = []
            if pc_detector and display_mode != 2:
                pc_dets = pc_detector.detect(img, args.threshold)

            # 記錄到 log (每 10 幀記錄一次，避免檔案太大)
            if frame_count % 10 == 1 or (pc_dets and board_dets):
                log_result(frame_count, pc_dets, board_dets, LOG_FILE)

            # 建立顯示圖像
            display_img = img.copy()
            img_h, img_w = display_img.shape[:2]

            # 根據模式繪製
            if display_mode == 1 or display_mode == 3:
                # PC 結果 (綠色)
                draw_detections(display_img, pc_dets, (0, 255, 0), "PC:",
                               ref_size=(INPUT_SIZE, INPUT_SIZE))

            if display_mode == 2 or display_mode == 3:
                # 板子結果 (紅色)
                draw_detections(display_img, board_dets, (0, 0, 255), "FW:", ref_size=None)

            # 顯示資訊
            mode_text = {1: "PC Only", 2: "Board Only", 3: "Both"}[display_mode]
            info_text = f"Mode: {mode_text} | FPS: {fps} | Frame: {frame_count}"
            cv2.putText(display_img, info_text, (10, 25),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)

            count_text = f"PC: {len(pc_dets)} | Board: {len(board_dets)}"
            cv2.putText(display_img, count_text, (10, 50),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)

            # 顯示
            cv2.imshow("Palm Detection Compare", display_img)

            # 處理按鍵
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q') or key == ord('Q'):
                break
            elif key == ord('1'):
                display_mode = 1
                print("Mode: PC Only")
            elif key == ord('2'):
                display_mode = 2
                print("Mode: Board Only")
            elif key == ord('3'):
                display_mode = 3
                print("Mode: Both")
            elif key == ord('s') or key == ord('S'):
                filename = f"compare_frame_{frame_count}.png"
                cv2.imwrite(filename, display_img)
                print(f"Saved: {filename}")

    except KeyboardInterrupt:
        print("\n中斷")
    finally:
        reader.disconnect()
        cv2.destroyAllWindows()
        print("Done")

if __name__ == "__main__":
    main()
