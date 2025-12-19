/*
 * palm_postprocessing.h
 * Palm Detection Post-processing
 */

#ifndef SCENARIO_APP_PALM_POSTPROCESSING_H_
#define SCENARIO_APP_PALM_POSTPROCESSING_H_

#include <stdint.h>
#include "cvapp_hand_tracking.h"
#include "tensorflow/lite/c/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Post-process palm detection output
 *
 * @param boxes_tensor      Output tensor containing bounding boxes
 * @param scores_tensor     Output tensor containing confidence scores
 * @param detected_palms    Array to store detected palm bboxes
 * @param max_palms         Maximum number of palms to detect
 * @param img_w             Original image width
 * @param img_h             Original image height
 * @param score_threshold   Minimum confidence threshold
 * @param nms_threshold     NMS IoU threshold
 * @return Number of detected palms
 */
int palm_detection_postprocess(
    TfLiteTensor* boxes_tensor,
    TfLiteTensor* scores_tensor,
    palm_bbox_t* detected_palms,
    int max_palms,
    uint32_t img_w,
    uint32_t img_h,
    float score_threshold,
    float nms_threshold
);

/**
 * @brief Calculate IoU between two bounding boxes
 */
float calculate_iou(palm_bbox_t* a, palm_bbox_t* b);

/**
 * @brief Apply Non-Maximum Suppression
 */
int apply_nms(
    palm_bbox_t* detections,
    int num_detections,
    float nms_threshold
);

#ifdef __cplusplus
}
#endif

#endif /* SCENARIO_APP_PALM_POSTPROCESSING_H_ */
