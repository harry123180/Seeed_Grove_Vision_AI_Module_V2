#include "app_platform.h"
#include "app_api.h"
#include "app_i2c_m.h"
#include "hx_drv_scu_export.h"

/* local variables */
#if defined(HX6538_AIoT_EVB_QFN88_V10) || defined(HX6538_AIoT_EVB_LQFP128_V10)
#define LM3559_ADDR_NUM 16

uint8_t LM3559RegAddr[LM3559_ADDR_NUM] = 
{
	0x10, //0: Enable Register
	0x11,
	0x12,
	0x13,
	0x14,
	0x20,
	0x30,
	0x31,
	0x80,
	0x81,
	0xA0, //10: Torch Brightness Register
	0xB0,
	0xC0,
	0xD0,
	0xE0, //14: Configuration Register
	0xF0
};
#define I2C_M_ID (I2C_1_ID)

#define LM3559_SLAVE_ADDR 0x53


int8_t app_white_led_init()
{
	dbg_printf(DBG_LESS_INFO,"app_ir_init()\n");
	/* LM3559TLX/NOPB */
	int32_t LM3559I2Cret = 0;


	/* Int led torch */
	// Torch Brightness Register (0xA0)
	
	uint8_t LM3559WBuffer = 0x10; // default val

	LM3559I2Cret = app_i2c_mst_write(I2C_M_ID, LM3559_SLAVE_ADDR, &LM3559RegAddr[10], 1, &LM3559WBuffer, 1);

	if(LM3559I2Cret < E_OK)
	{
		dbg_printf(DBG_LESS_INFO,"app_ir_init(): hx_drv_i2cm_write_data fail\n");
	}

	
	/* Int led flash brightness */
	// Flash Brightness Register (0xB0)
	
	LM3559WBuffer = 0x00; // default val

	LM3559I2Cret = app_i2c_mst_write(I2C_M_ID, LM3559_SLAVE_ADDR, &LM3559RegAddr[11], 1, &LM3559WBuffer, 1);

	if(LM3559I2Cret < E_OK)
	{
		dbg_printf(DBG_LESS_INFO,"app_ir_init(): hx_drv_i2cm_write_data fail\n");
	}

}

void app_white_led_on()
{
	dbg_printf(DBG_LESS_INFO,"priv_test_white_led_on()\n");

	int32_t LM3559I2Cret = 0;
	
	// Enable Register (0x10)
	uint8_t LM3559WBuffer = 0x0A; /* Turn ON IR LED on torch mode */

	LM3559I2Cret = app_i2c_mst_write(I2C_M_ID, LM3559_SLAVE_ADDR, &LM3559RegAddr[0], 1, &LM3559WBuffer, 1);
	if(LM3559I2Cret < E_OK)
	{
		dbg_printf(DBG_LESS_INFO,"priv_test_ir_led_on(): hx_drv_i2cm_write_data fail\n");
	}	
}

void app_white_led_off()
{
	dbg_printf(DBG_LESS_INFO,"priv_test_white_led_off()\n");
	int32_t LM3559I2Cret = 0;
	
	// Enable Register (0x10)
	uint8_t LM3559WBuffer = 0x00; /* Turn OFF IR LED 1*/

	LM3559I2Cret = app_i2c_mst_write(I2C_M_ID, LM3559_SLAVE_ADDR, &LM3559RegAddr[0], 1, &LM3559WBuffer, 1);
	if(LM3559I2Cret < E_OK)
	{
		dbg_printf(DBG_LESS_INFO,"priv_test_ir_led_off(): hx_drv_i2cm_write_data fail\n");
	}
}

#elif defined(HX6538_ISM028_03M)

#define LM3643_ADDR_NUM 14
#define I2C_M_ID (I2C_0_ID)
#define LM3643_SLAVE_ADDR 0x67

uint8_t LM3643RegAddr[LM3643_ADDR_NUM] = 
{
	0x00,
	0x01,
	0x02,
	0x03,
	0x04,
	0x05,
	0x06,
	0x07,
	0x08,
	0x09,
	0x0A,
	0x0B,
	0x0C,
	0x0D,
};

uint8_t LM3643RBuffer[LM3643_ADDR_NUM] = {0x00};



int8_t app_ir_led_init()
{
	dbg_printf(DBG_LESS_INFO,"app_ir_init()\n");
	/* LM3643TLX/NOPB */
	int32_t LM3643I2Cret = 0;


	/* Int led torch */
	uint8_t LM3643WBuffer = 0x60;

	LM3643I2Cret = app_i2c_mst_write(I2C_M_ID, LM3643_SLAVE_ADDR, &LM3643RegAddr[5], 1, &LM3643WBuffer, 1);

	if(LM3643I2Cret < E_OK)
	{
		dbg_printf(DBG_LESS_INFO,"app_ir_init(): hx_drv_i2cm_write_data fail\n");
	}

	
	/* Int led flash brightness */
	
	LM3643WBuffer = 0x60; // default val

	LM3643I2Cret = app_i2c_mst_write(I2C_M_ID, LM3643_SLAVE_ADDR, &LM3643RegAddr[6], 1, &LM3643WBuffer, 1);

	if(LM3643I2Cret < E_OK)
	{
		dbg_printf(DBG_LESS_INFO,"app_ir_init(): hx_drv_i2cm_write_data fail\n");
	}

	//Read register
    for(uint32_t LM3643Index = 1; LM3643Index < LM3643_ADDR_NUM; LM3643Index++)
    {
        LM3643I2Cret = app_i2c_mst_write_read(I2C_M_ID, LM3643_SLAVE_ADDR, &LM3643RegAddr[LM3643Index], 1, &LM3643RBuffer[LM3643Index], 1);
        if(LM3643I2Cret < E_OK)
        {
            dbg_printf(DBG_LESS_INFO,"hx_drv_i2cm_writeread fail\n");
        }
        dbg_printf(DBG_LESS_INFO,"LM3643Reg[%02x] = 0x%02x\n", LM3643Index, LM3643RBuffer[LM3643Index]);
    }
}

void app_ir_led_on()
{
	dbg_printf(DBG_LESS_INFO,"priv_test_ir_led_on()\n");

	int32_t LM3643I2Cret = 0;
	
	// Enable Register (0x01)
	uint8_t LM3643WBuffer = 0x0B; /* Turn on IR LED led 1 & 2*/

	LM3643I2Cret = app_i2c_mst_write(I2C_M_ID, LM3643_SLAVE_ADDR, &LM3643RegAddr[1], 1, &LM3643WBuffer, 1);
	if(LM3643I2Cret < E_OK)
	{
		dbg_printf(DBG_LESS_INFO,"priv_test_white_led_on(): hx_drv_i2cm_write_data fail\n");
	}	
}

void app_ir_led_off()
{
	dbg_printf(DBG_LESS_INFO,"priv_test_ir_led_off\n");

	int32_t LM3643I2Cret = 0;
	
	// Enable Register (0x01)
	uint8_t LM3643WBuffer = 0x00; /* Turn on IR led LED 1 & 2*/

	LM3643I2Cret = app_i2c_mst_write(I2C_M_ID, LM3643_SLAVE_ADDR, &LM3643RegAddr[1], 1, &LM3643WBuffer, 1);
	if(LM3643I2Cret < E_OK)
	{
		dbg_printf(DBG_LESS_INFO,"priv_test_white_led_off(): hx_drv_i2cm_write_data fail\n");
	}	
}
#endif

void app_main()
{
    int i;

    app_board_pinmux_settings();
    app_i2c_mst_init(I2C_M_ID, I2C_SPEED_400K);
#if defined(HX6538_AIoT_EVB_QFN88_V10) || defined(HX6538_AIoT_EVB_LQFP128_V10)
    app_white_led_init();
#elif defined(HX6538_ISM028_03M)
    app_ir_led_init();
#endif

    for (i = 0; i < 20; i++) {
#if defined(HX6538_AIoT_EVB_QFN88_V10) || defined(HX6538_AIoT_EVB_LQFP128_V10)        
        app_white_led_on();
#elif defined(HX6538_ISM028_03M)
        app_ir_led_on();
#endif
        app_delay_ms(100);
#if defined(HX6538_AIoT_EVB_QFN88_V10) || defined(HX6538_AIoT_EVB_LQFP128_V10)        
        app_white_led_off();
#elif defined(HX6538_ISM028_03M)
        app_ir_led_off();
#endif
        app_delay_ms(100);
    }
}
