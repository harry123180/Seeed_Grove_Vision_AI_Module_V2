#include "app_platform.h"
#include "app_api.h"
#include "app_datapath.h"
#include "app_board.h"
#include "app_pmu.h"
#include "app_xdma_cfg.h"


#include "spi_master_protocol.h"

#include "powermode.h"

#include "hx_drv_pmu.h"
#include "hx_drv_scu_export.h"
#include "hx_drv_swreg_aon_export.h"
#include "hx_drv_swreg_aon.h"

#ifdef IP_rtc
#include "hx_drv_rtc.h"
#endif

#ifdef IP_timer
#include "hx_drv_timer.h"
#endif

#ifdef IP_watchdog
#include "hx_drv_watchdog.h"
#endif

#ifdef IP_gpio
#include "hx_drv_gpio.h"
#endif

#ifdef IP_hxautoi2c_mst
#include "hx_drv_hxautoi2c_mst.h"
#endif

#define DBG_APP_SIM_LOG             (1)

#if DBG_APP_SIM_LOG
    #define dbg_app_log(fmt, ...)       xprintf(fmt, ##__VA_ARGS__)
#else
    #define dbg_app_log(fmt, ...)
#endif

#define SYS_RTC_MS     (120)

/* local variables */
static volatile uint8_t g_wdma1_data[0/*WDMA1_SIZE*/] = {};
static volatile uint8_t g_wdma2_data[WDMA2_SLOT_SIZE*WDMA2_CYCLIC_BUF_CNT] = {};
static volatile uint8_t g_wdma3_data[WDMA3_SIZE] = {};
static volatile uint8_t g_jpeg_autofill_data[JPEG_AUTOFILL_SIZE] = {};


void clear_wakeup_evt()
{
	uint32_t wakeup_event, wakeup_event1;
	hx_lib_pm_clear_event();
	hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT, &wakeup_event);
    dbg_app_log("after clear WakeupEvt=0x%x\n", wakeup_event);
	hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT1, &wakeup_event1);
    dbg_app_log("after clear WakeupEvt1=0x%x\n", wakeup_event1);
}

void get_wakeup_ip_int()
{
	uint32_t status;

#ifdef IP_rtc
#if defined(IP_INST_RTC0) || defined(IP_INST_NS_RTC0)
	hx_drv_rtc_get_alarm_rawint_status(RTC_ID_0, &status);
	dbg_app_log("RTC0 raw status=%d\n", status);
	hx_drv_rtc_get_alarm_maskint_status(RTC_ID_0, &status);
	dbg_app_log("RTC0 mask status=%d\n", status);
#endif
#if defined(IP_INST_RTC1) || defined(IP_INST_NS_RTC1)
	hx_drv_rtc_get_alarm_rawint_status(RTC_ID_1, &status);
	dbg_app_log("RTC1 raw status=%d\n", status);
	hx_drv_rtc_get_alarm_maskint_status(RTC_ID_1, &status);
	dbg_app_log("RTC1 mask status=%d\n", status);
#endif
#if defined(IP_INST_RTC2) || defined(IP_INST_NS_RTC2)
	hx_drv_rtc_get_alarm_rawint_status(RTC_ID_2, &status);
	dbg_app_log("RTC2 raw status=%d\n", status);
	hx_drv_rtc_get_alarm_maskint_status(RTC_ID_2, &status);
	dbg_app_log("RTC2 mask status=%d\n", status);
#endif
#endif
#ifdef IP_timer
#if defined(IP_INST_TIMER0) || defined(IP_INST_NS_TIMER0)
	status = hx_drv_timer_StatusIRQ(TIMER_ID_0);
	dbg_app_log("Timer0 raw status=%d\n",  status);
#endif
#if defined(IP_INST_TIMER1) || defined(IP_INST_NS_TIMER1)
	status = hx_drv_timer_StatusIRQ(TIMER_ID_1);
	dbg_app_log("Timer1 raw status=%d\n",  status);
#endif
#if defined(IP_INST_TIMER2) || defined(IP_INST_NS_TIMER2)
	status = hx_drv_timer_StatusIRQ(TIMER_ID_2);
	dbg_app_log("Timer2 raw status=%d\n",  status);
#endif
#if defined(IP_INST_TIMER3) || defined(IP_INST_NS_TIMER3)
	status = hx_drv_timer_StatusIRQ(TIMER_ID_3);
	dbg_app_log("Timer3 raw status=%d\n",  status);
#endif
#ifdef WE2_SINGLE_CORE
#if defined(IP_INST_TIMER4) || defined(IP_INST_NS_TIMER4)
	status = hx_drv_timer_StatusIRQ(TIMER_ID_4);
	dbg_app_log("Timer4 raw status=%d\n",  status);
#endif
#if defined(IP_INST_TIMER5) || defined(IP_INST_NS_TIMER5)
	status = hx_drv_timer_StatusIRQ(TIMER_ID_5);
	dbg_app_log("Timer5 raw status=%d\n",  status);
#endif
#if defined(IP_INST_TIMER6) || defined(IP_INST_NS_TIMER6)
	status = hx_drv_timer_StatusIRQ(TIMER_ID_6);
	dbg_app_log("Timer6 raw status=%d\n",  status);
#endif
#if defined(IP_INST_TIMER7) || defined(IP_INST_NS_TIMER7)
	status = hx_drv_timer_StatusIRQ(TIMER_ID_7);
	dbg_app_log("Timer7 raw status=%d\n",  status);
#endif
#if defined(IP_INST_TIMER8) || defined(IP_INST_NS_TIMER8)
	status = hx_drv_timer_StatusIRQ(TIMER_ID_8);
	dbg_app_log("Timer8 raw status=%d\n",  status);
#endif
#endif
#endif
#ifdef IP_gpio
#if defined(IP_INST_AON_GPIO) || defined(IP_INST_NS_AON_GPIO)
    status = hx_drv_gpio_get_int_status(AON_GPIO0);
    dbg_app_log("AON GPIO_0, int_status:%d \n", status);
    status = hx_drv_gpio_get_int_status(AON_GPIO1);
    dbg_app_log("AON GPIO_1, int_status:%d \n", status);
#endif
#if defined(IP_INST_SB_GPIO) || defined(IP_INST_NS_SB_GPIO)
    status = hx_drv_gpio_get_int_status(SB_GPIO0);
    dbg_app_log("SB GPIO_0, int_status:%d \n", status);
	status = hx_drv_gpio_get_int_status(SB_GPIO1);
    dbg_app_log("SB GPIO_1, int_status:%d \n", status);
#endif
#endif

#ifdef IP_hxautoi2c_mst
	uint32_t autoi2c_rawstatus = 0;
	autoi2c_rawstatus = hx_drv_hxautoi2c_get_rawstatus();
    dbg_app_log("autoi2c_rawstatus:0x%x \n", autoi2c_rawstatus);
#endif
}

void clear_all_wakeupip_int()
{
#ifdef IP_rtc
#if defined(IP_INST_RTC0) || defined(IP_INST_NS_RTC0)
	hx_drv_rtc_clear_alarm_int_status(RTC_ID_0);
#endif
#if defined(IP_INST_RTC1) || defined(IP_INST_NS_RTC1)
	hx_drv_rtc_clear_alarm_int_status(RTC_ID_1);
#endif
#if defined(IP_INST_RTC2) || defined(IP_INST_NS_RTC2)
	hx_drv_rtc_clear_alarm_int_status(RTC_ID_2);
#endif
#endif
#ifdef IP_timer
#if defined(IP_INST_TIMER0) || defined(IP_INST_NS_TIMER0)
	hx_drv_timer_ClearIRQ(TIMER_ID_0);
#endif
#if defined(IP_INST_TIMER1) || defined(IP_INST_NS_TIMER1)
	hx_drv_timer_ClearIRQ(TIMER_ID_1);
#endif
#if defined(IP_INST_TIMER2) || defined(IP_INST_NS_TIMER2)
	hx_drv_timer_ClearIRQ(TIMER_ID_2);
#endif
#if defined(IP_INST_TIMER3) || defined(IP_INST_NS_TIMER3)
	hx_drv_timer_ClearIRQ(TIMER_ID_3);
#endif
#ifdef WE2_SINGLE_CORE
#if defined(IP_INST_TIMER4) || defined(IP_INST_NS_TIMER4)
	hx_drv_timer_ClearIRQ(TIMER_ID_4);
#endif
#if defined(IP_INST_TIMER5) || defined(IP_INST_NS_TIMER5)
	hx_drv_timer_ClearIRQ(TIMER_ID_5);
#endif
#if defined(IP_INST_TIMER6) || defined(IP_INST_NS_TIMER6)
	hx_drv_timer_ClearIRQ(TIMER_ID_6);
#endif
#if defined(IP_INST_TIMER7) || defined(IP_INST_NS_TIMER7)
	hx_drv_timer_ClearIRQ(TIMER_ID_7);
#endif
#if defined(IP_INST_TIMER8) || defined(IP_INST_NS_TIMER8)
	hx_drv_timer_ClearIRQ(TIMER_ID_8);
#endif
#endif
#endif
#ifdef IP_watchdog
	hx_drv_watchdog_irq_clear(WATCHDOG_ID_0);
	hx_drv_watchdog_irq_clear(WATCHDOG_ID_1);
#endif
#ifdef IP_gpio
#if defined(IP_INST_AON_GPIO) || defined(IP_INST_NS_AON_GPIO)
	hx_drv_gpio_clr_int_status(AON_GPIO0);
	hx_drv_gpio_clr_int_status(AON_GPIO1);
#endif
#if defined(IP_INST_SB_GPIO) || defined(IP_INST_NS_SB_GPIO)
	hx_drv_gpio_clr_int_status(SB_GPIO0);
	hx_drv_gpio_clr_int_status(SB_GPIO1);
#endif
#endif
#ifdef IP_hxautoi2c_mst
	hx_drv_hxautoi2c_clr_noack_int(HXAUTOI2CHC_INTSTATUS_DATA_NO_ACK);
	hx_drv_hxautoi2c_clr_noack_int(HXAUTOI2CHC_INTSTATUS_ADDR_NO_ACK);
#endif
}

void app_stop_dplib()
{
	dbg_app_log("app_stop_dplib start\n");
	
	app_cisdp_sensor_stop();
	hx_drv_swreg_aon_set_sensorinit(SWREG_AON_SENSOR_INIT_NO);

	dbg_app_log("app_stop_dplib end\n");
}

void app_dump_jpeginfo()
{
	uint8_t jpegloop_flag;
	uint8_t nextjpeg_frame_no;
	uint8_t jpeg_total_slot;
	uint32_t jpegip_size;

	hx_drv_xdma_get_WDMA2_loopflag(&jpegloop_flag);
	hx_drv_xdma_get_WDMA2NextFrameIdx(&nextjpeg_frame_no);
	hx_drv_xdma_get_WDMA2_bufferNo(&jpeg_total_slot);
	dbg_app_log("jpegloop_flag=%d,nextjpeg_frame_no=%d,jpeg_total_slot=%d\n",
			jpegloop_flag, nextjpeg_frame_no, jpeg_total_slot);
	hx_drv_jpeg_get_EncOutRealMEMSize(&jpegip_size);
	dbg_app_log("jpegip_size=0x%x\n", jpegip_size);
}

void app_pmu_chkrdy(void)
{
#ifdef IP_xdma
	uint32_t wdma1_status, wdma2_status, wdma3_status, rdma_status;
#endif

	dbg_app_log("Check and Clr PMU/DP status\n");

	hx_lib_pm_pmu_dpdone_check(sensordplib_waitpmudmadone);

    hx_drv_xdma_get_WDMA1INTStatus(&wdma1_status);
    hx_drv_xdma_get_WDMA2INTStatus(&wdma2_status);
    hx_drv_xdma_get_WDMA3INTStatus(&wdma3_status);
    hx_drv_xdma_get_RDMAINTStatus(&rdma_status);
	dbg_app_log("wdma1_status=0x%x, wdma2_status=0x%x, wdma3_status=0x%x, rdma_status=0x%x\n", wdma1_status, wdma2_status, wdma3_status, rdma_status);
	if(((wdma1_status || wdma2_status || wdma3_status) & 0xFF) != 0x1)
	{
		/*abnormal*/
		app_stop_dplib();
	}
	app_dump_jpeginfo();

	hx_lib_pm_ctrl_fromPMUtoCPU(sensordplib_pmutocpuctrl);

}

void check_mem_attr()
{
	SCU_ERROR_E ret = SCU_NO_ERROR;

	ret = hx_drv_scu_check_mem_attr((uint32_t)g_wdma2_data, WDMA2_SLOT_SIZE*WDMA2_CYCLIC_BUF_CNT, SCU_MEM_ATTR_SECURE);
	dbg_app_log("ret=%d, wdma2_addr=0x%x,WDMA2_SLOT_SIZE*WDMA2_CYCLIC_BUF_CNT=0x%x, attr=%d\n", ret, g_wdma2_data, WDMA2_SLOT_SIZE*WDMA2_CYCLIC_BUF_CNT, SCU_MEM_ATTR_SECURE);
	if(ret != SCU_NO_ERROR)
	{
		while(1)
		{
			dbg_printf(DBG_LESS_INFO, "Check Memory Fail1\n");
		}
	}
	ret = hx_drv_scu_check_mem_attr((uint32_t)g_wdma3_data, WDMA3_SIZE, SCU_MEM_ATTR_SECURE);
	dbg_app_log("ret=%d, wdma3_addr=0x%x,WDMA3_SIZE=0x%x, attr=%d\n", ret, g_wdma3_data, WDMA3_SIZE, SCU_MEM_ATTR_SECURE);
	if(ret != SCU_NO_ERROR)
	{
		while(1)
		{
			dbg_printf(DBG_LESS_INFO, "Check Memory Fail2\n");
		}
	}
	ret = hx_drv_scu_check_mem_attr((uint32_t)g_jpeg_autofill_data, JPEG_AUTOFILL_SIZE, SCU_MEM_ATTR_SECURE);
	dbg_app_log("ret=%d, jpeg_autofill=0x%x,JPEG_AUTOFILL_SIZE=0x%x, attr=%d\n", ret, g_jpeg_autofill_data, JPEG_AUTOFILL_SIZE, SCU_MEM_ATTR_SECURE);
	if(ret != SCU_NO_ERROR)
	{
		while(1)
		{
			dbg_printf(DBG_LESS_INFO, "Check Memory Fail3\n");
		}
	}
}

void app_init()
{
	app_dp_cfg_t app_dp_init_cfg = {0};
    APP_SPI_CFG_T spi_cfg = {0};
	uint32_t ret = 0;

    app_board_pinmux_settings();

    #ifdef SPI_MASTER_SEND /*master*/
    spi_cfg.mst_id = SPI_MST_1; /**< id*/
    spi_cfg.mst_freq = 25000000;  /**< frequency*/
    //spi_cfg.mst_freq = 5000000;  /**< frequency*/
    #endif
    
    #ifdef SPI_SLAVE_SEND /*slave*/
    spi_cfg.slv_handshake_pin = HX_AON_GPIO_0;         /**< handshake gpio pin number*/
    spi_cfg.slv_handshake_actv_lvl = HX_GPIO_HIGH;     /**< handshake gpio pin active level*/

    dbg_app_log("HX_AON_GPIO_0 %02x\n", HX_AON_GPIO_0);
    dbg_app_log("HX_GPIO_HIGH %u\n", HX_GPIO_HIGH);
    
    dbg_app_log("spi_cfg.slv_handshake_pin %02x\n", spi_cfg.slv_handshake_pin);
    dbg_app_log("spi_cfg.slv_handshake_actv_lvl %u\n", spi_cfg.slv_handshake_actv_lvl);
    #endif
	
    #if defined(SPI_MASTER_SEND) || defined(SPI_SLAVE_SEND)
    app_spi_init(&spi_cfg);
	#endif

    app_dp_get_def_init_cfg(&app_dp_init_cfg);

    /* Sensor */
    app_sensor_init(&app_dp_init_cfg);

    /* Datapath. */
    app_dp_init_cfg.wdma1 = (uint32_t)g_wdma1_data;
    app_dp_init_cfg.wdma2 = (uint32_t)g_wdma2_data;
    app_dp_init_cfg.wdma3 = (uint32_t)g_wdma3_data;
    app_dp_init_cfg.jpeg_auto_fill_data = (uint32_t)g_jpeg_autofill_data;
    app_dp_init_cfg.wdma2_cyclic_buffer_cnt = (uint8_t)WDMA2_CYCLIC_BUF_CNT;
    app_dp_init(&app_dp_init_cfg);
}

void gpio_cb(uint8_t group, uint8_t idx)
{
    dbg_app_log("gpio_cb 0x%x 0x%x\n", group, idx);
}

void app_main()
{
    uint32_t wakeup_event = 0;
    uint32_t wakeup_event1 = 0;
    
    app_dp_cap_t app_dp_cap_set;
    uint32_t raw_img_addr = 0, img_width = 0, img_heigh = 0;
    uint32_t jpg_addr = 0, jpg_size = 0;
    AppCfgCustGpio_t gpio_cfg = {0};
	
    hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT, &wakeup_event);
    dbg_app_log("WakeupEvt=0x%x\n", wakeup_event);
	
	hx_drv_pmu_get_ctrl(PMU_pmu_wakeup_EVT1, &wakeup_event1);
    dbg_app_log("WakeupEvt1=0x%x\n", wakeup_event1);
    
    //print_wakeup_event(wakeup_event, wakeup_event1);
    
	#if defined(PMU_SIMO_VOLT)
	set_simo_0p9v();
	hx_set_memory(0x56101030,0x00100000);
	hx_drv_pmu_set_ctrl(PMU_REG_SIMO_ULPSIMO_EN, 0x1);//COT mode
	#endif

	#if defined(PMU_SLEEP_CLK_DISABLE)
	app_pmu_clk_enable();
	#endif
	
    hx_drv_timer_hw_stop(APP_PMU_SYS_RTC_ID); /*TIMER_ID_1*/
	//get_wakeup_ip_int();
	app_pmu_chkrdy();
    
    dbg_app_log("app_main\n");
    app_init();
    
    if((wakeup_event == PMU_WAKEUP_NONE) && (wakeup_event1 == PMU_WAKEUPEVENT1_NONE))
    {
    	//check_mem_attr();
    }
    else
    {		
		uint8_t frame_no = 0;
		uint8_t buffer_no = 0;
		uint32_t jpeg_enc_addr;
		uint32_t jpeg_enc_filesize;
		
		hx_drv_xdma_get_WDMA2_bufferNo(&buffer_no);
		hx_drv_xdma_get_WDMA2NextFrameIdx(&frame_no);
		frame_no = (buffer_no + frame_no - 1) % buffer_no;
	
		hx_drv_jpeg_get_FillFileSizeToMem(frame_no, g_jpeg_autofill_data, &jpeg_enc_filesize);
		hx_drv_jpeg_get_MemAddrByFrameNo(frame_no, g_wdma2_data, &jpeg_enc_addr);
	
		dbg_app_log("current frame_no=%d, jpeg_size=0x%x,addr=0x%x\n",frame_no,jpeg_enc_filesize,jpeg_enc_addr);

		#if defined(SPI_MASTER_SEND) || defined(SPI_SLAVE_SEND)
		if (jpeg_enc_addr != 0 && jpeg_enc_filesize != 0)
        	app_spi_write(jpeg_enc_addr, jpeg_enc_filesize, DATA_TYPE_JPG);
		#endif
    }

	clear_wakeup_evt();
    clear_all_wakeupip_int();

	app_dp_get_def_cap_settings(&app_dp_cap_set);
	app_dp_jpeg_raw_cfg(&app_dp_cap_set);
	
	hx_drv_dmac_deinit(USE_HX_DMAC_ALL);
#if defined(SPI_MASTER_SEND)
	app_spi_m_close(SPI_MST_1);
#endif
#if defined(SPI_SLAVE_SEND)
	app_spi_s_close();
#endif

	app_pmu_enter_sleep1_bootwithcap(gpio_cfg, SYS_RTC_MS);
}
