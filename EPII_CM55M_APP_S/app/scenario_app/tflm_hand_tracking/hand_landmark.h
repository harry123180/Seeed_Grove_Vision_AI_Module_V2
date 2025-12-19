/*
 * hand_landmark.h
 * Hand Landmark Post-processing
 */

#ifndef SCENARIO_APP_HAND_LANDMARK_H_
#define SCENARIO_APP_HAND_LANDMARK_H_

#include <stdint.h>
#include "cvapp_hand_tracking.h"
#include "tensorflow/lite/c/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
 * Landmark Indices
 * ============================================ */
#define WRIST           0
#define THUMB_CMC       1
#define THUMB_MCP       2
#define THUMB_IP        3
#define THUMB_TIP       4
#define INDEX_MCP       5
#define INDEX_PIP       6
#define INDEX_DIP       7
#define INDEX_TIP       8
#define MIDDLE_MCP      9
#define MIDDLE_PIP      10
#define MIDDLE_DIP      11
#define MIDDLE_TIP      12
#define RING_MCP        13
#define RING_PIP        14
#define RING_DIP        15
#define RING_TIP        16
#define PINKY_MCP       17
#define PINKY_PIP       18
#define PINKY_DIP       19
#define PINKY_TIP       20

/**
 * @brief Post-process hand landmark output
 *
 * @param landmarks_tensor  Output tensor containing 21 landmarks (x, y, z)
 * @param palm_bbox         Detected palm bounding box (for coordinate transform)
 * @param img_w             Original image width
 * @param img_h             Original image height
 * @param output_landmarks  Array to store 21 output landmarks
 */
void hand_landmark_postprocess(
    TfLiteTensor* landmarks_tensor,
    palm_bbox_t* palm_bbox,
    uint32_t img_w,
    uint32_t img_h,
    hand_landmark_point_t* output_landmarks
);

/**
 * @brief Calculate hand orientation from landmarks
 *
 * @param landmarks     Array of 21 landmarks
 * @return Rotation angle in radians
 */
float calculate_hand_rotation(hand_landmark_point_t* landmarks);

/**
 * @brief Determine handedness (left or right)
 *
 * @param landmarks     Array of 21 landmarks
 * @return 0.0 for left, 1.0 for right
 */
float determine_handedness(hand_landmark_point_t* landmarks);

#ifdef __cplusplus
}
#endif

#endif /* SCENARIO_APP_HAND_LANDMARK_H_ */
