#include "app_platform.h"
#include "app_api.h"
#include "app_pdm.h"
#include "hx_drv_scu_export.h"

/* local variables */
#define PDM_BLK_NUM     16
#define PDM_BLK_SIZE    4096

static volatile uint8_t g_pdm_rec_buf[PDM_BLK_NUM*PDM_BLK_SIZE] __attribute__ ((aligned(32))) = {};

void priv_pmu_audio_cb(void* pcont)
{
	//app_pdm_config_t *ppdm_cfg = (app_pdm_config_t*)pcont;

	//dbg_printf(DBG_LESS_INFO, "priv_pmu_audio_cb\n");
}

void app_init()
{
	APP_SPI_CFG_T spi_cfg = {0};
	app_pdm_config_t app_pdm_cfg = {0};
	#if 1
	app_board_pinmux_settings();

	#ifdef SPI_MASTER_SEND /*master*/
	spi_cfg.mst_id = SPI_MST_1; /**< id*/
	spi_cfg.mst_freq = 10000000;	/**< frequency*/
	//spi_cfg.mst_freq = 5000000;  /**< frequency*/
	#endif
	
	#ifdef SPI_SLAVE_SEND /*slave*/
	spi_cfg.slv_handshake_pin = HX_AON_GPIO_0;		   /**< handshake gpio pin number*/
	spi_cfg.slv_handshake_actv_lvl = HX_GPIO_HIGH;	   /**< handshake gpio pin active level*/
	
	dbg_printf(DBG_LESS_INFO, "HX_AON_GPIO_0 %02x\n", HX_AON_GPIO_0);
	dbg_printf(DBG_LESS_INFO, "HX_GPIO_HIGH %u\n", HX_GPIO_HIGH);
	
	dbg_printf(DBG_LESS_INFO, "spi_cfg.slv_handshake_pin %02x\n", spi_cfg.slv_handshake_pin);
	dbg_printf(DBG_LESS_INFO, "spi_cfg.slv_handshake_actv_lvl %u\n", spi_cfg.slv_handshake_actv_lvl);
	#endif
	
	app_spi_init(&spi_cfg);
	#endif
	app_pdm_cfg.sample_rate = PDM_PCM_FREQ_16K;
	app_pdm_cfg.buffer_addr = g_pdm_rec_buf;
	app_pdm_cfg.block_num = PDM_BLK_NUM;
	app_pdm_cfg.block_sz = PDM_BLK_SIZE;
	app_pdm_cfg.cb_evt_blk = 2;
	app_pdm_cfg.g_audio_rx_cb_fn = priv_pmu_audio_cb;

	app_pdm_init(&app_pdm_cfg);
}

void app_main()
{
	uint32_t block_size;
	uint32_t pcm_buf_addr;
	uint32_t block;
	
	#ifdef WE2_DUAL_CORE
	dbg_printf(DBG_LESS_INFO, "CM55M_ENABLE_CM55S \n\n\n");
    hx_drv_scu_set_cm55s_state(SCU_CM55S_STATE_RESET);
    hx_drv_scu_set_cm55s_state(SCU_CM55S_STATE_NORMAL);
    hx_drv_scu_set_CM55S_CPUWAIT(SCU_CM55_CPU_RUN);
    
    dbg_printf(DBG_LESS_INFO, "app_main start(dual core).\n");
	#endif
	
	app_init();
	app_pdm_rec_start();
	
	block_size = hx_lib_audio_get_block_size();
	
	while(1)
	{
		if(true == app_pdm_is_buf_ready(&pcm_buf_addr, &block))
		{
			if(block > 0)
			{
				dbg_printf(DBG_LESS_INFO, "pcm_buf_addr = 0x%x\n", pcm_buf_addr);
				dbg_printf(DBG_LESS_INFO, "block = 0x%x\n", block);
				
				app_spi_write((uint32_t)pcm_buf_addr, (uint32_t)block*block_size, DATA_TYPE_PDM);
				
				app_pdm_proc_buf_done(block);
			}
		}

		board_delay_ms(10);
	}

	app_pdm_rec_stop();
}
