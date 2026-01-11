# Grove Vision AI Module V2 - Python Tool

PC 端上位機工具，用於與 Grove Vision AI Module V2 進行串流監控和韌體燒錄。

## 系統需求

- Python 3.8+
- Windows / macOS / Linux
- USB 連接至 Grove Vision AI Module V2

## 安裝

```bash
cd pythontool
pip install -r requirements.txt
```

**依賴套件：**
- `customtkinter` - 現代化 GUI 框架
- `pillow` - 影像處理
- `pyserial` - 串口通訊
- `opencv-python` - 影像顯示
- `numpy` - 數值計算

## 快速開始

### 啟動 GUI 工具

```bash
python main.py
```

### GUI 功能

| 分頁 | 功能 |
|------|------|
| **串流監控** | 即時顯示攝影機畫面、偵測結果繪製、FPS 統計 |
| **韌體燒錄** | 選擇預設應用、燒錄韌體和模型 |

---

## 支援的應用程式

### 1. Face Mesh (人臉網格) - `tflm_fd_fm`

三模型串聯推論：人臉偵測 → 人臉網格 → 虹膜關鍵點

**功能特點：**
- 人臉偵測 (YOLO-Fastest)
- 468 點人臉網格 (MediaPipe Face Mesh)
- 虹膜關鍵點偵測 (MediaPipe Iris)

**模型配置：**

| 模型 | 輸入尺寸 | Flash 位址 | 大小 |
|------|----------|------------|------|
| Face Detection | 160×160 | 0x200000 | ~470 KB |
| Face Mesh | 192×192 | 0x280000 | ~688 KB |
| Iris Landmark | 64×64 | 0x32A000 | ~789 KB |

**輸出格式：**
```json
{
  "type": 1,
  "name": "INVOKE",
  "data": {
    "resolution": [640, 480],
    "image": "<BASE64_JPEG>",
    "boxes": [[x, y, w, h, score, 0], ...],
    "landmarks": [[x0, y0], [x1, y1], ..., [x467, y467]]
  }
}
```

### 2. Hand Tracking (手部追蹤) - `tflm_hand_tracking`

**狀態：** 開發中 (`feature/hand-tracking` 分支)

兩階段推論：手掌偵測 → 手部關鍵點

**功能特點：**
- Palm Detection (手掌偵測)
- 21 點手部關鍵點 (MediaPipe Hand Landmark)
- 支援最多 2 隻手同時偵測

**模型配置：**

| 模型 | 輸入尺寸 | Flash 位址 | 大小 |
|------|----------|------------|------|
| Palm Detection | 256×256 | 0x400000 | ~2.1 MB |
| Hand Landmark | 256×256 | 0x620000 | ~1.8 MB |

**手部關鍵點定義 (21 點)：**
```
      8   12  16  20
      |   |   |   |
      7   11  15  19
      |   |   |   |
      6   10  14  18
      |   |   |   |
      5---9---13--17
       \         /
        4
        |
        3
        |
        2
        |
        1
        |
        0 (Wrist)
```

| 手指 | 關鍵點 |
|------|--------|
| Wrist | 0 |
| Thumb | 1 (CMC), 2 (MCP), 3 (IP), 4 (TIP) |
| Index | 5 (MCP), 6 (PIP), 7 (DIP), 8 (TIP) |
| Middle | 9 (MCP), 10 (PIP), 11 (DIP), 12 (TIP) |
| Ring | 13 (MCP), 14 (PIP), 15 (DIP), 16 (TIP) |
| Pinky | 17 (MCP), 18 (PIP), 19 (DIP), 20 (TIP) |

### 3. 其他支援應用

| 應用 | 說明 |
|------|------|
| `tflm_yolov8_od` | YOLOv8 物件偵測 (80 類 COCO) |
| `tflm_yolov8_pose` | YOLOv8 人體姿態估計 (17 點) |
| `tflm_yolov8_gender_cls` | YOLOv8 性別分類 |
| `tflm_peoplenet` | PeopleNet 人物偵測 |
| `tflm_yolo11_od` | YOLO11 物件偵測 |

---

## 串口通訊

### 連接設定

| 參數 | 值 |
|------|-----|
| Baud Rate | 921600 |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |

### AT 指令

```bash
# 測試連線
AT

# 啟動推論串流 (帶圖片)
AT+INVOKE=1,0,0

# 停止推論
AT+INVOKE=0,0,0
```

### JSON 資料格式

**通用結構：**
```json
{
  "type": 1,
  "name": "INVOKE",
  "code": 0,
  "data": {
    "count": 0,
    "resolution": [width, height],
    "image": "<BASE64_JPEG>",
    "boxes": [[x, y, w, h, score, target], ...],
    "keypoints": [...]
  }
}
```

**欄位說明：**

| 欄位 | 說明 |
|------|------|
| `type` | 訊息類型 (1=推論結果) |
| `resolution` | 影像解析度 [寬, 高] |
| `image` | Base64 編碼的 JPEG 影像 |
| `boxes` | 偵測框 [x, y, w, h, score, class] |
| `keypoints` | 關鍵點座標陣列 |

---

## 工具腳本

### 主要工具

| 腳本 | 用途 | 指令 |
|------|------|------|
| `main.py` | GUI 主程式 | `python main.py` |
| `test_palm_model.py` | PC 端模型測試 | `python test_palm_model.py --image test.png` |
| `realtime_compare.py` | PC vs 板子即時比對 | `python realtime_compare.py --port COM3` |

### 除錯工具

| 腳本 | 用途 | 指令 |
|------|------|------|
| `capture_raw_tensor.py` | 擷取 Raw Tensor | `python capture_raw_tensor.py --port COM3` |
| `compare_raw_tensor.py` | 比對 Tensor 輸出 | `python compare_raw_tensor.py --port COM3` |
| `capture_frame.py` | 擷取單張影像 | `python capture_frame.py --port COM3` |

### 即時比對工具操作

```bash
python realtime_compare.py --port COM3
```

**快捷鍵：**
- `1` - 只顯示 PC 結果 (綠框)
- `2` - 只顯示板子結果 (紅框)
- `3` - 疊圖顯示兩者
- `S` - 儲存當前畫面
- `Q` - 退出

**Log 輸出：** `logs/compare_*.log`

---

## 韌體燒錄

### 使用 GUI 燒錄

1. 啟動 `python main.py`
2. 切換到「韌體燒錄」分頁
3. 選擇預設應用 (如 Face Mesh)
4. 點擊「燒錄韌體」

### 使用 XMODEM 命令列燒錄

**燒錄韌體：**
```bash
cd xmodem
python xmodem_send.py COM3 ../we2_image_gen_local/output_case1_sec_wlcsp/output.img
```

**燒錄模型：**
```bash
# Face Mesh 模型
python xmodem_send.py COM3 ../model_zoo/tflm_fd_fm/0_yolo_face_detect_0x200000_vela.tflite 0x200000
python xmodem_send.py COM3 ../model_zoo/tflm_fd_fm/1_face_mesh_0x280000_vela.tflite 0x280000
python xmodem_send.py COM3 ../model_zoo/tflm_fd_fm/2_iris_landmark_0x32A000_vela.tflite 0x32A000

# Hand Tracking 模型
python xmodem_send.py COM3 ../model_zoo/tflm_hand_tracking/0_palm_det_0x400000_vela.tflite 0x400000
python xmodem_send.py COM3 ../model_zoo/tflm_hand_tracking/1_hand_lm_0x620000_vela.tflite 0x620000
```

---

## 目錄結構

```
pythontool/
├── main.py                    # GUI 入口
├── requirements.txt           # 依賴套件
├── README.md                  # 本文件
│
├── src/                       # 核心模組
│   ├── main_window.py         # 主視窗 (CustomTkinter)
│   ├── serial_reader.py       # 串口讀取器
│   ├── image_processor.py     # 影像處理
│   ├── drawing.py             # 繪圖工具 (人臉/姿態)
│   ├── hand_drawing.py        # 手部繪圖
│   ├── model_config.py        # 模型配置管理
│   └── xmodem_flasher.py      # XMODEM 燒錄器
│
├── logs/                      # Log 輸出目錄
│   ├── compare_*.log          # 比對結果
│   ├── raw_tensor_*.log       # Raw Tensor 擷取
│   └── serial_*.log           # 串口原始資料
│
├── test_palm_model.py         # Palm Detection 測試
├── realtime_compare.py        # 即時比對工具
├── capture_raw_tensor.py      # Tensor 擷取
├── compare_raw_tensor.py      # Tensor 比對
└── capture_frame.py           # 影像擷取
```

---

## 故障排除

### 無法連接串口

1. 確認 USB 線已連接
2. 檢查裝置管理員中的 COM Port
3. 確認沒有其他程式佔用串口

### 無影像顯示

1. 發送 `AT+INVOKE=1,0,0` 啟動串流
2. 檢查韌體是否正確燒錄
3. 確認模型已燒錄到正確的 Flash 位址

### 偵測框位置錯誤

1. 確認使用正確版本的 Vela 模型
2. 檢查 anchor 配置是否與模型匹配
3. 使用 `realtime_compare.py` 比對 PC 和板子的輸出

### 燒錄失敗

1. 確認板子處於燒錄模式
2. 檢查 XMODEM 傳輸是否逾時
3. 嘗試降低波特率或重新連接

---

## 相關連結

- [Grove Vision AI Module V2 主 README](../README.md)
- [Face Mesh 應用說明](../EPII_CM55M_APP_S/app/scenario_app/tflm_fd_fm/README.md)
- [Hand Tracking 規格書](../EPII_CM55M_APP_S/app/scenario_app/tflm_hand_tracking/SPECIFICATION.md)
- [模型來源 - PINTO Model Zoo](https://github.com/PINTO0309/PINTO_model_zoo)
- [MediaPipe](https://github.com/google/mediapipe)

---

## 授權

本專案遵循 Apache 2.0 授權。
