/*
 * cvapp_hand_tracking.h
 * Hand Tracking Application Header
 */

#ifndef SCENARIO_APP_CVAPP_HAND_TRACKING_H_
#define SCENARIO_APP_CVAPP_HAND_TRACKING_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
 * Data Structures
 * ============================================ */

// Single landmark point (x, y, z)
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} hand_landmark_point_t;

// Palm bounding box
typedef struct {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
    float score;
    float rotation;     // Palm rotation angle
} palm_bbox_t;

// Complete hand result (21 landmarks + bbox)
typedef struct {
    bool detected;
    palm_bbox_t palm;
    hand_landmark_point_t landmarks[21];
    float handedness;   // 0.0 = left, 1.0 = right
    float score;
} hand_result_t;

// Algorithm output structure
typedef struct {
    int num_hands;
    hand_result_t hands[2];     // Max 2 hands
} struct_hand_algoResult;

/* ============================================
 * API Functions
 * ============================================ */

/**
 * @brief Initialize hand tracking models
 * @param security_enable Enable security mode
 * @param privilege_enable Enable privilege mode
 * @param palm_model_addr Flash address of palm detection model
 * @param hand_model_addr Flash address of hand landmark model
 * @return 0 on success, -1 on failure
 */
int cv_hand_tracking_init(
    bool security_enable,
    bool privilege_enable,
    uint32_t palm_model_addr,
    uint32_t hand_model_addr
);

/**
 * @brief Run hand tracking inference
 * @param result Output structure for detected hands
 * @return 0 on success, -1 on failure
 */
int cv_hand_tracking_run(struct_hand_algoResult *result);

/**
 * @brief Deinitialize hand tracking
 */
void cv_hand_tracking_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* SCENARIO_APP_CVAPP_HAND_TRACKING_H_ */
