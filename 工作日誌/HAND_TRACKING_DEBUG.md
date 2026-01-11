# Hand Tracking 除錯記錄

## 目前狀態：PC 模型偵測位置錯誤

### 問題描述
- 手在畫面**中間偏右上方**
- PC 模型偵測框在**左下角**（完全錯誤）
- 試過多種 anchor 配置都不對

### 已嘗試的修正

1. **Anchor 數量調整**
   - 原本：2944 anchors (256x256 輸入)
   - 嘗試：896 anchors (128x128 輸入) → 模型報錯，輸入確實是 256x256
   - 結論：模型是 256x256 輸入、2944 anchors

2. **Anchor 順序調整**
   - y-first (原本)：框在左下角
   - x-first：框跑到右上角
   - 交換 cx/cy：還是右上角
   - 交換 dx/dy：待測試

3. **Anchor 配置**
   ```
   stride=8  -> 32x32 grid x 2 = 2048 anchors
   stride=16 -> 16x16 grid x 2 = 512 anchors
   stride=32 -> 8x8 grid x 6 = 384 anchors
   Total = 2944 anchors
   ```

### 關鍵發現

從 `pc_detection_result.png` 觀察：
- 偵測到的 anchor 位置 `[29%, 79%]` 對應**左下角**
- 但手實際在**中間偏右**
- 這表示 **anchor 索引到座標的映射有問題**

### 正在進行

使用 `realtime_compare.py` 工具即時比較：
- PC 模型結果 (綠框)
- 板子韌體結果 (紅框)
- Log 記錄到 `logs/compare_*.log`

```bash
cd pythontool
python realtime_compare.py --port COM3
```

### 待分析

1. **anchor 索引與座標的對應關係**
   - 模型輸出的 anchor index 是如何排序的？
   - 是 row-major 還是 column-major？
   - 是否需要轉置？

2. **dx/dy 的解碼方式**
   - 標準方式：`cx = anchor_x + dx / 256`
   - 是否需要交換 dx/dy？
   - 是否需要其他 scale factor？

### 檔案位置

| 檔案 | 用途 |
|------|------|
| `EPII_CM55M_APP_S/.../palm_postprocessing.cpp` | 韌體後處理 |
| `pythontool/test_palm_model.py` | PC 單張測試 |
| `pythontool/realtime_compare.py` | 即時比較工具 |
| `pythontool/capture_frame.py` | 從板子擷取照片 |
| `pythontool/logs/compare_*.log` | 比較結果記錄 |

### 模型資訊

- 來源：PINTO_model_zoo (https://github.com/PINTO0309/PINTO_model_zoo)
- 路徑：`model_zoo/tflm_hand_tracking/0_palm_det_0x400000.tflite`
- 輸入：256x256x3 INT8
- 輸出：
  - boxes: [1, 2944, 18] (dx, dy, dw, dh + 7 keypoints)
  - scores: [1, 2944, 1]

### 下一步

1. 執行 `realtime_compare.py` 收集 log
2. 分析 PC vs Board 的 bbox 差異
3. 找出正確的 anchor 映射方式
4. 同步修正韌體和 PC 測試腳本
