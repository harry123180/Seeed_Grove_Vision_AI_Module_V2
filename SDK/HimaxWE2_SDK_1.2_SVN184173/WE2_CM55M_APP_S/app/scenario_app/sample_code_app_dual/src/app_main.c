#include "app_platform.h"
#include "app_api.h"
#include "app_sys_info_cmd.h"
#include "app_datapath.h"
#include "app_board.h"
#include "app_xdma_cfg.h"
#include "app_i2c_cmd.h"

#include "app_evt_mb.h"

#ifdef TFLITE_ALGO_ENABLED
#include "app_algo.h"
#endif
#ifdef FLASH_AS_SRAM
#include "app_flash.h"
#endif

#include "hx_drv_pmu.h"
#include "hx_drv_scu.h"
#include "hx_drv_scu_export.h"

typedef enum APP_MASTER_STATE_S
{
	APP_MASTER_STATE_IDLE                		= 0,
	APP_MASTER_STATE_META_TX					= 0x01,

	APP_MASTER_STATE_RES						= 0x80000000,
} APP_SLAVE_STATE_E;

/* local variables */
static uint16_t data_types = TX_DATA_JPEG | TX_DATA_META;

static volatile uint8_t g_wdma1_data[0/*WDMA1_SIZE*/] = {};
static volatile uint8_t g_wdma2_data[WDMA2_SLOT_SIZE*WDMA2_CYCLIC_BUF_CNT] = {};
static volatile uint8_t g_wdma3_data[WDMA3_SIZE] = {};
static volatile uint8_t g_jpeg_autofill_data[JPEG_AUTOFILL_SIZE] = {};

static volatile uint32_t g_share_raw_addr = 0;
static volatile uint32_t g_share_raw_width = 0;
static volatile uint32_t g_share_raw_height = 0;
static volatile uint32_t g_share_jpeg_addr = 0;
static volatile uint32_t g_share_jpeg_size = 0;

#ifdef TFLITE_ALGO_ENABLED
static volatile ALGO_RESULT g_share_algo_result;
#endif

static volatile APP_DP_SHARE_ADDR_T g_share_dp_data_addr = { 
								.wdma1_data = &g_wdma1_data,
								.wdma2_data = &g_wdma2_data,
								.wdma3_data = &g_wdma3_data,
								.jpeg_autofill_data = &g_jpeg_autofill_data,
								.wdma2_cyclic_buffer_cnt = WDMA2_CYCLIC_BUF_CNT,
								.raw_addr = &g_share_raw_addr,
								.raw_width = &g_share_raw_width,
								.raw_height = &g_share_raw_height,
								.jpeg_addr = &g_share_jpeg_addr,
								.jpeg_size = &g_share_jpeg_size
							};

static uint32_t app_master_state = APP_MASTER_STATE_IDLE;

static void priv_i2c_cstm_cmd_handler(uint8_t cmd_id, uint8_t *cmd_payload, uint16_t cmd_payload_len);

/***************************************************
 * Function Implementation
 **************************************************/
// raise MB event
static void priv_evt_mb_master_callback_fun(uint32_t event)
{
	//dbg_printf(DBG_LESS_INFO, "priv_evt_mb_master_callback_fun: 0x%08X.....trigger........\n", event);

	//undefine event
	if(event == APP_EVT_MB_MASTER_EVT_NO)
		return;

	app_evt_mb_master_clr_evt(event);

	switch (event)
	{
		case APP_EVT_MB_MASTER_EVT_META_TX_DONE:
			app_master_state = APP_MASTER_STATE_IDLE;
			break;
		default:
			break;
	}
}

void app_init()
{
    app_dp_cfg_t app_dp_init_cfg;
    APP_SPI_CFG_T spi_cfg;

#ifdef TFLITE_ALGO_ENABLED
#ifdef FLASH_AS_SRAM
    flash_init();
#endif
    dbg_printf(DBG_LESS_INFO, "app_algo_init...\n");
    app_algo_init();
#endif

    /* PINMUX. */
    app_board_pinmux_settings();

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
	app_dp_cap_t app_dp_cap_set;
	uint32_t wakeup_event;
	uint32_t wakeup_event1;

	dbg_printf(DBG_LESS_INFO, "sample_code_app_dual.\n");
    
	hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT, &wakeup_event);
	hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT1, &wakeup_event1);
    dbg_printf(DBG_LESS_INFO, "wakeup_event=0x%x,WakeupEvt1=0x%x\n", wakeup_event, wakeup_event1);

	app_evt_mb_master_init(priv_evt_mb_master_callback_fun);
	
	app_evt_mb_set_share_data(APP_EVT_MB_SHAREDATA_IDX_XDMA_ADDR, (uint32_t)&g_share_dp_data_addr);
	dbg_printf(DBG_LESS_INFO, "&g_share_dp_data_addr = 0x%08x\n", &g_share_dp_data_addr);
	dbg_printf(DBG_LESS_INFO, "wdma1_data = 0x%08x\n", g_share_dp_data_addr.wdma1_data);
	dbg_printf(DBG_LESS_INFO, "wdma2_data = 0x%08x\n", g_share_dp_data_addr.wdma2_data);
	dbg_printf(DBG_LESS_INFO, "wdma3_data = 0x%08x\n", g_share_dp_data_addr.wdma3_data);
	dbg_printf(DBG_LESS_INFO, "jpeg_autofill_data = 0x%08x\n", g_share_dp_data_addr.jpeg_autofill_data);
	dbg_printf(DBG_LESS_INFO, "raw_addr = 0x%08x\n", g_share_dp_data_addr.raw_addr);
	dbg_printf(DBG_LESS_INFO, "raw_width = 0x%08x\n", g_share_dp_data_addr.raw_width);
	dbg_printf(DBG_LESS_INFO, "raw_height = 0x%08x\n", g_share_dp_data_addr.raw_height);
	dbg_printf(DBG_LESS_INFO, "jpeg_addr = 0x%08x\n", g_share_dp_data_addr.jpeg_addr);
	dbg_printf(DBG_LESS_INFO, "jpeg_size = 0x%08x\n", g_share_dp_data_addr.jpeg_size);
	
	#ifdef TFLITE_ALGO_ENABLED
	app_evt_mb_set_share_data(APP_EVT_MB_SHAREDATA_IDX_ALGOMETA_ADDR, (uint32_t)&g_share_algo_result);
	dbg_printf(DBG_LESS_INFO, "algo_result = 0x%08x\n", &g_share_algo_result);
	#endif
	
    dbg_printf(DBG_LESS_INFO, "app_main start(dual core).\n");

    app_init();

	dbg_printf(DBG_LESS_INFO, "CM55M_ENABLE_CM55S \n\n\n");
	hx_drv_scu_set_cm55s_state(SCU_CM55S_STATE_RESET);
	hx_drv_scu_set_cm55s_state(SCU_CM55S_STATE_NORMAL);
	hx_drv_scu_set_CM55S_CPUWAIT(SCU_CM55_CPU_RUN);
	
	/* Set DP settings */
    app_dp_get_def_cap_settings(&app_dp_cap_set);

	while(1)
	{
		uint32_t raw_img_addr = 0, raw_img_width = 0, raw_img_height = 0;
        uint32_t jpg_addr = 0, jpg_size = 0;
				
		switch(app_master_state)
		{
			case APP_MASTER_STATE_IDLE:
			{
				dbg_printf(DBG_LESS_INFO, "[app_dp_capture_frame]\n");
				
				//acquire xdma mutex
				while(APP_EVT_MB_MUTEX_STATUS_NOT_GET == app_evt_mb_acquire_mutex(APP_EVT_MB_MUTEX_IDX_XDMA_UPDATE, 1024));

				/* Capture jpeg and raw image */
				app_dp_capture_frame(&app_dp_cap_set);
				
				/* Capture sensor output to jpeg and raw */
				app_dp_get_frame(&raw_img_addr, &raw_img_width, &raw_img_height, &jpg_addr, &jpg_size);

				g_share_raw_addr = raw_img_addr;
				g_share_raw_width = raw_img_width;
				g_share_raw_height = raw_img_height;
				g_share_jpeg_addr = jpg_addr;
				g_share_jpeg_size = jpg_size;
				
				hx_CleanDCache_by_Addr((void *)&g_share_raw_addr, sizeof(uint32_t));
				hx_CleanDCache_by_Addr((void *)&g_share_raw_width, sizeof(uint32_t));
				hx_CleanDCache_by_Addr((void *)&g_share_raw_height, sizeof(uint32_t));
				hx_CleanDCache_by_Addr((void *)&g_share_jpeg_addr, sizeof(uint32_t));
				hx_CleanDCache_by_Addr((void *)&g_share_jpeg_size, sizeof(uint32_t));
				
				dbg_printf(DBG_LESS_INFO, "[app_dp_get_frame] raw_img_addr = %x img_width = %d img_height = %d, jpg_addr = %x, jpg_size = %d\n", raw_img_addr, raw_img_width, raw_img_height, jpg_addr, jpg_size);
				dbg_printf(DBG_LESS_INFO, "[app_dp_get_frame] raw_img_addr = %x img_width = %d img_height = %d, jpg_addr = %x, jpg_size = %d\n", g_share_raw_addr, g_share_raw_width, g_share_raw_height, g_share_jpeg_addr, g_share_jpeg_size);
				
				#ifdef TFLITE_ALGO_ENABLED
		        //acquire algo meta mutex
				while(APP_EVT_MB_MUTEX_STATUS_NOT_GET == app_evt_mb_acquire_mutex(APP_EVT_MB_MUTEX_IDX_ALGOMETA_UPDATE, 1024));

				/* Execute Algorithm */
		        memset(&g_share_algo_result, 0, sizeof(g_share_algo_result));
		        app_algo_run(raw_img_addr, raw_img_width, raw_img_height, &g_share_algo_result);
				hx_CleanDCache_by_Addr((void *)&g_share_algo_result, sizeof(g_share_algo_result));

				//release algo meta mutex
				app_evt_mb_release_mutex(APP_EVT_MB_MUTEX_IDX_ALGOMETA_UPDATE);
				#endif

				//release xdma mutex
				app_evt_mb_release_mutex(APP_EVT_MB_MUTEX_IDX_XDMA_UPDATE);
				
				app_evt_mb_master_trig_evt(APP_EVT_MB_SLAVE_EVT_META_READY);
				app_master_state = APP_MASTER_STATE_META_TX;
			}
			break;

			case APP_MASTER_STATE_META_TX:
			{
			}
			break;
			
			default:
			break;
		}
		
		board_delay_ms(10);;
	}
}

