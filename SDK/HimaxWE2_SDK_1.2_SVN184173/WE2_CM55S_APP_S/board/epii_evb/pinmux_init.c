/*
 * pinmux_init.c
 *
 *  Created on: 2023�~9��8��
 *      Author: 902447
 */


#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "WE2_device.h"
#ifdef IP_scu
#include "hx_drv_scu.h"
#endif
#include "pinmux_init.h"

void __attribute__((weak)) pinmux_init()
{
	SCU_PINMUX_CFG_T pinmux_cfg;

	hx_drv_scu_get_all_pinmux_cfg(&pinmux_cfg);


#if defined(IC_PACKAGE_WLCSP65) || defined(IC_PACKAGE_QFN88) || defined(IC_PACKAGE_BGA64)
	/*Change UART1 pin mux to PB6 and PB7*/
	pinmux_cfg.pin_pb6 = SCU_PB6_PINMUX_UART1_RX; /*!< pin PB6*/
	pinmux_cfg.pin_pb7 = SCU_PB7_PINMUX_UART1_TX; /*!< pin PB7*/
#endif
#ifdef IC_PACKAGE_LQFP128
	/*Change UART1 pin mux to Pin UART*/
	pinmux_cfg.pin_uart = SCU_UART_PINMUX_UART1;/*!< pin UART*/
#endif

	hx_drv_scu_set_all_pinmux_cfg(&pinmux_cfg, 1);


}
