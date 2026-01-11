"""
Palm Detection 模型比較測試

用法：
1. 用 webcam 拍一張照片存成 test_hand.png
   python test_palm_model.py --capture

2. 用已存在的圖片測試
   python test_palm_model.py --image test_hand.png

3. 直接用 webcam 即時測試
   python test_palm_model.py
"""

import numpy as np
import cv2
import tensorflow as tf
import argparse
import os

MODEL_PATH = "../model_zoo/tflm_hand_tracking/0_palm_det_0x400000.tflite"

# PINTO palm_detection: 256x256 input, 2944 anchors
INPUT_SIZE = 256

# Anchor 生成（與板子相同）
def generate_anchors():
    anchors = []
    # PINTO palm_detection 256x256 input, 2944 anchors
    # stride=8 -> 32x32 grid, 2 anchors/cell = 2048
    # stride=16 -> 16x16 grid, 2 anchors/cell = 512
    # stride=32 -> 8x8 grid, 6 anchors/cell = 384
    # Total = 2944 anchors
    layer_configs = [
        (8, 2),   # stride=8, 2 anchors per cell -> 32x32x2 = 2048
        (16, 2),  # stride=16, 2 anchors per cell -> 16x16x2 = 512
        (32, 6),  # stride=32, 6 anchors per cell -> 8x8x6 = 384
    ]
    for stride, num_anchors in layer_configs:
        grid_size = INPUT_SIZE // stride
        # Standard y-first ordering
        for y in range(grid_size):
            for x in range(grid_size):
                cx = (x + 0.5) / grid_size
                cy = (y + 0.5) / grid_size
                for _ in range(num_anchors):
                    anchors.append((cx, cy))
    return np.array(anchors)

def sigmoid(x):
    return 1 / (1 + np.exp(-np.clip(x, -500, 500)))

def run_inference(img_256, interpreter, anchors):
    """對 256x256 圖片做推理"""
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    # 預處理
    img_rgb = cv2.cvtColor(img_256, cv2.COLOR_BGR2RGB)
    if input_details[0]['dtype'] == np.int8:
        img_input = (img_rgb.astype(np.float32) - 128).astype(np.int8)
    else:
        img_input = img_rgb.astype(np.float32) / 127.5 - 1.0
    img_input = np.expand_dims(img_input, axis=0)

    # 推理
    interpreter.set_tensor(input_details[0]['index'], img_input)
    interpreter.invoke()

    # 取得輸出
    boxes = None
    scores = None
    for out in output_details:
        output = interpreter.get_tensor(out['index'])
        if output.shape[-1] == 1:
            scores = output
        elif output.shape[-1] == 18:
            boxes = output

    return boxes, scores

def process_detections(boxes, scores, anchors, threshold=0.5, print_raw=False):
    """處理偵測結果"""
    score_sigmoid = sigmoid(scores.flatten())
    detections = []

    for idx in range(len(score_sigmoid)):
        if score_sigmoid[idx] > threshold:
            box = boxes[0, idx, :4]
            dx, dy, dw, dh = box
            anchor_cx, anchor_cy = anchors[idx]

            detections.append({
                'idx': idx,
                'score': score_sigmoid[idx],
                'raw': (dx, dy, dw, dh),
                'anchor': (anchor_cx, anchor_cy),
            })

    detections.sort(key=lambda x: x['score'], reverse=True)

    # Print raw tensor data in same format as firmware for comparison
    if print_raw:
        # Get top 5 by score
        top5_indices = np.argsort(score_sigmoid)[-5:][::-1]
        print("\n[RAW_TENSOR] PC Model")
        print("[RAW_TENSOR] Top 5 scores:")
        for j, idx in enumerate(top5_indices):
            box = boxes[0, idx, :4]
            dx, dy, dw, dh = box
            anchor_cx, anchor_cy = anchors[idx]
            score_pct = int(score_sigmoid[idx] * 100)
            anc_x_pct = int(anchor_cx * 100)
            anc_y_pct = int(anchor_cy * 100)
            print(f"  [{j}] idx={idx} score={score_pct}% anc=[{anc_x_pct},{anc_y_pct}] raw=[{int(dx)},{int(dy)},{int(dw)},{int(dh)}]")

    return detections

def main():
    parser = argparse.ArgumentParser(description='Palm Detection Test')
    parser.add_argument('--capture', action='store_true', help='拍照並存成 test_hand.png')
    parser.add_argument('--image', type=str, help='使用指定圖片測試')
    args = parser.parse_args()

    # 載入模型
    print("載入模型...")
    interpreter = tf.lite.Interpreter(model_path=MODEL_PATH)
    interpreter.allocate_tensors()
    anchors = generate_anchors()
    print(f"Generated {len(anchors)} anchors")

    # 取得圖片
    if args.capture:
        # 拍照模式
        cap = cv2.VideoCapture(0)
        print("按空白鍵拍照並存檔...")
        while True:
            ret, frame = cap.read()
            if not ret:
                print("無法開啟攝影機")
                return
            cv2.imshow("Press SPACE to capture", frame)
            if cv2.waitKey(1) == 32:
                cv2.imwrite("test_hand.png", frame)
                print("已存成 test_hand.png")
                break
        cap.release()
        cv2.destroyAllWindows()
        img = frame

    elif args.image:
        # 使用指定圖片
        if not os.path.exists(args.image):
            print(f"找不到圖片: {args.image}")
            return
        img = cv2.imread(args.image)
        print(f"載入圖片: {args.image}")

    else:
        # 即時 webcam 模式
        cap = cv2.VideoCapture(0)
        print("按空白鍵拍照...")
        while True:
            ret, frame = cap.read()
            if not ret:
                print("無法開啟攝影機")
                return
            cv2.imshow("Press SPACE to capture", frame)
            if cv2.waitKey(1) == 32:
                break
        cap.release()
        cv2.destroyAllWindows()
        img = frame

    # Resize 到 256x256 (PINTO palm_detection input size)
    img_256 = cv2.resize(img, (INPUT_SIZE, INPUT_SIZE))

    # 推理
    print("\n推理中...")
    boxes, scores = run_inference(img_256, interpreter, anchors)

    if boxes is None or scores is None:
        print("無法取得輸出")
        return

    # 處理偵測 (print_raw=True 印出 raw tensor 供比對)
    detections = process_detections(boxes, scores, anchors, threshold=0.5, print_raw=True)

    print(f"\n{'='*50}")
    print(f"PC 模型偵測結果 (共 {len(detections)} 個)")
    print(f"{'='*50}")

    # 印出前 3 個偵測的 raw 值
    for i, det in enumerate(detections[:3]):
        dx, dy, dw, dh = det['raw']
        anc_x, anc_y = det['anchor']
        print(f"\n[PC] Det {i}: score={int(det['score']*100)}%")
        print(f"  raw: dx={dx:.1f} dy={dy:.1f} dw={dw:.1f} dh={dh:.1f}")
        print(f"  anc: [{int(anc_x*100)},{int(anc_y*100)}]")

    # 繪製結果在 256x256 圖上
    img_draw = img_256.copy()
    for i, det in enumerate(detections[:3]):
        dx, dy, dw, dh = det['raw']
        anc_x, anc_y = det['anchor']

        # 解碼 (PC 版本，不需要 VELA scale)
        # 嘗試交換 dx/dy
        cx = anc_x + dy / float(INPUT_SIZE)
        cy = anc_y + dx / float(INPUT_SIZE)
        w = dw / float(INPUT_SIZE)
        h = dh / float(INPUT_SIZE)

        # 轉像素
        x1 = int((cx - w/2) * INPUT_SIZE)
        y1 = int((cy - h/2) * INPUT_SIZE)
        x2 = int((cx + w/2) * INPUT_SIZE)
        y2 = int((cy + h/2) * INPUT_SIZE)

        color = (0, 255, 0) if i == 0 else (0, 255, 255)
        cv2.rectangle(img_draw, (x1, y1), (x2, y2), color, 2)
        cv2.putText(img_draw, f"{det['score']:.0%}", (x1, y1-5),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 1)

    # 存檔
    cv2.imwrite("pc_detection_result.png", img_draw)
    print(f"\n結果已存成 pc_detection_result.png")

    # 顯示
    cv2.imshow("PC Detection Result (256x256)", img_draw)
    print("\n按任意鍵關閉...")
    cv2.waitKey(0)
    cv2.destroyAllWindows()

    # 提示用戶
    print(f"\n{'='*50}")
    print("下一步：")
    print("1. 把 pc_detection_result.png 全螢幕顯示")
    print("2. 把板子鏡頭對準螢幕")
    print("3. 看板子 log 的 raw 值")
    print("4. 比較 PC 和板子的 dx, dy, dw, dh")
    print(f"{'='*50}")

if __name__ == "__main__":
    main()
