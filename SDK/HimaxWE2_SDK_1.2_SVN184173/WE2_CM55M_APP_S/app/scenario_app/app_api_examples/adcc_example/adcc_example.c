#include "app_platform.h"
#include "app_adcc.h"

void app_init()
{
    /* PINMUX. */
    app_board_pinmux_settings();

	app_adcc_init();
}

void app_main()
{
	APP_ADCC_CFG_T adcc_cfg;
	
	app_init();
	app_adcc_get_def_cfg(&adcc_cfg);

	adcc_cfg.analog_vol = ADCC_VOL_1P8V;
	adcc_cfg.enable = 1;
	adcc_cfg.single_shot_enable = 0;
	//adcc_cfg.channel_mask = 0; /*Enable CH0 CH1 CH2 CH3*/
	adcc_cfg.channel_mask = ADCC_CH1 | ADCC_CH2 | ADCC_CH3; /*Enable CH0*/
	//adcc_cfg.channel_mask = ADCC_CH0 | ADCC_CH2 | ADCC_CH3; /*Enable CH1*/
	//adcc_cfg.channel_mask = ADCC_CH0 | ADCC_CH1 | ADCC_CH3; /*Enable CH2*/
	//adcc_cfg.channel_mask = ADCC_CH0 | ADCC_CH1 | ADCC_CH2; /*Enable CH3*/
	//adcc_cfg.channel_mask = ADCC_CH0 | ADCC_CH1 | ADCC_CH2 | ADCC_CH3; /*Disable CH0 CH1 CH2 CH3*/
	adcc_cfg.sample_rate[0] = 3;
	adcc_cfg.sample_rate[1] = 3;
	adcc_cfg.sample_rate[2] = 3;
	adcc_cfg.sample_rate[3] = 3;
	adcc_cfg.data_type[0] = ADCC_DATA_AVERAGE;
	adcc_cfg.data_type[1] = ADCC_DATA_AVERAGE;
	adcc_cfg.data_type[2] = ADCC_DATA_AVERAGE;
	adcc_cfg.data_type[3] = ADCC_DATA_AVERAGE;
	adcc_cfg.offset[0] = 0;
	adcc_cfg.offset[1] = 0;
	adcc_cfg.offset[2] = 0;
	adcc_cfg.offset[3] = 0;
	adcc_cfg.high_threshold[0] = 0x900;
	adcc_cfg.high_threshold[1] = 0x900;
	adcc_cfg.high_threshold[2] = 0x900;
	adcc_cfg.high_threshold[3] = 0x900;
	adcc_cfg.low_threshold[0] = 0x300;
	adcc_cfg.low_threshold[1] = 0x300;
	adcc_cfg.low_threshold[2] = 0x300;
	adcc_cfg.low_threshold[3] = 0x300;
	adcc_cfg.int_delay = 0;
	
	//adcc_cfg.int_mask = ADCC_CH0_INT_MASK | ADCC_CH1_INT_MASK | ADCC_CH2_INT_MASK | ADCC_CH3_INT_MASK | ADCC_CONSTRAIN_INT_MASK | ADCC_SG_INT_MASK; /*Enable CH0 CH1 CH2 CH3*/
	adcc_cfg.int_mask = ADCC_CH0_INT_MASK | ADCC_CONSTRAIN_INT_MASK | ADCC_SG_INT_MASK; /*Enable CH0*/
	//adcc_cfg.int_mask = ADCC_CH1_INT_MASK | ADCC_CONSTRAIN_INT_MASK | ADCC_SG_INT_MASK; /*Enable CH0*/
	//adcc_cfg.int_mask = ADCC_CH2_INT_MASK | ADCC_CONSTRAIN_INT_MASK | ADCC_SG_INT_MASK; /*Enable CH0*/
	//adcc_cfg.int_mask = ADCC_CH3_INT_MASK | ADCC_CONSTRAIN_INT_MASK | ADCC_SG_INT_MASK; /*Enable CH0*/
	adcc_cfg.data_select[0] = ADCC_DATA_SEL_MUX;
	adcc_cfg.data_select[1] = ADCC_DATA_SEL_MUX;
	adcc_cfg.data_select[2] = ADCC_DATA_SEL_MUX;
	adcc_cfg.data_select[3] = ADCC_DATA_SEL_MUX;
	
	app_adcc_set_cfg(&adcc_cfg);
	
    while(1)
    {
    	APP_ADCC_STATUS_T adcc_status;
		
		memset(&adcc_status, 0, sizeof(APP_ADCC_STATUS_T));
		app_adcc_read_status(ADCC_CHANNEL0, &adcc_status);
		board_delay_ms(500);
		
		memset(&adcc_status, 0, sizeof(APP_ADCC_STATUS_T));
		app_adcc_read_status(ADCC_CHANNEL1, &adcc_status);
		board_delay_ms(500);
		
		memset(&adcc_status, 0, sizeof(APP_ADCC_STATUS_T));
		app_adcc_read_status(ADCC_CHANNEL2, &adcc_status);
		board_delay_ms(500);
		
		memset(&adcc_status, 0, sizeof(APP_ADCC_STATUS_T));
		app_adcc_read_status(ADCC_CHANNEL3, &adcc_status);
		board_delay_ms(500);
		
		app_adcc_dump_reg();
		board_delay_ms(1000);
    }
}
