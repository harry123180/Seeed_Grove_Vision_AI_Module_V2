#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""繪圖工具模組"""

from PIL import Image, ImageDraw
from typing import List

# Import hand drawing functions
from .hand_drawing import draw_hands, draw_hand_landmarks, draw_hand_bbox

# COCO 80 類別
COCO_CLASSES = [
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck",
    "boat", "traffic light", "fire hydrant", "stop sign", "parking meter", "bench",
    "bird", "cat", "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra",
    "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove",
    "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup",
    "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
    "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse",
    "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
    "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier",
    "toothbrush"
]

# 姿態骨架連接
POSE_SKELETON = [
    (0, 1), (0, 2), (1, 3), (2, 4),
    (5, 6), (5, 7), (7, 9), (6, 8), (8, 10),
    (5, 11), (6, 12), (11, 12),
    (11, 13), (13, 15), (12, 14), (14, 16)
]

# 顏色
COLORS = ["#FF0000", "#00FF00", "#0000FF", "#FFFF00", "#FF00FF", "#00FFFF",
          "#FFA500", "#800080", "#008000", "#000080"]


def get_class_name(class_id: int) -> str:
    """取得類別名稱"""
    if 0 <= class_id < len(COCO_CLASSES):
        return COCO_CLASSES[class_id]
    return f"class_{class_id}"


def draw_boxes(image: Image.Image, boxes: List, img_w: int, img_h: int) -> Image.Image:
    """繪製偵測框"""
    try:
        draw = ImageDraw.Draw(image)
        actual_w, actual_h = image.size

        scale_x = actual_w / img_w if img_w > 0 else 1
        scale_y = actual_h / img_h if img_h > 0 else 1

        for i, box in enumerate(boxes):
            try:
                if not isinstance(box, list) or len(box) < 6:
                    continue

                x, y, w, h, score, target = box[:6]

                x1 = int(x * scale_x)
                y1 = int(y * scale_y)
                x2 = int((x + w) * scale_x)
                y2 = int((y + h) * scale_y)

                color = COLORS[int(target) % len(COLORS)]
                class_name = get_class_name(int(target))

                draw.rectangle([x1, y1, x2, y2], outline=color, width=3)
                label = f"{class_name}: {score}%"
                draw.text((x1, max(0, y1 - 15)), label, fill=color)

            except Exception as e:
                print(f"[Draw Boxes] Error on box {i}: {e}")
                continue

    except Exception as e:
        print(f"[Draw Boxes] Fatal error: {e}")

    return image


def draw_keypoints(image: Image.Image, keypoints_list: List, img_w: int, img_h: int) -> Image.Image:
    """繪製姿態關鍵點"""
    try:
        draw = ImageDraw.Draw(image)
        actual_w, actual_h = image.size

        scale_x = actual_w / img_w if img_w > 0 else 1
        scale_y = actual_h / img_h if img_h > 0 else 1

        for person_idx, person in enumerate(keypoints_list):
            try:
                if not isinstance(person, list) or len(person) < 2:
                    continue

                bbox = person[0]
                kpts = person[1:] if len(person) > 1 else []

                if isinstance(bbox, list) and len(bbox) >= 4:
                    x, y, w, h = bbox[:4]
                    x1, y1 = int(x * scale_x), int(y * scale_y)
                    x2, y2 = int((x + w) * scale_x), int((y + h) * scale_y)
                    draw.rectangle([x1, y1, x2, y2], outline="#00FF00", width=2)

                points = []
                for kpt in kpts:
                    try:
                        if isinstance(kpt, list) and len(kpt) >= 2:
                            px = int(kpt[0] * scale_x)
                            py = int(kpt[1] * scale_y)
                            points.append((px, py))
                            r = 4
                            draw.ellipse([px-r, py-r, px+r, py+r], fill="#FF0000", outline="#FFFFFF")
                    except (TypeError, ValueError):
                        pass

                for start_idx, end_idx in POSE_SKELETON:
                    if start_idx < len(points) and end_idx < len(points):
                        draw.line([points[start_idx], points[end_idx]], fill="#00FFFF", width=2)

            except Exception as e:
                print(f"[Draw Keypoints] Error on person {person_idx}: {e}")
                continue

    except Exception as e:
        print(f"[Draw Keypoints] Fatal error: {e}")

    return image


def draw_face_mesh(
    image: Image.Image,
    fm_points: List,
    img_w: int,
    img_h: int,
    debug: bool = False,
    offset_x: int = 0,
    offset_y: int = 0,
    scale: float = 1.0
) -> Image.Image:
    """繪製人臉網格 (468 點)

    Args:
        image: PIL Image
        fm_points: Face mesh 資料 [[bbox, [points...]], ...]
        img_w, img_h: 座標參考解析度（關鍵點座標的原始範圍）
        debug: 啟用調試輸出
        offset_x, offset_y: 偏移量（像素）
        scale: 額外縮放係數
    """
    try:
        draw = ImageDraw.Draw(image)
        actual_w, actual_h = image.size

        # 計算縮放比例：從參考解析度到實際圖像尺寸
        scale_x = (actual_w / img_w) * scale if img_w > 0 else scale
        scale_y = (actual_h / img_h) * scale if img_h > 0 else scale

        if debug:
            print(f"[Face Mesh] actual={actual_w}x{actual_h}, ref={img_w}x{img_h}, scale={scale_x:.2f},{scale_y:.2f}, offset=({offset_x},{offset_y})")

        for face_idx, face in enumerate(fm_points):
            try:
                if not isinstance(face, list) or len(face) < 2:
                    continue

                # face[0] = bbox [x, y, w, h, score, ...]
                bbox = face[0]
                if isinstance(bbox, list) and len(bbox) >= 4:
                    x, y, w, h = bbox[:4]
                    x1 = int(x * scale_x) + offset_x
                    y1 = int(y * scale_y) + offset_y
                    x2 = int((x + w) * scale_x) + offset_x
                    y2 = int((y + h) * scale_y) + offset_y
                    draw.rectangle([x1, y1, x2, y2], outline="#00FF00", width=2)

                    if debug:
                        print(f"[Face {face_idx}] bbox=({x},{y},{w},{h}) -> ({x1},{y1})-({x2},{y2})")

                # face[1] = mesh points 列表
                if len(face) > 1 and isinstance(face[1], list):
                    mesh_points = face[1]

                    if debug and mesh_points:
                        # 顯示前幾個點以驗證座標範圍
                        sample = mesh_points[:3]
                        print(f"[Face {face_idx}] Sample points: {sample}")

                    # 批量繪製點（跳過無效點 [0,0,0,0]）
                    valid_count = 0
                    for pt in mesh_points:
                        try:
                            if isinstance(pt, list) and len(pt) >= 2:
                                # 跳過無效點（全零或接近零）
                                if pt[0] < 2 and pt[1] < 2:
                                    continue

                                px = int(pt[0] * scale_x) + offset_x
                                py = int(pt[1] * scale_y) + offset_y
                                if 0 <= px < actual_w and 0 <= py < actual_h:
                                    draw.ellipse([px-1, py-1, px+1, py+1], fill="#00FFFF")
                                    valid_count += 1
                        except (TypeError, ValueError):
                            pass

                    if debug:
                        print(f"[Face {face_idx}] Drew {valid_count}/{len(mesh_points)} valid points")

            except Exception as e:
                if debug:
                    print(f"[Face Mesh] Error face {face_idx}: {e}")
                continue

    except Exception as e:
        print(f"[Face Mesh] Fatal error: {e}")

    return image


def resize_image(image: Image.Image, max_width: int, max_height: int) -> Image.Image:
    """調整影像大小"""
    width, height = image.size
    ratio = min(max_width / width, max_height / height)

    if ratio < 1:
        new_size = (int(width * ratio), int(height * ratio))
        return image.resize(new_size, Image.Resampling.LANCZOS)
    return image
