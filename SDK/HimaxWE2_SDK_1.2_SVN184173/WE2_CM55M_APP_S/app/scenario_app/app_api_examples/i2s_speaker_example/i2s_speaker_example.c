#include "app_platform.h"
#include "app_sys_info_cmd.h"
#include "app_api.h"
#include "app_speaker.h"
#include "hx_drv_scu_export.h"
//#include "spi_eeprom_comm.h"

#define FRAME_SIZE_SAMPLES  0x10000					/*64KB*/

volatile uint8_t pcm_data[] __attribute__ ((section(".data.pcm_data"), aligned(4))) = {
#include "hello.in"
};

void priv_set_i2s_i2cm_pinmux()
{
#ifdef HX6538_ISM028_03M
	/*Set 3, the  */
    // I2C Master
	hx_drv_scu_set_PB6_pinmux(SCU_PB6_PINMUX_I2C_M_SDA, 1);
	hx_drv_scu_set_PB7_pinmux(SCU_PB7_PINMUX_I2C_M_SCL, 1);
    // I2S
	hx_drv_scu_set_PB2_pinmux(SCU_PB2_PINMUX_I2S_M_SDO, 1);
	hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_I2S_S_SDI, 1);
	hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_I2S_M_SCLK, 1);
	hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_I2S_M_WS, 1);
#endif
}

void app_init()
{
	app_board_pinmux_settings();
    priv_set_i2s_i2cm_pinmux();

	app_speaker_i2s_init();
	app_speaker_init();

}

void app_main()
{
	app_init();

	while(1)
	{
		if(app_speaker_get_tx_status() == 0)
		{
			dbg_printf(DBG_LESS_INFO, "addr = 0x%x, sizeof(pcm_data) = 0x%x\n", pcm_data, sizeof(pcm_data));
			app_speaker_output((uint32_t)pcm_data, (uint32_t)sizeof(pcm_data), FRAME_SIZE_SAMPLES);
		}
		
		dbg_printf(DBG_LESS_INFO, ".");
		board_delay_ms(1000);
	}
}
