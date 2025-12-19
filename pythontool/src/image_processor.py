#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
高性能圖像處理模組

架構設計：
1. 生產者-消費者模式：串口讀取 → 處理佇列 → GUI 顯示
2. 多線程流水線：解碼 → 繪製 → 縮放 在獨立線程執行
3. 幀丟棄策略：處理不過來時丟棄舊幀，保持響應性
4. 緩存優化：重用 buffer，避免重複分配
"""

import threading
import base64
import io
import time
from queue import Queue, Empty, Full
from typing import Optional, Callable, Dict, Any, Tuple
from dataclasses import dataclass
from PIL import Image, ImageDraw

from .drawing import draw_boxes, draw_keypoints, draw_face_mesh, draw_hands


@dataclass
class ProcessedFrame:
    """處理完成的幀"""
    frame_id: int
    image: Image.Image  # 已繪製、已縮放的圖像
    timestamp: float
    processing_time: float  # 處理耗時 (ms)
    fm_count: int  # Face Mesh 點數
    box_count: int  # 檢測框數
    hands_count: int = 0  # Hand tracking 數量


class ImageProcessor(threading.Thread):
    """
    高性能圖像處理器

    特點：
    1. 獨立線程處理，不阻塞 GUI
    2. 最多保留最新 N 幀，丟棄舊幀
    3. 批量處理優化
    4. 性能統計
    """

    def __init__(
        self,
        input_queue: Queue,
        output_queue: Queue,
        max_output_size: int = 2,  # 輸出佇列最大長度（保持最新）
        target_size: Tuple[int, int] = (640, 480),  # 目標顯示尺寸
    ):
        super().__init__(daemon=True, name="ImageProcessor")
        self.input_queue = input_queue
        self.output_queue = output_queue
        self.max_output_size = max_output_size
        self.target_size = target_size

        self.running = False
        self.frame_id = 0

        # Face Mesh 偏移設定
        self.offset_x = 0
        self.offset_y = 0
        self.offset_scale = 1.0

        # 性能統計
        self.stats = {
            'processed': 0,
            'dropped': 0,
            'avg_time_ms': 0,
            'total_time_ms': 0,
        }

        # 緩存
        self._resize_cache: Optional[Image.Image] = None

    def set_offset(self, x: int, y: int, scale: float = 1.0):
        """設定 Face Mesh 偏移"""
        self.offset_x = x
        self.offset_y = y
        self.offset_scale = scale

    def run(self):
        """處理線程主循環"""
        self.running = True
        print("[ImageProcessor] Started")

        while self.running:
            try:
                # 從輸入佇列取得數據（阻塞等待，最多 100ms）
                data = self.input_queue.get(timeout=0.1)
                start_time = time.perf_counter()

                # 處理數據
                result = self._process_frame(data)

                if result:
                    # 計算處理時間
                    elapsed_ms = (time.perf_counter() - start_time) * 1000
                    result.processing_time = elapsed_ms

                    # 嘗試放入輸出佇列（非阻塞）
                    self._put_result(result)

                    # 更新統計
                    self.stats['processed'] += 1
                    self.stats['total_time_ms'] += elapsed_ms
                    self.stats['avg_time_ms'] = self.stats['total_time_ms'] / self.stats['processed']

            except Empty:
                continue
            except Exception as e:
                print(f"[ImageProcessor] Error: {e}")

        print("[ImageProcessor] Stopped")

    def _process_frame(self, data: Dict[str, Any]) -> Optional[ProcessedFrame]:
        """處理單幀數據"""
        try:
            if not isinstance(data, dict):
                return None

            msg_type = data.get("type", -1)
            if msg_type != 1:  # 只處理推論結果
                return None

            payload = data.get("data", {})
            if not isinstance(payload, dict):
                return None

            # 取得圖像和元數據
            image_b64 = payload.get("image", "")
            if not image_b64:
                return None

            resolution = payload.get("resolution", [240, 240])
            boxes = payload.get("boxes", [])
            keypoints = payload.get("keypoints", [])
            fm_points = payload.get("fm_points", [])
            hands = payload.get("hands", [])

            # === 階段 1：Base64 + JPEG 解碼 ===
            image_data = base64.b64decode(image_b64)
            image = Image.open(io.BytesIO(image_data))

            # 確保 RGB 模式
            if image.mode != 'RGB':
                image = image.convert('RGB')

            # 取得座標參考尺寸
            actual_w, actual_h = image.size

            # 從 resolution 欄位取得座標參考尺寸
            if isinstance(resolution, list) and len(resolution) >= 2:
                ref_w, ref_h = resolution[0], resolution[1]
            else:
                ref_w, ref_h = actual_w, actual_h

            # 自動偵測座標系統：檢查 fm_points 的最大值
            if fm_points and isinstance(fm_points, list):
                max_x, max_y = 0, 0
                for face in fm_points:
                    if isinstance(face, list) and len(face) > 1 and isinstance(face[1], list):
                        for pt in face[1]:
                            if isinstance(pt, list) and len(pt) >= 2:
                                if pt[0] > max_x:
                                    max_x = pt[0]
                                if pt[1] > max_y:
                                    max_y = pt[1]

                # 如果座標超出 reference 範圍，自動調整
                if max_x > ref_w * 1.1 or max_y > ref_h * 1.1:
                    # 推測正確的座標範圍
                    if max_x > 500 or max_y > 400:
                        ref_w, ref_h = 640, 480
                    elif max_x > 300 or max_y > 300:
                        ref_w, ref_h = 480, 480
                    elif max_x > 240 or max_y > 240:
                        ref_w, ref_h = 320, 320

                    if self.frame_id % 60 == 1:
                        print(f"[AutoDetect] max=({max_x},{max_y}), using ref={ref_w}x{ref_h}")

            # === 階段 2：繪製疊加層 ===
            box_count = 0
            fm_count = 0
            hands_count = 0

            if boxes and isinstance(boxes, list):
                image = draw_boxes(image, boxes, ref_w, ref_h)
                box_count = len(boxes)

            if keypoints and isinstance(keypoints, list):
                image = draw_keypoints(image, keypoints, ref_w, ref_h)

            if fm_points and isinstance(fm_points, list):
                image = draw_face_mesh(
                    image, fm_points, ref_w, ref_h,
                    offset_x=self.offset_x,
                    offset_y=self.offset_y,
                    scale=self.offset_scale
                )
                fm_count = sum(len(face[1]) if len(face) > 1 and isinstance(face[1], list) else 0
                              for face in fm_points if isinstance(face, list))

            if hands and isinstance(hands, list):
                # Parse hand tracking data from firmware:
                # [[[bbox], [lm0], [lm1],...[lm20]], handedness], ...]
                # hand[0] = [[bbox], [lm0],...[lm20]] (22 elements)
                # hand[1] = handedness (0=left, 1=right)
                hands_data = []
                for hand in hands:
                    if isinstance(hand, list) and len(hand) >= 2:
                        hand_points = hand[0]  # [[bbox], [lm0], [lm1],...[lm20]]
                        if isinstance(hand_points, list) and len(hand_points) >= 2:
                            bbox = hand_points[0] if len(hand_points) > 0 else []
                            landmarks = hand_points[1:22]  # 21 landmarks
                            hands_data.append([bbox, landmarks])
                if hands_data:
                    image = draw_hands(image, hands_data, ref_w, ref_h)
                    hands_count = len(hands_data)

            # === 階段 3：縮放至目標尺寸 ===
            # 使用 BILINEAR 比 LANCZOS 快 3-5 倍
            target_w, target_h = self.target_size
            img_w, img_h = image.size
            ratio = min(target_w / img_w, target_h / img_h)

            if ratio < 1:
                new_size = (int(img_w * ratio), int(img_h * ratio))
                image = image.resize(new_size, Image.Resampling.BILINEAR)

            self.frame_id += 1
            return ProcessedFrame(
                frame_id=self.frame_id,
                image=image,
                timestamp=time.time(),
                processing_time=0,  # 稍後填入
                fm_count=fm_count,
                box_count=box_count,
                hands_count=hands_count,
            )

        except Exception as e:
            print(f"[ImageProcessor] Frame error: {e}")
            return None

    def _put_result(self, result: ProcessedFrame):
        """放入輸出佇列，必要時丟棄舊幀"""
        try:
            # 如果佇列滿了，丟棄最舊的幀
            while self.output_queue.qsize() >= self.max_output_size:
                try:
                    self.output_queue.get_nowait()
                    self.stats['dropped'] += 1
                except Empty:
                    break

            self.output_queue.put_nowait(result)
        except Full:
            self.stats['dropped'] += 1

    def stop(self):
        """停止處理器"""
        self.running = False

    def get_stats(self) -> Dict[str, Any]:
        """取得性能統計"""
        return dict(self.stats)


class DoubleBufferDisplay:
    """
    雙緩衝顯示管理器

    解決 tkinter PhotoImage 必須在主線程創建的問題
    """

    def __init__(self):
        self._current_photo = None
        self._pending_image: Optional[Image.Image] = None
        self._lock = threading.Lock()

    def submit_image(self, image: Image.Image):
        """提交新圖像（可從任何線程調用）"""
        with self._lock:
            self._pending_image = image

    def get_pending_image(self) -> Optional[Image.Image]:
        """取得待顯示的圖像（主線程調用）"""
        with self._lock:
            image = self._pending_image
            self._pending_image = None
            return image


class FrameDropPolicy:
    """
    幀丟棄策略

    當處理速度跟不上輸入速度時，智能丟棄幀
    """

    def __init__(self, target_fps: float = 30.0):
        self.target_fps = target_fps
        self.frame_interval = 1.0 / target_fps
        self.last_frame_time = 0

    def should_process(self) -> bool:
        """判斷是否應該處理這幀"""
        current_time = time.time()
        if current_time - self.last_frame_time >= self.frame_interval:
            self.last_frame_time = current_time
            return True
        return False

    def force_process(self):
        """強制處理（重置計時器）"""
        self.last_frame_time = time.time()
