#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
模型配置模組 - 集中管理所有模型和燒錄配置

這個模組解決以下問題：
1. 模型地址配置分散在多處
2. 缺少模型重疊驗證
3. Preset 方法重複代碼
4. 硬編碼路徑
"""

import os
from dataclasses import dataclass, field
from typing import List, Optional, Dict
from enum import Enum


# ============================================
# 常數定義
# ============================================

class FlashLayout:
    """Flash 記憶體配置"""
    FIRMWARE_START = 0x000000
    FIRMWARE_MAX_SIZE = 0x200000  # 2MB
    MODEL_AREA_START = 0x200000   # 2MB offset
    FLASH_TOTAL_SIZE = 0x1000000  # 16MB (typical)
    ALIGNMENT = 0x1000            # 4KB alignment


class Paths:
    """路徑配置"""
    MODEL_ZOO_DIR = "model_zoo"
    IMAGE_GEN_DIR = "we2_image_gen_local"
    OUTPUT_SUBDIR = "output_case1_sec_wlcsp"
    FIRMWARE_NAME = "output.img"


# ============================================
# 資料結構
# ============================================

@dataclass
class ModelInfo:
    """模型資訊"""
    name: str                    # 檔案名稱
    flash_addr: int              # Flash 地址
    description: str = ""        # 描述
    estimated_size: int = 0      # 預估大小 (bytes)

    @property
    def end_addr(self) -> int:
        """計算結束地址"""
        return self.flash_addr + self.estimated_size

    def is_aligned(self) -> bool:
        """檢查地址是否 4KB 對齊"""
        return self.flash_addr % FlashLayout.ALIGNMENT == 0


@dataclass
class AppPreset:
    """應用程式預設配置"""
    name: str                           # 顯示名稱
    app_type: str                       # APP_TYPE (makefile)
    model_subdir: str                   # model_zoo 子目錄
    models: List[ModelInfo] = field(default_factory=list)
    description: str = ""


# ============================================
# 預設配置定義
# ============================================

PRESETS: Dict[str, AppPreset] = {
    "face_mesh": AppPreset(
        name="Face Mesh",
        app_type="tflm_fd_fm",
        model_subdir="tflm_fd_fm",
        models=[
            ModelInfo("0_fd_0x200000.tflite", 0x200000, "Face Detection", 500*1024),
            ModelInfo("1_fm_0x280000.tflite", 0x280000, "Face Mesh", 650*1024),
            ModelInfo("2_il_0x32A000.tflite", 0x32A000, "Iris Landmark", 200*1024),
        ],
        description="人臉偵測 + 468 點 Face Mesh"
    ),

    "yolov8_od": AppPreset(
        name="YOLOv8 OD",
        app_type="tflm_yolov8_od",
        model_subdir="tflm_yolov8_od",
        models=[],  # 動態從目錄載入
        description="YOLOv8 物件偵測"
    ),

    "yolov8_pose": AppPreset(
        name="YOLOv8 Pose",
        app_type="tflm_yolov8_pose",
        model_subdir="tflm_yolov8_pose",
        models=[],  # 動態從目錄載入
        description="YOLOv8 人體姿態估計"
    ),

    "hand_tracking": AppPreset(
        name="Hand Tracking",
        app_type="tflm_hand_tracking",
        model_subdir="tflm_hand_tracking",
        models=[
            ModelInfo("0_palm_det_0x400000_vela.tflite", 0x400000, "Palm Detection", 2200*1024),
            ModelInfo("1_hand_lm_0x620000_vela.tflite", 0x620000, "Hand Landmark", 1900*1024),
        ],
        description="手部追蹤 (Palm Detection + 21 Landmarks)"
    ),
}


# ============================================
# 驗證函數
# ============================================

class ValidationError(Exception):
    """驗證錯誤"""
    pass


def validate_model_layout(models: List[ModelInfo]) -> List[str]:
    """
    驗證模型配置是否有效

    Returns:
        錯誤訊息列表 (空列表表示通過)
    """
    errors = []

    if not models:
        return errors

    # 按地址排序
    sorted_models = sorted(models, key=lambda m: m.flash_addr)

    for i, model in enumerate(sorted_models):
        # 檢查對齊
        if not model.is_aligned():
            errors.append(
                f"模型 '{model.name}' 地址 0x{model.flash_addr:X} 未 4KB 對齊"
            )

        # 檢查是否在模型區域內
        if model.flash_addr < FlashLayout.MODEL_AREA_START:
            errors.append(
                f"模型 '{model.name}' 地址 0x{model.flash_addr:X} 在韌體區域內"
            )

        # 檢查重疊
        if i > 0 and model.estimated_size > 0:
            prev_model = sorted_models[i - 1]
            if prev_model.estimated_size > 0:
                if prev_model.end_addr > model.flash_addr:
                    overlap = prev_model.end_addr - model.flash_addr
                    errors.append(
                        f"模型重疊! '{prev_model.name}' (結束於 0x{prev_model.end_addr:X}) "
                        f"與 '{model.name}' (起始於 0x{model.flash_addr:X}) 重疊 {overlap} bytes"
                    )

        # 檢查是否超出 Flash
        if model.end_addr > FlashLayout.FLASH_TOTAL_SIZE:
            errors.append(
                f"模型 '{model.name}' 超出 Flash 容量 (結束於 0x{model.end_addr:X})"
            )

    return errors


def validate_models_with_actual_sizes(models: List[tuple], base_path: str) -> List[str]:
    """
    使用實際檔案大小驗證模型配置

    Args:
        models: [(path, addr, offset), ...] 格式
        base_path: 基礎路徑

    Returns:
        錯誤訊息列表
    """
    model_infos = []

    for path, addr, offset in models:
        if os.path.exists(path):
            size = os.path.getsize(path)
            name = os.path.basename(path)
            model_infos.append(ModelInfo(name, addr, "", size))

    return validate_model_layout(model_infos)


# ============================================
# 輔助函數
# ============================================

def get_project_root() -> str:
    """取得專案根目錄"""
    # pythontool/src/model_config.py -> 專案根目錄
    return os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def get_model_dir(preset_key: str) -> str:
    """取得模型目錄路徑"""
    preset = PRESETS.get(preset_key)
    if not preset:
        raise ValueError(f"Unknown preset: {preset_key}")

    root = get_project_root()
    return os.path.join(root, Paths.MODEL_ZOO_DIR, preset.model_subdir)


def get_firmware_path() -> str:
    """取得韌體路徑"""
    root = get_project_root()
    return os.path.join(
        root,
        Paths.IMAGE_GEN_DIR,
        Paths.OUTPUT_SUBDIR,
        Paths.FIRMWARE_NAME
    )


def load_preset_models(preset_key: str) -> List[tuple]:
    """
    載入預設配置的模型列表

    Returns:
        [(path, addr, offset), ...] 格式
    """
    preset = PRESETS.get(preset_key)
    if not preset:
        raise ValueError(f"Unknown preset: {preset_key}")

    model_dir = get_model_dir(preset_key)
    models = []

    if preset.models:
        # 使用預定義的模型列表
        for model_info in preset.models:
            path = os.path.join(model_dir, model_info.name)
            if os.path.exists(path):
                models.append((path, model_info.flash_addr, 0))
    else:
        # 動態從目錄載入 (解析檔名中的地址)
        import re
        if os.path.exists(model_dir):
            for f in sorted(os.listdir(model_dir)):
                if f.endswith('.tflite'):
                    match = re.search(r'0x([0-9A-Fa-f]+)', f)
                    if match:
                        addr = int(match.group(1), 16)
                        path = os.path.join(model_dir, f)
                        models.append((path, addr, 0))

    return models


def format_size(size_bytes: int) -> str:
    """格式化檔案大小"""
    if size_bytes < 1024:
        return f"{size_bytes} B"
    elif size_bytes < 1024 * 1024:
        return f"{size_bytes / 1024:.1f} KB"
    else:
        return f"{size_bytes / (1024 * 1024):.2f} MB"


def print_flash_layout(models: List[tuple]):
    """印出 Flash 記憶體配置圖"""
    print("\n" + "=" * 60)
    print("Flash Memory Layout")
    print("=" * 60)
    print(f"{'Address':<12} {'Size':<10} {'End':<12} {'File'}")
    print("-" * 60)

    for path, addr, _ in sorted(models, key=lambda x: x[1]):
        if os.path.exists(path):
            size = os.path.getsize(path)
            end = addr + size
            name = os.path.basename(path)
            print(f"0x{addr:08X}  {format_size(size):<10} 0x{end:08X}  {name}")

    print("=" * 60 + "\n")
