#include "app_platform.h"
#include "app_api.h"
#include "app_datapath.h"
#include "app_board.h"
#include "app_xdma_cfg.h"

#include "app_mipi.h"

#include "hx_drv_pmu.h"
#include "hx_drv_scu.h"
#include "hx_drv_scu_export.h"

/* local variables */
static volatile uint8_t g_wdma1_data[0/*WDMA1_SIZE*/] = {};
static volatile uint8_t g_wdma2_data[WDMA2_SLOT_SIZE*WDMA2_CYCLIC_BUF_CNT] = {};
static volatile uint8_t g_wdma3_data[WDMA3_SIZE] = {};
static volatile uint8_t g_jpeg_autofill_data[JPEG_AUTOFILL_SIZE] = {};

static void priv_i2c_cstm_cmd_handler(uint8_t cmd_id, uint8_t *cmd_payload, uint16_t cmd_payload_len);

void app_init()
{
    app_dp_cfg_t app_dp_init_cfg;
    APP_SPI_CFG_T spi_cfg;
    
    app_board_pinmux_settings();

    #ifdef SPI_MASTER_SEND /*master*/
    spi_cfg.mst_id = SPI_MST_1; /**< id*/
    spi_cfg.mst_freq = 5000000;  /**< frequency*/
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
    app_dp_init_cfg.with_passthru = 1; // Init sensor with mipi passthrough.
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

	hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT, &wakeup_event);
	hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT1, &wakeup_event1);
    dbg_printf(DBG_LESS_INFO, "wakeup_event=0x%x,WakeupEvt1=0x%x\n", wakeup_event, wakeup_event1);

    app_init();
	
    /* Set DP settings */
    app_dp_get_def_cap_settings(&app_dp_cap_set);
	
	app_mipi_set_paththrough_on(); /*mipi path through on*/
	
    while(1)
    {
        uint32_t raw_img_addr = 0, img_width = 0, img_heigh = 0;
        uint32_t jpg_addr = 0, jpg_size = 0;
		#if 1
		volatile uint32_t *a =(0x530F804C);
        volatile uint32_t *b =(0x530F8020);
        volatile uint32_t *c =(0x530F8028);
        volatile uint32_t *d =(0x530F8048);
        volatile uint32_t *e =(0x53060004);
        volatile uint32_t *f =(0x53060008);
        volatile uint32_t *g =(0x55006040);

        dbg_printf(DBG_LESS_INFO, "0x530F804C: 0x%08X\n",*a);
        dbg_printf(DBG_LESS_INFO, "0x530F8020: 0x%08X\n",*b);
        dbg_printf(DBG_LESS_INFO, "0x530F8028: 0x%08X\n",*c);
        dbg_printf(DBG_LESS_INFO, "0x530F8048: 0x%08X\n",*d);
        dbg_printf(DBG_LESS_INFO, "0x53060004: 0x%08X\n",*e);
        dbg_printf(DBG_LESS_INFO, "0x53060008: 0x%08X\n",*f);
        dbg_printf(DBG_LESS_INFO, "0x55006040: 0x%08X\n",*g);
		#endif

        /* Capture jpeg and raw image */
        dbg_printf(DBG_LESS_INFO, "[app_dp_capture_frame]\n");
        app_dp_capture_frame(&app_dp_cap_set);

        /* Capture sensor output to jpeg and raw */
        app_dp_get_frame(&raw_img_addr, &img_width, &img_heigh, &jpg_addr, &jpg_size);
        dbg_printf(DBG_LESS_INFO, "[app_dp_get_frame] raw_img_addr = %x img_width = %d img_width = %d, jpg_addr = %x, jpg_size = %d\n", raw_img_addr, img_width, img_heigh, jpg_addr, jpg_size);

        /* Send out image by SPI */
        #if defined(SPI_MASTER_SEND) || defined(SPI_SLAVE_SEND)
        app_spi_write(jpg_addr, jpg_size, DATA_TYPE_JPG);
		#endif
    }
}


