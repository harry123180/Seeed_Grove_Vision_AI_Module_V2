#include "app_platform.h"
#include "hx_drv_scu_export.h"
#include "hx_drv_scu.h"
#include "hx_drv_gpio.h"


void priv_set_128_socket_board_pinmux(void);
void priv_set_88_socket_board_pinmux(void);
void priv_set_65_socket_board_pinmux(void);
void priv_set_128_aiot_evb_pinmux(void);
void priv_set_88_aiot_evb_pinmux(void);

void app_board_pinmux_settings(void)
{
#if defined(HX6538_AIoT_EVB_LQFP128_V10)
    priv_set_128_aiot_evb_pinmux();
#elif defined(HX6538_AIoT_EVB_QFN88_V10)
    priv_set_88_aiot_evb_pinmux();
#elif defined(HX6538_ISM028_03M)
    priv_set_88_ism028_03m_pinmux();
#elif defined(HX6538_NB_EVB_WLCSP65)
    priv_set_65_socket_board_pinmux();
#endif
}

void priv_set_128_socket_board_pinmux(void)
{
#ifdef NSC
	veneer_sys_set_PB9_pinmux(SCU_PB9_PINMUX_PDM_CLK_1, 1); /* PDM_PIN_CLK*/
	veneer_sys_set_PB10_pinmux(SCU_PB10_PINMUX_PDM_DATA0_1, 1); /*PDM_PIN_DATA_0*/
	veneer_sys_set_PB11_pinmux(SCU_PB11_PINMUX_PDM_DATA1_1, 1); /*PDM_PIN_DATA_1*/
	veneer_sys_set_PC3_pinmux(SCU_PC3_PINMUX_PDM_DATA2); /*PDM_PIN_DATA_2*/
	veneer_sys_set_PC4_pinmux(SCU_PC4_PINMUX_PDM_DATA3); /*PDM_PIN_DATA_3*/
#else
	hx_drv_scu_set_PB9_pinmux(SCU_PB9_PINMUX_PDM_CLK_1, 1); /*PDM_PIN_CLK*/
	hx_drv_scu_set_PB10_pinmux(SCU_PB10_PINMUX_PDM_DATA0_1, 1); /*PDM_PIN_DATA_0*/
	hx_drv_scu_set_PB11_pinmux(SCU_PB11_PINMUX_PDM_DATA1_1, 1); /*PDM_PIN_DATA_1*/
	hx_drv_scu_set_PC3_pinmux(SCU_PC3_PINMUX_PDM_DATA2); /*PDM_PIN_DATA_2*/
	hx_drv_scu_set_PC4_pinmux(SCU_PC4_PINMUX_PDM_DATA3); /*PDM_PIN_DATA_3*/
#endif

    /* SPI Master */
    hx_drv_scu_set_PB2_pinmux(SCU_PB2_PINMUX_SPI_M_DO_1, 1);
    hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_SPI_M_DI_1, 1);
    hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_SPI_M_SCLK_1, 1);
    hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SPI_M_CS_1, 1);

    /* SPI Slave */
    hx_drv_scu_set_SPIS_DO_pinmux(SCU_SPIS_DO_PINMUX_SPI_S_DO);
    hx_drv_scu_set_SPIS_DI_pinmux(SCU_SPIS_DI_PINMUX_SPI_S_DI);
    hx_drv_scu_set_SPIS_SCLK_pinmux(SCU_SPIS_SCLK_PINMUX_SPI_S_SCLK);
    hx_drv_scu_set_SPIS_CS_pinmux(SCU_SPIS_CS_PINMUX_SPI_S_CS);

    /* AON_GPIO_0 for SPI Slave Handshake(to FT_GPIO_2) */
    hx_drv_scu_set_PA0_pinmux(SCU_PA0_PINMUX_AON_GPIO0_2, 1);
    hx_drv_gpio_set_output(AON_GPIO0, GPIO_OUT_LOW);
}

void priv_set_88_socket_board_pinmux(void)
{
#ifdef SPI_MASTER_SEND
    /* SPI Master */
    hx_drv_scu_set_PB2_pinmux(SCU_PB2_PINMUX_SPI_M_DO_1, 1);
    hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_SPI_M_DI_1, 1);
    hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_SPI_M_SCLK_1, 1);
    hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SPI_M_CS_1, 1);
#elif defined(SPI_SLAVE_SEND)
    /* SPI Slave */
    hx_drv_scu_set_PB2_pinmux(SCU_PB2_PINMUX_SPI_S_DO, 1);
    hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_SPI_S_DI, 1);
    hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_SPI_S_CLK, 1);
    hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SPI_S_CS, 1);
	
	/* AON_GPIO_0 for SPI Slave Handshake(to FT_GPIO_2) */
    hx_drv_scu_set_PA0_pinmux(SCU_PA0_PINMUX_AON_GPIO0_2, 1);
    hx_drv_gpio_set_output(AON_GPIO0, GPIO_OUT_LOW);
#endif

#if defined(UART_PROTOCOL) && defined(WE2_SINGLE_CORE)
    /* UART_1 */
    hx_drv_scu_set_PB6_pinmux(SCU_PB6_PINMUX_UART1_RX, 1);
    hx_drv_scu_set_PB7_pinmux(SCU_PB7_PINMUX_UART1_TX, 1);
#endif

    /* AON_GPIO_1 for GPIO wakeup */
    hx_drv_scu_set_PA1_pinmux(SCU_PA1_PINMUX_AON_GPIO1, 1);
    hx_drv_gpio_set_input(AON_GPIO1);
}

#ifdef HX6538_NB_EVB_WLCSP65
void priv_set_65_socket_board_pinmux(void)
{
    /* Sensor IO */
    #if defined(HM0360_MONO) || defined(HM0360_BAYER)
    /**
     * Switch pinmux for 65pin: HM0360 (No Trigger)
     *
     * SEN_INT -----> SEN_FVALID
     * SEN_GPIO ----> SEN_FVALID
     * SEN_XSLEEP --> SEN_XSLEPP
     * SEN_TRIGER --> Keep low
     */
    hx_drv_scu_set_SEN_INT_pinmux(SCU_SEN_INT_PINMUX_FVALID);
    hx_drv_scu_set_SEN_GPIO_pinmux(SCU_SEN_GPIO_PINMUX_LVALID);
    hx_drv_scu_set_SEN_XSLEEP_pinmux(SCU_SEN_XSLEEP_PINMUX_SEN_XSLEEP_0);
	
    dbg_printf(DBG_LESS_INFO, "hx_drv_scu_set_SEN_INT_pinmux(SCU_SEN_INT_PINMUX_FVALID)\n");
    dbg_printf(DBG_LESS_INFO, "hx_drv_scu_set_SEN_GPIO_pinmux(SCU_SEN_GPIO_PINMUX_LVALID)\n");
    dbg_printf(DBG_LESS_INFO, "hx_drv_scu_set_SEN_XSLEEP_pinmux(SCU_SEN_XSLEEP_PINMUX_SEN_XSLEEP_0)\n");
	#elif defined(HM2170_BAYER)
	hx_drv_scu_set_SEN_INT_pinmux(SCU_SEN_INT_PINMUX_GPIO10);
	hx_drv_scu_set_SEN_GPIO_pinmux(SCU_SEN_GPIO_PINMUX_SENAONGPIO2_0);
    hx_drv_scu_set_SEN_XSLEEP_pinmux(SCU_SEN_XSLEEP_PINMUX_SEN_XSLEEP_0);
	
    dbg_printf(DBG_LESS_INFO, "hx_drv_scu_set_SEN_INT_pinmux(SCU_SEN_INT_PINMUX_GPIO10)\n");
    dbg_printf(DBG_LESS_INFO, "hx_drv_scu_set_SEN_GPIO_pinmux(SCU_SEN_GPIO_PINMUX_SENAONGPIO2_0)\n");
    dbg_printf(DBG_LESS_INFO, "hx_drv_scu_set_SEN_XSLEEP_pinmux(SCU_SEN_XSLEEP_PINMUX_SEN_XSLEEP_0)\n");
    #endif

#ifdef SPI_MASTER_SEND
    /* SPI Master */
    hx_drv_scu_set_PB2_pinmux(SCU_PB2_PINMUX_SPI_M_DO_1, 1);
    hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_SPI_M_DI_1, 1);
    hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_SPI_M_SCLK_1, 1);
    hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SPI_M_CS_1, 1);
#elif defined(SPI_SLAVE_SEND)
    /* SPI Slave */
    hx_drv_scu_set_PB2_pinmux(SCU_PB2_PINMUX_SPI_S_DO, 1);
    hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_SPI_S_DI, 1);
    hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_SPI_S_CLK, 1);
    hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SPI_S_CS, 1);
	
	/* AON_GPIO_0 for SPI Slave Handshake(to FT_GPIO_2) */
    hx_drv_scu_set_PA0_pinmux(SCU_PA0_PINMUX_AON_GPIO0_2, 1);
    hx_drv_gpio_set_output(AON_GPIO0, GPIO_OUT_LOW);
#endif
}
#endif /* HX6538_NB_EVB_WLCSP65 */

#ifdef HX6538_AIoT_EVB_LQFP128_V10
void priv_set_128_aiot_evb_pinmux(void)
{
#ifdef NSC
        veneer_sys_set_PB9_pinmux(SCU_PB9_PINMUX_PDM_CLK_1, 1); /* PDM_PIN_CLK*/
        veneer_sys_set_PB10_pinmux(SCU_PB10_PINMUX_PDM_DATA0_1, 1); /*PDM_PIN_DATA_0*/
        veneer_sys_set_PB11_pinmux(SCU_PB11_PINMUX_PDM_DATA1_1, 1); /*PDM_PIN_DATA_1*/
        veneer_sys_set_PC3_pinmux(SCU_PC3_PINMUX_PDM_DATA2); /*PDM_PIN_DATA_2*/
        veneer_sys_set_PC4_pinmux(SCU_PC4_PINMUX_PDM_DATA3); /*PDM_PIN_DATA_3*/
#else
        hx_drv_scu_set_PB9_pinmux(SCU_PB9_PINMUX_PDM_CLK_1, 1); /*PDM_PIN_CLK*/
        hx_drv_scu_set_PB10_pinmux(SCU_PB10_PINMUX_PDM_DATA0_1, 1); /*PDM_PIN_DATA_0*/
        hx_drv_scu_set_PB11_pinmux(SCU_PB11_PINMUX_PDM_DATA1_1, 1); /*PDM_PIN_DATA_1*/
        hx_drv_scu_set_PC3_pinmux(SCU_PC3_PINMUX_PDM_DATA2); /*PDM_PIN_DATA_2*/
        hx_drv_scu_set_PC4_pinmux(SCU_PC4_PINMUX_PDM_DATA3); /*PDM_PIN_DATA_3*/
#endif

    /* SPI Master */
    hx_drv_scu_set_PB2_pinmux(SCU_PB2_PINMUX_SPI_M_DO_1, 1);
    hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_SPI_M_DI_1, 1);
    hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_SPI_M_SCLK_1, 1);
    hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SPI_M_CS_1, 1);

    /* SPI Slave */
    hx_drv_scu_set_SPIS_DO_pinmux(SCU_SPIS_DO_PINMUX_SPI_S_DO);
    hx_drv_scu_set_SPIS_DI_pinmux(SCU_SPIS_DI_PINMUX_SPI_S_DI);
    hx_drv_scu_set_SPIS_SCLK_pinmux(SCU_SPIS_SCLK_PINMUX_SPI_S_SCLK);
    hx_drv_scu_set_SPIS_CS_pinmux(SCU_SPIS_CS_PINMUX_SPI_S_CS);

    /* GPIO_2 for SPI Slave Handshake(to FT_GPIO_2) */
    hx_drv_scu_set_PB8_pinmux(SCU_PB8_PINMUX_GPIO2_2, 1);
    hx_drv_gpio_set_output(GPIO2, GPIO_OUT_LOW);

    /* GPIO_0 for GPIO wakeup */
    hx_drv_scu_set_PA0_pinmux(SCU_PA0_PINMUX_AON_GPIO0_0, 1);
    hx_drv_gpio_set_input(AON_GPIO0);
}
#endif /* HX6538_AIoT_EVB_LQFP128_V10 */

#ifdef HX6538_AIoT_EVB_QFN88_V10
void priv_set_88_aiot_evb_pinmux(void)
{
#ifdef NSC
        veneer_sys_set_PB9_pinmux(SCU_PB9_PINMUX_PDM_CLK_1, 1); /* PDM_PIN_CLK*/
        veneer_sys_set_PB10_pinmux(SCU_PB10_PINMUX_PDM_DATA0_1, 1); /*PDM_PIN_DATA_0*/
        veneer_sys_set_PB11_pinmux(SCU_PB11_PINMUX_PDM_DATA1_1, 1); /*PDM_PIN_DATA_1*/
        veneer_sys_set_PC3_pinmux(SCU_PC3_PINMUX_PDM_DATA2); /*PDM_PIN_DATA_2*/
        veneer_sys_set_PC4_pinmux(SCU_PC4_PINMUX_PDM_DATA3); /*PDM_PIN_DATA_3*/
#else
        hx_drv_scu_set_PB9_pinmux(SCU_PB9_PINMUX_PDM_CLK_1, 1); /*PDM_PIN_CLK*/
        hx_drv_scu_set_PB10_pinmux(SCU_PB10_PINMUX_PDM_DATA0_1, 1); /*PDM_PIN_DATA_0*/
        hx_drv_scu_set_PB11_pinmux(SCU_PB11_PINMUX_PDM_DATA1_1, 1); /*PDM_PIN_DATA_1*/
        hx_drv_scu_set_PC3_pinmux(SCU_PC3_PINMUX_PDM_DATA2); /*PDM_PIN_DATA_2*/
        hx_drv_scu_set_PC4_pinmux(SCU_PC4_PINMUX_PDM_DATA3); /*PDM_PIN_DATA_3*/
#endif

#ifdef SPI_MASTER_SEND
    /* SPI Master */
    hx_drv_scu_set_PB2_pinmux(SCU_PB2_PINMUX_SPI_M_DO_1, 1);
    hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_SPI_M_DI_1, 1);
    hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_SPI_M_SCLK_1, 1);
    hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SPI_M_CS_1, 1);
#elif defined(SPI_SLAVE_SEND)
    /* SPI Slave */
    hx_drv_scu_set_PB2_pinmux(SCU_PB2_PINMUX_SPI_S_DO, 1);
    hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_SPI_S_DI, 1);
    hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_SPI_S_CLK, 1);
    hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SPI_S_CS, 1);
#endif

#if defined(UART_PROTOCOL) && defined(WE2_SINGLE_CORE)
    /* UART_1 */
    hx_drv_scu_set_PB6_pinmux(SCU_PB6_PINMUX_UART1_RX, 1);
    hx_drv_scu_set_PB7_pinmux(SCU_PB7_PINMUX_UART1_TX, 1);
#endif

    /* GPIO_2 for SPI Slave Handshake(to FT_GPIO_2) */
    hx_drv_scu_set_PB8_pinmux(SCU_PB8_PINMUX_GPIO2_2, 1);
    hx_drv_gpio_set_output(GPIO2, GPIO_OUT_LOW);

    /* GPIO_0 for GPIO wakeup */
    hx_drv_scu_set_PA0_pinmux(SCU_PA0_PINMUX_AON_GPIO0_0, 1);
    hx_drv_gpio_set_input(AON_GPIO0);
}
#endif /* HX6538_AIoT_EVB_QFN88_V10 */

#ifdef HX6538_ISM028_03M
void priv_set_88_ism028_03m_pinmux(void)
{	
	hx_drv_scu_set_PB0_pinmux(SCU_PB0_PINMUX_UART0_RX_1, 1);
	hx_drv_scu_set_PB1_pinmux(SCU_PB1_PINMUX_UART0_TX_1, 1);

	/*Set 1*/
	hx_drv_scu_set_PB2_pinmux(SCU_PB2_PINMUX_I2C_M_SDA, 1);
	hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_I2C_M_SCL, 1);
#ifdef SPI_MASTER_SEND
	hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_SPI_M_SCLK_1, 1);
	hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SPI_M_CS_1, 1);
	hx_drv_scu_set_PB6_pinmux(SCU_PB6_PINMUX_SPI_M_DI, 1);
	hx_drv_scu_set_PB7_pinmux(SCU_PB7_PINMUX_SPI_M_DO, 1);
#elif defined(SPI_SLAVE_SEND)
	hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_SPI_S_CLK, 1);
	hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SPI_S_CS, 1);
	hx_drv_scu_set_PB6_pinmux(SCU_PB6_PINMUX_SPI_S_DI, 1);
	hx_drv_scu_set_PB7_pinmux(SCU_PB7_PINMUX_SPI_S_DO, 1);
#endif

	#if 0
	/*Set 2*/
	hx_drv_scu_set_PB2_pinmux(SCU_PB2_PINMUX_SDIO_DAT1, 1);
	hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_SDIO_DAT0, 1);
	hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_SDIO_DAT2, 1);
	hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SDIO_DAT3, 1);
	hx_drv_scu_set_PB6_pinmux(SCU_PB6_PINMUX_SDIO_CMD, 1);
	hx_drv_scu_set_PB7_pinmux(SCU_PB7_PINMUX_SDIO_SDCLK, 1);

	/*Set 3*/
	hx_drv_scu_set_PB2_pinmux(SCU_PB2_PINMUX_I2S_M_SDO, 1);
	hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_I2S_M_SDI, 1);
	//hx_drv_scu_set_PB3_pinmux(SCU_PB3_PINMUX_GPIO14, 1);
	hx_drv_scu_set_PB4_pinmux(SCU_PB4_PINMUX_I2S_M_SCLK, 1);
	hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_I2S_M_WS, 1);
	hx_drv_scu_set_PB6_pinmux(SCU_PB6_PINMUX_I2C_M_SDA, 1);
	hx_drv_scu_set_PB7_pinmux(SCU_PB7_PINMUX_I2C_M_SCL, 1);
	#endif

	hx_drv_scu_set_PB8_pinmux(SCU_PB8_PINMUX_PWM2, 1);
	hx_drv_scu_set_PB9_pinmux(SCU_PB9_PINMUX_PDM_CLK_1, 1);
	hx_drv_scu_set_PB10_pinmux(SCU_PB10_PINMUX_PDM_DATA0_1, 1);
	
	/* AON_GPIO_0 */
	hx_drv_scu_set_PA0_pinmux(SCU_PA0_PINMUX_AON_GPIO0_2, 1);
	//hx_drv_gpio_set_input(AON_GPIO0);
	hx_drv_gpio_set_output(AON_GPIO0, GPIO_OUT_LOW);
	
	/* AON_GPIO_1 */
    hx_drv_scu_set_PA1_pinmux(SCU_PA1_PINMUX_AON_GPIO1, 1);
	hx_drv_gpio_set_input(AON_GPIO1);
    //hx_drv_gpio_set_output(AON_GPIO1, GPIO_OUT_LOW);

	/* GPIO_2 */
    hx_drv_scu_set_PB11_pinmux(SCU_PB11_PINMUX_GPIO2, 1);
	//hx_drv_gpio_set_input(GPIO2);
    //hx_drv_gpio_set_output(GPIO2, GPIO_OUT_LOW);
}
#endif /* HX6538_ISM028_03M */

