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
 * Based on MediaPipe SSD anchors
 * ============================================ */

// Simplified anchor generation for 128x128 input
// Real implementation should match MediaPipe's anchor generation
#define NUM_ANCHORS     896     // Typical for 128x128 input
#define ANCHOR_OFFSET   0.5f

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

    // Get quantization parameters
    float boxes_scale = boxes_tensor->params.scale;
    int boxes_zp = boxes_tensor->params.zero_point;
    float scores_scale = scores_tensor->params.scale;
    int scores_zp = scores_tensor->params.zero_point;

    int8_t* boxes_data = boxes_tensor->data.int8;
    int8_t* scores_data = scores_tensor->data.int8;

    // Get tensor dimensions
    int num_anchors = boxes_tensor->dims->data[1];
    int box_size = boxes_tensor->dims->data[2];  // Usually 18 (4 + 7*2)

    #ifdef PALM_DETECTION_DEBUG
    xprintf("[Palm] num_anchors=%d, box_size=%d\n", num_anchors, box_size);
    xprintf("[Palm] boxes: scale=%.6f, zp=%d\n", boxes_scale, boxes_zp);
    xprintf("[Palm] scores: scale=%.6f, zp=%d\n", scores_scale, scores_zp);
    #endif

    // Temporary storage for detections before NMS
    palm_bbox_t temp_detections[32];
    int num_detections = 0;

    // Scale factors from model input to original image
    float scale_x = (float)img_w / PALM_DET_INPUT_WIDTH;
    float scale_y = (float)img_h / PALM_DET_INPUT_HEIGHT;

    // Process each anchor
    for (int i = 0; i < num_anchors && num_detections < 32; i++) {
        // Get score
        float score = dequantize(scores_data[i], scores_scale, scores_zp);
        score = sigmoid(score);

        if (score < score_threshold) {
            continue;
        }

        // Decode bounding box
        // Box format: [cx, cy, w, h, kp0_x, kp0_y, kp1_x, kp1_y, ...]
        int box_offset = i * box_size;

        float cx = dequantize(boxes_data[box_offset + 0], boxes_scale, boxes_zp);
        float cy = dequantize(boxes_data[box_offset + 1], boxes_scale, boxes_zp);
        float w = dequantize(boxes_data[box_offset + 2], boxes_scale, boxes_zp);
        float h = dequantize(boxes_data[box_offset + 3], boxes_scale, boxes_zp);

        // TODO: Add anchor offset based on MediaPipe anchor generation
        // For now, assume coordinates are in [0, 1] normalized format

        // Convert to pixel coordinates
        float x = (cx - w / 2.0f) * PALM_DET_INPUT_WIDTH * scale_x;
        float y = (cy - h / 2.0f) * PALM_DET_INPUT_HEIGHT * scale_y;
        float width = w * PALM_DET_INPUT_WIDTH * scale_x;
        float height = h * PALM_DET_INPUT_HEIGHT * scale_y;

        // Clamp to image bounds
        x = std::max(0.0f, std::min(x, (float)img_w));
        y = std::max(0.0f, std::min(y, (float)img_h));
        width = std::min(width, (float)img_w - x);
        height = std::min(height, (float)img_h - y);

        // Store detection
        temp_detections[num_detections].x = (int16_t)x;
        temp_detections[num_detections].y = (int16_t)y;
        temp_detections[num_detections].width = (int16_t)width;
        temp_detections[num_detections].height = (int16_t)height;
        temp_detections[num_detections].score = score;
        temp_detections[num_detections].rotation = 0.0f;  // TODO: Calculate from keypoints

        num_detections++;
    }

    #ifdef PALM_DETECTION_DEBUG
    xprintf("[Palm] Pre-NMS detections: %d\n", num_detections);
    #endif

    // Apply NMS
    num_detections = apply_nms(temp_detections, num_detections, nms_threshold);

    #ifdef PALM_DETECTION_DEBUG
    xprintf("[Palm] Post-NMS detections: %d\n", num_detections);
    #endif

    // Copy to output
    int output_count = std::min(num_detections, max_palms);
    for (int i = 0; i < output_count; i++) {
        detected_palms[i] = temp_detections[i];
    }

    return output_count;
}
