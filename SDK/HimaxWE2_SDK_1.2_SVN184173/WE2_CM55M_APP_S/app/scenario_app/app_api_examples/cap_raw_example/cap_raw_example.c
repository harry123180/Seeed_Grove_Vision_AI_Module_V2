#include "app_platform.h"
#include "app_api.h"
#include "app_datapath.h"
#include "app_board.h"
#include "app_xdma_cfg.h"

#include "hx_drv_pmu.h"
#include "hx_drv_scu.h"
#include "hx_drv_scu_export.h"

/* local variables */
static volatile uint8_t g_wdma1_data[0/*WDMA1_SIZE*/] = {};
static volatile uint8_t g_wdma2_data[DP_INP_OUT_WIDTH*DP_INP_OUT_HEIGHT] = {};
static volatile uint8_t g_wdma3_data[0/*WDMA3_SIZE*/] = {};
static volatile uint8_t g_jpeg_autofill_data[JPEG_AUTOFILL_SIZE] = {};

void app_init()
{
    app_dp_cfg_t app_dp_init_cfg;
    APP_SPI_CFG_T spi_cfg;

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

	dbg_printf(DBG_LESS_INFO, "sample_code_app.\n");
    
	hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT, &wakeup_event);
	hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT1, &wakeup_event1);
    dbg_printf(DBG_LESS_INFO, "wakeup_event=0x%x,WakeupEvt1=0x%x\n", wakeup_event, wakeup_event1);

    app_init();

    /* Set DP settings */
    app_dp_get_def_cap_settings(&app_dp_cap_set);

    while(1)
    {
        uint32_t raw_img_addr = 0, img_width = 0, img_heigh = 0;
        uint32_t jpg_addr = 0, jpg_size = 0;

        /* Capture raw image */
        dbg_printf(DBG_LESS_INFO, "[app_dp_capture_frame]\n");
        app_dp_capture_raw_frame(&app_dp_cap_set);

        /* Capture sensor output to raw */
        app_dp_get_frame(&raw_img_addr, &img_width, &img_heigh, &jpg_addr, &jpg_size);
        dbg_printf(DBG_LESS_INFO, "[app_dp_get_frame] raw_img_addr = %x img_width = %d img_height = %d, jpg_addr = %x, jpg_size = %d\n", raw_img_addr, img_width, img_heigh, jpg_addr, jpg_size);

		/* Send out Un-compressed Image */
        app_spi_write(raw_img_addr, img_width*img_heigh, DATA_TYPE_RAW_IMG);
    }
}
