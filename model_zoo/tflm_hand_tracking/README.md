# Hand Tracking Models

MediaPipe Hand Tracking 模型，用於 Grove Vision AI V2。

---

## ⚠️ 重要前置條件

| 需求 | 說明 |
|------|------|
| Python | 3.8 - 3.10 |
| 作業系統 | Linux (推薦) 或 WSL2 on Windows |
| 磁碟空間 | 至少 2GB (PINTO repo 很大) |
| 網路 | 需要連接 GitHub 和 Google Drive |

---

## 步驟 1: 下載 PINTO_model_zoo

### 方法 A: 完整 Clone (約 1-2GB)

```bash
# 在你想放置的目錄執行
cd ~/Downloads  # 或任意目錄

git clone --depth 1 https://github.com/PINTO0309/PINTO_model_zoo.git
cd PINTO_model_zoo/033_Hand_Detection_and_Tracking
```

### 方法 B: 只下載需要的資料夾 (推薦)

```bash
# 使用 sparse checkout
mkdir PINTO_hand && cd PINTO_hand
git init
git remote add origin https://github.com/PINTO0309/PINTO_model_zoo.git
git config core.sparseCheckout true
echo "033_Hand_Detection_and_Tracking/" >> .git/info/sparse-checkout
git pull origin main --depth 1

cd 033_Hand_Detection_and_Tracking
```

### 方法 C: 手動下載 (Windows 推薦)

1. 開啟瀏覽器前往：
   https://github.com/PINTO0309/PINTO_model_zoo/tree/main/033_Hand_Detection_and_Tracking

2. 查看 `download.sh` 內容，找到 Google Drive 連結

3. 或直接前往 PINTO 的 Release 頁面下載預編譯模型

---

## 步驟 2: 執行下載腳本

### Linux / WSL2:

```bash
cd 033_Hand_Detection_and_Tracking
chmod +x download.sh
./download.sh
```

**預期輸出：** 會下載多個 `.tflite` 檔案

### Windows (無 WSL):

需要手動下載，`download.sh` 內容通常是：

```bash
# 查看 download.sh 找到類似這樣的連結：
# curl -L "https://drive.google.com/uc?id=XXXXX" -o model.tflite
```

**替代方案：** 使用 [gdown](https://github.com/wkentaro/gdown) 工具：

```bash
pip install gdown
gdown --id <GOOGLE_DRIVE_FILE_ID> -O palm_detection.tflite
gdown --id <GOOGLE_DRIVE_FILE_ID> -O hand_landmark.tflite
```

---

## 步驟 3: 找到正確的模型檔案

下載完成後，尋找以下檔案（名稱可能略有不同）：

### Palm Detection (尋找 INT8 版本):
```
palm_detection_builtin_256_integer_quant.tflite
palm_detection_full_integer_quant.tflite
palm_detection_lite_integer_quant.tflite
```

**優先選擇：** `palm_detection_lite_integer_quant.tflite` (較小較快)

### Hand Landmark (尋找 INT8 版本):
```
hand_landmark_full_integer_quant.tflite
hand_landmark_lite_integer_quant.tflite
hand_landmark_3d_256_integer_quant.tflite
```

**優先選擇：** `hand_landmark_lite_integer_quant.tflite`

### 驗證模型:

```bash
# 確認檔案存在且大小合理 (通常 500KB - 2MB)
ls -la *integer_quant*.tflite

# 使用 Python 驗證模型可讀取
python3 -c "
import tensorflow as tf
model = tf.lite.Interpreter('palm_detection_lite_integer_quant.tflite')
model.allocate_tensors()
print('Input:', model.get_input_details()[0]['shape'])
print('Output:', [o['shape'] for o in model.get_output_details()])
"
```

---

## 步驟 4: Vela 編譯 (⚠️ 需要 Linux)

### 4.1 安裝 Vela

```bash
# 建議使用虛擬環境
python3 -m venv vela_env
source vela_env/bin/activate

pip install ethos-u-vela

# 驗證安裝
vela --version
# 預期輸出: Vela 3.x.x 或 4.x.x
```

**Windows 用戶：** Vela 不官方支援 Windows，請使用：
- WSL2 (Ubuntu)
- Docker
- Linux 虛擬機

### 4.2 編譯模型

```bash
# Palm Detection
vela palm_detection_lite_integer_quant.tflite \
    --accelerator-config=ethos-u55-128 \
    --optimise Performance \
    --output-dir=./vela_output/

# Hand Landmark
vela hand_landmark_lite_integer_quant.tflite \
    --accelerator-config=ethos-u55-128 \
    --optimise Performance \
    --output-dir=./vela_output/
```

### 4.3 驗證 Vela 輸出

```bash
ls -la vela_output/

# 應該看到:
# palm_detection_lite_integer_quant_vela.tflite
# hand_landmark_lite_integer_quant_vela.tflite
```

**如果編譯失敗：**
```bash
# 生成支援報告查看不支援的算子
vela --supported-ops-report

# 常見問題：
# - 模型使用了不支援的算子 → 嘗試其他模型版本
# - 記憶體不足 → 使用 --memory-mode Shared_Sram
```

---

## 步驟 5: 複製到專案目錄

### 目標路徑:
```
Seeed_Grove_Vision_AI_Module_V2/
└── model_zoo/
    └── tflm_hand_tracking/
        ├── 0_palm_det_0x400000.tflite    ← Palm Detection
        └── 1_hand_lm_0x480000.tflite     ← Hand Landmark
```

### 複製命令:

```bash
# 假設你在 PINTO 的 033 目錄
# 且 Grove Vision AI 專案在 ~/Projects/Seeed_Grove_Vision_AI_Module_V2

TARGET_DIR=~/Projects/Seeed_Grove_Vision_AI_Module_V2/model_zoo/tflm_hand_tracking

# 複製 Vela 編譯後的模型 (推薦)
cp vela_output/palm_detection_*_vela.tflite $TARGET_DIR/0_palm_det_0x400000.tflite
cp vela_output/hand_landmark_*_vela.tflite $TARGET_DIR/1_hand_lm_0x480000.tflite

# 或複製原始 INT8 模型 (如果 Vela 編譯失敗)
cp palm_detection_lite_integer_quant.tflite $TARGET_DIR/0_palm_det_0x400000.tflite
cp hand_landmark_lite_integer_quant.tflite $TARGET_DIR/1_hand_lm_0x480000.tflite
```

### 驗證:

```bash
ls -la $TARGET_DIR/
# 確認兩個檔案都存在且大小 > 100KB
```

---

## 步驟 6: 燒錄到裝置

### 6.1 連接裝置

1. 用 USB-C 連接 Grove Vision AI V2 到電腦
2. 確認 COM Port (Windows: 裝置管理員, Linux: `ls /dev/ttyUSB*`)

### 6.2 燒錄模型

```bash
cd Seeed_Grove_Vision_AI_Module_V2/xmodem

# 燒錄 Palm Detection 到 0x400000
python xmodem_send.py COM3 ../model_zoo/tflm_hand_tracking/0_palm_det_0x400000.tflite

# 燒錄 Hand Landmark 到 0x480000
python xmodem_send.py COM3 ../model_zoo/tflm_hand_tracking/1_hand_lm_0x480000.tflite
```

**注意：**
- 每個模型燒錄需要 1-3 分鐘
- 燒錄期間不要中斷連接
- 如果失敗，重新按 Reset 按鈕再試

---

## 模型規格參考

### 所需模型

| 檔案名稱 | 用途 | Flash 位址 | 輸入尺寸 | 預估大小 |
|----------|------|------------|----------|----------|
| `0_palm_det_0x400000.tflite` | Palm Detection | 0x400000 | 128×128 或 256×256 | 300-500KB |
| `1_hand_lm_0x480000.tflite` | Hand Landmark | 0x480000 | 256×256 | 500-900KB |

### Palm Detection 輸出
- **boxes**: `[1, 896, 18]` - 邊界框 + 7 個關鍵點
  - `[cx, cy, w, h, kp0_x, kp0_y, ..., kp6_x, kp6_y]`
- **scores**: `[1, 896, 1]` - 置信度分數

### Hand Landmark 輸出
- **landmarks**: `[1, 63]` 或 `[1, 21, 3]` - 21 個關鍵點
  - 每個關鍵點有 `(x, y, z)` 座標
- **handedness**: `[1, 1]` - 左右手判別 (可選)

### 21 點關鍵點索引

```
        8   12  16  20     (指尖)
        |   |   |   |
    4   7   11  15  19
    |   |   |   |   |
    3   6   10  14  18
    |   |   |   |   |
    2   5   9   13  17     (MCP)
    |    \  |  /   /
    1      \|/   /         (CMC/根部)
     \      0---/          (WRIST)

WRIST = 0
THUMB: CMC=1, MCP=2, IP=3, TIP=4
INDEX: MCP=5, PIP=6, DIP=7, TIP=8
MIDDLE: MCP=9, PIP=10, DIP=11, TIP=12
RING: MCP=13, PIP=14, DIP=15, TIP=16
PINKY: MCP=17, PIP=18, DIP=19, TIP=20
```

---

## 常見問題

### Q: 下載速度很慢？
A: PINTO repo 很大，使用方法 B (sparse checkout) 或方法 C (手動下載)

### Q: Vela 編譯失敗？
A:
1. 確認使用 Linux 或 WSL2
2. 嘗試其他模型版本 (lite vs full)
3. 檢查 `--supported-ops-report` 報告

### Q: 燒錄失敗？
A:
1. 確認 COM Port 正確
2. 降低 baudrate 到 115200 試試
3. 重新按 Reset 按鈕

### Q: 模型太大無法燒錄？
A: 每個模型應 < 1MB，使用 lite 版本而非 full 版本

---

## 給下一位 Claude Code 的筆記

如果你是接續這份工作的 Claude Code：

1. **已完成的工作：**
   - 韌體骨架：`EPII_CM55M_APP_S/app/scenario_app/tflm_hand_tracking/`
   - Python 測試：`pythontool/test_hand_tracking.py`
   - 規格書：`pythontool/規格書.md`

2. **待完成的工作：**
   - 下載並放置模型到 `model_zoo/tflm_hand_tracking/`
   - 編譯韌體 (`make` in `EPII_CM55M_APP_S/`)
   - 測試推論流程

3. **關鍵檔案：**
   - `cvapp_hand_tracking.cpp` - 主推論邏輯
   - `palm_postprocessing.cpp` - Palm Detection 後處理
   - `hand_landmark.cpp` - Hand Landmark 後處理

4. **注意事項：**
   - 模型輸出格式需要根據實際 PINTO 模型調整
   - Anchor 配置可能需要修改
   - 建議先用 Python 測試模型輸出格式
