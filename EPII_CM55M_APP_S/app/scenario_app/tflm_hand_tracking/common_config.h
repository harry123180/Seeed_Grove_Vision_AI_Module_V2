/*
 * common_config.h
 * Hand Tracking Configuration
 */

#ifndef SCENARIO_APP_HAND_TRACKING_COMMON_CONFIG_H_
#define SCENARIO_APP_HAND_TRACKING_COMMON_CONFIG_H_

#include "WE2_device_addr.h"

/* ============================================
 * Model Flash Addresses
 * ============================================ */
#define PALM_DETECT_FLASH_ADDR      (BASE_ADDR_FLASH1_R_ALIAS + 0x400000)  // 4MB offset
#define HAND_LANDMARK_FLASH_ADDR    (BASE_ADDR_FLASH1_R_ALIAS + 0x480000)  // 4.5MB offset

/* ============================================
 * Input Tensor Dimensions
 * (Both models use 256x256 input from PINTO)
 * ============================================ */
#define PALM_DET_INPUT_WIDTH        256
#define PALM_DET_INPUT_HEIGHT       256

#define HAND_LM_INPUT_WIDTH         256
#define HAND_LM_INPUT_HEIGHT        256

/* ============================================
 * Detection Parameters
 * ============================================ */
#define PALM_DETECTION_THRESHOLD    0.5f
#define PALM_NMS_THRESHOLD          0.3f
#define HAND_LANDMARK_THRESHOLD     0.5f

/* ============================================
 * Output Configuration
 * ============================================ */
#define NUM_HAND_LANDMARKS          21      // 21 keypoints per hand
#define MAX_HANDS_DETECTED          2       // Maximum hands to detect

/* ============================================
 * Memory Configuration
 * ============================================ */
#define TENSOR_ARENA_SIZE           (512 * 1024)    // 512KB
#define TENSOR_ARENA_TAIL_SIZE      1536            // Model separation offset

/* ============================================
 * Debug Options
 * ============================================ */
// #define HAND_TRACKING_DEBUG
// #define PALM_DETECTION_DEBUG
// #define HAND_LANDMARK_DEBUG

#endif /* SCENARIO_APP_HAND_TRACKING_COMMON_CONFIG_H_ */
