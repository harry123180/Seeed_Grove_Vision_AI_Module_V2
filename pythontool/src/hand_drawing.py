#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""手部關鍵點繪製模組"""

from PIL import Image, ImageDraw
from typing import List, Tuple

# 21 個手部關鍵點名稱
HAND_LANDMARKS = [
    "WRIST",
    "THUMB_CMC", "THUMB_MCP", "THUMB_IP", "THUMB_TIP",
    "INDEX_MCP", "INDEX_PIP", "INDEX_DIP", "INDEX_TIP",
    "MIDDLE_MCP", "MIDDLE_PIP", "MIDDLE_DIP", "MIDDLE_TIP",
    "RING_MCP", "RING_PIP", "RING_DIP", "RING_TIP",
    "PINKY_MCP", "PINKY_PIP", "PINKY_DIP", "PINKY_TIP"
]

# 手部骨架連接
HAND_SKELETON = [
    # Thumb
    (0, 1), (1, 2), (2, 3), (3, 4),
    # Index finger
    (0, 5), (5, 6), (6, 7), (7, 8),
    # Middle finger
    (0, 9), (9, 10), (10, 11), (11, 12),
    # Ring finger
    (0, 13), (13, 14), (14, 15), (15, 16),
    # Pinky
    (0, 17), (17, 18), (18, 19), (19, 20),
    # Palm connections
    (5, 9), (9, 13), (13, 17)
]

# 手指顏色 (Thumb, Index, Middle, Ring, Pinky)
FINGER_COLORS = [
    "#FF6B6B",  # Thumb - 紅
    "#4ECDC4",  # Index - 青
    "#45B7D1",  # Middle - 藍
    "#96CEB4",  # Ring - 綠
    "#FFEAA7",  # Pinky - 黃
]

# 骨架連接對應的手指索引
SKELETON_FINGER_MAP = [
    0, 0, 0, 0,      # Thumb (4 connections)
    1, 1, 1, 1,      # Index (4 connections)
    2, 2, 2, 2,      # Middle (4 connections)
    3, 3, 3, 3,      # Ring (4 connections)
    4, 4, 4, 4,      # Pinky (4 connections)
    1, 2, 3,         # Palm connections
]


def get_landmark_name(idx: int) -> str:
    """取得關鍵點名稱"""
    if 0 <= idx < len(HAND_LANDMARKS):
        return HAND_LANDMARKS[idx]
    return f"POINT_{idx}"


def draw_hand_landmarks(
    image: Image.Image,
    landmarks: List,
    img_w: int,
    img_h: int,
    hand_idx: int = 0,
    draw_labels: bool = False,
    debug: bool = False
) -> Image.Image:
    """繪製手部關鍵點

    Args:
        image: PIL Image
        landmarks: 關鍵點列表 [[x, y, z], ...] 共 21 點
        img_w, img_h: 座標參考解析度
        hand_idx: 手的索引 (用於區分顏色)
        draw_labels: 是否繪製標籤
        debug: 是否輸出調試信息
    """
    try:
        draw = ImageDraw.Draw(image)
        actual_w, actual_h = image.size

        # 計算縮放比例
        scale_x = actual_w / img_w if img_w > 0 else 1
        scale_y = actual_h / img_h if img_h > 0 else 1

        if debug:
            print(f"[Hand Draw] actual={actual_w}x{actual_h}, ref={img_w}x{img_h}")

        # 收集有效點座標
        points: List[Tuple[int, int]] = []
        for i, lm in enumerate(landmarks):
            try:
                if isinstance(lm, (list, tuple)) and len(lm) >= 2:
                    px = int(lm[0] * scale_x)
                    py = int(lm[1] * scale_y)
                    points.append((px, py))
                else:
                    points.append(None)
            except (TypeError, ValueError):
                points.append(None)

        if debug:
            valid_count = sum(1 for p in points if p is not None)
            print(f"[Hand Draw] Valid points: {valid_count}/{len(landmarks)}")

        # 繪製骨架連線
        for conn_idx, (start_idx, end_idx) in enumerate(HAND_SKELETON):
            if start_idx < len(points) and end_idx < len(points):
                p1 = points[start_idx]
                p2 = points[end_idx]
                if p1 is not None and p2 is not None:
                    # 根據手指選擇顏色
                    finger_idx = SKELETON_FINGER_MAP[conn_idx] if conn_idx < len(SKELETON_FINGER_MAP) else 0
                    color = FINGER_COLORS[finger_idx % len(FINGER_COLORS)]
                    draw.line([p1, p2], fill=color, width=3)

        # 繪製關鍵點
        for i, pt in enumerate(points):
            if pt is None:
                continue

            px, py = pt

            # 根據關鍵點位置決定顏色和大小
            if i == 0:  # Wrist
                color = "#FFFFFF"
                r = 6
            elif i in [4, 8, 12, 16, 20]:  # Fingertips
                finger_idx = (i - 4) // 4
                color = FINGER_COLORS[finger_idx]
                r = 5
            else:
                finger_idx = (i - 1) // 4 if i > 0 else 0
                color = FINGER_COLORS[finger_idx]
                r = 4

            # 繪製點
            draw.ellipse([px-r, py-r, px+r, py+r], fill=color, outline="#FFFFFF", width=1)

            # 繪製標籤
            if draw_labels and i in [0, 4, 8, 12, 16, 20]:
                label = get_landmark_name(i)
                draw.text((px + r + 2, py - 6), label, fill="#FFFFFF")

    except Exception as e:
        print(f"[Hand Draw] Error: {e}")

    return image


def draw_hand_bbox(
    image: Image.Image,
    bbox: List,
    img_w: int,
    img_h: int,
    color: str = "#00FF00",
    label: str = None
) -> Image.Image:
    """繪製手部邊界框

    Args:
        image: PIL Image
        bbox: [x, y, w, h, score] 或 [x, y, w, h]
        img_w, img_h: 座標參考解析度
        color: 框線顏色
        label: 標籤文字
    """
    try:
        if not isinstance(bbox, (list, tuple)) or len(bbox) < 4:
            return image

        draw = ImageDraw.Draw(image)
        actual_w, actual_h = image.size

        scale_x = actual_w / img_w if img_w > 0 else 1
        scale_y = actual_h / img_h if img_h > 0 else 1

        x, y, w, h = bbox[:4]
        x1 = int(x * scale_x)
        y1 = int(y * scale_y)
        x2 = int((x + w) * scale_x)
        y2 = int((y + h) * scale_y)

        draw.rectangle([x1, y1, x2, y2], outline=color, width=2)

        if label:
            draw.text((x1, max(0, y1 - 15)), label, fill=color)
        elif len(bbox) >= 5:
            score = bbox[4]
            draw.text((x1, max(0, y1 - 15)), f"Hand: {score:.0%}", fill=color)

    except Exception as e:
        print(f"[Hand BBox] Error: {e}")

    return image


def draw_hands(
    image: Image.Image,
    hands_data: List,
    img_w: int,
    img_h: int,
    debug: bool = False
) -> Image.Image:
    """繪製多個手部

    Args:
        image: PIL Image
        hands_data: 手部資料列表 [[bbox, landmarks], ...]
                    bbox: [x, y, w, h, score]
                    landmarks: [[x, y, z], ...] 21 points
        img_w, img_h: 座標參考解析度
        debug: 調試模式
    """
    try:
        for hand_idx, hand in enumerate(hands_data):
            if not isinstance(hand, (list, tuple)) or len(hand) < 2:
                continue

            bbox = hand[0]
            landmarks = hand[1] if len(hand) > 1 else []

            # 繪製邊界框
            if bbox:
                label = f"Hand {hand_idx + 1}"
                image = draw_hand_bbox(image, bbox, img_w, img_h, label=label)

            # 繪製關鍵點
            if landmarks:
                image = draw_hand_landmarks(
                    image, landmarks, img_w, img_h,
                    hand_idx=hand_idx, debug=debug
                )

    except Exception as e:
        print(f"[Draw Hands] Error: {e}")

    return image
