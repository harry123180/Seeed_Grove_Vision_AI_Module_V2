#include "app_platform.h"
#include "app_api.h"
#include "app_sys_info_cmd.h"
#include "app_datapath.h"
#include "app_board.h"
#include "app_xdma_cfg.h"
#include "app_i2c_cmd.h"
#ifdef TFLITE_ALGO_ENABLED
#include "app_algo.h"
#endif
#include "app_pmu.h"
#ifdef FLASH_AS_SRAM
#include "spi_eeprom_comm.h"
#endif

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "projdefs.h"

#include "hx_drv_pmu.h"
#include "hx_drv_scu.h"
#include "hx_drv_scu_export.h"

//#define PA0_WAKEUP HX_AON_GPIO_0
#define SYS_RTC_MS     (3*1000)

/* local variables */
static uint16_t data_types = TX_DATA_JPEG | TX_DATA_META;

static volatile uint8_t g_wdma1_data[0/*WDMA1_SIZE*/] = {};
static volatile uint8_t g_wdma2_data[WDMA2_SLOT_SIZE*WDMA2_CYCLIC_BUF_CNT] = {};
static volatile uint8_t g_wdma3_data[WDMA3_SIZE] = {};
static volatile uint8_t g_jpeg_autofill_data[JPEG_AUTOFILL_SIZE] = {};

#ifdef TFLITE_ALGO_ENABLED
static ALGO_RESULT algo_result;
#endif

static void priv_i2c_cstm_cmd_handler(uint8_t cmd_id, uint8_t *cmd_payload, uint16_t cmd_payload_len);

void clear_wakeup_evt()
{
	uint32_t wakeup_event, wakeup_event1;
	hx_lib_pm_clear_event();
	hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT, &wakeup_event);
    dbg_printf(DBG_LESS_INFO, "after clear WakeupEvt=0x%x\n", wakeup_event);
	hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT1, &wakeup_event1);
    dbg_printf(DBG_LESS_INFO, "after clear WakeupEvt1=0x%x\n", wakeup_event1);
}

void clear_all_sysip_int()
{
#ifdef IP_rtc
	hx_drv_rtc_clear_alarm_int_status(RTC_ID_0);
	hx_drv_rtc_clear_alarm_int_status(RTC_ID_1);
	hx_drv_rtc_clear_alarm_int_status(RTC_ID_2);
#endif
#ifdef IP_timer
	hx_drv_timer_ClearIRQ(TIMER_ID_0);
	hx_drv_timer_ClearIRQ(TIMER_ID_1);
	hx_drv_timer_ClearIRQ(TIMER_ID_2);
	hx_drv_timer_ClearIRQ(TIMER_ID_3);
	hx_drv_timer_ClearIRQ(TIMER_ID_4);
	hx_drv_timer_ClearIRQ(TIMER_ID_5);
	hx_drv_timer_ClearIRQ(TIMER_ID_6);
	hx_drv_timer_ClearIRQ(TIMER_ID_7);
	hx_drv_timer_ClearIRQ(TIMER_ID_8);
#endif
#ifdef IP_watchdog
	hx_drv_watchdog_irq_clear(WATCHDOG_ID_0);
	hx_drv_watchdog_irq_clear(WATCHDOG_ID_1);
#endif
#ifdef IP_gpio
	hx_drv_gpio_clr_int_status(AON_GPIO0);
	hx_drv_gpio_clr_int_status(AON_GPIO1);
	hx_drv_gpio_clr_int_status(SB_GPIO0);
	hx_drv_gpio_clr_int_status(SB_GPIO1);
#endif
#ifdef IP_hxautoi2c_mst
	hx_drv_hxautoi2c_clr_noack_int(HXAUTOI2CHC_INTSTATUS_DATA_NO_ACK);
	hx_drv_hxautoi2c_clr_noack_int(HXAUTOI2CHC_INTSTATUS_ADDR_NO_ACK);
#endif
}

static void main_task(void *pvParameters)
{
	app_dp_cap_t app_dp_cap_set;

	/* Set DP settings */
    app_dp_get_def_cap_settings(&app_dp_cap_set);
	
    //while(1)
    {
        uint32_t raw_img_addr = 0, img_width = 0, img_heigh = 0;
        uint32_t jpg_addr = 0, jpg_size = 0;

        /* Capture jpeg and raw image */
        dbg_printf(DBG_LESS_INFO, "[app_dp_capture_frame]\n");
        app_dp_capture_frame(&app_dp_cap_set);

        /* Capture sensor output to jpeg and raw */
        app_dp_get_frame(&raw_img_addr, &img_width, &img_heigh, &jpg_addr, &jpg_size);
        dbg_printf(DBG_LESS_INFO, "[app_dp_get_frame] raw_img_addr = %x img_width = %d img_width = %d, jpg_addr = %x, jpg_size = %d\n", raw_img_addr, img_width, img_heigh, jpg_addr, jpg_size);

#ifdef TFLITE_ALGO_ENABLED
		/* Execute Algorithm */
		memset(&algo_result, 0, sizeof(algo_result));
		app_algo_run(raw_img_addr, img_width, img_heigh, &algo_result);
#endif

		/* Send out JPG */
		if (TX_DATA_JPEG == (data_types & TX_DATA_JPEG)) {
        #if defined(SPI_MASTER_SEND) || defined(SPI_SLAVE_SEND)
			if (jpg_addr != 0)
				app_spi_write(jpg_addr, jpg_size, DATA_TYPE_JPG);
        #else defined(UART_PROTOCOL)
			if (jpg_addr != 0)
				app_uart_tx(jpg_addr, jpg_size, DATA_TYPE_JPG);
        #endif
		}

		/* Send out Un-compressed Image */
		if (TX_DATA_RAW == (data_types & TX_DATA_RAW)) {
        #if defined(SPI_MASTER_SEND) || defined(SPI_SLAVE_SEND)
			if(raw_img_addr != 0)
				app_spi_write(raw_img_addr, img_width*img_heigh, DATA_TYPE_RAW_IMG);
        #else defined(UART_PROTOCOL)
			if(raw_img_addr != 0)
				app_uart_tx(raw_img_addr, img_width*img_heigh, DATA_TYPE_RAW_IMG);
        #endif
		}
		
#ifdef TFLITE_ALGO_ENABLED
		/* Send out Metadata */
		if (TX_DATA_META == (data_types & TX_DATA_META)) {
        #if defined(SPI_MASTER_SEND) || defined(SPI_SLAVE_SEND)
			app_spi_write((uint32_t)&algo_result, sizeof(algo_result), DATA_TYPE_META_DATA);
        #else defined(UART_PROTOCOL)
			app_uart_tx((uint32_t)&algo_result, sizeof(algo_result), DATA_TYPE_META_DATA);
        #endif
		}
#endif
    }
	
	sensordplib_stop_capture();
	clear_wakeup_evt();
	clear_all_sysip_int();
	
#ifdef TFLITE_ALGO_ENABLED
	app_algo_npu_deinit();
#ifdef FLASH_AS_SRAM
	flash_deinit();
#endif
#endif
	
	hx_drv_dmac_deinit(USE_HX_DMAC_ALL);
#if defined(SPI_MASTER_SEND)
	app_spi_m_close(SPI_MST_1);
#endif
#if defined(SPI_SLAVE_SEND)
	app_spi_s_close();
#endif
#if defined(UART_PROTOCOL)
	app_uart_close(uart_id);
#endif
	
	AppCfgCustGpio_t gpio_cfg = {0};
	gpio_cfg.act_wakeupCPU_pin_cnt = 0;
#ifdef PA0_WAKEUP
	gpio_cfg.wakeupCPU_int_pin[gpio_cfg.act_wakeupCPU_pin_cnt] = HX_AON_GPIO_0;
	gpio_cfg.act_wakeupCPU_pin_cnt += 1;
#endif
#ifdef PA1_WAKEUP
	gpio_cfg.wakeupCPU_int_pin[gpio_cfg.act_wakeupCPU_pin_cnt] = HX_AON_GPIO_1;
	gpio_cfg.act_wakeupCPU_pin_cnt += 1;
#endif
	
	app_pmu_enter_sleep1(gpio_cfg, SYS_RTC_MS, 1);
}

void app_init()
{
    app_dp_cfg_t app_dp_init_cfg;
    APP_SPI_CFG_T spi_cfg;

#ifdef TFLITE_ALGO_ENABLED
#ifdef FLASH_AS_SRAM
    flash_init();
#endif
	static volatile uint8_t algo_inited = 0;

	dbg_printf(DBG_LESS_INFO, "algo_inited = %u\n", algo_inited);
	
	if(algo_inited == 0)
	{
		uint32_t ret=0;
		
		dbg_printf(DBG_LESS_INFO, "app_algo_init...\n");
		ret = app_algo_init();
		
		if(ret==0)
        	algo_inited=1;
    	else
        	dbg_printf(DBG_LESS_INFO, "app_algo_init failed\n");
	}
	else
	{
		app_npu_hw_init();
		app_algo_npu_init();
	}
#endif

    /* PINMUX. */
    app_board_pinmux_settings();

    /* SPI. */
    #ifdef SPI_MASTER_SEND /*master*/
    spi_cfg.mst_id = SPI_MST_1; /**< id*/
    spi_cfg.mst_freq = 25000000;  /**< frequency*/
    #endif

    #ifdef SPI_SLAVE_SEND /*slave*/
    spi_cfg.slv_handshake_pin = HX_GPIO_2;         /**< handshake gpio pin number*/
    spi_cfg.slv_handshake_actv_lvl = HX_GPIO_HIGH;     /**< handshake gpio pin active level*/

    dbg_printf(DBG_LESS_INFO, "HX_GPIO_2 %02x\n", HX_GPIO_2);
    dbg_printf(DBG_LESS_INFO, "HX_GPIO_HIGH %u\n", HX_GPIO_HIGH);

    dbg_printf(DBG_LESS_INFO, "spi_cfg.slv_handshake_pin %02x\n", spi_cfg.slv_handshake_pin);
    dbg_printf(DBG_LESS_INFO, "spi_cfg.slv_handshake_actv_lvl %u\n", spi_cfg.slv_handshake_actv_lvl);
    #endif

	#if defined(SPI_MASTER_SEND) || defined(SPI_SLAVE_SEND)
    app_spi_init(&spi_cfg);
    #endif

    /* UART. */
	#if defined(UART_PROTOCOL)
	if(API_SUCC != app_uart_open(uart_id, 921600))
	{
		dbg_printf(DBG_LESS_INFO, "app_uart_open fail\n");
	}
    #endif
    
	/* I2C Slave. */
	app_i2c_cmd_reg_cstm_feature(I2C_CMD_CSTM_FEATURE_1, (I2C_CMD_CSTM_CB)priv_i2c_cstm_cmd_handler);
    app_i2c_cmd_init(USE_DW_IIC_SLV_0);
	
    //
    app_dp_get_def_init_cfg(&app_dp_init_cfg);

    /* Sensor */
    app_sensor_init(&app_dp_init_cfg);

    /* Datapath. */
    app_dp_init_cfg.wdma1 = (uint32_t)g_wdma1_data;
    app_dp_init_cfg.wdma2 = (uint32_t)g_wdma2_data;
    app_dp_init_cfg.wdma3 = (uint32_t)g_wdma3_data;
    app_dp_init_cfg.jpeg_auto_fill_data = (uint32_t)g_jpeg_autofill_data;
    app_dp_init_cfg.wdma2_cyclic_buffer_cnt = (uint8_t)WDMA2_CYCLIC_BUF_CNT;
    app_dp_init(&app_dp_init_cfg);
}

void app_main()
{
	uint32_t wakeup_event;
	uint32_t wakeup_event1;
	
	dbg_printf(DBG_LESS_INFO, "app_main_freertos.\n");
    
	hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT, &wakeup_event);
	hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT1, &wakeup_event1);
    dbg_printf(DBG_LESS_INFO, "wakeup_event=0x%x,WakeupEvt1=0x%x\n", wakeup_event, wakeup_event1);

	hx_drv_timer_hw_stop(APP_PMU_SYS_RTC_ID); /*TIMER_ID_1*/

	app_init();
	
	if (xTaskCreate(main_task, "main_task", configMINIMAL_STACK_SIZE + 1024, NULL, (configMAX_PRIORITIES - 3), NULL) != pdPASS)
	{
		dbg_printf(DBG_LESS_INFO, "main task creation failed!.\r\n");
		return;
	}
	
    dbg_printf(DBG_LESS_INFO, "vTaskStartScheduler...\n");
	vTaskStartScheduler();
    dbg_printf(DBG_LESS_INFO, "vTaskStartScheduler done.\n");
	while(1)
		;
}

typedef enum
{
    CSTM_CMD_1 = 0x00,
    CSTM_CMD_2 = 0x01,
    CSTM_CMD_3 = 0x02,
} CSTM_I2C_CMDS;

/*
* i2c data[0] = I2C_CMD_CSTM_FEATURE_1 (0x81)
* i2c data[1] = CSTM_I2C_CMD (0x00 = CSTM_CMD_1, 0x01 = CSTM_CMD_2, ...)
* i2c data[2] = length of command payload (LSB)
* i2c data[3] = length of command payload (MSB)
* i2c data[4 ~ n] = payload (only present when payload length > 0)
* e.g. IR Cut enable = [0x81, 0x00, 0x01, 0x00, 0x01]
*/
static void priv_i2c_cstm_cmd_handler(uint8_t cmd_id, uint8_t *cmd_payload, uint16_t cmd_payload_len)
{
    dbg_printf(DBG_LESS_INFO, "priv_i2c_cstm_cmd_handler executed. (cmd_id = 0x%02x, cmd_payload_len = %d)\n", cmd_id, cmd_payload_len);

    switch (cmd_id)
    {
        case CSTM_CMD_1:
        {
            break;
        }
        case CSTM_CMD_2:
        {
            break;
        }
        case CSTM_CMD_3:
        {
            break;
        }
        break;
    }
}

