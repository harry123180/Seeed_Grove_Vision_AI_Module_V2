# NPU MCU 韌體架構規範

## 概述

本文件描述一套適用於具備 NPU (神經網路處理器) 的 MCU 韌體架構，支援：
- OTA 模型更新（透過 XMODEM）
- AT 指令控制介面
- 即時影像串流與 AI 推論
- 多協議通訊（UART/SPI/I2C）

此架構源自 Seeed Grove Vision AI Module V2 (Himax WiseEye2 HX6538)，可遷移至其他 NPU MCU 平台。

---

## 目錄

1. [系統架構總覽](#1-系統架構總覽)
2. [記憶體佈局規範](#2-記憶體佈局規範)
3. [開機流程設計](#3-開機流程設計)
4. [模型載入機制](#4-模型載入機制)
5. [OTA 更新協議](#5-ota-更新協議)
6. [通訊協議規範](#6-通訊協議規範)
7. [推論引擎整合](#7-推論引擎整合)
8. [事件驅動架構](#8-事件驅動架構)
9. [遷移檢查清單](#9-遷移檢查清單)

---

## 1. 系統架構總覽

### 1.1 架構圖

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Host (PC/SBC)                               │
├─────────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                 │
│  │ Python Tool │  │ Web Toolkit │  │ XMODEM Tool │                 │
│  │ (AT 指令)    │  │ (視覺化)    │  │ (韌體更新)  │                 │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘                 │
└─────────┼────────────────┼────────────────┼─────────────────────────┘
          │ UART           │ UART           │ UART
          │ 921600 bps     │ 921600 bps     │ 115200 bps
┌─────────┴────────────────┴────────────────┴─────────────────────────┐
│                       NPU MCU Device                                │
├─────────────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    Application Layer                         │   │
│  │  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌─────────────┐  │   │
│  │  │ AT Server │ │ Inference │ │ Protocol  │ │ Data Stream │  │   │
│  │  │           │ │ Engine    │ │ Handler   │ │ Manager     │  │   │
│  │  └───────────┘ └───────────┘ └───────────┘ └─────────────┘  │   │
│  └─────────────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    HAL (Hardware Abstraction)                │   │
│  │  ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌───────────────┐  │   │
│  │  │ UART  │ │ SPI   │ │ I2C   │ │ Flash │ │ NPU Driver    │  │   │
│  │  └───────┘ └───────┘ └───────┘ └───────┘ └───────────────┘  │   │
│  └─────────────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    Bootloader                                │   │
│  │  ┌─────────────────┐  ┌─────────────────────────────────┐   │   │
│  │  │ 1st Bootloader  │  │ 2nd Bootloader (OTA Handler)    │   │   │
│  │  └─────────────────┘  └─────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

### 1.2 核心模組

| 模組 | 功能 | 必要性 |
|------|------|--------|
| Bootloader | 系統啟動、OTA 接收 | 必要 |
| AT Server | 指令解析與執行 | 必要 |
| Inference Engine | NPU 推論控制 | 必要 |
| Protocol Handler | 通訊協議處理 | 必要 |
| Data Stream Manager | 影像/資料串流 | 可選 |

---

## 2. 記憶體佈局規範

### 2.1 Flash 分區設計

```
Flash Memory Layout (建議最小 8MB)
┌────────────────────────────────────────┐ 0x00000000
│           Bootloader (1st)             │ 64-128 KB
│           - 系統初始化                  │
│           - 安全驗證                    │
├────────────────────────────────────────┤ 0x00020000
│           Bootloader (2nd)             │ 64-128 KB
│           - XMODEM 接收                 │
│           - Flash 燒錄                  │
│           - OTA 標誌檢查                │
├────────────────────────────────────────┤ 0x00040000
│           Application Firmware         │ 512 KB - 1 MB
│           - AT Server                  │
│           - 推論邏輯                    │
│           - 通訊協議                    │
├────────────────────────────────────────┤ 0x00140000
│           Configuration                │ 4-64 KB
│           - 設備 ID                    │
│           - 校準參數                    │
│           - 使用者設定                  │
├────────────────────────────────────────┤ 0x00150000
│           Model Slot 1                 │ 1-4 MB
│           - TFLite Model               │
│           - 可 XIP 執行                 │
├────────────────────────────────────────┤ 0x00550000
│           Model Slot 2 (可選)          │ 1-4 MB
│           - 備用模型                    │
│           - A/B 切換                    │
├────────────────────────────────────────┤ 0x00950000
│           Reserved / OTA Staging       │ 剩餘空間
│           - OTA 暫存區                  │
└────────────────────────────────────────┘
```

### 2.2 SRAM 分配

```
SRAM Layout (建議最小 512KB)
┌────────────────────────────────────────┐ 0x20000000
│           Stack                        │ 32-64 KB
├────────────────────────────────────────┤
│           Heap                         │ 32-64 KB
├────────────────────────────────────────┤
│           Tensor Arena                 │ 128-256 KB
│           (32-byte aligned)            │
├────────────────────────────────────────┤
│           Frame Buffer                 │ 150-300 KB
│           (影像擷取緩衝)                │
├────────────────────────────────────────┤
│           DMA Ring Buffers             │ 8-16 KB
│           (UART/SPI TX/RX)             │
├────────────────────────────────────────┤
│           Application Data             │ 剩餘空間
└────────────────────────────────────────┘
```

### 2.3 記憶體對齊要求

| 區域 | 對齊要求 | 原因 |
|------|----------|------|
| Tensor Arena | 32 bytes | NPU DMA 效率 |
| Frame Buffer | 32 bytes | 影像處理效率 |
| Flash Model | 4 KB | Flash Sector 大小 |
| DMA Buffer | 4 bytes | DMA 控制器要求 |

---

## 3. 開機流程設計

### 3.1 多階段開機

```
┌─────────────────────────────────────────────────────────────────┐
│                        Power On / Reset                         │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     1st Bootloader                              │
│  1. 硬體初始化 (時鐘、電源、記憶體控制器)                         │
│  2. 安全啟動驗證 (可選)                                          │
│  3. 檢查 OTA 標誌                                                │
└─────────────────────────────────────────────────────────────────┘
                              │
              ┌───────────────┴───────────────┐
              │                               │
         OTA Flag = 0                    OTA Flag = 1
              │                               │
              ▼                               ▼
┌─────────────────────────┐    ┌─────────────────────────────────┐
│   Jump to Application   │    │        2nd Bootloader           │
│                         │    │  1. 初始化 UART (115200 bps)     │
│                         │    │  2. 輸出 XMODEM 提示訊息         │
│                         │    │  3. 等待 XMODEM 資料             │
│                         │    │  4. 寫入 Flash                   │
│                         │    │  5. 清除 OTA 標誌                │
│                         │    │  6. 重啟系統                     │
└─────────────────────────┘    └─────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────────────┐
│                       Application                               │
│  1. 外設初始化 (UART 921600, SPI, I2C, Camera)                  │
│  2. 載入模型 (Embedded 或 XIP)                                   │
│  3. 初始化推論引擎                                               │
│  4. 啟動 AT Server                                              │
│  5. 進入主迴圈                                                   │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 OTA 標誌管理

```c
// OTA 標誌位置：Always-On (AON) 暫存器或 Flash 特定區域
// 必須在 Power Cycle 後仍可保留

typedef enum {
    OTA_FLAG_NONE     = 0x00000000,  // 正常啟動
    OTA_FLAG_FIRMWARE = 0x4F544146,  // "OTAF" - 更新韌體
    OTA_FLAG_MODEL    = 0x4F54414D,  // "OTAM" - 更新模型
    OTA_FLAG_ALL      = 0x4F544141,  // "OTAA" - 全部更新
} ota_flag_t;

// 設定 OTA 標誌
void set_ota_flag(ota_flag_t flag);

// 讀取 OTA 標誌
ota_flag_t get_ota_flag(void);

// 清除 OTA 標誌
void clear_ota_flag(void);
```

---

## 4. 模型載入機制

### 4.1 兩種載入模式

#### 模式 A：嵌入式模型 (Embedded)

```c
// 模型編譯進韌體，存放於 ROM/Flash
// 優點：簡單、啟動快
// 缺點：更新模型需重燒韌體

// model_data.h
extern const unsigned char g_model_data[];
extern const unsigned int g_model_data_len;

// model_data.cc (由 xxd 或工具生成)
const unsigned char g_model_data[] = {
    0x1c, 0x00, 0x00, 0x00, 0x54, 0x46, 0x4c, 0x33, ...
};
const unsigned int g_model_data_len = 123456;

// 載入
const tflite::Model* model = tflite::GetModel(g_model_data);
```

#### 模式 B：XIP 模式 (Execute-In-Place)

```c
// 模型存放於 Flash，透過 XIP 直接執行
// 優點：模型可獨立更新
// 缺點：需要 Flash 支援 XIP

// 定義模型在 Flash 的位置
#define MODEL_FLASH_ADDR  0x3A180000

// 啟用 Flash XIP 模式
void enable_flash_xip(void) {
    // 1. 配置 SPI Flash 為 QSPI/OSPI 模式
    flash_set_qspi_mode();

    // 2. 啟用 XIP
    flash_enable_xip(MODEL_FLASH_ADDR);

    // 3. 配置 Memory Map
    // 使 MODEL_FLASH_ADDR 可被 CPU 直接存取
}

// 載入
const tflite::Model* model = tflite::GetModel((void*)MODEL_FLASH_ADDR);
```

### 4.2 模型驗證

```c
typedef struct {
    uint32_t magic;           // "TFLM" = 0x4D4C4654
    uint32_t version;         // 模型版本
    uint32_t size;            // 模型大小
    uint32_t crc32;           // CRC32 校驗
    uint32_t input_size[4];   // 輸入維度 [N, H, W, C]
    uint32_t output_size[4];  // 輸出維度
    char     name[32];        // 模型名稱
} model_header_t;

bool validate_model(uint32_t addr) {
    model_header_t* header = (model_header_t*)addr;

    // 1. 檢查 Magic Number
    if (header->magic != 0x4D4C4654) return false;

    // 2. 驗證 CRC
    uint32_t calc_crc = crc32((uint8_t*)(addr + sizeof(model_header_t)),
                               header->size);
    if (calc_crc != header->crc32) return false;

    // 3. 驗證 TFLite 格式
    const tflite::Model* model = tflite::GetModel(
        (void*)(addr + sizeof(model_header_t)));
    if (model->version() != TFLITE_SCHEMA_VERSION) return false;

    return true;
}
```

---

## 5. OTA 更新協議

### 5.1 XMODEM 協議規範

#### 封包格式

```
Standard XMODEM (128 bytes payload):
┌──────┬─────────┬──────────┬──────────────┬─────────┐
│ SOH  │ Block # │ ~Block # │ Data         │ CRC16   │
│ 0x01 │ 1 byte  │ 1 byte   │ 128 bytes    │ 2 bytes │
└──────┴─────────┴──────────┴──────────────┴─────────┘

XMODEM-1K (1024 bytes payload):
┌──────┬─────────┬──────────┬──────────────┬─────────┐
│ STX  │ Block # │ ~Block # │ Data         │ CRC16   │
│ 0x02 │ 1 byte  │ 1 byte   │ 1024 bytes   │ 2 bytes │
└──────┴─────────┴──────────┴──────────────┴─────────┘

控制字元:
- SOH (0x01): 128-byte 封包開始
- STX (0x02): 1024-byte 封包開始
- EOT (0x04): 傳輸結束
- ACK (0x06): 確認
- NAK (0x15): 重傳請求
- CAN (0x18): 取消傳輸
- 'C' (0x43): CRC 模式請求
```

#### 模型傳輸 Preamble

```
Model Preamble Header (在模型資料前發送):
┌────────┬────────┬─────────────┬─────────────┬────────┬────────┬─────────┐
│ Magic1 │ Magic2 │ Flash Addr  │ Offset      │ Magic3 │ Magic4 │ Padding │
│ 0xC0   │ 0x5A   │ 4 bytes LE  │ 4 bytes LE  │ 0x5A   │ 0xC0   │ 0xFF... │
└────────┴────────┴─────────────┴─────────────┴────────┴────────┴─────────┘
Total: 128 or 1024 bytes (取決於 XMODEM 模式)

欄位說明:
- Flash Addr: 模型要寫入的 Flash 絕對位址 (Little Endian)
- Offset: 模型在記憶體中的偏移量 (Little Endian)
```

### 5.2 OTA 流程

```
Host                                    Device
  │                                        │
  │  1. 開啟串口 (115200 bps)               │
  │────────────────────────────────────────►│
  │                                        │
  │  2. 等待 Reset                          │
  │         "Please press reset button"    │
  │◄────────────────────────────────────────│
  │                                        │
  │         [User presses RESET]           │
  │                                        │
  │  3. Bootloader 輸出提示                  │
  │    "Send data using xmodem protocol"   │
  │◄────────────────────────────────────────│
  │                                        │
  │  4. 發送 '1' 開始傳輸                    │
  │────────────────────────────────────────►│
  │                                        │
  │  5. XMODEM 傳輸韌體 (可選)               │
  │══════════════════════════════════════►│
  │         [ACK/NAK handshake]            │
  │                                        │
  │  6. "Reboot? (y/n)"                    │
  │◄────────────────────────────────────────│
  │                                        │
  │  7. 發送 'n' 繼續傳輸模型                │
  │────────────────────────────────────────►│
  │                                        │
  │  8. XMODEM 傳輸 Model Preamble          │
  │══════════════════════════════════════►│
  │                                        │
  │  9. "Reboot? (y/n)"                    │
  │◄────────────────────────────────────────│
  │                                        │
  │  10. 發送 'n' 繼續                       │
  │────────────────────────────────────────►│
  │                                        │
  │  11. XMODEM 傳輸 Model Data             │
  │══════════════════════════════════════►│
  │                                        │
  │  12. "Reboot? (y/n)"                   │
  │◄────────────────────────────────────────│
  │                                        │
  │  13. 發送 'y' 重啟                       │
  │────────────────────────────────────────►│
  │                                        │
  │         [Device Reboots]               │
  │                                        │
```

### 5.3 2nd Bootloader 實作要點

```c
void bootloader_2nd_main(void) {
    // 1. 初始化 UART
    uart_init(115200);

    // 2. 輸出提示
    uart_print("Send data using the xmodem protocol from your terminal\r\n");

    // 3. 等待開始訊號
    while (uart_getc() != '1');

    // 4. 初始化 XMODEM
    xmodem_init();

    // 5. 接收迴圈
    while (1) {
        xmodem_packet_t pkt;
        int ret = xmodem_receive_packet(&pkt);

        if (ret == XMODEM_EOT) {
            // 傳輸結束
            uart_print("Do you want to end file transmission and reboot? (y)\r\n");
            char c = uart_getc();
            if (c == 'y') {
                clear_ota_flag();
                system_reset();
            }
            // 'n' 則繼續等待下一個檔案
            continue;
        }

        if (ret == XMODEM_OK) {
            // 檢查是否為 Preamble
            if (is_model_preamble(pkt.data)) {
                parse_preamble(pkt.data, &current_flash_addr, &current_offset);
            } else {
                // 寫入 Flash
                flash_write(current_flash_addr, pkt.data, pkt.size);
                current_flash_addr += pkt.size;
            }
            xmodem_send_ack();
        } else {
            xmodem_send_nak();
        }
    }
}
```

---

## 6. 通訊協議規範

### 6.1 UART 協議 (AT 指令)

#### 串口設定

| 參數 | 值 |
|------|-----|
| Baudrate | 921600 bps (應用層) / 115200 bps (Bootloader) |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Flow control | None |

#### AT 指令格式

```
請求格式:
AT+<COMMAND>[=<param1>,<param2>,...]\r\n

回應格式 (JSON):
\r{"type":<type>,"name":"<name>","code":<code>,"data":{...}}\n

欄位說明:
- type: 0=查詢回應, 1=推論結果, 2=錯誤
- name: 指令名稱
- code: 0=成功, 非0=錯誤碼
- data: 回應資料 (JSON 物件)
```

#### 標準 AT 指令集

| 指令 | 參數 | 功能 | 回應範例 |
|------|------|------|----------|
| `AT` | 無 | 測試連接 | `{"type":0,"name":"","code":0}` |
| `AT+INFO?` | 無 | 查詢設備資訊 | `{"type":0,"name":"INFO","code":0,"data":{"fw_ver":"1.0.0","model":"yolov8n"}}` |
| `AT+ID?` | 無 | 查詢設備 ID | `{"type":0,"name":"ID","code":0,"data":{"id":"ABCD1234"}}` |
| `AT+ID=` | ID | 設定設備 ID | `{"type":0,"name":"ID","code":0}` |
| `AT+INVOKE` | en,period,flags | 觸發推論 | `{"type":1,"name":"INVOKE","code":0,"data":{...}}` |
| `AT+RESET` | 無 | 重啟系統 | 無回應 (直接重啟) |
| `AT+OTA` | 無 | 進入 OTA 模式 | `{"type":0,"name":"OTA","code":0}` 後重啟 |

#### 推論結果格式

```json
{
    "type": 1,
    "name": "INVOKE",
    "code": 0,
    "data": {
        "count": 3,
        "perf": [12, 45, 8],
        "resolution": [640, 480],
        "image": "<BASE64_JPEG>",
        "rotate": 0,
        "boxes": [
            [120, 80, 200, 300, 85, 0],
            [400, 100, 150, 250, 72, 0]
        ],
        "keypoints": []
    }
}
```

#### 資料欄位說明

| 欄位 | 類型 | 說明 |
|------|------|------|
| `count` | int | 偵測數量 |
| `perf` | array | 效能計數 [preprocess, inference, postprocess] (ms) |
| `resolution` | array | 原始影像解析度 [width, height] |
| `image` | string | Base64 編碼的 JPEG 影像 |
| `rotate` | int | 影像旋轉角度 (0, 90, 180, 270) |
| `boxes` | array | 偵測框 [[x, y, w, h, score, class], ...] |
| `keypoints` | array | 關鍵點 (pose 模型) |

### 6.2 SPI 協議 (高速資料傳輸)

#### 封包格式

```
SPI Protocol Frame:
┌─────────────────────────┬──────────────────┬─────────────────┐
│ Header (7 bytes)        │ Extra (≤17 bytes)│ Payload         │
├─────┬──────┬────────────┤                  │ (≤976 KB)       │
│ ID  │ Type │ Size       │                  │                 │
│ 2B  │ 1B   │ 4B         │                  │                 │
└─────┴──────┴────────────┴──────────────────┴─────────────────┘
```

#### 資料類型定義

```c
typedef enum {
    // 影像類型
    DATA_TYPE_JPG            = 0x01,  // JPEG 影像
    DATA_TYPE_BMP            = 0x02,  // BMP 影像
    DATA_TYPE_RAW_YUV422     = 0x72,  // YUV422 原始影像
    DATA_TYPE_RAW_RGB        = 0x71,  // RGB 原始影像

    // 推論結果類型
    DATA_TYPE_META_YOLOV8_OD = 0x9A,  // YOLOv8 物件偵測
    DATA_TYPE_META_YOLOV8_POSE = 0x9C,  // YOLOv8 姿態偵測
    DATA_TYPE_META_FACE_MESH = 0x95,  // 人臉網格

    // 控制類型
    DATA_TYPE_END_OF_PACKET  = 0xF0,  // 封包結束標記
} data_type_t;
```

### 6.3 模式切換

```c
// 透過發送單一 byte 切換傳輸模式
typedef enum {
    MODE_UART     = 0xFF,  // 僅 UART 輸出
    MODE_SPI      = 0xFE,  // 僅 SPI 輸出
    MODE_UART_SPI = 0xFD,  // UART + SPI 同時輸出
} output_mode_t;
```

---

## 7. 推論引擎整合

### 7.1 TensorFlow Lite Micro 整合

```c
// 必要元件
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

// Tensor Arena 配置
#define TENSOR_ARENA_SIZE  (128 * 1024)  // 依模型調整
__attribute__((aligned(32)))
static uint8_t tensor_arena[TENSOR_ARENA_SIZE];

// 初始化函數
bool inference_init(const uint8_t* model_data) {
    // 1. 載入模型
    model = tflite::GetModel(model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        return false;
    }

    // 2. 設定 Op Resolver (依模型需求)
    static tflite::MicroMutableOpResolver<10> resolver;
    resolver.AddConv2D();
    resolver.AddDepthwiseConv2D();
    resolver.AddMaxPool2D();
    resolver.AddReshape();
    resolver.AddSoftmax();
    // ... 其他需要的 Op

    // 3. 建立 Interpreter
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, TENSOR_ARENA_SIZE);
    interpreter = &static_interpreter;

    // 4. 分配 Tensors
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        return false;
    }

    // 5. 取得輸入/輸出 Tensor
    input_tensor = interpreter->input(0);
    output_tensor = interpreter->output(0);

    return true;
}

// 推論函數
bool inference_run(uint8_t* input_data, float* output_data) {
    // 1. 複製輸入資料
    memcpy(input_tensor->data.uint8, input_data, input_tensor->bytes);

    // 2. 執行推論
    if (interpreter->Invoke() != kTfLiteOk) {
        return false;
    }

    // 3. 取得輸出
    memcpy(output_data, output_tensor->data.f, output_tensor->bytes);

    return true;
}
```

### 7.2 NPU 驅動整合

```c
// NPU 初始化 (以 Arm Ethos-U 為例)
#include "ethosu_driver.h"

static struct ethosu_driver ethosu_drv;

bool npu_init(void) {
    // 1. 初始化 NPU 驅動
    if (ethosu_init(&ethosu_drv,
                    NPU_BASE_ADDR,      // NPU 暫存器基底位址
                    NULL,               // Fast memory (可選)
                    0,                  // Fast memory size
                    true,               // Security enable
                    true)) {            // Privilege enable
        return false;
    }

    // 2. 註冊 IRQ Handler
    NVIC_SetVector(NPU_IRQn, (uint32_t)npu_irq_handler);
    NVIC_EnableIRQ(NPU_IRQn);

    return true;
}

// NPU IRQ Handler
void npu_irq_handler(void) {
    ethosu_irq_handler(&ethosu_drv);
}
```

### 7.3 影像前處理

```c
// 影像縮放 (雙線性插值)
void img_resize(
    const uint8_t* src, int src_w, int src_h,
    uint8_t* dst, int dst_w, int dst_h,
    int channels
) {
    float x_ratio = (float)src_w / dst_w;
    float y_ratio = (float)src_h / dst_h;

    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            float src_x = x * x_ratio;
            float src_y = y * y_ratio;

            int x0 = (int)src_x;
            int y0 = (int)src_y;
            int x1 = MIN(x0 + 1, src_w - 1);
            int y1 = MIN(y0 + 1, src_h - 1);

            float x_diff = src_x - x0;
            float y_diff = src_y - y0;

            for (int c = 0; c < channels; c++) {
                float val =
                    src[(y0 * src_w + x0) * channels + c] * (1 - x_diff) * (1 - y_diff) +
                    src[(y0 * src_w + x1) * channels + c] * x_diff * (1 - y_diff) +
                    src[(y1 * src_w + x0) * channels + c] * (1 - x_diff) * y_diff +
                    src[(y1 * src_w + x1) * channels + c] * x_diff * y_diff;

                dst[(y * dst_w + x) * channels + c] = (uint8_t)val;
            }
        }
    }
}

// 正規化 (uint8 -> int8/float)
void img_normalize(
    const uint8_t* src,
    int8_t* dst,
    int size,
    float scale,
    int zero_point
) {
    for (int i = 0; i < size; i++) {
        dst[i] = (int8_t)((src[i] - zero_point) * scale);
    }
}
```

---

## 8. 事件驅動架構

### 8.1 事件系統設計

```c
// 事件類型定義
typedef enum {
    EVT_UART_RX,           // UART 接收
    EVT_UART_TX_DONE,      // UART 傳送完成
    EVT_SPI_RX,            // SPI 接收
    EVT_SPI_TX_DONE,       // SPI 傳送完成
    EVT_FRAME_READY,       // 影像擷取完成
    EVT_INFERENCE_DONE,    // 推論完成
    EVT_TIMER,             // 定時器
    EVT_MAX
} event_type_t;

// 事件回調類型
typedef void (*event_handler_t)(void* param);

// 事件註冊
void event_register(event_type_t type, event_handler_t handler);

// 事件觸發 (可從 ISR 呼叫)
void event_trigger(event_type_t type, void* param);

// 事件處理迴圈
void event_loop(void);
```

### 8.2 主迴圈架構

```c
int main(void) {
    // 1. 硬體初始化
    system_init();
    uart_init(921600);
    spi_init();
    camera_init();

    // 2. 載入模型
    if (!inference_init(MODEL_FLASH_ADDR)) {
        error_handler();
    }

    // 3. 註冊事件處理器
    event_register(EVT_UART_RX, uart_rx_handler);
    event_register(EVT_FRAME_READY, frame_ready_handler);
    event_register(EVT_INFERENCE_DONE, inference_done_handler);

    // 4. 啟動擷取
    camera_start_capture();

    // 5. 主迴圈
    while (1) {
        event_loop();

        // 低功耗等待 (可選)
        __WFE();
    }
}

// 影像就緒處理
void frame_ready_handler(void* param) {
    frame_buffer_t* frame = (frame_buffer_t*)param;

    // 1. 前處理
    img_resize(frame->data, frame->width, frame->height,
               input_buffer, MODEL_INPUT_W, MODEL_INPUT_H, 3);

    // 2. 啟動推論
    inference_run_async(input_buffer, inference_done_callback);
}

// 推論完成處理
void inference_done_handler(void* param) {
    inference_result_t* result = (inference_result_t*)param;

    // 1. 後處理 (NMS, 格式化)
    postprocess(result);

    // 2. 發送結果
    send_result_json(result);

    // 3. 觸發下一次擷取
    camera_start_capture();
}
```

---

## 9. 遷移檢查清單

### 9.1 硬體需求

| 項目 | 最低要求 | 建議規格 |
|------|----------|----------|
| Flash | 4 MB | 8-16 MB |
| SRAM | 512 KB | 1-2 MB |
| NPU | 支援 INT8 | 支援 INT8 + FP16 |
| UART | 1 組 (921600 bps) | 2 組 |
| SPI | 1 組 (Master) | QSPI/OSPI |
| Camera | MIPI CSI / DVP | MIPI CSI-2 |

### 9.2 軟體移植項目

| 項目 | 說明 | 優先級 |
|------|------|--------|
| **Bootloader** | 實作 2-stage boot + OTA flag | 高 |
| **Flash Driver** | 支援 XIP、Sector Erase/Write | 高 |
| **UART Driver** | Ring Buffer、DMA 支援 | 高 |
| **XMODEM** | CRC16 模式、1K 封包 | 高 |
| **NPU Driver** | 整合 TFLite Micro | 高 |
| **AT Parser** | JSON 格式輸出 | 中 |
| **SPI Protocol** | 高速資料傳輸 | 中 |
| **Camera Driver** | Frame Buffer 管理 | 中 |
| **Power Management** | 低功耗模式 | 低 |

### 9.3 測試項目

- [ ] Bootloader 正常啟動
- [ ] OTA Flag 設定/清除/保留
- [ ] XMODEM 韌體傳輸
- [ ] XMODEM 模型傳輸 (含 Preamble)
- [ ] 模型 XIP 載入
- [ ] AT 指令回應
- [ ] 推論結果正確
- [ ] 影像串流穩定
- [ ] 長時間運行穩定性

### 9.4 常見問題

| 問題 | 可能原因 | 解決方案 |
|------|----------|----------|
| 模型載入失敗 | Flash 位址未對齊 | 確保 4KB 對齊 |
| 推論結果錯誤 | Tensor Arena 不足 | 增加 Arena 大小 |
| XMODEM 傳輸失敗 | 超時設定太短 | 增加 timeout |
| XIP 讀取錯誤 | Flash 模式錯誤 | 檢查 QSPI 設定 |
| 串流卡頓 | Buffer 不足 | 增加 Ring Buffer |

---

## 附錄

### A. 參考實作

- [Grove Vision AI V2 Firmware](https://github.com/Seeed-Studio/Seeed_Grove_Vision_AI_Module_V2)
- [TensorFlow Lite Micro](https://github.com/tensorflow/tflite-micro)
- [XMODEM Protocol](https://en.wikipedia.org/wiki/XMODEM)

### B. 相關工具

- `xmodem_send.py` - XMODEM 傳輸工具
- `grove_vision_ai_tool.py` - Python 上位機工具
- Model Converter - TFLite 模型轉換

### C. 版本歷史

| 版本 | 日期 | 變更 |
|------|------|------|
| 1.0 | 2024-01 | 初版 |

---

*本文件基於 Seeed Grove Vision AI Module V2 韌體架構整理，適用於具備 NPU 的 MCU 平台移植參考。*
