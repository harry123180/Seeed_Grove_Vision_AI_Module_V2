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

#include "WE2_device.h"
#include "board.h"
#include "cvapp_hand_tracking.h"
#include "cisdp_sensor.h"
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
static uint32_t palm_crop_buffer = 0;
static uint32_t hand_crop_buffer = 0;

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
TfLiteTensor *hand_output_handedness = nullptr;

/* ============================================
 * Op Resolver
 * ============================================ */
static tflite::MicroMutableOpResolver<2> op_resolver;

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
 * Resize and normalize to INT8 range
 */
static void preprocess_for_palm_detection(
    uint8_t* src_image,
    int8_t* dst_tensor,
    uint32_t src_w,
    uint32_t src_h
) {
    float w_scale = (float)(src_w - 1) / (PALM_DET_INPUT_WIDTH - 1);
    float h_scale = (float)(src_h - 1) / (PALM_DET_INPUT_HEIGHT - 1);

    // Resize using Helium acceleration
    hx_lib_image_resize_helium(
        src_image,
        (uint8_t*)dst_tensor,
        src_w, src_h, 1,  // Grayscale
        PALM_DET_INPUT_WIDTH,
        PALM_DET_INPUT_HEIGHT,
        w_scale, h_scale
    );

    // Normalize: [0, 255] -> [-128, 127]
    int total_pixels = PALM_DET_INPUT_WIDTH * PALM_DET_INPUT_HEIGHT;
    for (int i = 0; i < total_pixels; ++i) {
        dst_tensor[i] = dst_tensor[i] - 128;
    }
}

/**
 * @brief Crop and resize palm region for Hand Landmark
 */
static void crop_and_resize_palm(
    uint8_t* src_image,
    uint32_t src_w,
    uint32_t src_h,
    palm_bbox_t* palm,
    int8_t* dst_tensor
) {
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

    // Resize cropped region to Hand Landmark input size
    float w_scale = (float)(crop_w - 1) / (HAND_LM_INPUT_WIDTH - 1);
    float h_scale = (float)(crop_h - 1) / (HAND_LM_INPUT_HEIGHT - 1);

    hx_lib_image_resize_helium(
        src_image + y1 * src_w + x1,
        (uint8_t*)dst_tensor,
        crop_w, crop_h, 1,
        HAND_LM_INPUT_WIDTH,
        HAND_LM_INPUT_HEIGHT,
        w_scale, h_scale
    );

    // Normalize
    int total_pixels = HAND_LM_INPUT_WIDTH * HAND_LM_INPUT_HEIGHT;
    for (int i = 0; i < total_pixels; ++i) {
        dst_tensor[i] = dst_tensor[i] - 128;
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
    palm_crop_buffer = mm_reserve_align(PALM_DET_INPUT_WIDTH * PALM_DET_INPUT_HEIGHT, 0x20);
    hand_crop_buffer = mm_reserve_align(HAND_LM_INPUT_WIDTH * HAND_LM_INPUT_HEIGHT, 0x20);

    if (tensor_arena == 0 || palm_crop_buffer == 0 || hand_crop_buffer == 0) {
        xprintf("[Hand Tracking] Memory allocation failed\n");
        return -1;
    }

    xprintf("[Hand Tracking] Memory allocated: arena=%x, palm=%x, hand=%x\n",
            tensor_arena, palm_crop_buffer, hand_crop_buffer);

    /* ---- NPU Initialization ---- */
    if (_arm_npu_init(security_enable, privilege_enable) != 0) {
        return -1;
    }

    /* ---- Load Models ---- */
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
    palm_output_boxes = palm_interpreter.output(0);
    palm_output_scores = palm_interpreter.output(1);

    hand_int_ptr = &hand_interpreter;
    hand_input = hand_interpreter.input(0);
    hand_output_landmarks = hand_interpreter.output(0);
    // hand_output_handedness = hand_interpreter.output(1);  // If available

    xprintf("[Hand Tracking] Initialization complete\n");
    xprintf("  Palm input: %dx%dx%d\n",
            palm_input->dims->data[1],
            palm_input->dims->data[2],
            palm_input->dims->data[3]);
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

    #ifdef HAND_TRACKING_DEBUG
    if (frame_count % 30 == 1) {
        xprintf("[Hand Tracking] Frame %d: %dx%d @ %x\n",
                frame_count, img_w, img_h, raw_addr);
    }
    #endif

    /* ---- Stage 1: Palm Detection ---- */
    preprocess_for_palm_detection(
        (uint8_t*)raw_addr,
        palm_input->data.int8,
        img_w, img_h
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
        // No hands detected
        return 0;
    }

    /* ---- Stage 2: Hand Landmark for each palm ---- */
    for (int i = 0; i < num_palms && i < MAX_HANDS_DETECTED; i++) {
        palm_bbox_t* palm = &detected_palms[i];

        // Crop and resize palm region
        crop_and_resize_palm(
            (uint8_t*)raw_addr,
            img_w, img_h,
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

    return 0;
}

void cv_hand_tracking_deinit(void) {
    xprintf("[Hand Tracking] Deinitializing...\n");
    g_hand_init = 0;
    palm_int_ptr = nullptr;
    hand_int_ptr = nullptr;
}
