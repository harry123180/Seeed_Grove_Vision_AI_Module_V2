/*
 * cvapp_hand_tracking.cpp
 *
 * Hand Tracking Application for Grove Vision AI V2
 * Based on MediaPipe Hand architecture: Palm Detection + Hand Landmark
 *
 * Created: 2025/12/19
 */

#include <cstdio>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <forward_list>

#include "WE2_device.h"
#include "board.h"
#include "cvapp_hand_tracking.h"
#include "cisdp_sensor.h"
#include "sensor_dp_lib.h"
#include "WE2_core.h"

#include "ethosu_driver.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/c/common.h"

#include "img_proc_helium.h"
#include "palm_postprocessing.h"
#include "hand_landmark.h"

#include "xprintf.h"
#include "memory_manage.h"
#include "common_config.h"
#include "send_result.h"

#ifdef TRUSTZONE_SEC
#define U55_BASE    BASE_ADDR_APB_U55_CTRL_ALIAS
#else
#ifndef TRUSTZONE
#define U55_BASE    BASE_ADDR_APB_U55_CTRL_ALIAS
#else
#define U55_BASE    BASE_ADDR_APB_U55_CTRL
#endif
#endif

namespace {

/* ============================================
 * Memory Configuration
 * ============================================ */
constexpr int tensor_arena_size = TENSOR_ARENA_SIZE;
constexpr int tensor_arena_tail_size = TENSOR_ARENA_TAIL_SIZE;

// Memory buffers
static uint32_t tensor_arena = 0;
static uint32_t hand_crop_buffer = 0;  // Removed palm_crop_buffer (not used)

/* ============================================
 * NPU Driver
 * ============================================ */
struct ethosu_driver ethosu_drv;

/* ============================================
 * Palm Detection Model
 * ============================================ */
tflite::MicroInterpreter *palm_int_ptr = nullptr;
TfLiteTensor *palm_input = nullptr;
TfLiteTensor *palm_output_boxes = nullptr;
TfLiteTensor *palm_output_scores = nullptr;

/* ============================================
 * Hand Landmark Model
 * ============================================ */
tflite::MicroInterpreter *hand_int_ptr = nullptr;
TfLiteTensor *hand_input = nullptr;
TfLiteTensor *hand_output_landmarks = nullptr;

/* ============================================
 * Op Resolver
 * ============================================ */
static tflite::MicroMutableOpResolver<4> op_resolver;

/* ============================================
 * State Variables
 * ============================================ */
static uint32_t g_hand_init = 0;
static uint32_t frame_count = 0;

}  // namespace

/* ============================================
 * NPU IRQ Handler
 * ============================================ */
static void _arm_npu_irq_handler(void) {
    ethosu_irq_handler(&ethosu_drv);
}

static void _arm_npu_irq_init(void) {
    const IRQn_Type ethosu_irqnum = (IRQn_Type)U55_IRQn;
    EPII_NVIC_SetVector(ethosu_irqnum, (uint32_t)_arm_npu_irq_handler);
    NVIC_EnableIRQ(ethosu_irqnum);
}

static int _arm_npu_init(bool security_enable, bool privilege_enable) {
    int err = 0;

    _arm_npu_irq_init();

    void * const ethosu_base_address = (void *)(U55_BASE);

    if (0 != (err = ethosu_init(
                    &ethosu_drv,
                    ethosu_base_address,
                    NULL,
                    0,
                    security_enable,
                    privilege_enable))) {
        xprintf("[Hand Tracking] Failed to initialize Ethos-U device\n");
        return err;
    }

    xprintf("[Hand Tracking] Ethos-U55 initialized\n");
    return 0;
}

/* ============================================
 * Image Preprocessing
 * ============================================ */

/**
 * @brief Preprocess image for Palm Detection
 * Resize RGB planar image and convert to RGB interleaved
 * Normalize to INT8 range [-128, 127]
 */
static void preprocess_for_palm_detection(
    uint8_t* src_image,
    int8_t* dst_tensor,
    uint32_t src_w,
    uint32_t src_h,
    uint32_t src_ch
) {
    float w_scale = (float)(src_w - 1) / (PALM_DET_INPUT_WIDTH - 1);
    float h_scale = (float)(src_h - 1) / (PALM_DET_INPUT_HEIGHT - 1);

    // Resize and convert BGR planar to RGB interleaved (same as YOLO)
    // Output is uint8 RGB interleaved format
    hx_lib_image_resize_BGR8U3C_to_RGB24_helium(
        src_image,
        (uint8_t*)dst_tensor,  // Temporarily store as uint8
        src_w, src_h, src_ch,
        PALM_DET_INPUT_WIDTH,
        PALM_DET_INPUT_HEIGHT,
        w_scale, h_scale
    );

    // Convert uint8 [0-255] to int8 [-128, 127]
    int total_bytes = PALM_DET_INPUT_WIDTH * PALM_DET_INPUT_HEIGHT * 3;
    uint8_t* src_ptr = (uint8_t*)dst_tensor;
    for (int i = 0; i < total_bytes; ++i) {
        dst_tensor[i] = (int8_t)(src_ptr[i] - 128);
    }
}

/**
 * @brief Crop and resize palm region for Hand Landmark
 * Crops RGB planar image and converts to RGB interleaved
 */
static void crop_and_resize_palm(
    uint8_t* src_image,
    uint32_t src_w,
    uint32_t src_h,
    uint32_t src_ch,
    palm_bbox_t* palm,
    int8_t* dst_tensor
) {
    // Use hand_crop_buffer as temporary storage for cropped RGB planes
    uint8_t* temp_crop = (uint8_t*)hand_crop_buffer;

    // Calculate crop region with padding
    float scale_factor = 2.0f;  // Expand bbox for better landmark detection
    int cx = palm->x + palm->width / 2;
    int cy = palm->y + palm->height / 2;
    int size = (int)(fmax(palm->width, palm->height) * scale_factor);

    int x1 = cx - size / 2;
    int y1 = cy - size / 2;
    int x2 = cx + size / 2;
    int y2 = cy + size / 2;

    // Clamp to image bounds
    x1 = (x1 < 0) ? 0 : x1;
    y1 = (y1 < 0) ? 0 : y1;
    x2 = (x2 > (int)src_w) ? src_w : x2;
    y2 = (y2 > (int)src_h) ? src_h : y2;

    int crop_w = x2 - x1;
    int crop_h = y2 - y1;

    // Limit crop size to buffer capacity (256x256)
    if (crop_w > HAND_LM_INPUT_WIDTH) crop_w = HAND_LM_INPUT_WIDTH;
    if (crop_h > HAND_LM_INPUT_HEIGHT) crop_h = HAND_LM_INPUT_HEIGHT;

    if (crop_w <= 0 || crop_h <= 0) {
        // Invalid crop region, fill with zeros
        memset(dst_tensor, -128, HAND_LM_INPUT_WIDTH * HAND_LM_INPUT_HEIGHT * 3);
        return;
    }

    // RGB planar format: R plane at offset 0, G at W*H, B at 2*W*H
    uint32_t plane_size = src_w * src_h;

    // Copy cropped region for each plane to temp buffer (also in planar format)
    for (int ch = 0; ch < 3; ch++) {
        uint8_t* src_plane = src_image + ch * plane_size;
        uint8_t* dst_plane = temp_crop + ch * crop_w * crop_h;
        for (int y = 0; y < crop_h; y++) {
            memcpy(dst_plane + y * crop_w,
                   src_plane + (y1 + y) * src_w + x1,
                   crop_w);
        }
    }

    // Resize and convert to RGB interleaved
    float w_scale = (float)(crop_w - 1) / (HAND_LM_INPUT_WIDTH - 1);
    float h_scale = (float)(crop_h - 1) / (HAND_LM_INPUT_HEIGHT - 1);

    hx_lib_image_resize_BGR8U3C_to_RGB24_helium(
        temp_crop,
        (uint8_t*)dst_tensor,
        crop_w, crop_h, 3,
        HAND_LM_INPUT_WIDTH,
        HAND_LM_INPUT_HEIGHT,
        w_scale, h_scale
    );

    // Convert uint8 [0-255] to int8 [-128, 127]
    int total_bytes = HAND_LM_INPUT_WIDTH * HAND_LM_INPUT_HEIGHT * 3;
    for (int i = 0; i < total_bytes; ++i) {
        dst_tensor[i] = ((int8_t*)dst_tensor)[i] - 128;
    }
}

/* ============================================
 * Public API Implementation
 * ============================================ */

int cv_hand_tracking_init(
    bool security_enable,
    bool privilege_enable,
    uint32_t palm_model_addr,
    uint32_t hand_model_addr
) {
    int ercode = 0;

    xprintf("[Hand Tracking] Initializing...\n");

    /* ---- Memory Allocation ---- */
    tensor_arena = mm_reserve_align(tensor_arena_size, 0x20);
    // hand_crop_buffer needs to store cropped region for hand landmark
    // Max size: 256x256x3 = 196KB (same as hand landmark input)
    hand_crop_buffer = mm_reserve_align(HAND_LM_INPUT_WIDTH * HAND_LM_INPUT_HEIGHT * 3, 0x20);

    if (tensor_arena == 0 || hand_crop_buffer == 0) {
        xprintf("[Hand Tracking] Memory allocation failed\n");
        return -1;
    }

    xprintf("[Hand Tracking] Memory allocated: arena=%x, hand_crop=%x\n",
            tensor_arena, hand_crop_buffer);

    /* ---- NPU Initialization ---- */
    if (_arm_npu_init(security_enable, privilege_enable) != 0) {
        return -1;
    }

    /* ---- Load Models ---- */
    // Debug: dump first 32 bytes at flash addresses to verify model data
    xprintf("[Hand Tracking] Flash debug:\n");
    xprintf("  Palm addr: 0x%08X\n", palm_model_addr);
    xprintf("  Hand addr: 0x%08X\n", hand_model_addr);
    uint8_t* palm_bytes = (uint8_t*)palm_model_addr;
    uint8_t* hand_bytes = (uint8_t*)hand_model_addr;
    xprintf("  Palm data: ");
    for (int i = 0; i < 16; i++) xprintf("%02X ", palm_bytes[i]);
    xprintf("\n  Hand data: ");
    for (int i = 0; i < 16; i++) xprintf("%02X ", hand_bytes[i]);
    xprintf("\n");

    static const tflite::Model* palm_model = tflite::GetModel((const void*)palm_model_addr);
    static const tflite::Model* hand_model = tflite::GetModel((const void*)hand_model_addr);

    if (palm_model->version() != TFLITE_SCHEMA_VERSION) {
        xprintf("[Hand Tracking] Palm model schema version mismatch: %d vs %d\n",
                palm_model->version(), TFLITE_SCHEMA_VERSION);
        return -1;
    }

    if (hand_model->version() != TFLITE_SCHEMA_VERSION) {
        xprintf("[Hand Tracking] Hand model schema version mismatch: %d vs %d\n",
                hand_model->version(), TFLITE_SCHEMA_VERSION);
        return -1;
    }

    xprintf("[Hand Tracking] Models loaded successfully\n");

    /* ---- Register Operators ---- */
    if (g_hand_init == 0) {
        if (kTfLiteOk != op_resolver.AddEthosU()) {
            xprintf("[Hand Tracking] Failed to add Ethos-U operator\n");
            return -1;
        }
        if (kTfLiteOk != op_resolver.AddPad()) {
            xprintf("[Hand Tracking] Failed to add Pad operator\n");
            return -1;
        }
        if (kTfLiteOk != op_resolver.AddQuantize()) {
            xprintf("[Hand Tracking] Failed to add Quantize operator\n");
            return -1;
        }
        if (kTfLiteOk != op_resolver.AddDequantize()) {
            xprintf("[Hand Tracking] Failed to add Dequantize operator\n");
            return -1;
        }
    }

    /* ---- Create Interpreters ---- */
    static tflite::MicroInterpreter palm_interpreter(
        palm_model,
        op_resolver,
        (uint8_t*)tensor_arena,
        tensor_arena_size
    );

    static tflite::MicroInterpreter hand_interpreter(
        hand_model,
        op_resolver,
        (uint8_t*)tensor_arena,
        tensor_arena_size - tensor_arena_tail_size
    );

    /* ---- Allocate Tensors ---- */
    if (palm_interpreter.AllocateTensors() != kTfLiteOk) {
        xprintf("[Hand Tracking] Palm model tensor allocation failed\n");
        return -1;
    }

    if (hand_interpreter.AllocateTensors() != kTfLiteOk) {
        xprintf("[Hand Tracking] Hand model tensor allocation failed\n");
        return -1;
    }

    /* ---- Get Tensor Pointers ---- */
    palm_int_ptr = &palm_interpreter;
    palm_input = palm_interpreter.input(0);
    // Note: Output order depends on model - check dims to determine which is which
    // scores: [1, num_anchors, 1], boxes: [1, num_anchors, 18]
    TfLiteTensor* out0 = palm_interpreter.output(0);
    TfLiteTensor* out1 = palm_interpreter.output(1);
    if (out0->dims->data[2] == 1) {
        // output0 is scores, output1 is boxes
        palm_output_scores = out0;
        palm_output_boxes = out1;
    } else {
        // output0 is boxes, output1 is scores
        palm_output_boxes = out0;
        palm_output_scores = out1;
    }

    hand_int_ptr = &hand_interpreter;
    hand_input = hand_interpreter.input(0);
    hand_output_landmarks = hand_interpreter.output(0);
    // hand_output_handedness = hand_interpreter.output(1);  // If available

    xprintf("[Hand Tracking] Initialization complete\n");
    xprintf("  Palm input: %dx%dx%d\n",
            palm_input->dims->data[1],
            palm_input->dims->data[2],
            palm_input->dims->data[3]);
    xprintf("  Palm output count: %d\n", palm_interpreter.outputs_size());
    xprintf("  Palm output0 (boxes): dims=%d [", palm_output_boxes->dims->size);
    for (int i = 0; i < palm_output_boxes->dims->size; i++) {
        xprintf("%d%s", palm_output_boxes->dims->data[i],
                i < palm_output_boxes->dims->size - 1 ? "," : "");
    }
    xprintf("] scale=%.6f zp=%d\n", palm_output_boxes->params.scale, palm_output_boxes->params.zero_point);
    xprintf("  Palm output1 (scores): dims=%d [", palm_output_scores->dims->size);
    for (int i = 0; i < palm_output_scores->dims->size; i++) {
        xprintf("%d%s", palm_output_scores->dims->data[i],
                i < palm_output_scores->dims->size - 1 ? "," : "");
    }
    xprintf("] scale=%.6f zp=%d\n", palm_output_scores->params.scale, palm_output_scores->params.zero_point);
    xprintf("  Hand input: %dx%dx%d\n",
            hand_input->dims->data[1],
            hand_input->dims->data[2],
            hand_input->dims->data[3]);

    g_hand_init = 1;
    return ercode;
}

int cv_hand_tracking_run(struct_hand_algoResult *result) {
    TfLiteStatus invoke_status;
    palm_bbox_t detected_palms[MAX_HANDS_DETECTED];
    int num_palms = 0;

    frame_count++;

    /* ---- Get Camera Frame ---- */
    uint32_t raw_addr = app_get_raw_addr();
    uint32_t img_w = app_get_raw_width();
    uint32_t img_h = app_get_raw_height();
    uint32_t img_ch = app_get_raw_channels();

    #ifdef HAND_TRACKING_DEBUG
    if (frame_count % 30 == 1) {
        xprintf("[Hand Tracking] Frame %d: %dx%dx%d @ %x\n",
                frame_count, img_w, img_h, img_ch, raw_addr);
    }
    #endif

    /* ---- Stage 1: Palm Detection ---- */
    preprocess_for_palm_detection(
        (uint8_t*)raw_addr,
        palm_input->data.int8,
        img_w, img_h, img_ch
    );

    invoke_status = palm_int_ptr->Invoke();
    if (invoke_status != kTfLiteOk) {
        xprintf("[Hand Tracking] Palm detection invoke failed\n");
        return -1;
    }

    // Post-process palm detections
    num_palms = palm_detection_postprocess(
        palm_output_boxes,
        palm_output_scores,
        detected_palms,
        MAX_HANDS_DETECTED,
        img_w, img_h,
        PALM_DETECTION_THRESHOLD,
        PALM_NMS_THRESHOLD
    );

    result->num_hands = 0;

    if (num_palms == 0) {
        // No hands detected - still need to retrigger for next frame!
        sensordplib_retrigger_capture();
        return 0;
    }

    /* ---- Stage 2: Hand Landmark for each palm ---- */
    for (int i = 0; i < num_palms && i < MAX_HANDS_DETECTED; i++) {
        palm_bbox_t* palm = &detected_palms[i];

        // Crop and resize palm region
        crop_and_resize_palm(
            (uint8_t*)raw_addr,
            img_w, img_h, img_ch,
            palm,
            hand_input->data.int8
        );

        // Run hand landmark model
        invoke_status = hand_int_ptr->Invoke();
        if (invoke_status != kTfLiteOk) {
            xprintf("[Hand Tracking] Hand landmark invoke failed for hand %d\n", i);
            continue;
        }

        // Post-process landmarks
        hand_result_t* hand = &result->hands[result->num_hands];
        hand->detected = true;
        hand->palm = *palm;

        hand_landmark_postprocess(
            hand_output_landmarks,
            palm,
            img_w, img_h,
            hand->landmarks
        );

        result->num_hands++;
    }

    #ifdef HAND_TRACKING_DEBUG
    if (result->num_hands > 0 && frame_count % 10 == 1) {
        xprintf("[Hand Tracking] Detected %d hand(s)\n", result->num_hands);
        for (int i = 0; i < result->num_hands; i++) {
            xprintf("  Hand %d: wrist=(%d,%d)\n",
                    i,
                    result->hands[i].landmarks[0].x,
                    result->hands[i].landmarks[0].y);
        }
    }
    #endif

    /* ---- Send Results via UART ---- */
    // Convert to el_hand_t format for JSON serialization
    std::forward_list<el_hand_t> el_hands;
    for (int i = 0; i < result->num_hands; i++) {
        el_hand_t el_hand;
        hand_result_t* h = &result->hands[i];

        // Copy bounding box
        el_hand.el_box.x = h->palm.x;
        el_hand.el_box.y = h->palm.y;
        el_hand.el_box.w = h->palm.width;
        el_hand.el_box.h = h->palm.height;
        el_hand.el_box.score = (uint8_t)(h->palm.score * 100);
        el_hand.el_box.target = 0;

        // Copy 21 landmarks (el_point_t only has x, y, score, target)
        for (int j = 0; j < HAND_LANDMARK_NUM; j++) {
            el_hand.el_landmark[j].x = h->landmarks[j].x;
            el_hand.el_landmark[j].y = h->landmarks[j].y;
            el_hand.el_landmark[j].score = 100;  // Confidence
            el_hand.el_landmark[j].target = 0;
        }

        el_hand.handedness = h->handedness;
        el_hands.emplace_front(el_hand);
    }

    // Get JPEG info
    uint32_t jpeg_addr, jpeg_sz;
    cisdp_get_jpginfo(&jpeg_sz, &jpeg_addr);

    // Create image info
    el_img_t img_info;
    img_info.data = (uint8_t*)jpeg_addr;
    img_info.size = jpeg_sz;
    img_info.width = img_w;
    img_info.height = img_h;
    img_info.format = EL_PIXEL_FORMAT_JPEG;
    img_info.rotate = EL_PIXEL_ROTATE_0;

    // Send JSON result
    send_device_id();
    event_reply(concat_strings(", ",
        hands_results_2_json_str(el_hands), ", ",
        img_2_json_str(&img_info)));

    // Retrigger capture for next frame
    sensordplib_retrigger_capture();

    return 0;
}

void cv_hand_tracking_deinit(void) {
    xprintf("[Hand Tracking] Deinitializing...\n");
    g_hand_init = 0;
    palm_int_ptr = nullptr;
    hand_int_ptr = nullptr;
}
