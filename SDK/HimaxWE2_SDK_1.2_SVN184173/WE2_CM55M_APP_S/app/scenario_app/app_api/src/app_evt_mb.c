/**
 ********************************************************************************************
 *  @file      app_evt_mb.c
 *  @details   This file contains all app mailbox event related function
 *  @copyright (C) COPYRIGHT, Himax, Inc. ALL RIGHTS RESERVED
 *******************************************************************************************/
#include <stdio.h>
#include <string.h>

#include "WE2_device.h"
#include "board.h"

#include "hx_drv_mb.h"
#include "WE2_debug.h"

#include "app_evt_mb.h"

//#define APP_EVT_MB_DEBUG_FUNC {dbg_printf(DBG_LESS_INFO, "%s()\n", __FUNCTION__);}
#define APP_EVT_MB_DEBUG_FUNC

/****************************************************
 * Constant Definition                              *
 ***************************************************/


/***************************************************
 * Static Function Declaration
 **************************************************/


/****************************************************
 * Variable Declaration                             *
 ***************************************************/


/***************************************************
 * Function Implementation
 **************************************************/
#ifdef CM55_BIG
APP_EVT_MB_ERR_E app_evt_mb_master_init(APP_EVT_MB_CB app_evt_mb_cb_fun)
{
	APP_EVT_MB_DEBUG_FUNC;
	
	hx_drv_mb_init(MB_CORE_CM55M, HX_MAILBOX_INT_BASE);
	hx_drv_mb_register_cm55m_cb(app_evt_mb_cb_fun);

    return APP_EVT_MB_ERR_NO_ERROR;
}

APP_EVT_MB_ERR_E app_evt_mb_master_clr_evt(APP_EVT_MB_MASTER_EVT_E mst_evt)
{
	APP_EVT_MB_DEBUG_FUNC;
	
	hx_drv_mb_clear_cm55m_irq(mst_evt);

    return APP_EVT_MB_ERR_NO_ERROR;
}

APP_EVT_MB_ERR_E app_evt_mb_master_trig_evt(APP_EVT_MB_SLAVE_EVT_E slv_evt)
{
	APP_EVT_MB_DEBUG_FUNC;
	
	hx_drv_mb_trigger_cm55s_irq(slv_evt);

    return APP_EVT_MB_ERR_NO_ERROR;
}
#endif

#ifdef CM55_SMALL
APP_EVT_MB_ERR_E app_evt_mb_slave_init(APP_EVT_MB_CB app_evt_mb_cb_fun)
{
	APP_EVT_MB_DEBUG_FUNC;
	
	hx_drv_mb_init(MB_CORE_CM55S, HX_MAILBOX_INT_BASE);
	hx_drv_mb_register_cm55s_cb(app_evt_mb_cb_fun);

    return APP_EVT_MB_ERR_NO_ERROR;
}

APP_EVT_MB_ERR_E app_evt_mb_slave_clr_evt(APP_EVT_MB_SLAVE_EVT_E slv_evt)
{
	APP_EVT_MB_DEBUG_FUNC;
	
	hx_drv_mb_clear_cm55s_irq(slv_evt);

    return APP_EVT_MB_ERR_NO_ERROR;
}

APP_EVT_MB_ERR_E app_evt_mb_slave_trig_evt(APP_EVT_MB_MASTER_EVT_E mst_evt)
{
	APP_EVT_MB_DEBUG_FUNC;
	
	hx_drv_mb_trigger_cm55m_irq(mst_evt);

    return APP_EVT_MB_ERR_NO_ERROR;
}
#endif

APP_EVT_MB_ERR_E app_evt_mb_set_share_data(APP_EVT_MB_SHAREDATA_IDX_E share_idx, uint32_t share_addr)
{
	APP_EVT_MB_DEBUG_FUNC;
	
	hx_drv_mb_set_sharedata(share_idx, (uint32_t)share_addr);

    return APP_EVT_MB_ERR_NO_ERROR;
}

APP_EVT_MB_ERR_E app_evt_mb_get_share_data(APP_EVT_MB_SHAREDATA_IDX_E share_idx, uint32_t** ppshare_addr)
{
	APP_EVT_MB_DEBUG_FUNC;
	
	hx_drv_mb_get_sharedata(share_idx, (uint32_t*)ppshare_addr);

    return APP_EVT_MB_ERR_NO_ERROR;
}

APP_EVT_MB_MUTEX_STATUS_E app_evt_mb_acquire_mutex(APP_EVT_MB_MUTEX_IDX_E mutex_num, uint32_t retry_times)
{
	APP_EVT_MB_DEBUG_FUNC;
	
	MB_MUTEX_STATUS_E mutex = MB_MUTEX_STATUS_NOT_GET;

	uint32_t cnt = 0;
	
	do
	{
		hx_drv_mb_require_mutex(mutex_num, &mutex);

		if(mutex == MB_MUTEX_STATUS_GET)
			return APP_EVT_MB_MUTEX_STATUS_GET;	
	}while(cnt++ < retry_times);

	return APP_EVT_MB_MUTEX_STATUS_NOT_GET;
}

APP_EVT_MB_ERR_E app_evt_mb_release_mutex(APP_EVT_MB_MUTEX_IDX_E mutex_num)
{
	APP_EVT_MB_DEBUG_FUNC;
	
	hx_drv_mb_release_mutex(mutex_num);
	
	return APP_EVT_MB_ERR_NO_ERROR;	
}

