/*
 * palm_postprocessing.cpp
 * Palm Detection Post-processing Implementation
 *
 * MediaPipe Palm Detection outputs:
 * - boxes: [1, num_anchors, 18] - (cx, cy, w, h, 7 keypoints x 2)
 * - scores: [1, num_anchors, 1] - confidence scores
 */

#include "palm_postprocessing.h"
#include "common_config.h"
#include <cmath>
#include <algorithm>
#include "xprintf.h"

/* ============================================
 * Anchor Configuration for Palm Detection
 * PINTO palm_detection model: 256x256 input, 2944 anchors
 * ============================================ */

// Anchor structure
struct Anchor {
    float x_center;
    float y_center;
};

// Anchor configuration for 256x256 input (PINTO model)
// Total anchors: 32*32*2 + 16*16*2 + 8*8*6 = 2048 + 512 + 384 = 2944
#define NUM_ANCHORS_TOTAL   2944
#define INPUT_SIZE          256.0f

// Layer configuration for 256x256 input with 2944 anchors
struct AnchorLayerConfig {
    int stride;
    int num_anchors;
};

static const AnchorLayerConfig anchor_layers[] = {
    {8, 2},    // 32x32 grid (256/8=32), 2 anchors per cell = 2048
    {16, 2},   // 16x16 grid (256/16=16), 2 anchors per cell = 512
    {32, 6},   // 8x8 grid (256/32=8), 6 anchors per cell = 384
};
#define NUM_LAYERS 3

// Pre-computed anchors (generated once at startup)
static Anchor anchors[NUM_ANCHORS_TOTAL];
static bool anchors_initialized = false;

/* ============================================
 * Anchor Generation
 * ============================================ */
static void generate_anchors() {
    if (anchors_initialized) return;

    int anchor_idx = 0;

    for (int layer = 0; layer < NUM_LAYERS; layer++) {
        int stride = anchor_layers[layer].stride;
        int num_anchors_per_cell = anchor_layers[layer].num_anchors;
        int grid_size = (int)(INPUT_SIZE / stride);

        for (int y = 0; y < grid_size; y++) {
            for (int x = 0; x < grid_size; x++) {
                // Anchor center is at the center of each grid cell
                float x_center = (x + 0.5f) / grid_size;
                float y_center = (y + 0.5f) / grid_size;

                // Add multiple anchors at same location
                for (int a = 0; a < num_anchors_per_cell; a++) {
                    if (anchor_idx < NUM_ANCHORS_TOTAL) {
                        anchors[anchor_idx].x_center = x_center;
                        anchors[anchor_idx].y_center = y_center;
                        anchor_idx++;
                    }
                }
            }
        }
    }

    anchors_initialized = true;
    xprintf("[Palm] Generated %d anchors\n", anchor_idx);
}

/* ============================================
 * Dequantization Helper
 * ============================================ */
static inline float dequantize(int8_t value, float scale, int zero_point) {
    return (float)(value - zero_point) * scale;
}

/* ============================================
 * Sigmoid Activation
 * ============================================ */
static inline float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

/* ============================================
 * IoU Calculation
 * ============================================ */
float calculate_iou(palm_bbox_t* a, palm_bbox_t* b) {
    int x1 = std::max(a->x, b->x);
    int y1 = std::max(a->y, b->y);
    int x2 = std::min(a->x + a->width, b->x + b->width);
    int y2 = std::min(a->y + a->height, b->y + b->height);

    if (x2 <= x1 || y2 <= y1) {
        return 0.0f;
    }

    float intersection = (float)(x2 - x1) * (y2 - y1);
    float area_a = (float)a->width * a->height;
    float area_b = (float)b->width * b->height;
    float union_area = area_a + area_b - intersection;

    return intersection / union_area;
}

/* ============================================
 * Non-Maximum Suppression
 * ============================================ */
int apply_nms(
    palm_bbox_t* detections,
    int num_detections,
    float nms_threshold
) {
    if (num_detections <= 1) {
        return num_detections;
    }

    // Sort by score (descending) - simple bubble sort for small arrays
    for (int i = 0; i < num_detections - 1; i++) {
        for (int j = 0; j < num_detections - i - 1; j++) {
            if (detections[j].score < detections[j + 1].score) {
                palm_bbox_t temp = detections[j];
                detections[j] = detections[j + 1];
                detections[j + 1] = temp;
            }
        }
    }

    // Mark suppressed detections
    bool suppressed[32] = {false};
    int count = 0;

    for (int i = 0; i < num_detections; i++) {
        if (suppressed[i]) continue;

        count++;

        for (int j = i + 1; j < num_detections; j++) {
            if (suppressed[j]) continue;

            float iou = calculate_iou(&detections[i], &detections[j]);
            if (iou > nms_threshold) {
                suppressed[j] = true;
            }
        }
    }

    // Compact array
    int write_idx = 0;
    for (int i = 0; i < num_detections; i++) {
        if (!suppressed[i]) {
            if (write_idx != i) {
                detections[write_idx] = detections[i];
            }
            write_idx++;
        }
    }

    return count;
}

/* ============================================
 * Main Post-processing Function
 * ============================================ */
int palm_detection_postprocess(
    TfLiteTensor* boxes_tensor,
    TfLiteTensor* scores_tensor,
    palm_bbox_t* detected_palms,
    int max_palms,
    uint32_t img_w,
    uint32_t img_h,
    float score_threshold,
    float nms_threshold
) {
    if (boxes_tensor == nullptr || scores_tensor == nullptr) {
        return 0;
    }

    // Generate anchors on first call
    generate_anchors();

    // Check tensor types - vela models output FLOAT after DEQUANTIZE ops
    bool boxes_is_float = (boxes_tensor->type == kTfLiteFloat32);
    bool scores_is_float = (scores_tensor->type == kTfLiteFloat32);

    // Get quantization parameters (only valid for INT8)
    float boxes_scale = boxes_is_float ? 1.0f : boxes_tensor->params.scale;
    int boxes_zp = boxes_is_float ? 0 : boxes_tensor->params.zero_point;
    float scores_scale = scores_is_float ? 1.0f : scores_tensor->params.scale;
    int scores_zp = scores_is_float ? 0 : scores_tensor->params.zero_point;

    // Get data pointers based on type
    float* boxes_float = boxes_is_float ? boxes_tensor->data.f : nullptr;
    int8_t* boxes_int8 = boxes_is_float ? nullptr : boxes_tensor->data.int8;
    float* scores_float = scores_is_float ? scores_tensor->data.f : nullptr;
    int8_t* scores_int8 = scores_is_float ? nullptr : scores_tensor->data.int8;

    // Get tensor dimensions
    int num_anchors = boxes_tensor->dims->data[1];
    int box_size = boxes_tensor->dims->data[2];  // Usually 18 (4 + 7*2)

    #ifdef PALM_DETECTION_DEBUG
    static int debug_frame_count = 0;
    debug_frame_count++;

    // Print raw tensor data for comparison with PC
    xprintf("\n[RAW_TENSOR] Frame %d\n", debug_frame_count);
    xprintf("[RAW_TENSOR] boxes_type=%s scores_type=%s\n",
            boxes_is_float ? "F32" : "I8",
            scores_is_float ? "F32" : "I8");
    xprintf("[RAW_TENSOR] num_anchors=%d box_size=%d\n", num_anchors, box_size);

    // Find and print max score directly (simpler approach)
    float max_score = -999.0f;
    int max_idx = -1;
    for (int i = 0; i < num_anchors; i++) {
        float s = scores_is_float ? scores_float[i] : dequantize(scores_int8[i], scores_scale, scores_zp);
        float sig_s = sigmoid(s);
        if (sig_s > max_score) {
            max_score = sig_s;
            max_idx = i;
        }
    }

    xprintf("[RAW_TENSOR] max_score=%d%% at idx=%d\n", (int)(max_score * 100), max_idx);

    // Print raw values for max score anchor
    if (max_idx >= 0 && max_idx < num_anchors) {
        int box_off = max_idx * box_size;
        float dx = boxes_is_float ? boxes_float[box_off + 0] : dequantize(boxes_int8[box_off + 0], boxes_scale, boxes_zp);
        float dy = boxes_is_float ? boxes_float[box_off + 1] : dequantize(boxes_int8[box_off + 1], boxes_scale, boxes_zp);
        float dw = boxes_is_float ? boxes_float[box_off + 2] : dequantize(boxes_int8[box_off + 2], boxes_scale, boxes_zp);
        float dh = boxes_is_float ? boxes_float[box_off + 3] : dequantize(boxes_int8[box_off + 3], boxes_scale, boxes_zp);

        int anc_x_pct = (int)(anchors[max_idx].x_center * 100);
        int anc_y_pct = (int)(anchors[max_idx].y_center * 100);

        xprintf("[RAW_TENSOR] best: idx=%d anc=[%d,%d] raw=[%d,%d,%d,%d]\n",
                max_idx, anc_x_pct, anc_y_pct,
                (int)dx, (int)dy, (int)dw, (int)dh);
    }
    #endif

    // Temporary storage for detections before NMS
    palm_bbox_t temp_detections[32];
    int num_detections = 0;

    // Scale factors from model input to original image
    float scale_x = (float)img_w / PALM_DET_INPUT_WIDTH;
    float scale_y = (float)img_h / PALM_DET_INPUT_HEIGHT;

    // Process each anchor
    for (int i = 0; i < num_anchors && num_detections < 32; i++) {
        // Get score - handle both FLOAT and INT8
        float score;
        if (scores_is_float) {
            score = scores_float[i];
        } else {
            score = dequantize(scores_int8[i], scores_scale, scores_zp);
        }
        score = sigmoid(score);

        if (score < score_threshold) {
            continue;
        }


        // Decode bounding box using anchor
        // Model output format: [dx, dy, dw, dh, ...keypoints...]
        // dx, dy are offsets from anchor center (in input image pixels)
        // dw, dh are width/height (in input image pixels)
        int box_offset = i * box_size;

        float dx, dy, dw, dh;
        if (boxes_is_float) {
            dx = boxes_float[box_offset + 0];
            dy = boxes_float[box_offset + 1];
            dw = boxes_float[box_offset + 2];
            dh = boxes_float[box_offset + 3];
        } else {
            dx = dequantize(boxes_int8[box_offset + 0], boxes_scale, boxes_zp);
            dy = dequantize(boxes_int8[box_offset + 1], boxes_scale, boxes_zp);
            dw = dequantize(boxes_int8[box_offset + 2], boxes_scale, boxes_zp);
            dh = dequantize(boxes_int8[box_offset + 3], boxes_scale, boxes_zp);
        }

        // MediaPipe palm detection decoding
        // Vela compiler scales dimensions differently than offsets
        // - Position offsets (dx, dy): NO scale needed
        // - Box dimensions (dw, dh): NEED VELA_SCALE
        const float VELA_SCALE = 10.0f;

        // Position: anchor + offset (dx/dy swapped based on PC testing)
        float cx = anchors[i].x_center + dy / INPUT_SIZE;  // Use dy for x (swapped)
        float cy = anchors[i].y_center + dx / INPUT_SIZE;  // Use dx for y (swapped)

        // Box dimensions: need VELA_SCALE for Vela-compiled model
        float w = (dw / INPUT_SIZE) * VELA_SCALE;
        float h = (dh / INPUT_SIZE) * VELA_SCALE;

        // Convert from center format to corner format and scale to image size
        float x = (cx - w / 2.0f) * img_w;
        float y = (cy - h / 2.0f) * img_h;
        float width = w * img_w;
        float height = h * img_h;

        // Clamp to image bounds
        x = std::max(0.0f, std::min(x, (float)img_w));
        y = std::max(0.0f, std::min(y, (float)img_h));
        width = std::max(1.0f, std::min(width, (float)img_w - x));
        height = std::max(1.0f, std::min(height, (float)img_h - y));

        // Debug output moved to top5 section above

        // Store detection
        temp_detections[num_detections].x = (int16_t)x;
        temp_detections[num_detections].y = (int16_t)y;
        temp_detections[num_detections].width = (int16_t)width;
        temp_detections[num_detections].height = (int16_t)height;
        temp_detections[num_detections].score = score;
        temp_detections[num_detections].rotation = 0.0f;  // TODO: Calculate from keypoints

        num_detections++;
    }

    // Apply NMS
    int pre_nms_count = num_detections;
    num_detections = apply_nms(temp_detections, num_detections, nms_threshold);

    #ifdef PALM_DETECTION_DEBUG
    xprintf("[RAW_TENSOR] pre_nms=%d post_nms=%d\n", pre_nms_count, num_detections);
    #endif

    // Copy to output
    int output_count = std::min(num_detections, max_palms);
    for (int i = 0; i < output_count; i++) {
        detected_palms[i] = temp_detections[i];
    }

    return output_count;
}
