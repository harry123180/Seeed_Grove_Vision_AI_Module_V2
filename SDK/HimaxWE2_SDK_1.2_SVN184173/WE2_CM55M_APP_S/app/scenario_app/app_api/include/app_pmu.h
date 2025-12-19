/**
 * @file app_pmu.h
 * @brief 
 * @version V1.0.0
 * @date 2022-07-14
 * 
 * @copyright (C) COPYRIGHT, Himax, Inc. ALL RIGHTS RESERVED
 * 
 */
 
#ifndef __APP_PMU_H__
#define __APP_PMU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "app_api.h"

#define MAX_SUPPORT_WAKEUP_CPU_INT_PIN_MAX (4)
#define APP_PMU_SYS_RTC_ID (TIMER_ID_1)

/**
 * \struct AppCfgCustGpio_t
 * \brief gpio pin config for pmu to wake up CPU
 */
typedef struct
{
	uint8_t act_wakeupCPU_pin_cnt;                                           /**< gpio pin count*/
	uint8_t wakeupCPU_int_pin[MAX_SUPPORT_WAKEUP_CPU_INT_PIN_MAX];    /**< gpio pin number to wake up CPU*/
}AppCfgCustGpio_t;

#if defined(PMU_SLEEP_CLK_DISABLE)
void app_pmu_clk_enable();
#endif

/**
 * \brief	set pmu to enter into sleep 1 mode
 *
 * \param[in]	gpio_cfg	        gpio pin config for pmu to wake up CPU
 * \param[in]	sensor_rtc_ms	    sensor rtc timer in ms to wake up CPU
 * \param[in]	quickboot	        enable quick boot mode to wake up CPU 
 * \return	void
 */
void app_pmu_enter_sleep1(AppCfgCustGpio_t gpio_cfg, uint32_t sensor_rtc_ms, uint8_t quickboot);

/**
 * \brief	set pmu to enter into sleep 1 aos mode
 *
 * \param[in]	gpio_cfg	        gpio pin config for pmu to wake up CPU
 * \param[in]	sensor_rtc_ms	    sensor rtc timer in ms to wake up CPU
 * \param[in]	quickboot	        enable quick boot mode to wake up CPU 
 * \return	void
 */
void app_pmu_enter_sleep1_aos(AppCfgCustGpio_t gpio_cfg, uint32_t sensor_rtc_ms, uint32_t sys_rtc_ms, uint8_t quickboot);

/**
 * \brief	set pmu to enter into sleep 1 boot with capture
 *
 * \param[in]	gpio_cfg	        gpio pin config for pmu to wake up CPU
 * \param[in]	sensor_rtc_ms	    sensor rtc timer in ms to wake up CPU
 * \param[in]	quickboot	        enable quick boot mode to wake up CPU 
 * \return	void
 */
void app_pmu_enter_sleep1_bootwithcap(AppCfgCustGpio_t gpio_cfg, uint32_t sys_rtc_ms);
/**
 * \brief	set pmu to enter into sleep 2 mode
 *
 * \param[in]	gpio_cfg	        gpio pin config for pmu to wake up CPU
 * \param[in]	sensor_rtc_ms	    sensor rtc timer in ms to wake up CPU
 * \return	void
 */
void app_pmu_enter_sleep2(AppCfgCustGpio_t gpio_cfg, uint32_t sensor_rtc_ms);

/**
 * \brief	set pmu to enter into shut down mode
 *
 * \param[in]	gpio_cfg	        gpio pin config for pmu to wake up CPU
 * \return	void
 */
void app_pmu_enter_shutdown(AppCfgCustGpio_t gpio_cfg);

#if defined(PMU_SIMO_VOLT)
void set_simo_0p9v(void);
void set_simo_0p8v(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /*__APP_PMU_H__*/
