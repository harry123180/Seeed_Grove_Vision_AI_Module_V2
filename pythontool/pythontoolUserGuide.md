# Grove Vision AI V2 Python Tool 使用指南

## 概述

這是一個基於 CustomTkinter 的上位機工具，用於與 Seeed Grove Vision AI Module V2 通訊，接收並顯示即時影像串流和 AI 推論結果。

## 系統需求

- Python 3.8+
- Windows / macOS / Linux

## 安裝依賴

```bash
pip install customtkinter pyserial pillow
```

## 快速開始

1. 將 Grove Vision AI V2 透過 USB 連接到電腦
2. 執行程式：
   ```bash
   python grove_vision_ai_tool.py
   ```
3. 在下拉選單中選擇對應的 COM Port
4. 點擊「Connect」按鈕
5. 影像和偵測結果將自動顯示

## 通訊協議

### 串口設定
- 鮑率：921600 bps
- 資料位元：8 bits
- 校驗位元：None
- 停止位元：1 bit

### JSON 資料格式

韌體透過 UART 傳送 JSON 格式資料，每筆訊息以 `\r` 開頭，`\n` 結尾：

```json
{
  "type": 1,
  "name": "INVOKE",
  "code": 0,
  "data": {
    "count": 0,
    "resolution": [640, 480],
    "image": "<BASE64_JPEG>",
    "boxes": [[x, y, w, h, score, target], ...]
  }
}
```

### 欄位說明

| 欄位 | 類型 | 說明 |
|------|------|------|
| `type` | int | 訊息類型（0=查詢, 1=推論結果） |
| `name` | string | 指令名稱 |
| `code` | int | 錯誤碼（0=成功） |
| `data.resolution` | array | 影像解析度 [width, height] |
| `data.image` | string | Base64 編碼的 JPEG 圖片 |
| `data.boxes` | array | 偵測框陣列 |
| `data.keypoints` | array | 姿態關鍵點（pose 模型） |

### 偵測框格式 (boxes)

每個偵測框為一個陣列：`[x, y, w, h, score, target]`

| 索引 | 說明 |
|------|------|
| 0 | x - 左上角 X 座標 |
| 1 | y - 左上角 Y 座標 |
| 2 | w - 寬度 |
| 3 | h - 高度 |
| 4 | score - 信心分數 (0-100) |
| 5 | target - 類別 ID |

### 姿態關鍵點格式 (keypoints)

用於 YOLOv8 Pose 模型，每個人體為一個陣列：
```
[
  [bbox_x, bbox_y, bbox_w, bbox_h, bbox_score, target],
  [kpt0_x, kpt0_y, kpt0_score, kpt0_target],
  [kpt1_x, kpt1_y, kpt1_score, kpt1_target],
  ...
  [kpt16_x, kpt16_y, kpt16_score, kpt16_target]
]
```

17 個關鍵點順序（COCO 格式）：
0. 鼻子
1. 左眼
2. 右眼
3. 左耳
4. 右耳
5. 左肩
6. 右肩
7. 左手肘
8. 右手肘
9. 左手腕
10. 右手腕
11. 左髖
12. 右髖
13. 左膝
14. 右膝
15. 左腳踝
16. 右腳踝

## 模式切換

可透過發送單一 byte 切換傳輸模式：

| 發送值 | 效果 |
|--------|------|
| `0xFF` (255) | 切換到 UART 模式 |
| `0xFE` (254) | 切換到 SPI 模式 |
| `0xFD` (253) | 切換到 UART+SPI 模式 |

## 故障排除

### 無法連接串口
- 確認裝置已正確連接
- 檢查是否有其他程式佔用該串口
- 嘗試重新插拔 USB

### 無資料輸出
- 確認韌體已正確燒錄
- 嘗試發送 `0xFF` 切換到 UART 模式
- 檢查韌體是否為支援 UART 輸出的版本

### 圖片顯示異常
- Base64 解碼可能不完整，檢查 JSON 是否完整接收
- 確認影像格式為 JPEG

## 參考資源

- [Seeed Grove Vision AI V2 GitHub](https://github.com/Seeed-Studio/Seeed_Grove_Vision_AI_Module_V2)
- [SenseCraft AI Web Toolkit](https://seeed-studio.github.io/SenseCraft-Web-Toolkit/)
