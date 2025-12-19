#include "app_platform.h"
#include "app_api.h"
#include "app_sys_info_cmd.h"
#include "app_board.h"

#include "app_i2c_cmd.h"
#if defined(UART_PROTOCOL)
#include "app_uart_cmd.h"
#endif
#include "app_evt_mb.h"
#include "algo_metadata.h"

#include "hx_drv_pmu.h"
#include "hx_drv_scu.h"
#include "hx_drv_scu_export.h"

#include "timer_interface.h"

typedef enum APP_SLAVE_STATE_S
{
	APP_SLAVE_STATE_IDLE                		= 0,
	APP_SLAVE_STATE_ALGOMETA_READY				= 0x01,
	
	APP_SLAVE_STATE_RES							= 0x80000000,
} APP_SLAVE_STATE_E;

/* local variables */
static uint16_t data_types = TX_DATA_JPEG | TX_DATA_META;

#if defined(UART_PROTOCOL)
static uint32_t uart_id = UART_1_ID;
#endif

static volatile ALGO_RESULT *palgo_result = NULL;
static volatile APP_DP_SHARE_ADDR_T *pdp_data_addr = NULL;

static volatile uint32_t wdma1_data;
static volatile uint32_t wdma2_data;
static volatile uint32_t wdma3_data;
static volatile uint32_t jpeg_autofill_data;
static volatile uint32_t wdma2_cyclic_buffer_cnt;

static uint32_t app_slave_state = APP_SLAVE_STATE_IDLE;

static void priv_i2c_cstm_cmd_handler(uint8_t cmd_id, uint8_t *cmd_payload, uint16_t cmd_payload_len);

/***************************************************
 * Function Implementation
 **************************************************/
// raise MB event
static void priv_evt_mb_slave_callback_fun(uint32_t event)
{
	//dbg_printf(DBG_LESS_INFO, "priv_evt_mb_slave_callback_fun: 0x%08X.....trigger........\n", event);

	//undefine event
	if(event == APP_EVT_MB_SLAVE_EVT_NO)
		return;

	app_evt_mb_slave_clr_evt(event);

	switch (event)
	{
		case APP_EVT_MB_SLAVE_EVT_META_READY:
		app_slave_state = APP_SLAVE_STATE_ALGOMETA_READY;
	    break;

		default:
		break;
	}
}

void app_init()
{
	APP_SPI_CFG_T spi_cfg;

	/* PINMUX. */
	app_board_pinmux_settings();

	/* SPI. */
    #ifdef SPI_MASTER_SEND /*master*/
	spi_cfg.mst_id = SPI_MST_1; /**< id*/
	spi_cfg.mst_freq = 25000000;  /**< frequency*/
    #endif
	
    #ifdef SPI_SLAVE_SEND /*slave*/
	spi_cfg.slv_handshake_pin = HX_GPIO_2;		   /**< handshake gpio pin number*/
	spi_cfg.slv_handshake_actv_lvl = HX_GPIO_HIGH;	   /**< handshake gpio pin active level*/

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
	app_i2c_cmd_init(USE_DW_IIC_SLV_0);
	app_i2c_cmd_reg_cstm_feature(I2C_CMD_CSTM_FEATURE_1, (I2C_CMD_CSTM_CB)priv_i2c_cstm_cmd_handler);
	
}

void app_main()
{
    dbg_printf(DBG_LESS_INFO, "CM55S sample_code_app_dualcore_s\n");

	app_evt_mb_slave_init(priv_evt_mb_slave_callback_fun);
	
	app_evt_mb_get_share_data(APP_EVT_MB_SHAREDATA_IDX_XDMA_ADDR, (uint32_t**)&pdp_data_addr);
	dbg_printf(DBG_LESS_INFO, "pdp_data_addr = 0x%08x\n", pdp_data_addr);

	wdma1_data = pdp_data_addr->wdma1_data;
	wdma2_data = pdp_data_addr->wdma2_data;
	wdma3_data = pdp_data_addr->wdma3_data;
	jpeg_autofill_data = pdp_data_addr->jpeg_autofill_data;
	wdma2_cyclic_buffer_cnt = pdp_data_addr->wdma2_cyclic_buffer_cnt;

	app_evt_mb_get_share_data(APP_EVT_MB_SHAREDATA_IDX_ALGOMETA_ADDR, (uint32_t**)&palgo_result);
	dbg_printf(DBG_LESS_INFO, "palgo_result = 0x%08x\n", palgo_result);
	
	app_init();
	
	while(1)
	{
		static uint32_t raw_img_addr=0, img_width=0, img_height=0;
		static uint32_t jpg_addr=0, jpg_size=0;

		app_i2c_cmd_polling(USE_DW_IIC_SLV_0);

		#if defined(UART_PROTOCOL)
		app_uart_cmd_polling(uart_id);
		#endif
		
		switch (app_slave_state)
		{
			case APP_SLAVE_STATE_IDLE:
			{
			}
			break;
			
			case APP_SLAVE_STATE_ALGOMETA_READY:
			{
				//acquire xdma mutex
				while(APP_EVT_MB_MUTEX_STATUS_NOT_GET == app_evt_mb_acquire_mutex(APP_EVT_MB_MUTEX_IDX_XDMA_UPDATE, 1024));

				hx_InvalidateDCache_by_Addr((void *)pdp_data_addr->raw_addr, sizeof(uint32_t));
				hx_InvalidateDCache_by_Addr((void *)pdp_data_addr->raw_width, sizeof(uint32_t));
				hx_InvalidateDCache_by_Addr((void *)pdp_data_addr->raw_height, sizeof(uint32_t));
				hx_InvalidateDCache_by_Addr((void *)pdp_data_addr->jpeg_addr, sizeof(uint32_t));
				hx_InvalidateDCache_by_Addr((void *)pdp_data_addr->jpeg_size, sizeof(uint32_t));
				
				raw_img_addr = *((uint32_t*)pdp_data_addr->raw_addr);
				img_width = *((uint32_t*)pdp_data_addr->raw_width);
				img_height = *((uint32_t*)pdp_data_addr->raw_height);
				jpg_addr = *((uint32_t*)pdp_data_addr->jpeg_addr);
				jpg_size = *((uint32_t*)pdp_data_addr->jpeg_size);
				
				//dbg_printf(DBG_LESS_INFO, "[meta data ready] raw_img_addr = %x img_width = %d img_height = %d, jpg_addr = %x, jpg_size = %d\n", *praw_addr, *praw_width, *praw_height, *pjpeg_addr, *pjpeg_size);
				dbg_printf(DBG_LESS_INFO, "[meta data ready] raw_img_addr = %x img_width = %d img_height = %d, jpg_addr = %x, jpg_size = %d\n", raw_img_addr, img_width, img_height, jpg_addr, jpg_size);

				/* Send out JPG */
				if (TX_DATA_JPEG == (data_types & TX_DATA_JPEG))
				{
			        #if defined(SPI_MASTER_SEND) || defined(SPI_SLAVE_SEND)
					if (jpg_addr != 0)
					{
						app_spi_write(jpg_addr, jpg_size, DATA_TYPE_JPG);
					}
			        #endif
				}

				/* Send out Un-compressed Image */
				if (TX_DATA_RAW == (data_types & TX_DATA_RAW))
				{
			        #if defined(SPI_MASTER_SEND) || defined(SPI_SLAVE_SEND)
					if(raw_img_addr != 0)
					{
						app_spi_write(raw_img_addr, img_width*img_height, DATA_TYPE_RAW_IMG);
					}
		        	#endif
				}
				
				//release xdma mutex
				app_evt_mb_release_mutex(APP_EVT_MB_MUTEX_IDX_XDMA_UPDATE);
				
				//acquire algo meta mutex
				while(APP_EVT_MB_MUTEX_STATUS_NOT_GET == app_evt_mb_acquire_mutex(APP_EVT_MB_MUTEX_IDX_ALGOMETA_UPDATE, 1024));

		        /* Send out Metadata */
		        if (TX_DATA_META == (data_types & TX_DATA_META))
				{
			        #if defined(SPI_MASTER_SEND) || defined(SPI_SLAVE_SEND)
					if(palgo_result)
					{
						hx_InvalidateDCache_by_Addr((void *)palgo_result, sizeof(ALGO_RESULT));
				        app_spi_write((uint32_t)palgo_result, sizeof(ALGO_RESULT), DATA_TYPE_META_DATA);
						
					}
		        	#endif
		        }
				
				//release algo meta mutex
				app_evt_mb_release_mutex(APP_EVT_MB_MUTEX_IDX_ALGOMETA_UPDATE);

				app_evt_mb_slave_trig_evt(APP_EVT_MB_MASTER_EVT_META_TX_DONE);
				app_slave_state = APP_SLAVE_STATE_IDLE;
			}
			break;
			
			default:
			break;
		}

		board_delay_ms(10);
	}
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

