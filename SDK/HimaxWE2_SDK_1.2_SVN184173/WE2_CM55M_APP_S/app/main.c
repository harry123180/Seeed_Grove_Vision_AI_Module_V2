/**************************************************************************************************
 (C) COPYRIGHT, Himax Technologies, Inc. ALL RIGHTS RESERVED
 ------------------------------------------------------------------------
 File        : main.c
 Project     : EPII
 DATE        : 2021/04/01
 AUTHOR      : 902447
 BRIFE       : main function
 HISTORY     : Initial version - 2021/04/01 created by Jacky
 **************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "WE2_device.h"
#include "WE2_core.h"
#include "board.h"
#ifdef LIB_COMMON
#include "xprintf.h"
#include "console_io.h"
#endif
#include "pinmux_init.h"
#include "platform_driver_init.h"

#define CIS_XSHUT_SGPIO0

#ifdef HM_COMMON
#include "hx_drv_CIS_common.h"
#endif
#if defined(CIS_HM0360_MONO_REVB) || defined(CIS_HM0360_MONO_OSC_REVB) \
	|| defined(CIS_HM0360_BAYER_REVB) || defined(CIS_HM0360_BAYER_OSC_REVB) \
	||  defined(CIS_HM0360_MONO) || defined(CIS_HM0360_BAYER)
#error "WHYY???"
#include "hx_drv_hm0360.h"
#endif
#ifdef CIS_HM11B1
#include "hx_drv_hm11b1.h"
#endif
#if defined(CIS_HM01B0_MONO) || defined(CIS_HM01B0_BAYER)
#include "hx_drv_hm01b0.h"
#endif
#ifdef CIS_HM2140
#include "hx_drv_hm2140.h"
#endif
#ifdef CIS_XSHUT_SGPIO0
#define DEAULT_XHSUTDOWN_PIN    AON_GPIO2 
#else
#define DEAULT_XHSUTDOWN_PIN    AON_GPIO2 
#endif

/******************************************************/
// ifeq ($(APP_TYPE), CLI_PERIPHERAL)
/******************************************************/
#ifdef MID_CLI
#ifdef TRUSTZONE_SEC
#if (__ARM_FEATURE_CMSE & 1) == 0
#error "Need ARMv8-M security extensions"
#elif (__ARM_FEATURE_CMSE & 2) == 0
#error "Compile with --cmse"
#endif
#include "arm_cmse.h"
#ifndef TRUSTZONE_SEC_ONLY
#include "veneer_table.h"

#endif
#endif

#include "cli.h"
#include "board.h"
#include "console_io.h"
#ifdef LIB_COMMON
#include "xprintf.h"
#endif
#include "hx_drv_timer.h"
#ifdef IP_iic
#include "hx_drv_iic.h"
#endif
#include "hx_drv_scu.h"
#include "hx_drv_pmu.h"
#ifdef IP_swreg_aon
#include "hx_drv_swreg_aon.h"
#endif
#ifdef IP_timer
#include "timer_interface.h"
#endif
void cli_app_start()
{
#if defined(CLI_TZS_MPC_AT_S) || defined(CLI_TZS_TZ_AT_S_MSC_DMA) || defined(CLI_TZS_TZ_AT_S_MSC_DPDMA)
	hx_enable_ICache(0);
	hx_enable_DCache(0);
	hx_set_memory(0xE001E510, 0xF0);
	hx_set_memory(0xE001E610, 0xF0);
#endif
#ifdef CLI_CM55S_SRAM2_TEST
	hx_drv_scu_set_CM55S_INITSVTOR(0x34080000);
	hx_drv_scu_set_CM55S_INITNSVTOR(0x24080000);
#endif
#ifdef FREERTOS
	  const TickType_t xDelay = pdMS_TO_TICKS(1);
#endif
#ifdef CLI_ENABLE
#ifdef MID_CLI
	xprintf("[CLI Program V0.3]\r\n");

	uint32_t wakeup_event;
	uint32_t wakeup_event1;
#ifdef IP_swreg_aon
	SWREG_AON_PMUWAKE_CM55S_RERESET_E resetcm55s_flag;
#endif
	hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT, &wakeup_event);
	hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT1, &wakeup_event1);
    xprintf("wakeup_event=0x%x,WakeupEvt1=0x%x\n", wakeup_event, wakeup_event1);
    if((wakeup_event == PMU_WAKEUP_NONE) && (wakeup_event1 == PMU_WAKEUPEVENT1_NONE))
    {
		hx_drv_scu_set_cm55s_state(SCU_CM55S_STATE_RESET);
		hx_drv_scu_set_cm55s_state(SCU_CM55S_STATE_NORMAL);
		hx_drv_scu_set_CM55S_CPUWAIT(SCU_CM55_CPU_RUN);
    }else{
#ifdef IP_swreg_aon
    	hx_drv_swreg_aon_get_rerest_cm55s_flag(&resetcm55s_flag);
    	if(resetcm55s_flag == SWREG_AON_PMUWAKE_CM55S_RERESET_YES)
    	{
#endif
    		hx_drv_scu_set_cm55s_state(SCU_CM55S_STATE_RESET);
    		hx_drv_scu_set_cm55s_state(SCU_CM55S_STATE_NORMAL);
    		hx_drv_scu_set_CM55S_CPUWAIT(SCU_CM55_CPU_RUN);
#ifdef IP_swreg_aon
    	}
#endif
    }
#ifdef HM_COMMON
    if(wakeup_event == PMU_WAKEUP_NONE)
    {
        xprintf("Cold boot ini\n");
        /*disable INTC side effect period is accurate*/
        //	int_disable(BOARD_SYS_TIMER_INTNO); /* disable timer interrupt (method 1 to stop capture frame)*/
        //	int_disable(BOARD_STD_TIMER_ID); /* disable timer interrupt (method 1 to stop capture frame)*/
#if defined(CIS_HM0360_MONO_OSC_REVB) || defined(CIS_HM0360_BAYER_OSC_REVB)
        hx_drv_dp_set_mclk_src(DP_MCLK_SRC_INTERNAL, DP_MCLK_SRC_INT_SEL_RC36M);
#else
#ifdef HM_COMMON
        hx_drv_dp_set_mclk_src(DP_MCLK_SRC_INTERNAL, DP_MCLK_SRC_INT_SEL_XTAL);
#endif
#endif
#if defined(CIS_HM01B0_MONO) || defined(CIS_HM01B0_BAYER) || defined(CIS_COMMON) || defined(CIS_HM1245) || defined(CIS_HM2140) \
        || defined(CIS_HM0360_MONO) || defined(CIS_HM0360_MONO_OSC) || defined(CIS_HM0360_BAYER) || defined(CIS_HM0360_BAYER_OSC) \
        || defined(CIS_HM0360_MONO_REVB) || defined(CIS_HM0360_MONO_OSC_REVB) || defined(CIS_HM0360_BAYER_REVB) \
        || defined(CIS_HM0360_BAYER_OSC_REVB) || defined(CIS_HM2130) || defined(CIS_HM2170) || defined(CIS_HM1246) \
		|| defined(CIS_HM0435) || defined(CIS_HM2056) || defined(CIS_OV02C10) || defined(CIS_HM5532) || defined(CIS_SOIK351P) \
        || defined(CIS_OV5647) || defined(CIS_IMX477) || defined(CIS_IMX219) || defined(CIS_IMX708) || defined(CIS_IMX296) || defined(CIS_IMX681)

#if defined(CIS_SOIK351P)
        hx_drv_cis_enable_soi_init_flow();
#endif

#if defined(CIS_HM0435)
        hx_drv_cis_set_xsleep_def_val(0);
#endif

#if defined(CIS_OV5647) || defined(CIS_IMX477) || defined(CIS_IMX219) || defined(CIS_IMX296)
        hx_drv_cis_set_xsleep_def_val(0);
#endif

        DP_MCLK_SRC_INT_EXT_E clk_int_ext;
        DP_MCLK_SRC_INT_SEL_E clk_int_src;
        hx_drv_dp_get_mclk_src(&clk_int_ext, &clk_int_src);
        xprintf("mclk_int_ext=%d,mclk_int_src=%d\n",clk_int_ext, clk_int_src);
        if(clk_int_src == DP_MCLK_SRC_INT_SEL_XTAL)
        {
            xprintf("mclk DIV2, xshutdown_pin=%d\n",DEAULT_XHSUTDOWN_PIN);
#if defined(CIS_HM01B0_MONO) || defined(CIS_HM01B0_BAYER)
#if (HM01B0_MCLK_MHZ == 24)
            hx_drv_cis_init(CIS_XHSUTDOWN_IOMUX_NONE, SENSORCTRL_MCLK_DIV1);
#elif (HM01B0_MCLK_MHZ == 18)
            hx_drv_dp_set_mclk_src(DP_MCLK_SRC_INTERNAL, DP_MCLK_SRC_INT_SEL_RC36M);
            hx_drv_cis_init(CIS_XHSUTDOWN_IOMUX_NONE, SENSORCTRL_MCLK_DIV2);
#elif (HM01B0_MCLK_MHZ == 12)
            hx_drv_cis_init(CIS_XHSUTDOWN_IOMUX_NONE, SENSORCTRL_MCLK_DIV2);
#endif
#elif defined(HM0360_MONO_OSC) || defined(HM0360_BAYER_OSC) || defined(CIS_HM0360_MONO_OSC_REVB) || defined(CIS_HM0360_BAYER_OSC_REVB)
            hx_drv_cis_init(DEAULT_XHSUTDOWN_PIN, SENSORCTRL_MCLK_NO);
#else
#if defined(CIS_HM0360_MONO_REVB) || defined(CIS_HM0360_BAYER_REVB)
#if(HM0360_MCLK_MHZ == 12)
            hx_drv_cis_init(DEAULT_XHSUTDOWN_PIN, SENSORCTRL_MCLK_DIV2);
#elif(HM0360_MCLK_MHZ == 24)
            hx_drv_cis_init(DEAULT_XHSUTDOWN_PIN, SENSORCTRL_MCLK_DIV1);
#endif
#else
            hx_drv_cis_init(DEAULT_XHSUTDOWN_PIN, SENSORCTRL_MCLK_DIV2);
#endif
#endif
        }else{
            xprintf("mclk DIV3, xshutdown_pin=%d\n",DEAULT_XHSUTDOWN_PIN);
#if defined(CIS_HM01B0_MONO) || defined(CIS_HM01B0_BAYER)
            hx_drv_cis_init(CIS_XHSUTDOWN_IOMUX_NONE, SENSORCTRL_MCLK_DIV3);
#else
            hx_drv_cis_init(DEAULT_XHSUTDOWN_PIN, SENSORCTRL_MCLK_DIV3);
#endif
        }
#endif
#ifdef CIS_HM0360_FPGA
        hx_drv_hm0360fpga_init(DEAULT_XHSUTDOWN_PIN);
#endif
#ifdef CIS_HM11B1

        hx_drv_hm11b1_init(DEAULT_XHSUTDOWN_PIN);
#endif

    }// end of cold boot init
#endif

    CLI_InitMenu();
#ifdef CLI_ENABLE_PASSWORD
    CLI_EnablePassword(FALSE);
    CLI_EnablePassword(TRUE);
#endif
	CLI_CommandLoop();
#endif
#endif
	while(1)
	{
#ifdef FREERTOS
		vTaskDelay( xDelay );
#else
		hx_drv_timer_cm55x_delay_ms(1, TIMER_STATE_DC);
#endif
	}
}


/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
#ifdef FREERTOS
    if (xTaskCreate(cli_task, "cli_task", configMINIMAL_STACK_SIZE + 100, NULL, hello_task_PRIORITY, NULL) !=
        pdPASS)
    {
        xprintf("Task creation failed!.\r\n");
        while (1)
            ;
    }
    vTaskStartScheduler();
    for (;;)
        ;
#else
    cli_app_start();
    while (1){
    	printf("end of program");
    }
#endif
	return 0;
}
#endif

#ifdef HELLO_WORLD_TZ
#include "hello_world_tz.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef HELLO_WORLD_FREERTOS_TZ
#include "hello_world_freertostz.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef HELLO_WORLD_FREERTOSTZ_MPU
#include "hello_world_freertostz_mpu.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef HELLO_WORLD_TZ_S_ONLY
#include "hello_world_tz_s_only.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef PMU_TZ_S_ONLY
#include "pmu_tz_s_only.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef SCENARIO_PD_PRECAP_TZ_S_ONLY
#include "pd_precap_tz_s_only.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef PMU_SIM_TZ_S_ONLY
#include "pmu_sim_tz_s_only.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef PMU_DPD_SIM_TZ_S_ONLY
#include "pmu_dpd_sim_tz_s_only.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef PMU_SIM_TZ_S_ONLY_VIDPRE
#include "pmu_sim_tz_s_only_vidpre.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef PMU_SIM_TZ_S_ONLY_AUDPRE_TIMER1WAKE
#include "pmu_sim_tz_s_only_cm55m_audpre_timer1.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef PMU_SIM_TZ_S_ONLY_AUDPRE_VADWAKE
#include "pmu_sim_tz_s_only_cm55m_audpre_vad.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef PMU_SIM_TZ_S_ONLY_AUDVIDPRE_TIMER1WAKE
#include "pmu_sim_tz_s_only_cm55m_audvidpre_timer1.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef SCENARIO_DC_SCL_SCH_TZ_S_ONLY
#include "dc_scl_sch_tz_s_only.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	app_main();
	return 0;
}
#endif

#ifdef TFLM_2IN1_FD_FL_PL
#include "tflm_2in1_fd_fl_pl.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	app_main();
	return 0;
}
#endif

#ifdef TFLM_2IN1_FD_FL_FR_ENROLL
#include "tflm_2in1_fd_fl_fr_enroll.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	app_main();
	return 0;
}
#endif

#ifdef TFLM_2IN1_FD_FL_FR_ENROLL_YOLOV8
#include "tflm_2in1_fd_fl_fr_enroll_yolov8.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	app_main();
	return 0;
}
#endif

#ifdef TFLM_EFFICIENTNET_LITE0
#include "tflm_efficientnet_lite0.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
	#ifdef LIB_COMMON
		xprintf_setup();
	#endif
	app_main();
	return 0;
}
#endif

#ifdef ALLON_SENSOR
#include "allon_sensor.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	app_main();
	return 0;
}
#endif

#ifdef ALLON_SENSOR_MIPI
#include "allon_sensor_mipi.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	app_main();
	return 0;
}
#endif


#ifdef ALLON_SENSOR_TFLM
#include "allon_sensor_tflm.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	app_main();
	return 0;
}
#endif

#ifdef FOR_DRIVER_PREBUILT
#include "for_driver_prebuilt.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	app_main();
	return 0;
}
#endif

#ifdef PDM_SINGLE
#include "pdm_single.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	app_main();
	return 0;
}
#endif


#ifdef INTERNAL_PULL
#include "set_internal_pull.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	app_main();
	return 0;
}
#endif
#ifdef PMU_SIM_TZ_S_ONLY_AUDVIDPRE_VADWAKE
#include "pmu_sim_tz_s_only_cm55m_audvidpre_vad.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif


#ifdef HELLO_WORLD_FREERTOS_TZ_S_ONLY
#include "hello_world_freertos_tz_s_only.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef HELLO_WORLD_FREERTOS_TZ_S_ONLY_TEST_STATIC
#include "hello_world_freertos_tz_s_only_teststatic.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef HELLO_WORLD_FREERTOS_MPU_TZ_S_ONLY
#include "hello_world_freertos_mpu_tz_s_only.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef HELLO_WORLD_NOTZ
#include "hello_world_notz.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef TFLM_NOTZ
#include "tflm_notz.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tflm_app();
	return 0;
}
#endif


#ifdef TFLM_MOVENET
#include "tflm_movenet.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tflm_movenet_app();
	return 0;
}
#endif

#ifdef TFLM_FD_FT 
#include "tflm_fd_fl.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tflm_fd_fl_app();
	return 0;
}
#endif
#ifdef TFLM_PL
#include "tflm_pl.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tflm_pl_app();
	return 0;
}
#endif
#ifdef TFLM_YOLOV8_OD
#include "tflm_yolov8_od.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tflm_yolov8_od_app();
	return 0;
}
#endif

#ifdef TFLM_YOLOV8_POSE
#include "tflm_yolov8_pose.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tflm_yolov8_pose_app();
	return 0;
}
#endif

#ifdef TFLM_YOLOFASTEST_OD
#include "tflm_yolofastest_od.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tflm_yolofastest_od_app();
	return 0;
}
#endif
#ifdef TFLM_HL
#include "tflm_hl.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tflm_hl_app();
	return 0;
}
#endif

#ifdef TFLM_FD_FL_FR
#include "tflm_fd_fl_fr.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tflm_fd_fl_fr_app();
	return 0;
}
#endif

#ifdef TFLM_FD_FL_FR_ENROLL
#include "tflm_fd_fl_fr_enroll.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tflm_fd_fl_fr_enroll_app();
	return 0;
}
#endif

#ifdef AUTO_TEST_NOTZ
#include "auto_test.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	auto_test_app();
	return 0;
}
#endif

#ifdef TZ_TO_CLI
#include "tz_to_cli.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tz_to_cli_app();
	return 0;
}
#endif

#ifdef TZ_TO_CLI_DPPMU
#include "tz_to_cli_dppmu.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tz_to_cli_dppmu_app();
	return 0;
}
#endif

#ifdef TZ_TO_CLI_DUAL_PERIPHERAL
#include "tz_to_cli_dual_peripheral.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tz_to_cli_dual_peripheral_app();
	return 0;
}
#endif

#ifdef TZ_TO_CLI_DPPMU_DUAL
#include "tz_to_cli_dppmu_dual.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tz_to_cli_dppmu_dual_app();
	return 0;
}
#endif

#ifdef TZ_TO_APP_SAMPLE
#include "tz_to_app_sample.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tz_to_app_sample();
	return 0;
}
#endif

#ifdef HELLO_WORLD_NOTZ_DUAL
#include "hello_world_notz_dual.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef HELLO_WORLD_FREERTOS_NOTZ
#include "hello_world_freertos_notz.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef HELLO_WORLD_FREERTOS_MPU_NOTZ
#include "hello_world_freertos_mpu_notz.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef TZ_TO_CLI_DUAL
#include "tz_to_cli_dual.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tz_to_cli_dual_app();
	return 0;
}
#endif

#ifdef HWACCBYTPG_AUTO_TEST
#ifdef BYPASS_TEST
#include "bypass_test.h"
#endif


int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	printf("AUTOTEST Start\n");
#ifdef BYPASS_TEST
	BYPASS_AUTOTEST_ALL();
#endif
	printf("AUTOTEST End\n");
}

#endif//HWACCBYTPG_AUTO_TEST

#ifdef HELLO_WORLD_DUAL_S
#include "hello_world_dual_s.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef FREERTOS_TZ_S_ONLY_SAMPLE
#include "freertos_tz_s_only_sample.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	freertos_tz_s_only_app();
	return 0;
}
#endif

#ifdef PMU_TZ_S_ONLY_DUAL
#include "pmu_tz_s_only_dual.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	pmu_tz_s_only_dual();
	return 0;
}
#endif

#ifdef HELLO_WORLD_EVT_TZ_S_ONLY
#include "hello_world_evt_tz_s_only.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
    hello_world_app();
    return 0;
}
#endif

#ifdef SAMPLE_CODE
#include "sample_code.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
    sample_code_app();
    return 0;
}
#endif
#ifdef SAMPLE_CODE_JPEG
#include "sampe_code_jpeg.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
    sample_code_app();
    return 0;
}
#endif
#ifdef AIOT_NB_EXAMPLE_TZ_S_ONLY
#include "aiot_nb_example_tz_s_only.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
    notebook_example_app();
    return 0;
}
#endif

#ifdef FREERTOS_TZ_S_DP_SAMPLE
#include "freertos_tz_s_dp_sample.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	freertos_tz_s_dp_sample_app();
    return 0;
}
#endif

#ifdef TZ_TO_CLI_TIMER
#include "tz_to_cli_timer.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tz_to_cli_app();
	return 0;
}
#endif

#ifdef TZ_TO_CLI_DUAL_TIMER
#include "tz_to_cli_dual_timer.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tz_to_cli_app();
	return 0;
}
#endif

#ifdef TFLM_RNN_NOISE
#include "tflm_rnn_noise.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tflm_rnn_noise_app();
	return 0;
}
#endif

#ifdef PMU_SIM_TZ_S_ONLY_DUAL_BUS_ACCESS
#include "pmu_sim_tz_s_only_dual_bus_access.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef SIM_TZ_S_ONLY
#include "sim_tz_s_only.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef PMU_SIM_TZ_S_ONLY_AUDPRE_FORCE
#include "pmu_sim_tz_s_only_cm55m_audpre_force.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef PMU_SIM_TZ_S_ONLY_AUDVIDPRE_FORCE
#include "pmu_sim_tz_s_only_cm55m_audvidpre_force.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef SIM_CLK
#include "sim_clk.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef PMU_SIM_RET
#include "pmu_sim_ret.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef TFLM_FD_FM
#include "tflm_fd_fm.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tflm_fd_fm_app();
	return 0;
}
#endif

#ifdef PMU_SIM_TZ_S_ONLY_LOWFREQ
#include "pmu_sim_tz_s_only_cm55m_lowfreq.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef SIM_PER
#include "sim_per.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#ifdef PMU_SIM_TZ_S_ONLY_MEA
#include "pmu_sim_tz_s_only_mea.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
	return 0;
}
#endif

#if defined(HX_TFM) && !defined(BUILD_TFM_NS)
#include "main_s.h"

int main(void)
{
    main_s();
	while (1) {};
	return 0;
}
#endif

#ifdef PMU_DPD_SAMPLE
#include "pmu_dpd_sample.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	pmu_dpd_app();
	return 0;
}
#endif

#ifdef PMU_PD_SAMPLE
#include "pmu_pd_sample.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	pmu_pd_app();
	return 0;
}
#endif

#ifdef PMU_VIDPRE_SAMPLE
#include "pmu_vidpre_sample.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	pmu_vidpre_app();
	return 0;
}
#endif

#ifdef PMU_VIDPRE_CDM_SAMPLE
#include "pmu_vidpre_cdm_sample.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	pmu_vidpre_cdm_app();
	return 0;
}
#endif


#ifdef PMU_AUDVIDPRE_CDM_WAKE_SAMPLE
#include "pmu_audvidpre_cm55m_cdm_sample.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	pmu_audvidpre_cm55m_cdm_app();
	return 0;
}
#endif

#ifdef TZ_PMU_SAMPLE
#include "tz_pmu_sample.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tz_pmu_sample_app();
	return 0;
}
#endif

#ifdef TZ_PMU_SAMPLE_DUAL
#include "tz_pmu_sample_dual.h"
/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	tz_pmu_sample_dual_app();
	return 0;
}
#endif

#ifdef SORTING_MIPI_PATH_THROUGH
#include "sorting_mipi_path_through.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	sorting_mipi_path_through_app();
    return 0;
}
#endif

#ifdef SORTING_MIPIRX
#include "sorting_mipirx.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	sorting_mipirx_app();
    return 0;
}
#endif

#ifdef PMU_AUDPRE_TIMER2_SAMPLE
#include "pmu_audpre_cm55m_timer2_sample.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	pmu_audpre_cm55m_timer2_app();
    return 0;
}
#endif

#ifdef PMU_AUDVIDPRE_TIMER2_WAKE_SAMPLE
#include "pmu_audvidpre_cm55m_timer2_sample.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	pmu_audvidpre_cm55m_timer2_app();
    return 0;
}
#endif

#ifdef SIMPLE_APP
#include "simple_app.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif

	simple_app();
    return 0;
}
#endif

#ifdef HELLO_WORLD_RTOS2_RTX_TZ_S_ONLY
#include "hello_world_rtos2_rtx_tz_s_only.h"

/** main entry */
int main(void)
{
    /*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
    return 0;
}
#endif

#ifdef HELLO_WORLD_RTX_TZ_S_ONLY_TEST_STATIC
#include "hello_world_rtx_tz_s_only_teststatic.h"

/** main entry */
int main(void)
{
    /*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
    return 0;
}
#endif

#ifdef HELLO_WORLD_RTOS2_FREERTOS_TZ_S_ONLY
#include "hello_world_rtos2_freertos_tz_s_only.h"

/** main entry */
int main(void)
{
    /*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
    return 0;
}
#endif

#ifdef HELLO_WORLD_RTOS2_FREERTOS_TZ_S_ONLY_STATIC
#include "hello_world_rtos2_freertos_tz_s_only_teststatic.h"

/** main entry */
int main(void)
{
    /*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
    return 0;
}
#endif

#if defined(ALLON_SENSOR_TFLM_RTOS2_RTX_TZ_S_ONLY) || \
    defined(ALLON_SENSOR_TFLM_RTOS2_FREERTOS_TZ_S_ONLY) || \
    defined(ALLON_SENSOR_TFLM_FREERTOS_OS_HAL_TZ_S_ONLY) || \
    defined(ALLON_SENSOR_TFLM_FREERTOS_TZ_S_ONLY) || \
    defined(ALLON_SENSOR_TFLM_RTOS2_RTX_TZ_S_ONLY_V2) || \
    defined(ALLON_SENSOR_TFLM_RTOS2_FREERTOS_TZ_S_ONLY_V2) || \
    defined(ALLON_SENSOR_TFLM_FREERTOS_OS_HAL_TZ_S_ONLY_V2) || \
    defined(ALLON_SENSOR_TFLM_FREERTOS_TZ_S_ONLY_V2)
#include "allon_sensor_tflm.h"

/** main entry */
int main(void)
{
    /*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	rtos_app();
    return 0;
}
#endif

#ifdef HELLO_WORLD_RTOS2_RTX_TZ
#include "hello_world_rtos2_rtx_tz.h"

/* main entry */
int main(void)
{
    /*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
    return 0;
}
#endif

#ifdef HELLO_WORLD_RTOS2_FREERTOS_TZ
#include "hello_world_rtos2_freertos_tz.h"

/* main entry */
int main(void)
{
    /*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	hello_world_app();
    return 0;
}
#endif

#ifdef SIM_DHRY
#include "dhry.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	sim_dhry();
    return 0;
}
#endif

#ifdef PMU_AUDPRE_TIMER2_SAMPLE_SIM
#include "pmu_audpre_cm55m_timer2_sample_sim.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	pmu_audpre_cm55m_timer2_app_sim();
    return 0;
}
#endif


#ifdef PMU_AUDPRE_TIMER2_SAMPLE_SIM_0P8V
#include "pmu_audpre_cm55m_timer2_sample_sim_0p8v.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	pmu_audpre_cm55m_timer2_app_sim_0p8v();
    return 0;
}
#endif

#ifdef PMU_AUDPRE_TIMER2_SAMPLE_0P8V
#include "pmu_audpre_cm55m_timer2_sample_0p8v.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	pmu_audpre_cm55m_timer2_sample_0p8v();
    return 0;
}
#endif

#ifdef PMU_PD_SAMPLE_GOOGLE
#include "pmu_pd_sample_google.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	pmu_pd_app_google();
    return 0;
}
#endif


#ifdef PMU_VIDPRE_SAMPLE_GOOGLE
#include "pmu_vidpre_sample_google.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	pmu_vidpre_sample_google();
    return 0;
}
#endif

#ifdef PMU_VIDPRE_SAMPLE_POWER
#include "pmu_vidpre_sample_power.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	pmu_vidpre_sample_power();
    return 0;
}
#endif

#ifdef APP_API
#include "app_main.h"

int main(void)
{
    board_init();
	app_main();

	return 0;
}
#endif

#ifdef PMU_AUDPRE_DP_CPU_SAMPLE
#include "pmu_audpre_cm55m_dp_cpu_sample.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif
	pmu_audpre_cm55m_dp_cpu_app();
    return 0;
}
#endif
#ifdef FATFS_TEST
#include "fatfs_test.h"
/** main entry */
int main(void)
{
	/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif	
	fatfs_test();
	return 0;
}
#endif

#ifdef SIMPLE_APP_CLK_CHANGE
#include "simple_app_clk_change.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif

	simple_app();
    return 0;
}
#endif

#ifdef SIMPLE_APP_DUAL_CORE
#include "simple_app_dual_core.h"

/** main entry */
int main(void)
{
		/*set pinmux init*/
	pinmux_init();
	/*platform driver init*/
	platform_driver_init();
#ifdef IP_uart
	console_setup(DW_UART_0_ID, UART_BAUDRATE_115200);
#endif
#ifdef LIB_COMMON
	xprintf_setup();
#endif

	simple_app_dual_core();
    return 0;
}
#endif
