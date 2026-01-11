#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Raw Tensor 比對工具

同時擷取韌體輸出和執行 PC 模型，比對 raw tensor 值

用法:
    python compare_raw_tensor.py --port COM3
"""

import serial
import argparse
import time
import os
import json
import base64
import numpy as np
import cv2
import tensorflow as tf
from datetime import datetime
from io import BytesIO

LOG_DIR = "logs"
os.makedirs(LOG_DIR, exist_ok=True)

MODEL_PATH = "../model_zoo/tflm_hand_tracking/0_palm_det_0x400000.tflite"
INPUT_SIZE = 256

def generate_anchors():
    anchors = []
    layer_configs = [
        (8, 2),
        (16, 2),
        (32, 6),
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

class PCModel:
    def __init__(self):
        print(f"[PC] 載入模型: {MODEL_PATH}")
        self.interpreter = tf.lite.Interpreter(model_path=MODEL_PATH)
        self.interpreter.allocate_tensors()
        self.input_details = self.interpreter.get_input_details()
        self.output_details = self.interpreter.get_output_details()
        self.anchors = generate_anchors()
        print(f"[PC] Anchors: {len(self.anchors)}")

    def get_raw_tensor(self, img_bgr):
        """取得 raw tensor 輸出"""
        img_256 = cv2.resize(img_bgr, (INPUT_SIZE, INPUT_SIZE))
        img_rgb = cv2.cvtColor(img_256, cv2.COLOR_BGR2RGB)

        if self.input_details[0]['dtype'] == np.int8:
            img_input = (img_rgb.astype(np.float32) - 128).astype(np.int8)
        else:
            img_input = img_rgb.astype(np.float32) / 127.5 - 1.0
        img_input = np.expand_dims(img_input, axis=0)

        self.interpreter.set_tensor(self.input_details[0]['index'], img_input)
        self.interpreter.invoke()

        boxes = None
        scores = None
        for out in self.output_details:
            output = self.interpreter.get_tensor(out['index'])
            if output.shape[-1] == 1:
                scores = output
            elif output.shape[-1] == 18:
                boxes = output

        return boxes, scores

    def print_top5(self, boxes, scores):
        """印出 top 5 scores 的 raw tensor"""
        score_sigmoid = sigmoid(scores.flatten())
        top5_indices = np.argsort(score_sigmoid)[-5:][::-1]

        print("\n[RAW_TENSOR] PC Model")
        print("[RAW_TENSOR] Top 5 scores:")
        for j, idx in enumerate(top5_indices):
            box = boxes[0, idx, :4]
            dx, dy, dw, dh = box
            anchor_cx, anchor_cy = self.anchors[idx]
            score_pct = int(score_sigmoid[idx] * 100)
            anc_x_pct = int(anchor_cx * 100)
            anc_y_pct = int(anchor_cy * 100)
            print(f"  [{j}] idx={idx} score={score_pct}% anc=[{anc_x_pct},{anc_y_pct}] raw=[{int(dx)},{int(dy)},{int(dw)},{int(dh)}]")

        return top5_indices, score_sigmoid

def parse_json_from_buffer(buffer):
    """從 buffer 中解析 JSON"""
    start_idx = buffer.find('{"')
    if start_idx == -1:
        return None, buffer

    # 找到對應的結尾
    depth = 0
    for i in range(start_idx, len(buffer)):
        if buffer[i] == '{':
            depth += 1
        elif buffer[i] == '}':
            depth -= 1
            if depth == 0:
                json_str = buffer[start_idx:i+1]
                remaining = buffer[i+1:]
                try:
                    data = json.loads(json_str)
                    return data, remaining
                except:
                    return None, buffer[start_idx+1:]

    return None, buffer

def extract_image_from_json(data):
    """從 JSON 中提取圖像"""
    if not isinstance(data, dict):
        return None

    if data.get("type") != 1:
        return None

    payload = data.get("data", {})
    image_b64 = payload.get("image", "")

    if not image_b64:
        return None

    try:
        image_data = base64.b64decode(image_b64)
        img_array = np.frombuffer(image_data, dtype=np.uint8)
        img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
        return img
    except:
        return None

def main():
    parser = argparse.ArgumentParser(description='Raw Tensor 比對')
    parser.add_argument('--port', type=str, default='COM3', help='串口')
    parser.add_argument('--baudrate', type=int, default=921600, help='波特率')
    parser.add_argument('--frames', type=int, default=10, help='比對幀數')
    args = parser.parse_args()

    log_file = os.path.join(LOG_DIR, f"tensor_compare_{datetime.now().strftime('%Y%m%d_%H%M%S')}.log")

    print("=" * 60)
    print("Raw Tensor 比對工具")
    print("=" * 60)
    print(f"Log: {log_file}")
    print(f"比對 {args.frames} 幀")
    print()

    # 載入 PC 模型
    pc_model = PCModel()

    # 連接串口
    print(f"\n連接 {args.port}...")
    try:
        ser = serial.Serial(port=args.port, baudrate=args.baudrate, timeout=0.1)
    except Exception as e:
        print(f"連接失敗: {e}")
        return

    # 啟動串流
    time.sleep(0.3)
    ser.write(b"AT\r\n")
    time.sleep(0.2)
    ser.write(b"AT+INVOKE=1,0,0\r\n")
    time.sleep(0.3)
    print("串流已啟動\n")

    buffer = ""
    frame_count = 0
    fw_raw_lines = []

    with open(log_file, 'w', encoding='utf-8') as f:
        f.write(f"Tensor Compare - {datetime.now()}\n")
        f.write("=" * 60 + "\n\n")

        try:
            while frame_count < args.frames:
                if ser.in_waiting > 0:
                    data = ser.read(ser.in_waiting)
                    text = data.decode('utf-8', errors='ignore')
                    buffer += text

                    # 收集 [RAW_TENSOR] 行
                    while '\n' in buffer:
                        line, buffer = buffer.split('\n', 1)
                        line = line.strip()

                        if '[RAW_TENSOR]' in line:
                            fw_raw_lines.append(line)
                            print(f"[FW] {line}")
                            f.write(f"[FW] {line}\n")

                    # 嘗試解析 JSON 取得圖片
                    json_data, buffer = parse_json_from_buffer(buffer)
                    if json_data:
                        img = extract_image_from_json(json_data)
                        if img is not None:
                            frame_count += 1
                            print(f"\n{'='*60}")
                            print(f"Frame {frame_count}")
                            print(f"{'='*60}")
                            f.write(f"\n{'='*60}\n")
                            f.write(f"Frame {frame_count}\n")
                            f.write(f"{'='*60}\n")

                            # PC 模型推理
                            boxes, scores = pc_model.get_raw_tensor(img)
                            if boxes is not None and scores is not None:
                                pc_model.print_top5(boxes, scores)

                                # 寫入 log
                                score_sigmoid = sigmoid(scores.flatten())
                                top5_indices = np.argsort(score_sigmoid)[-5:][::-1]
                                f.write("\n[PC] Top 5:\n")
                                for j, idx in enumerate(top5_indices):
                                    box = boxes[0, idx, :4]
                                    anchor_cx, anchor_cy = pc_model.anchors[idx]
                                    f.write(f"  [{j}] idx={idx} score={int(score_sigmoid[idx]*100)}% ")
                                    f.write(f"anc=[{int(anchor_cx*100)},{int(anchor_cy*100)}] ")
                                    f.write(f"raw=[{int(box[0])},{int(box[1])},{int(box[2])},{int(box[3])}]\n")

                            # 清空 FW raw lines (已記錄)
                            fw_raw_lines = []

                            print()
                            f.write("\n")
                            f.flush()

                else:
                    time.sleep(0.01)

        except KeyboardInterrupt:
            print("\n中斷")

    ser.close()
    print(f"\n完成! Log: {log_file}")

if __name__ == "__main__":
    main()
