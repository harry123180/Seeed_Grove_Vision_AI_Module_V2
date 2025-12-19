/**
 ********************************************************************************************
 *  @file      app_evt_mb.h
 *  @details   This file contains all event related function
 *  @copyright (C) COPYRIGHT, Himax, Inc. ALL RIGHTS RESERVED
 *******************************************************************************************/
#ifndef APP_EVT_MB_H_
#define APP_EVT_MB_H_
#include <stdint.h>

/**
 * \brief	APP MB interrupt status callback function
 *
 * \param[in]	event	 interrupt event
 * \return	void.
 */
typedef void (*APP_EVT_MB_CB) (uint32_t event);

/****************************************************
 * Constant Definition                              *
 ***************************************************/
 
/**
 * \enum APP_EVT_MB_ERR_E
 * \brief
 */
typedef enum APP_EVT_MB_ERR_S
{
	APP_EVT_MB_ERR_NO_ERROR	         	= 0,	/**< No Error */
	APP_EVT_MB_ERR_MUTEX_ACQUIRE_FAIL   = 1,	/**< MUTEX_ACQUIRE_FAIL */
	APP_EVT_MB_ERR_UNKNOWN_ERR			= 0xFF,	/**< Unknown FAIL  */
} APP_EVT_MB_ERR_E;

/**
 * \enum MB_MUTEX_STATUS_E
 * \brief Core of MAILBOX
 */
typedef enum APP_EVT_MB_MUTEX_STATUS_S{
	APP_EVT_MB_MUTEX_STATUS_NOT_GET = 0, /**< Not require Mutex */
	APP_EVT_MB_MUTEX_STATUS_GET, /**< Require Mutex */
} APP_EVT_MB_MUTEX_STATUS_E;


/**
 * \enum APP_MB_MASTER_EVT_E
 * \brief API Event Code
 * \trigger MASTER core event
 */
typedef enum APP_EVT_MB_MASTER_EVT_S
{
	APP_EVT_MB_MASTER_EVT_NO                	= 0,	/**< No EVENT */
	APP_EVT_MB_MASTER_EVT_XDMA_READY			= 0x01, /**< infom master core xdma data is ready */
	APP_EVT_MB_MASTER_EVT_META_TX_DONE			= 0x02, /**< infom master core meta data transmit done */
	
	APP_EVT_MB_MASTER_EVT_RES					= 0x80000000,	/**< RES  */
} APP_EVT_MB_MASTER_EVT_E;

/**
 * \enum APP_MB_SLAVE_EVT_E
 * \brief API Event Code
 * \trigger SLAVE core event
 */
typedef enum APP_EVT_MB_SLAVE_EVT_S
{
	APP_EVT_MB_SLAVE_EVT_NO                		= 0,	/**< No EVENT */
	APP_EVT_MB_SLAVE_EVT_META_READY				= 0x01, /**< infom slave core meta data is ready */
	
	APP_EVT_MB_SLAVE_EVT_RES					= 0x80000000,	/**< RES  */
} APP_EVT_MB_SLAVE_EVT_E;

/**
 * \enum APP_MBLIB_SHAREDATA_IDX_E
 * \brief PM API ERROR Code
 */
typedef enum APP_EVT_MB_SHAREDATA_IDX_S
{
    APP_EVT_MB_SHAREDATA_IDX_ALGOMETA_ADDR = 0,		/**< ALGOMETA ADDR */
    APP_EVT_MB_SHAREDATA_IDX_XDMA_ADDR = 1,			/**< XDMA_ADDR */
    APP_EVT_MB_SHAREDATA_IDX_RES3        ,/**< RES3 */
    APP_EVT_MB_SHAREDATA_IDX_RES4        ,/**< RES4 */
    APP_EVT_MB_SHAREDATA_IDX_RES5        ,/**< RES5 */
    APP_EVT_MB_SHAREDATA_IDX_RES6        ,/**< RES6 */
    APP_EVT_MB_SHAREDATA_IDX_RES7        ,/**< RES7 */
    APP_EVT_MB_SHAREDATA_IDX_RES8        ,/**< RES8 */
    APP_EVT_MB_SHAREDATA_IDX_RES9        ,/**< RES9 */
    APP_EVT_MB_SHAREDATA_IDX_RES10       ,/**< RES10 */
    APP_EVT_MB_SHAREDATA_IDX_RES11       ,/**< RES11 */
    APP_EVT_MB_SHAREDATA_IDX_RES12       ,/**< RES12 */
    APP_EVT_MB_SHAREDATA_IDX_RES13       ,/**< RES13 */
    APP_EVT_MB_SHAREDATA_IDX_RES14       ,/**< RES14 */
    APP_EVT_MB_SHAREDATA_IDX_RES15       ,/**< RES15 */
    APP_EVT_MB_SHAREDATA_IDX_RES16       ,/**< RES16 */
    APP_EVT_MB_SHAREDATA_IDX_RES17       ,/**< RES17 */
    APP_EVT_MB_SHAREDATA_IDX_RES18       ,/**< RES18 */
    APP_EVT_MB_SHAREDATA_IDX_RES19       ,/**< RES19 */
    APP_EVT_MB_SHAREDATA_IDX_RES20       ,/**< RES20 */
    APP_EVT_MB_SHAREDATA_IDX_RES21       ,/**< RES21 */
    APP_EVT_MB_SHAREDATA_IDX_RES22       ,/**< RES22 */
    APP_EVT_MB_SHAREDATA_IDX_RES23       ,/**< RES23 */
    APP_EVT_MB_SHAREDATA_IDX_RES24       ,/**< RES24 */
} APP_EVT_MB_SHAREDATA_IDX_E;

/**
 * \enum APP_MBLIB_MUTEX_IDX_E
 * \brief PM API ERROR Code
 */
typedef enum APP_EVT_MB_MUTEX_IDX_S
{
    APP_EVT_MB_MUTEX_IDX_ALGOMETA_UPDATE = 0,	/**< ALGOMETA_UPDATE */
    APP_EVT_MB_MUTEX_IDX_XDMA_UPDATE = 1,		/**< XDMA_UPDATE */
    APP_EVT_MB_MUTEX_IDX_RES3         ,/**< RES3 */
    APP_EVT_MB_MUTEX_IDX_RES4         ,/**< RES4 */
    APP_EVT_MB_MUTEX_IDX_RES5         ,/**< RES5 */
    APP_EVT_MB_MUTEX_IDX_RES6         ,/**< RES6 */
    APP_EVT_MB_MUTEX_IDX_RES7         ,/**< RES7 */
    APP_EVT_MB_MUTEX_IDX_RES8         ,/**< RES8 */
    APP_EVT_MB_MUTEX_IDX_RES9         ,/**< RES9 */
    APP_EVT_MB_MUTEX_IDX_RES10        ,/**< RES10 */
    APP_EVT_MB_MUTEX_IDX_RES11        ,/**< RES11 */
    APP_EVT_MB_MUTEX_IDX_RES12        ,/**< RES12 */
    APP_EVT_MB_MUTEX_IDX_RES13        ,/**< RES13 */
    APP_EVT_MB_MUTEX_IDX_RES14        ,/**< RES14 */
    APP_EVT_MB_MUTEX_IDX_RES15        ,/**< RES15 */
    APP_EVT_MB_MUTEX_IDX_RES16        ,/**< RES16 */
    APP_EVT_MB_MUTEX_IDX_RES17        ,/**< RES17 */
    APP_EVT_MB_MUTEX_IDX_RES18        ,/**< RES18 */
    APP_EVT_MB_MUTEX_IDX_RES19        ,/**< RES19 */
    APP_EVT_MB_MUTEX_IDX_RES20        ,/**< RES20 */
    APP_EVT_MB_MUTEX_IDX_RES21        ,/**< RES21 */
    APP_EVT_MB_MUTEX_IDX_RES22        ,/**< RES22 */
    APP_EVT_MB_MUTEX_IDX_RES23        ,/**< RES23 */
    APP_EVT_MB_MUTEX_IDX_RES24        /**< RES24 */
} APP_EVT_MB_MUTEX_IDX_E;

typedef struct APP_DP_SHARE_ADDR_S
{
	uint32_t wdma1_data; /**< WDMA1 data address */
	uint32_t wdma2_data; /**< WDMA2 data address */
	uint32_t wdma3_data; /**< WDMA3 data address */
	uint32_t jpeg_autofill_data; /**< jpeg_autofill_data address */
	uint32_t wdma2_cyclic_buffer_cnt; /**< wdma2_cyclic_buffer_cnt value*/
	uint32_t raw_addr; /**< raw frame data address */
	uint32_t raw_width; /**< raw frame width address */
	uint32_t raw_height; /**< raw frame height address */
	uint32_t jpeg_addr; /**< jpeg frame data address */
	uint32_t jpeg_size; /**<  jpeg frame size address */
} APP_DP_SHARE_ADDR_T;

/****************************************************
 * Function Declaration                             *
 ***************************************************/
#ifdef CM55_BIG
/**
 * \brief	master core mail box initial function.
 *
 * \param[in]	app_evt_mb_cb_fun	 master core mail box event call back function.
 * \return	 APP_EVT_MB_ERR_E.
 */
APP_EVT_MB_ERR_E app_evt_mb_master_init(APP_EVT_MB_CB app_evt_mb_cb_fun);

/**
 * \brief	master core clean mail box event.
 *
 * \param[in]	mst_evt	 master core mail box event code.
 * \return	 APP_EVT_MB_ERR_E.
 */
APP_EVT_MB_ERR_E app_evt_mb_master_clr_evt(APP_EVT_MB_MASTER_EVT_E mst_evt);

/**
 * \brief	master core trigger mail box event.
 *
 * \param[in]	slv_evt	 slave core mail box event code.
 * \return	APP_EVT_MB_ERR_E.
 */
APP_EVT_MB_ERR_E app_evt_mb_master_trig_evt(APP_EVT_MB_SLAVE_EVT_E slv_evt);
#endif
#ifdef CM55_SMALL
/**
 * \brief	slave core mail box initial function.
 *
 * \param[in]	app_evt_mb_cb_fun	 slave core mail box event call back function.
 * \return	 APP_EVT_MB_ERR_E.
 */
APP_EVT_MB_ERR_E app_evt_mb_slave_init(APP_EVT_MB_CB app_evt_mb_cb_fun);

/**
 * \brief	slave core clear mail box event.
 *
 * \param[in]	slv_evt	 slave core mail box event code.
 * \return APP_EVT_MB_ERR_E.
 */
APP_EVT_MB_ERR_E app_evt_mb_slave_clr_evt(APP_EVT_MB_SLAVE_EVT_E slv_evt);

/**
 * \brief	slave core trigger mail box event.
 *
 * \param[in]	mst_evt	 master core mail box event code.
 * \return	APP_EVT_MB_ERR_E.
 */
APP_EVT_MB_ERR_E app_evt_mb_slave_trig_evt(APP_EVT_MB_MASTER_EVT_E mst_evt);
#endif

/**
 * \brief	set mail box share data.
 *
 * \param[in]	share_idx	    share data index.
 * \param[in]	share_addr    share data address.
 * \return	 APP_EVT_MB_ERR_E.
 */
APP_EVT_MB_ERR_E app_evt_mb_set_share_data(APP_EVT_MB_SHAREDATA_IDX_E share_idx, uint32_t share_addr);

/**
 * \brief	get mail box share data.
 *
 * \param[in]	share_idx	    share data index.
 * \param[in]	ppshare_addr    share data pointer address to be filled.
 * \return	 APP_EVT_MB_ERR_E.
 */
APP_EVT_MB_ERR_E app_evt_mb_get_share_data(APP_EVT_MB_SHAREDATA_IDX_E share_idx, uint32_t** ppshare_addr);

/**
 * \brief	acquire mutex to enter into critical section.
 *
 * \param[in]	mutex_num    mutex index number.
 * \param[in]	retry_times    max retry times to acquire mutex.
 * \return  APP_EVT_MB_MUTEX_STATUS_E.
 */
APP_EVT_MB_MUTEX_STATUS_E app_evt_mb_acquire_mutex(APP_EVT_MB_MUTEX_IDX_E mutex_num, uint32_t retry_times);

/**
 * \brief	release mutex to leave critical section.
 *
 * \param[in]	mutex_num    mutex index number.
 * \return  APP_EVT_MB_ERR_E.
 */
APP_EVT_MB_ERR_E app_evt_mb_release_mutex(APP_EVT_MB_MUTEX_IDX_E mutex_num);

#endif /* APP_EVT_MB_H_ */

