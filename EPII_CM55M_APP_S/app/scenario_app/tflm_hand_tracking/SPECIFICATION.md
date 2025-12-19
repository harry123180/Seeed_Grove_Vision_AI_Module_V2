# MediaPipe Hand Tracking 可行性評估報告

**專案名稱**：Grove Vision AI V2 - MediaPipe Hand Tracking 整合
**文件版本**：V1.0
**撰寫日期**：2025/12/19
**目標平台**：Seeed Grove Vision AI Module V2 (Himax WiseEye2 HX6538)

---

## 1. 專案背景

### 1.1 硬體規格
| 項目 | 規格 |
|------|------|
| 處理器 | Himax WiseEye2 HX6538 |
| CPU | Dual-core Arm Cortex-M55 |
| NPU | Arm Ethos-U55 (128 MAC) |
| 記憶體 | 2MB SRAM |
| Flash | 16MB |
| 推論框架 | TensorFlow Lite Micro (TFLM) |

### 1.2 目標功能
在 Grove Vision AI V2 上實現即時手部追蹤，輸出 21 個手部關鍵點座標，支援手勢識別等應用場景。

---

## 2. 現有架構分析

### 2.1 參考實作：Face Detection + Face Mesh (tflm_fd_fm)

分析 `EPII_CM55M_APP_S/app/scenario_app/tflm_fd_fm/cvapp_fd_fm.cpp` 得知現有多模型推論架構：

#### 模型規格

| 模型 | 用途 | 輸入尺寸 | 檔案大小 | Flash 位址 |
|------|------|----------|----------|------------|
| Face Detection | 人臉偵測 | 160×160 | ~460KB | 0x200000 |
| Face Mesh | 468 點網格 | 192×192 | ~688KB | 0x280000 |
| Iris Landmark | 虹膜追蹤 | 64×64 | ~789KB | 0x32A000 |

#### 記憶體配置

```cpp
// 來源: cvapp_fd_fm.cpp:165
constexpr int tensor_arena_size = 460*1024;           // 460KB 共享 tensor arena
constexpr int tensor_arena_model_tail_size = 1224;    // 模型間偏移量
```

#### 關鍵架構設計

1. **共享 Tensor Arena**：三個 MicroInterpreter 共用同一塊記憶體
2. **偏移量管理**：使用 `tensor_arena_model_tail_size` 避免記憶體衝突
3. **最小化 Operator**：僅需 `AddEthosU()` + `AddPad()` 兩個算子

```cpp
// 來源: cvapp_fd_fm.cpp:1092-1099
if (kTfLiteOk != op_resolver.AddEthosU()){
    xprintf("Failed to add Arm NPU support to op resolver.");
    return false;
}
if(kTfLiteOk != op_resolver.AddPad()){
    xprintf("Failed to add Padding support to op resolver.");
    return false;
}
```

#### 模型載入機制

```cpp
// 來源: cvapp_fd_fm.cpp:1054-1056
static const tflite::Model*model = tflite::GetModel((const void *)fd_model_addr);
static const tflite::Model*FM_model = tflite::GetModel((const void *)fm_model_addr);
static const tflite::Model*IL_model = tflite::GetModel((const void *)il_model_addr);
```

---

## 3. MediaPipe Hand 模型分析

### 3.1 模型來源比較

| 來源 | 量化類型 | NPU 相容性 | 備註 |
|------|----------|------------|------|
| [Google MediaPipe 官方](https://github.com/google/mediapipe) | Float32 | ❌ 不支援 | NPU 需要 INT8 |
| [PINTO_model_zoo](https://github.com/PINTO0309/PINTO_model_zoo) | INT8 | ✅ 支援 | 社群量化版本 |

### 3.2 PINTO_model_zoo 手部模型規格

位於 `033_Hand_Detection_and_Tracking` 目錄：

| 模型 | 輸入尺寸 | 輸出 | 預估大小 | 檔案名稱 |
|------|----------|------|----------|----------|
| Palm Detection | 128×128 或 256×256 | BBox + confidence | ~300-500KB | `palm_detection_builtin_256_integer_quant.tflite` |
| Hand Landmark | 256×256 | 21 keypoints (x,y,z) | ~500-800KB | `hand_landmark_3d_256_integer_quant.tflite` |

### 3.3 手部關鍵點定義 (21 Points)

```
        8   12  16  20
        |   |   |   |
    4   7   11  15  19
    |   |   |   |   |
    3   6   10  14  18
    |   |   |   |   |
    2   5   9   13  17
    |    \  |  /   /
    1      \|/   /
     \      0---/
      \    /
       WRIST

0: WRIST
1-4: THUMB (CMC, MCP, IP, TIP)
5-8: INDEX (MCP, PIP, DIP, TIP)
9-12: MIDDLE (MCP, PIP, DIP, TIP)
13-16: RING (MCP, PIP, DIP, TIP)
17-20: PINKY (MCP, PIP, DIP, TIP)
```

---

## 4. Ethos-U55 NPU 算子支援度分析

### 4.1 支援的 TensorFlow Lite 算子

根據 [Vela SUPPORTED_OPS.md](https://github.com/nxp-imx/ethos-u-vela/blob/lf-6.12.3_1.0.0/SUPPORTED_OPS.md) 分析：

| 算子類別 | 算子名稱 | 支援狀態 | 備註 |
|----------|----------|----------|------|
| 卷積 | CONV_2D | ✅ 完全支援 | 核心操作 |
| 卷積 | DEPTHWISE_CONV_2D | ✅ 完全支援 | 深度分離卷積 |
| 卷積 | TRANSPOSE_CONV | ✅ 支援 | 反卷積 |
| 池化 | MAX_POOL_2D | ✅ 完全支援 | 最大池化 |
| 池化 | AVERAGE_POOL_2D | ✅ 完全支援 | 平均池化 |
| 激活 | RELU / RELU6 | ✅ 完全支援 | 激活函數 |
| 激活 | LOGISTIC / TANH | ✅ 完全支援 | Sigmoid/Tanh |
| 數學 | ADD / MUL / SUB | ✅ 完全支援 | 元素運算 |
| 數學 | MAXIMUM / MINIMUM | ✅ 完全支援 | 比較運算 |
| 縮放 | RESIZE_BILINEAR | ✅ 支援 | 雙線性插值 |
| 縮放 | RESIZE_NEAREST_NEIGHBOR | ✅ 支援 | 最近鄰插值 |
| 填充 | PAD | ✅ 支援 | 填充操作 |
| 資料 | RESHAPE / TRANSPOSE | ✅ 支援 | 形狀操作 |
| 資料 | CONCATENATION | ✅ 支援 | 張量拼接 |
| 資料 | SPLIT / SPLIT_V | ✅ 支援 | 張量分割 |
| 全連接 | FULLY_CONNECTED | ✅ 完全支援 | 全連接層 |
| 量化 | QUANTIZE | ✅ 支援 | 量化操作 |
| 分類 | SOFTMAX | ✅ 支援 | Softmax 輸出 |

### 4.2 約束條件

- **量化要求**：必須為 INT8 量化模型
- **Per-axis 量化**：僅支援 CONV_2D, DEPTHWISE_CONV_2D, FULLY_CONNECTED, TRANSPOSE_CONV
- **動態張量**：不支援動態形狀張量
- **Fallback 機制**：不支援的算子自動 fallback 到 CPU (Cortex-M55)

### 4.3 結論

**MediaPipe Hand 模型使用的主要算子皆被 Ethos-U55 支援**，預期大部分計算可在 NPU 上加速執行。

---

## 5. 實作架構設計

### 5.1 目錄結構

```
EPII_CM55M_APP_S/app/scenario_app/tflm_hand_tracking/
├── cvapp_hand_tracking.cpp      # 主要推論邏輯
├── cvapp_hand_tracking.h        # 標頭檔
├── common_config.h              # 模型位址配置
├── palm_postprocessing.cpp      # Palm Detection 後處理
├── palm_postprocessing.h
├── hand_landmark.cpp            # Hand Landmark 後處理
└── hand_landmark.h

model_zoo/tflm_hand_tracking/
├── 0_palm_det_0x400000.tflite   # Palm Detection 模型
└── 1_hand_lm_0x480000.tflite    # Hand Landmark 模型
```

### 5.2 推論流程

```
┌─────────────────────────────────────────────────────────────┐
│                    Camera Frame (640×480)                    │
└─────────────────────────────┬───────────────────────────────┘
                              │
                              ▼ [下採樣 + 預處理]
┌─────────────────────────────────────────────────────────────┐
│                     Palm Detection                           │
│                     輸入: 128×128                            │
│                     輸出: BBox + Confidence                  │
└─────────────────────────────┬───────────────────────────────┘
                              │
                              │ Palm BBox
                              ▼ [裁剪 + 旋轉 + 縮放]
┌─────────────────────────────────────────────────────────────┐
│                     Hand Landmark                            │
│                     輸入: 256×256                            │
│                     輸出: 21 Keypoints (x, y, z)            │
└─────────────────────────────┬───────────────────────────────┘
                              │
                              │ Landmarks
                              ▼ [座標轉換]
┌─────────────────────────────────────────────────────────────┐
│                     輸出結果                                 │
│                     - 21 個手部關鍵點 (原始影像座標)        │
│                     - 手部邊界框                            │
│                     - 置信度分數                            │
└─────────────────────────────────────────────────────────────┘
```

### 5.3 記憶體規劃

```cpp
// common_config.h

// Tensor Arena 配置 (基於 fd_fm 經驗，保留餘量)
constexpr int tensor_arena_size = 512 * 1024;  // 512KB
constexpr int tensor_arena_model_tail_size = 1536;  // 模型間偏移

// 輸入尺寸定義
#define PALM_DET_INPUT_WIDTH   128
#define PALM_DET_INPUT_HEIGHT  128
#define HAND_LM_INPUT_WIDTH    256
#define HAND_LM_INPUT_HEIGHT   256

// 模型 Flash 位址規劃 (避開現有模型區域)
#define PALM_DETECT_FLASH_ADDR     (BASE_ADDR_FLASH1_R_ALIAS + 0x400000)  // 4MB 位置
#define HAND_LANDMARK_FLASH_ADDR   (BASE_ADDR_FLASH1_R_ALIAS + 0x480000)  // 4.5MB 位置
```

### 5.4 關鍵程式碼結構

```cpp
// cvapp_hand_tracking.cpp (骨架)

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"

namespace {
    constexpr int tensor_arena_size = 512 * 1024;
    static uint32_t tensor_arena = 0;

    // Palm Detection
    tflite::MicroInterpreter *palm_int_ptr = nullptr;
    TfLiteTensor *palm_input, *palm_output_boxes, *palm_output_scores;

    // Hand Landmark
    tflite::MicroInterpreter *hand_int_ptr = nullptr;
    TfLiteTensor *hand_input, *hand_output_landmarks;

    static tflite::MicroMutableOpResolver<2> op_resolver;
}

int cv_hand_tracking_init(bool security_enable, bool privilege_enable,
                          uint32_t palm_model_addr, uint32_t hand_model_addr) {
    // 1. 分配 tensor arena
    tensor_arena = mm_reserve_align(tensor_arena_size, 0x20);

    // 2. 初始化 NPU
    if (_arm_npu_init(security_enable, privilege_enable) != 0)
        return -1;

    // 3. 載入模型
    static const tflite::Model* palm_model = tflite::GetModel((const void*)palm_model_addr);
    static const tflite::Model* hand_model = tflite::GetModel((const void*)hand_model_addr);

    // 4. 註冊算子
    op_resolver.AddEthosU();
    op_resolver.AddPad();

    // 5. 建立 Interpreter
    static tflite::MicroInterpreter palm_interpreter(palm_model, op_resolver,
        (uint8_t*)tensor_arena, tensor_arena_size);
    static tflite::MicroInterpreter hand_interpreter(hand_model, op_resolver,
        (uint8_t*)tensor_arena, tensor_arena_size - tensor_arena_model_tail_size);

    // 6. 分配張量
    palm_interpreter.AllocateTensors();
    hand_interpreter.AllocateTensors();

    // 7. 取得輸入輸出指標
    palm_int_ptr = &palm_interpreter;
    palm_input = palm_interpreter.input(0);

    hand_int_ptr = &hand_interpreter;
    hand_input = hand_interpreter.input(0);
    hand_output_landmarks = hand_interpreter.output(0);

    return 0;
}

int cv_hand_tracking_run(struct_hand_result *result) {
    // 1. 取得相機影像
    uint32_t raw_addr = app_get_raw_addr();
    uint32_t img_w = app_get_raw_width();
    uint32_t img_h = app_get_raw_height();

    // 2. 下採樣到 Palm Detection 輸入尺寸
    hx_lib_image_resize_helium((uint8_t*)raw_addr, (uint8_t*)palm_input->data.data,
        img_w, img_h, 1, PALM_DET_INPUT_WIDTH, PALM_DET_INPUT_HEIGHT, w_scale, h_scale);

    // 3. 執行 Palm Detection
    palm_int_ptr->Invoke();

    // 4. Palm Detection 後處理 (NMS)
    palm_detection_postprocess(palm_output_boxes, palm_output_scores, &palm_bbox);

    if (palm_bbox.score < threshold) {
        result->detected = false;
        return 0;
    }

    // 5. 裁剪手掌區域並縮放到 Hand Landmark 輸入尺寸
    crop_and_resize_palm(raw_addr, palm_bbox, (uint8_t*)hand_input->data.data,
        HAND_LM_INPUT_WIDTH, HAND_LM_INPUT_HEIGHT);

    // 6. 執行 Hand Landmark
    hand_int_ptr->Invoke();

    // 7. Hand Landmark 後處理
    hand_landmark_postprocess(hand_output_landmarks, &palm_bbox, result);

    result->detected = true;
    return 0;
}
```

---

## 6. 實作步驟

### Phase 1: 模型準備與驗證

**預估時間：1-2 週**

#### 步驟 1.1: 下載 PINTO 量化模型

```bash
# Clone PINTO_model_zoo
git clone https://github.com/PINTO0309/PINTO_model_zoo.git
cd PINTO_model_zoo/033_Hand_Detection_and_Tracking

# 下載模型
./download.sh

# 確認 INT8 模型存在
ls -la *.tflite | grep integer_quant
```

#### 步驟 1.2: 安裝 Vela 編譯器

```bash
pip install ethos-u-vela

# 確認版本
vela --version
```

#### 步驟 1.3: Vela 編譯優化

```bash
# 編譯 Palm Detection
vela palm_detection_builtin_256_integer_quant.tflite \
    --accelerator-config=ethos-u55-128 \
    --config vela.ini \
    --output-dir=./vela_output/

# 編譯 Hand Landmark
vela hand_landmark_3d_256_integer_quant.tflite \
    --accelerator-config=ethos-u55-128 \
    --config vela.ini \
    --output-dir=./vela_output/
```

#### 步驟 1.4: 驗證算子支援

```bash
# 生成支援報告
vela --supported-ops-report

# 檢查是否有不支援的算子
cat SUPPORTED_OPS.md | grep -i "not supported"
```

#### 驗證點
- [ ] Vela 編譯成功，無錯誤訊息
- [ ] 無不支援的算子報告
- [ ] 輸出模型檔案存在 (`*_vela.tflite`)

---

### Phase 2: 韌體開發

**預估時間：2-3 週**

#### 步驟 2.1: 建立 Scenario App 目錄

```bash
cd EPII_CM55M_APP_S/app/scenario_app/
mkdir tflm_hand_tracking
cp -r tflm_fd_fm/* tflm_hand_tracking/  # 複製模板
```

#### 步驟 2.2: 修改 Makefile

```makefile
# EPII_CM55M_APP_S/makefile
APP_TYPE = tflm_hand_tracking
```

#### 步驟 2.3: 實作 Palm Detection 後處理

參考現有 YOLO 後處理邏輯 (`yolo_postprocessing.cpp`)：

```cpp
// palm_postprocessing.cpp
void palm_detection_postprocess(
    TfLiteTensor* boxes,
    TfLiteTensor* scores,
    struct_palm_bbox* result
) {
    // 1. 解量化輸出
    float scale = boxes->params.scale;
    int zero_point = boxes->params.zero_point;

    // 2. 解析邊界框 (x, y, w, h, keypoints...)
    for (int i = 0; i < num_anchors; i++) {
        float score = dequantize(scores->data.int8[i], ...);
        if (score < threshold) continue;

        // 解析 box
        float cx = dequantize(boxes->data.int8[i * 18 + 0], ...);
        float cy = dequantize(boxes->data.int8[i * 18 + 1], ...);
        // ...
    }

    // 3. NMS (Non-Maximum Suppression)
    nms_sort(detections, nms_threshold);

    // 4. 輸出最高分數的手掌
    *result = detections[0];
}
```

#### 步驟 2.4: 實作 Hand Landmark 後處理

```cpp
// hand_landmark.cpp
void hand_landmark_postprocess(
    TfLiteTensor* landmarks,
    struct_palm_bbox* palm,
    struct_hand_result* result
) {
    float scale = landmarks->params.scale;
    int zero_point = landmarks->params.zero_point;

    // 21 個關鍵點，每個有 x, y, z
    for (int i = 0; i < 21; i++) {
        float x = dequantize(landmarks->data.int8[i * 3 + 0], scale, zero_point);
        float y = dequantize(landmarks->data.int8[i * 3 + 1], scale, zero_point);
        float z = dequantize(landmarks->data.int8[i * 3 + 2], scale, zero_point);

        // 座標轉換：從模型輸入尺寸 -> 裁剪區域 -> 原始影像
        x = x / HAND_LM_INPUT_WIDTH * palm->width + palm->x;
        y = y / HAND_LM_INPUT_HEIGHT * palm->height + palm->y;

        result->landmarks[i].x = (int16_t)x;
        result->landmarks[i].y = (int16_t)y;
        result->landmarks[i].z = (int16_t)(z * 100);  // 縮放 z 值
    }
}
```

---

### Phase 3: 整合測試

**預估時間：1-2 週**

#### 步驟 3.1: 模型燒錄

```bash
# 複製 Vela 編譯後的模型到 model_zoo
cp vela_output/palm_detection_*_vela.tflite model_zoo/tflm_hand_tracking/0_palm_det_0x400000.tflite
cp vela_output/hand_landmark_*_vela.tflite model_zoo/tflm_hand_tracking/1_hand_lm_0x480000.tflite

# 使用 xmodem 燒錄
cd xmodem
python xmodem_send.py COM3 ../model_zoo/tflm_hand_tracking/0_palm_det_0x400000.tflite
python xmodem_send.py COM3 ../model_zoo/tflm_hand_tracking/1_hand_lm_0x480000.tflite
```

#### 步驟 3.2: 編譯韌體

```bash
cd EPII_CM55M_APP_S
make clean
make
```

#### 步驟 3.3: 燒錄韌體

```bash
cd we2_image_gen_local
cp ../EPII_CM55M_APP_S/obj_epii_evb_icv30_bdv10/gnu_epii_evb_WLCSP65/EPII_CM55M_gnu_epii_evb_WLCSP65_s.elf input_case1_secboot/
./we2_local_image_gen project_case1_blp_wlcsp.json

cd ../xmodem
python xmodem_send.py COM3 ../we2_image_gen_local/output_case1_sec_wlcsp/output.img
```

#### 步驟 3.4: GUI 整合

修改 `pythontool/src/drawing.py`，新增手部關鍵點繪製：

```python
# 手部骨架連接
HAND_SKELETON = [
    # Thumb
    (0, 1), (1, 2), (2, 3), (3, 4),
    # Index
    (0, 5), (5, 6), (6, 7), (7, 8),
    # Middle
    (0, 9), (9, 10), (10, 11), (11, 12),
    # Ring
    (0, 13), (13, 14), (14, 15), (15, 16),
    # Pinky
    (0, 17), (17, 18), (18, 19), (19, 20),
    # Palm connections
    (5, 9), (9, 13), (13, 17)
]

def draw_hand_landmarks(image, landmarks, img_w, img_h):
    """繪製手部關鍵點"""
    draw = ImageDraw.Draw(image)
    actual_w, actual_h = image.size

    scale_x = actual_w / img_w
    scale_y = actual_h / img_h

    points = []
    for lm in landmarks:
        px = int(lm['x'] * scale_x)
        py = int(lm['y'] * scale_y)
        points.append((px, py))

        # 繪製關鍵點
        r = 5
        draw.ellipse([px-r, py-r, px+r, py+r], fill="#00FF00", outline="#FFFFFF")

    # 繪製骨架連線
    for start_idx, end_idx in HAND_SKELETON:
        if start_idx < len(points) and end_idx < len(points):
            draw.line([points[start_idx], points[end_idx]], fill="#00FFFF", width=2)

    return image
```

---

## 7. 風險評估與緩解

### 7.1 技術風險矩陣

| 風險項目 | 發生機率 | 影響程度 | 風險等級 | 緩解策略 |
|----------|----------|----------|----------|----------|
| INT8 量化精度損失 | 中 | 中 | **中** | 使用 PINTO 校準後模型，實測驗證精度 |
| 記憶體不足 | 低 | 高 | **中** | 460KB arena 已證明可行，預留 512KB |
| NPU 算子不支援 | 低 | 中 | **低** | Vela 自動 fallback 到 CPU |
| 效能不達標 | 中 | 中 | **中** | 可降低輸入解析度或使用輕量模型 |
| 模型輸出格式不符 | 中 | 中 | **中** | 詳細分析 PINTO 模型輸出結構 |

### 7.2 效能預估

基於現有 fd_fm 參考數據（160+192+64 輸入，~20 FPS）：

| 配置 | 輸入尺寸 | 預估 FPS | 備註 |
|------|----------|----------|------|
| Palm(128) + Hand(256) | 128 + 256 | 12-18 | 保守估計 |
| Palm(128) + Hand(192) | 128 + 192 | 15-22 | 降低精度換速度 |
| Palm(128) + Hand(128) | 128 + 128 | 18-25 | 最快配置 |

---

## 8. 驗收標準

### 8.1 功能驗收

- [ ] Palm Detection 正確偵測手掌區域
- [ ] Hand Landmark 輸出 21 個有效關鍵點
- [ ] 關鍵點座標正確對應到原始影像
- [ ] GUI 正確繪製手部骨架

### 8.2 效能驗收

- [ ] 推論幀率 ≥ 10 FPS
- [ ] 單幀延遲 ≤ 100ms
- [ ] 記憶體使用 ≤ 600KB (tensor arena + buffers)

### 8.3 精度驗收

- [ ] Palm Detection 精度 ≥ 85% (IOU > 0.5)
- [ ] Hand Landmark 誤差 ≤ 10 pixels (在 640×480 解析度)

---

## 9. 可行性結論

### 結論：**高度可行**

#### 支持因素

1. **架構成熟**：現有 fd_fm 已證明多模型推論架構可行
2. **模型現成**：PINTO_model_zoo 提供經過驗證的 INT8 量化模型
3. **算子支援**：Ethos-U55 支援 MediaPipe Hand 使用的所有關鍵算子
4. **資源充足**：記憶體與 Flash 空間滿足需求

#### 建議優先級

1. **立即可做**：下載 PINTO 模型，用 Vela 驗證編譯
2. **關鍵驗證點**：確認 Vela 編譯成功且無不支援算子
3. **決策點**：選擇 128×128 或 256×256 Palm Detection 輸入

---

## 10. 參考資料

### 官方文件
- [Arm Ethos-U55 Product Support](https://developer.arm.com/Processors/Ethos-U55)
- [TensorFlow Lite for Microcontrollers](https://www.tensorflow.org/lite/microcontrollers)

### 模型來源
- [PINTO_model_zoo - Hand Detection and Tracking](https://github.com/PINTO0309/PINTO_model_zoo/tree/main/033_Hand_Detection_and_Tracking)
- [MediaPipe Hands](https://google.github.io/mediapipe/solutions/hands.html)

### 相關討論
- [MediaPipe INT8 Quantization Issue #3243](https://github.com/google/mediapipe/issues/3243)
- [MediaPipe 8-bit Quantization Issue #3388](https://github.com/google-ai-edge/mediapipe/issues/3388)

### Vela 編譯器
- [NXP Ethos-U Vela](https://github.com/nxp-imx/ethos-u-vela)
- [Vela SUPPORTED_OPS.md](https://github.com/nxp-imx/ethos-u-vela/blob/lf-6.12.3_1.0.0/SUPPORTED_OPS.md)

---

*文件結束*
