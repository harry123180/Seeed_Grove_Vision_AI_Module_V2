/*
 * hand_landmark.cpp
 * Hand Landmark Post-processing Implementation
 *
 * MediaPipe Hand Landmark outputs:
 * - landmarks: [1, 63] or [1, 21, 3] - 21 keypoints with (x, y, z)
 * - handedness: [1, 1] - confidence of handedness (optional)
 */

#include "hand_landmark.h"
#include "common_config.h"
#include <cmath>
#include "xprintf.h"

/* ============================================
 * Dequantization Helper
 * ============================================ */
static inline float dequantize(int8_t value, float scale, int zero_point) {
    return (float)(value - zero_point) * scale;
}

/* ============================================
 * Coordinate Transform
 * ============================================ */

/**
 * Transform landmark from model coordinate to original image coordinate
 *
 * Model output: normalized [0, 1] or pixel in crop region
 * Target: pixel coordinate in original image
 */
static void transform_landmark(
    float model_x,
    float model_y,
    float model_z,
    palm_bbox_t* palm,
    uint32_t img_w,
    uint32_t img_h,
    hand_landmark_point_t* output
) {
    // Calculate crop region (same as in cvapp_hand_tracking.cpp)
    float scale_factor = 2.0f;
    int cx = palm->x + palm->width / 2;
    int cy = palm->y + palm->height / 2;
    int size = (int)(fmax(palm->width, palm->height) * scale_factor);

    int crop_x1 = cx - size / 2;
    int crop_y1 = cy - size / 2;

    // Clamp
    crop_x1 = (crop_x1 < 0) ? 0 : crop_x1;
    crop_y1 = (crop_y1 < 0) ? 0 : crop_y1;

    int crop_size = size;
    if (crop_x1 + crop_size > (int)img_w) crop_size = img_w - crop_x1;
    if (crop_y1 + crop_size > (int)img_h) crop_size = img_h - crop_y1;

    // Transform from model output to crop region
    // Model output is typically in [0, input_size] or normalized [0, 1]
    float norm_x = model_x / HAND_LM_INPUT_WIDTH;
    float norm_y = model_y / HAND_LM_INPUT_HEIGHT;

    // Map to crop region
    float img_x = norm_x * crop_size + crop_x1;
    float img_y = norm_y * crop_size + crop_y1;

    // Z is typically depth relative to wrist, scale it
    float img_z = model_z * 100.0f;

    // Clamp to image bounds
    img_x = fmax(0.0f, fmin(img_x, (float)img_w - 1));
    img_y = fmax(0.0f, fmin(img_y, (float)img_h - 1));

    output->x = (int16_t)img_x;
    output->y = (int16_t)img_y;
    output->z = (int16_t)img_z;
}

/* ============================================
 * Main Post-processing Function
 * ============================================ */
void hand_landmark_postprocess(
    TfLiteTensor* landmarks_tensor,
    palm_bbox_t* palm_bbox,
    uint32_t img_w,
    uint32_t img_h,
    hand_landmark_point_t* output_landmarks
) {
    if (landmarks_tensor == nullptr || palm_bbox == nullptr || output_landmarks == nullptr) {
        return;
    }

    // Check tensor type - vela models output FLOAT after DEQUANTIZE ops
    bool is_float = (landmarks_tensor->type == kTfLiteFloat32);

    // Get quantization parameters (only valid for INT8)
    float scale = is_float ? 1.0f : landmarks_tensor->params.scale;
    int zero_point = is_float ? 0 : landmarks_tensor->params.zero_point;

    // Get data pointer based on type
    float* data_float = is_float ? landmarks_tensor->data.f : nullptr;
    int8_t* data_int8 = is_float ? nullptr : landmarks_tensor->data.int8;

    // Get tensor shape
    int total_bytes = landmarks_tensor->bytes;
    // For FLOAT: 4 bytes per value, for INT8: 1 byte per value
    int num_coords = is_float ? (total_bytes / 4) : total_bytes;

    #ifdef HAND_LANDMARK_DEBUG
    xprintf("[Hand LM] type=%s, bytes=%d, coords=%d\n",
            is_float ? "FLOAT" : "INT8", total_bytes, num_coords);
    #endif

    // MediaPipe hand landmark output: 21 points * 3 coords (x, y, z) = 63 values
    // Layout: [x0, y0, z0, x1, y1, z1, ..., x20, y20, z20]

    for (int i = 0; i < NUM_HAND_LANDMARKS; i++) {
        int offset = i * 3;

        if (offset + 2 >= num_coords) {
            // Invalid data, set to zero
            output_landmarks[i].x = 0;
            output_landmarks[i].y = 0;
            output_landmarks[i].z = 0;
            continue;
        }

        // Get landmark values - handle both FLOAT and INT8
        float x, y, z;
        if (is_float) {
            x = data_float[offset + 0];
            y = data_float[offset + 1];
            z = data_float[offset + 2];
        } else {
            x = dequantize(data_int8[offset + 0], scale, zero_point);
            y = dequantize(data_int8[offset + 1], scale, zero_point);
            z = dequantize(data_int8[offset + 2], scale, zero_point);
        }

        // Transform to image coordinates
        transform_landmark(x, y, z, palm_bbox, img_w, img_h, &output_landmarks[i]);

        #ifdef HAND_LANDMARK_DEBUG
        if (i == WRIST || i == INDEX_TIP || i == MIDDLE_TIP) {
            xprintf("[Hand LM] Point %d: raw=(%.1f,%.1f,%.1f) -> img=(%d,%d,%d)\n",
                    i, x, y, z,
                    output_landmarks[i].x,
                    output_landmarks[i].y,
                    output_landmarks[i].z);
        }
        #endif
    }
}

/* ============================================
 * Hand Rotation Calculation
 * ============================================ */
float calculate_hand_rotation(hand_landmark_point_t* landmarks) {
    // Calculate rotation from wrist to middle finger MCP
    float dx = (float)(landmarks[MIDDLE_MCP].x - landmarks[WRIST].x);
    float dy = (float)(landmarks[MIDDLE_MCP].y - landmarks[WRIST].y);

    return atan2f(dy, dx);
}

/* ============================================
 * Handedness Determination
 * ============================================ */
float determine_handedness(hand_landmark_point_t* landmarks) {
    // Simple heuristic based on thumb position relative to pinky
    // For a right hand (palm facing camera): thumb is to the left of pinky
    // For a left hand (palm facing camera): thumb is to the right of pinky

    float thumb_x = (float)landmarks[THUMB_TIP].x;
    float pinky_x = (float)landmarks[PINKY_TIP].x;

    // Return 0.0 for left, 1.0 for right
    return (thumb_x < pinky_x) ? 1.0f : 0.0f;
}
