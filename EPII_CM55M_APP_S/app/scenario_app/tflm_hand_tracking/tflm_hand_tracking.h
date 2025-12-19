/*
 * tflm_hand_tracking.h
 *
 * Hand Tracking Application Header
 */

#ifndef SCENARIO_TFLM_HAND_TRACKING_
#define SCENARIO_TFLM_HAND_TRACKING_

#define APP_BLOCK_FUNC() do{ \
	__asm volatile("b    .");\
	}while(0)

typedef enum
{
	APP_STATE_HAND_TRACKING,
}APP_STATE_E;

int app_main(void);
void SetPSPDNoVid();
#endif /* SCENARIO_TFLM_HAND_TRACKING_ */
