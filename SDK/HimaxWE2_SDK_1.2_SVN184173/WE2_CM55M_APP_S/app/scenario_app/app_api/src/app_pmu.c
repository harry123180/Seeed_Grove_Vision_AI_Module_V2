#include "board.h"

#include "hx_drv_pmu.h"
#include "hx_drv_cdm.h"
#include "powermode.h"
#include "sensor_dp_lib.h"
#include "WE2_debug.h"

#include "app_pmu.h"

#define DBG_APP_SIM_LOG             (1)

#if DBG_APP_SIM_LOG
    #define dbg_app_log(fmt, ...)       xprintf(fmt, ##__VA_ARGS__)
#else
    #define dbg_app_log(fmt, ...)
#endif

static void priv_pmu_set_clk_cfg(SCU_SYSCLKSRC_HW_E clksrcsel, SCU_LSC_CLK_CFG_T *lsc_cfg, SCU_PDHSC_HSCCLK_CFG_T *hsc_cfg)
{
	switch(clksrcsel)
	{
		case SCU_SYSCLKSRC_HW_RC24MRC32K:// /**< CLK Source HW RC(RC24M and RC32K) */
			hsc_cfg->hscclk.hscclksrc = SCU_HSCCLKSRC_RC24M1M;
			hsc_cfg->hscclk.hscclkdiv = SCU_HSCCLKDIV_1;
			hsc_cfg->hscd12clksrc = SCU_HSCD12CLKSRC_HSC;
			hsc_cfg->i3chcdiv = SCU_HSCI3CHCLKDIV_1;
			hsc_cfg->sdiodiv = SCU_HSCSDIOCLKDIV_1;
			lsc_cfg->lscclksrc = SCU_LSCCLKSRC_RC24M1M;
			lsc_cfg->lscclkdiv = SCU_LSCCLKDIV_1;
			break;	
		case SCU_SYSCLKSRC_HW_RC24MRC48MRC32K://, /**< CLK Source HW RC24M RC48M and RC32K)  */
			hsc_cfg->hscclk.hscclksrc = SCU_HSCCLKSRC_RC96M48M;
			hsc_cfg->hscclk.hscclkdiv = SCU_HSCCLKDIV_1;
			hsc_cfg->hscd12clksrc = SCU_HSCD12CLKSRC_HSC;
			hsc_cfg->i3chcdiv = SCU_HSCI3CHCLKDIV_1;
			hsc_cfg->sdiodiv = SCU_HSCSDIOCLKDIV_1;
			lsc_cfg->lscclksrc = SCU_LSCCLKSRC_RC96M48M;
			lsc_cfg->lscclkdiv = SCU_LSCCLKDIV_1;
			break;
		case SCU_SYSCLKSRC_HW_RC24MRC96MRC32K://, /**< CLK Source HW RC24M RC96M and RC32K)  */
			hsc_cfg->hscclk.hscclksrc = SCU_HSCCLKSRC_RC96M48M;
			hsc_cfg->hscclk.hscclkdiv = SCU_HSCCLKDIV_1;
			hsc_cfg->hscd12clksrc = SCU_HSCD12CLKSRC_HSC;
			hsc_cfg->i3chcdiv = SCU_HSCI3CHCLKDIV_2;
			hsc_cfg->sdiodiv = SCU_HSCSDIOCLKDIV_2;
			lsc_cfg->lscclksrc = SCU_LSCCLKSRC_RC96M48M;
			lsc_cfg->lscclkdiv = SCU_LSCCLKDIV_1;
			break;
		case SCU_SYSCLKSRC_HW_XTAL24MXTAL32K://, /**< CLK Source HW ALL XTAL */
			hsc_cfg->hscclk.hscclksrc = SCU_HSCCLKSRC_XTAL24M;
			hsc_cfg->hscclk.hscclkdiv = SCU_HSCCLKDIV_1;
			hsc_cfg->hscd12clksrc = SCU_HSCD12CLKSRC_HSC;
			hsc_cfg->i3chcdiv = SCU_HSCI3CHCLKDIV_1;
			hsc_cfg->sdiodiv = SCU_HSCSDIOCLKDIV_1;
			lsc_cfg->lscclksrc = SCU_LSCCLKSRC_XTAL24M;
			lsc_cfg->lscclkdiv = SCU_LSCCLKDIV_1;
			break;
		case SCU_SYSCLKSRC_HW_XTAL24MRC48MXTAL32K://, /**< CLK Source HW ALL XTAL RC48M*/
			hsc_cfg->hscclk.hscclksrc = SCU_HSCCLKSRC_RC96M48M;
			hsc_cfg->hscclk.hscclkdiv = SCU_HSCCLKDIV_1;
			hsc_cfg->hscd12clksrc = SCU_HSCD12CLKSRC_HSC;
			hsc_cfg->i3chcdiv = SCU_HSCI3CHCLKDIV_1;
			hsc_cfg->sdiodiv = SCU_HSCSDIOCLKDIV_1;
			lsc_cfg->lscclksrc = SCU_LSCCLKSRC_RC96M48M;
			lsc_cfg->lscclkdiv = SCU_LSCCLKDIV_1;
			break;
		case SCU_SYSCLKSRC_HW_XTAL24MRC96MXTAL32K://, /**< CLK Source HW ALL XTAL RC96M*/
			hsc_cfg->hscclk.hscclksrc = SCU_HSCCLKSRC_RC96M48M;
			hsc_cfg->hscclk.hscclkdiv = SCU_HSCCLKDIV_1;
			hsc_cfg->hscd12clksrc = SCU_HSCD12CLKSRC_HSC;
			hsc_cfg->i3chcdiv = SCU_HSCI3CHCLKDIV_2;
			hsc_cfg->sdiodiv = SCU_HSCSDIOCLKDIV_2;
			lsc_cfg->lscclksrc = SCU_LSCCLKSRC_RC96M48M;
			lsc_cfg->lscclkdiv = SCU_LSCCLKDIV_1;
			break;
		case SCU_SYSCLKSRC_HW_XTAL24MRC32K://, /**< CLK Source XTAL24M and RC32K */
			hsc_cfg->hscclk.hscclksrc = SCU_HSCCLKSRC_XTAL24M;
			hsc_cfg->hscclk.hscclkdiv = SCU_HSCCLKDIV_1;
			hsc_cfg->hscd12clksrc = SCU_HSCD12CLKSRC_HSC;
			hsc_cfg->i3chcdiv = SCU_HSCI3CHCLKDIV_1;
			hsc_cfg->sdiodiv = SCU_HSCSDIOCLKDIV_1;
			lsc_cfg->lscclksrc = SCU_LSCCLKSRC_XTAL24M;
			lsc_cfg->lscclkdiv = SCU_LSCCLKDIV_1;
			break;
		case SCU_SYSCLKSRC_HW_XTAL24MRC48MRC32K://, /**< CLK Source XTAL24M, RC48M and RC32K */
			hsc_cfg->hscclk.hscclksrc = SCU_HSCCLKSRC_RC96M48M;
			hsc_cfg->hscclk.hscclkdiv = SCU_HSCCLKDIV_1;
			hsc_cfg->hscd12clksrc = SCU_HSCD12CLKSRC_HSC;
			hsc_cfg->i3chcdiv = SCU_HSCI3CHCLKDIV_1;
			hsc_cfg->sdiodiv = SCU_HSCSDIOCLKDIV_1;
			lsc_cfg->lscclksrc = SCU_LSCCLKSRC_RC96M48M;
			lsc_cfg->lscclkdiv = SCU_LSCCLKDIV_1;
			break;
		case SCU_SYSCLKSRC_HW_XTAL24MRC96MRC32K://, /**< CLK Source XTAL24M, RC96M and RC32K */
			hsc_cfg->hscclk.hscclksrc = SCU_HSCCLKSRC_RC96M48M;
			hsc_cfg->hscclk.hscclkdiv = SCU_HSCCLKDIV_1;
			hsc_cfg->hscd12clksrc = SCU_HSCD12CLKSRC_HSC;
			hsc_cfg->i3chcdiv = SCU_HSCI3CHCLKDIV_2;
			hsc_cfg->sdiodiv = SCU_HSCSDIOCLKDIV_2;
			lsc_cfg->lscclksrc = SCU_LSCCLKSRC_RC96M48M;
			lsc_cfg->lscclkdiv = SCU_LSCCLKDIV_1;
			break;
		case SCU_SYSCLKSRC_HW_RC24M_XTAL32K://, /**< CLK Source HW RC(RC24M and XTAL32K) */
			hsc_cfg->hscclk.hscclksrc = SCU_HSCCLKSRC_RC24M1M;
			hsc_cfg->hscclk.hscclkdiv = SCU_HSCCLKDIV_1;
			hsc_cfg->hscd12clksrc = SCU_HSCD12CLKSRC_HSC;
			hsc_cfg->i3chcdiv = SCU_HSCI3CHCLKDIV_1;
			hsc_cfg->sdiodiv = SCU_HSCSDIOCLKDIV_1;
			lsc_cfg->lscclksrc = SCU_LSCCLKSRC_RC24M1M;
			lsc_cfg->lscclkdiv = SCU_LSCCLKDIV_1;
			break;
		case SCU_SYSCLKSRC_HW_RC24MRC48M_XTAL32K://, /**< CLK Source HW RC(RC48M and XTAL32K)  */
			hsc_cfg->hscclk.hscclksrc = SCU_HSCCLKSRC_RC96M48M;
			hsc_cfg->hscclk.hscclkdiv = SCU_HSCCLKDIV_1;
			hsc_cfg->hscd12clksrc = SCU_HSCD12CLKSRC_HSC;
			hsc_cfg->i3chcdiv = SCU_HSCI3CHCLKDIV_1;
			hsc_cfg->sdiodiv = SCU_HSCSDIOCLKDIV_1;
			lsc_cfg->lscclksrc = SCU_LSCCLKSRC_RC96M48M;
			lsc_cfg->lscclkdiv = SCU_LSCCLKDIV_1;
			break;
		case SCU_SYSCLKSRC_HW_RC24MRC96M_XTAL32K://, /**< CLK Source HW RC(RC96M and XTAL32K)  */
			hsc_cfg->hscclk.hscclksrc = SCU_HSCCLKSRC_RC96M48M;
			hsc_cfg->hscclk.hscclkdiv = SCU_HSCCLKDIV_1;
			hsc_cfg->hscd12clksrc = SCU_HSCD12CLKSRC_HSC;
			hsc_cfg->i3chcdiv = SCU_HSCI3CHCLKDIV_2;
			hsc_cfg->sdiodiv = SCU_HSCSDIOCLKDIV_2;
			lsc_cfg->lscclksrc = SCU_LSCCLKSRC_RC96M48M;
			lsc_cfg->lscclkdiv = SCU_LSCCLKDIV_1;
			break;
	}
	return;
}

static void priv_pmu_print_vidpre_cfg(PM_PD_VIDPRE_CFG_T *cfg)
{
    dbg_app_log("bootromclkfreq=%u\n", cfg->bootromspeed.bootromclkfreq);
    dbg_app_log("pll_freq=%u\n", cfg->bootromspeed.pll_freq);
    dbg_app_log("cm55mdiv=%u\n", cfg->bootromspeed.cm55m_div);
    dbg_app_log("cm55sdiv=%u\n", cfg->bootromspeed.cm55s_div);
    dbg_app_log("sensor_timer=%u\n", cfg->sensor_timer);
    dbg_app_log("wdt_timer=%u\n", cfg->wdt_timer);
    dbg_app_log("nframeend_ctrl=%u\n", cfg->nframeend_ctrl);
    dbg_app_log("cm55s_reset=%u\n", cfg->cm55s_reset);
    dbg_app_log("pmu_rtc_mask=%u\n", cfg->pmu_rtc_mask);
    dbg_app_log("pmu_pad_pa01_mask=%u\n", cfg->pmu_pad_pa01_mask);
    dbg_app_log("pmu_pad_pa23_mask=%u\n", cfg->pmu_pad_pa23_mask);
    dbg_app_log("pmu_i2cw_mask=%u\n", cfg->pmu_i2cw_mask);
    dbg_app_log("pmu_timer_mask=%u\n", cfg->pmu_timer_mask);
    dbg_app_log("pmu_cmp_mask=%u\n", cfg->pmu_cmp_mask);
    dbg_app_log("pmu_ts_mask=%u\n", cfg->pmu_ts_mask);
    dbg_app_log("pmu_senint_mask=%u\n", cfg->pmu_senint_mask);
    dbg_app_log("pmu_mipii2c_noack_mask=%u\n", cfg->pmu_mipii2c_noack_mask);
    dbg_app_log("pmu_anti_mask=%u\n", cfg->pmu_anti_mask);
    
    dbg_app_log("tcm_retention=%u\n", cfg->tcm_retention);
    dbg_app_log("hscsram_retention[0]=%u\n", cfg->hscsram_retention[0]);
    dbg_app_log("hscsram_retention[1]=%u\n", cfg->hscsram_retention[1]);
    dbg_app_log("hscsram_retention[2]=%u\n", cfg->hscsram_retention[2]);
    dbg_app_log("hscsram_retention[3]=%u\n", cfg->hscsram_retention[3]);
    dbg_app_log("lscsram_retention=%u\n", cfg->lscsram_retention);
    
    dbg_app_log("sec_mem_flag=%u\n", cfg->skip_bootflow.sec_mem_flag);
    dbg_app_log("first_bl_flag=%u\n", cfg->skip_bootflow.first_bl_flag);
    dbg_app_log("cm55m_s_app_flag=%u\n", cfg->skip_bootflow.cm55m_s_app_flag);
    dbg_app_log("cm55m_ns_app_flag=%u\n", cfg->skip_bootflow.cm55m_ns_app_flag);
    dbg_app_log("cm55s_s_app_flag=%u\n", cfg->skip_bootflow.cm55s_s_app_flag);
    dbg_app_log("cm55s_ns_app_flag=%u\n", cfg->skip_bootflow.cm55s_ns_app_flag);
    dbg_app_log("cm55m_model_flag=%u\n", cfg->skip_bootflow.cm55m_model_flag);
    dbg_app_log("cm55s_model_flag=%u\n", cfg->skip_bootflow.cm55s_model_flag);
    dbg_app_log("cm55m_appcfg_flag=%u\n", cfg->skip_bootflow.cm55m_appcfg_flag);
    dbg_app_log("cm55s_appcfg_flag=%u\n", cfg->skip_bootflow.cm55s_appcfg_flag);
    dbg_app_log("cm55m_s_app_rwdata_flag=%u\n", cfg->skip_bootflow.cm55m_s_app_rwdata_flag);
    dbg_app_log("cm55m_ns_app_rwdata_flag=%u\n", cfg->skip_bootflow.cm55m_ns_app_rwdata_flag);
    dbg_app_log("cm55s_s_app_rwdata_flag=%u\n", cfg->skip_bootflow.cm55s_s_app_rwdata_flag);
    dbg_app_log("cm55s_ns_app_rwdata_flag=%u\n", cfg->skip_bootflow.cm55s_ns_app_rwdata_flag);
    dbg_app_log("secure_debug_flag=%u\n", cfg->skip_bootflow.secure_debug_flag);

    dbg_app_log("fast_vpr=%u\n", cfg->fast_vpr);
    dbg_app_log("pmu_dcdc_outpin=%u\n", cfg->pmu_dcdc_outpin);
    dbg_app_log("mipi_lane_en=%u\n", cfg->mipi_lane_en);
    dbg_app_log("sensor_type=%u\n", cfg->sensor_type);
    dbg_app_log("support_debugdump=%u\n", cfg->support_debugdump);;
}

static void priv_pmu_print_novidpre_cfg(PM_PD_NOVIDPRE_CFG_T *cfg)
{
    dbg_app_log("bootromclkfreq=%u\n", cfg->bootromspeed.bootromclkfreq);
    dbg_app_log("pll_freq=%u\n", cfg->bootromspeed.pll_freq);
    dbg_app_log("cm55mdiv=%u\n", cfg->bootromspeed.cm55m_div);
    dbg_app_log("cm55sdiv=%u\n", cfg->bootromspeed.cm55s_div);
    dbg_app_log("nframeend_ctrl=%u\n", cfg->nframeend_ctrl);
    dbg_app_log("cm55s_reset=%u\n", cfg->cm55s_reset);
    dbg_app_log("pmu_rtc_mask=%u\n", cfg->pmu_rtc_mask);
    dbg_app_log("pmu_pad_pa01_mask=%u\n", cfg->pmu_pad_pa01_mask);
    dbg_app_log("pmu_pad_pa23_mask=%u\n", cfg->pmu_pad_pa23_mask);
    dbg_app_log("pmu_i2cw_mask=%u\n", cfg->pmu_i2cw_mask);
    dbg_app_log("pmu_timer_mask=%u\n", cfg->pmu_timer_mask);
    dbg_app_log("pmu_cmp_mask=%u\n", cfg->pmu_cmp_mask);
    dbg_app_log("pmu_ts_mask=%u\n", cfg->pmu_ts_mask);
    dbg_app_log("pmu_anti_mask=%u\n", cfg->pmu_anti_mask);
    
    dbg_app_log("tcm_retention=%u\n", cfg->tcm_retention);
    dbg_app_log("hscsram_retention[0]=%u\n", cfg->hscsram_retention[0]);
    dbg_app_log("hscsram_retention[1]=%u\n", cfg->hscsram_retention[1]);
    dbg_app_log("hscsram_retention[2]=%u\n", cfg->hscsram_retention[2]);
    dbg_app_log("hscsram_retention[3]=%u\n", cfg->hscsram_retention[3]);
    dbg_app_log("lscsram_retention=%u\n", cfg->lscsram_retention);
    
    dbg_app_log("sec_mem_flag=%u\n", cfg->skip_bootflow.sec_mem_flag);
    dbg_app_log("first_bl_flag=%u\n", cfg->skip_bootflow.first_bl_flag);
    dbg_app_log("cm55m_s_app_flag=%u\n", cfg->skip_bootflow.cm55m_s_app_flag);
    dbg_app_log("cm55m_ns_app_flag=%u\n", cfg->skip_bootflow.cm55m_ns_app_flag);
    dbg_app_log("cm55s_s_app_flag=%u\n", cfg->skip_bootflow.cm55s_s_app_flag);
    dbg_app_log("cm55s_ns_app_flag=%u\n", cfg->skip_bootflow.cm55s_ns_app_flag);
    dbg_app_log("cm55m_model_flag=%u\n", cfg->skip_bootflow.cm55m_model_flag);
    dbg_app_log("cm55s_model_flag=%u\n", cfg->skip_bootflow.cm55s_model_flag);
    dbg_app_log("cm55m_appcfg_flag=%u\n", cfg->skip_bootflow.cm55m_appcfg_flag);
    dbg_app_log("cm55s_appcfg_flag=%u\n", cfg->skip_bootflow.cm55s_appcfg_flag);
    dbg_app_log("cm55m_s_app_rwdata_flag=%u\n", cfg->skip_bootflow.cm55m_s_app_rwdata_flag);
    dbg_app_log("cm55m_ns_app_rwdata_flag=%u\n", cfg->skip_bootflow.cm55m_ns_app_rwdata_flag);
    dbg_app_log("cm55s_s_app_rwdata_flag=%u\n", cfg->skip_bootflow.cm55s_s_app_rwdata_flag);
    dbg_app_log("cm55s_ns_app_rwdata_flag=%u\n", cfg->skip_bootflow.cm55s_ns_app_rwdata_flag);
    dbg_app_log("secure_debug_flag=%u\n", cfg->skip_bootflow.secure_debug_flag);

    dbg_app_log("support_bootwithcap=%u\n", cfg->support_bootwithcap);
    dbg_app_log("pmu_dcdc_outpin=%u\n", cfg->pmu_dcdc_outpin);
    dbg_app_log("mipi_lane_en=%u\n", cfg->mipi_lane_en);
    dbg_app_log("ioret=%u\n", cfg->ioret);
    dbg_app_log("sensor_type=%u\n", cfg->sensor_type);
    dbg_app_log("simo_pd_onoff=%u\n", cfg->simo_pd_onoff);
    dbg_app_log("support_debugdump=%u\n", cfg->support_debugdump);
}

static void priv_pmu_print_dpd_cfg(PM_DPD_CFG_T *cfg)
{
    dbg_app_log("bootromclkfreq=%u\n", cfg->bootromspeed.bootromclkfreq);
    dbg_app_log("pll_freq=%u\n", cfg->bootromspeed.pll_freq);
    dbg_app_log("cm55mdiv=%u\n", cfg->bootromspeed.cm55m_div);
    dbg_app_log("cm55sdiv=%u\n", cfg->bootromspeed.cm55s_div);
    dbg_app_log("cm55s_reset=%u\n", cfg->cm55s_reset);
    dbg_app_log("pmu_rtc_mask=%u\n", cfg->pmu_rtc_mask);
    dbg_app_log("pmu_anti_mask=%u\n", cfg->pmu_anti_mask);
    dbg_app_log("pmu_pad_pa0_mask=%u\n", cfg->pmu_pad_pa0_mask);
    dbg_app_log("pmu_pad_pa1_mask=%u\n", cfg->pmu_pad_pa1_mask);
    dbg_app_log("pmu_dcdc_outpin=%u\n", cfg->pmu_dcdc_outpin);
    dbg_app_log("support_debugdump=%u\n", cfg->support_debugdump);;
}

static void priv_pmu_gpio_cb(uint8_t group, uint8_t idx)
{
}

#if defined(PMU_SLEEP_CLK_DISABLE)
//enable clock
void app_pmu_clk_enable()
{
	SCU_PDAON_CLKEN_CFG_T aonclken = {0};
	SCU_PDSB_CLKEN_CFG_T pdsb_clken = {0};
	SCU_PDLSC_CLKEN_CFG_T pdlsc_clken = {0};
	SCU_PDHSC_CLKEN_CFG_T pdhsc_clken = {0};

	aonclken.rtc0_clk_en = 1;/*!< RTC0 Clock enable */
	aonclken.rtc1_clk_en = 1;/*!< RTC1 Clock enable */
	aonclken.rtc2_clk_en = 1;/*!< RTC2 Clock enable */
	aonclken.pmu_clk_en = 1;/*!< PMU Clock enable */
	aonclken.aon_gpio_clk_en = 1;/*!< AON GPIO Clock enable */
	aonclken.aon_swreg_clk_en = 1;/*!< AON SW REG Clock enable */
	aonclken.antitamper_clk_en = 1;/*!< ANTI TAMPER Clock enable */
	hx_drv_scu_set_pdaon_clken_cfg(aonclken);

	pdsb_clken.apb1_ahb4_pclk_en = 1;/*!< APB_1_PCLK & AHB_4_HCLKclk enable */
	pdsb_clken.ts_clk_en = 1;/*!< TS CLK clk enable */
	pdsb_clken.adc_lp_hv_clk_en = 1;/*!< ADC_LP_CK_IN_HV clk enable */
	pdsb_clken.I2C2AHB_DBG_en = 1;/*!< I2C2AHB_DBG clk enable */
	pdsb_clken.WDT0_en = 1;/*!< WDT0 clk enable */
	pdsb_clken.WDT1_en = 1;/*!< WDT1 clk enable */

	pdsb_clken.TIMER0_en = 1;/*!< TIMER0 clk enable */
	pdsb_clken.TIMER1_en = 1;/*!< TIMER1 clk enable */
	pdsb_clken.TIMER2_en = 1;/*!< TIMER2 clk enable */
	pdsb_clken.TIMER3_en = 1;/*!< TIMER3 clk enable */
	pdsb_clken.TIMER4_en = 1;/*!< TIMER4 clk enable */
	pdsb_clken.TIMER5_en = 1;/*!< TIMER5 clk enable */
	pdsb_clken.TIMER6_en = 1;/*!< TIMER6 clk enable */
	pdsb_clken.TIMER7_en = 1;/*!< TIMER7 clk enable */
	pdsb_clken.TIMER8_en = 1;/*!< TIMER8 clk enable */
	pdsb_clken.sb_gpio_en = 1;/*!< SB GPIO clk enable */
	pdsb_clken.hmxi2cm_en = 1;/*!< Himax I2C Master clk enable */
	hx_drv_scu_set_pdsb_clken_cfg(pdsb_clken);

	pdlsc_clken.cm55s_clk_en = 1;/*!< CM55S clk enable */
	pdlsc_clken.ahb_m_hclk_en = 1;/*!< AHB_M_HCLK clk enable */
	pdlsc_clken.ahb_2_hclk_en = 1;/*!< AHB_2_HCLK clk enable */
	pdlsc_clken.ahb_3_hclk_en = 1;/*!< AHB_3_HCLK clk enable */
	pdlsc_clken.apb_0_hclk_en = 1;/*!< APB_0_HCLK clk enable */
	pdlsc_clken.sram2_clk_en = 1;/*!< SRAM2 clk enable */
	pdlsc_clken.dma2_clk_en = 1;/*!< DMA2 clk enable */
	pdlsc_clken.dma3_clk_en = 1;/*!< DMA3 clk enable */
	pdlsc_clken.i2s_host_sclk_en = 1;/*!< i2s_host_sclk clk enable */
	pdlsc_clken.pdm_clk_en = 1;/*!< pdm_clk clk enable */
	pdlsc_clken.uart0_clk_en = 1;/*!< uart0_sclk clk enable */
	pdlsc_clken.uart1_clk_en = 1;/*!< uart1_sclk clk enable */
	pdlsc_clken.uart2_clk_en = 1;/*!< uart2_sclk clk enable */
	pdlsc_clken.i3c_slv0_sys_clk_en = 1;/*!< i3c_slv0_sys_clk clk enable */
	pdlsc_clken.i3c_slv1_sys_clk_en = 1;/*!< i3c_slv1_sys_clk clk enable */
	pdlsc_clken.pwm012_clk_en = 1;/*!< pwm012_clk clk enable */
	pdlsc_clken.i2s_slv_sclk_en = 1;/*!< i2s_slv_sclk clk enable */
	pdlsc_clken.ro_pd_clk_en = 1;/*!< RO_PD_CK_IN clk enable */
	pdlsc_clken.i2c_slv0_ic_clk_en = 1;/*!< i2c_slv0_ic_clk clk enable */
	pdlsc_clken.i2c_slv1_ic_clk_en = 1;/*!< i2c_slv1_ic_clk clk enable */
	pdlsc_clken.i2c_mst_ic_clk_en = 1;/*!< i2c_mst_ic_clk clk enable */
	pdlsc_clken.i2c_mst_sen_ic_clk_en = 1;/*!< i2c_mst_sen_ic_clk clk enable */
	pdlsc_clken.sw_clk_en = 1;/*!< sw_clk clk enable */
	pdlsc_clken.vad_d_clk_en = 1;/*!< VAD d clk enable */
	pdlsc_clken.adcck_en = 1;/*!< adcck clk enable */
	pdlsc_clken.gpio_en = 1;/*!< gpio clk enable */
	pdlsc_clken.sspim_en = 1;/*!< SSPI Master clk enable */
	pdlsc_clken.sspis_en = 1;/*!< SSPI Slave clk enable */
	pdlsc_clken.ckmon_en = 1;/*!< clock_monitor clock enable */
	pdlsc_clken.imageclk_en.sc_clk_lsc_en = 1;/*!< image clk enable */

	hx_drv_scu_set_pdlsc_clken_cfg(pdlsc_clken);

	pdhsc_clken.cm55m_clk_en = 1;/*!u55_clk_en< CM55M clk enable */
	pdhsc_clken.u55_clk_en = 1;/*!< U55 clk enable */
	pdhsc_clken.axi_clk_en = 1;/*!< AXI clk enable */
	pdhsc_clken.ahb0_clk_en = 1;/*!< AHB0 clk enable */
	pdhsc_clken.ahb5_clk_en = 1;/*!< AHB5 clk enable */
	pdhsc_clken.ahb1_clk_en = 1;/*!< AHB1 clk enable */
	pdhsc_clken.apb2_clk_en = 1;/*!< APB2 clk enable */
	pdhsc_clken.rom_clk_en = 1;/*!< ROM clk enable */
	pdhsc_clken.sram0_clk_en = 1;/*!< SRAM0 clk enable */
	pdhsc_clken.sram1_clk_en = 1;/*!< SRAM1 clk enable */
	pdhsc_clken.i3c_hc_clk_en = 1;/*!< I3C HC clk enable */
	pdhsc_clken.puf_clk_en = 1;/*!< puf clk enable */
	pdhsc_clken.dma0_clk_en = 1;/*!< DMA 0 clk enable */
	pdhsc_clken.dma1_clk_en = 1;/*!< DMA 1 clk enable */
	pdhsc_clken.sdio_clk_en = 1;/*!< SDIO clk enable */
	pdhsc_clken.i2c2ahb_flash_w_clk_en = 1;/*!< i2c2ahb flash write clk enable */
	pdhsc_clken.qspi_en = 1;/*!< QSPI clock enable */
	pdhsc_clken.ospi_en = 1;/*!< OSPI clock enable */
	pdhsc_clken.spi2ahb_en = 1;/*!< SPI2AHB clock enable */

	pdhsc_clken.imageclk_en.xdma_w1_clk_en = 1;/*!< XDMA W1 hclk & xdma_w1_pclk enable */
	pdhsc_clken.imageclk_en.xdma_w2_clk_en = 1;/*!< XDMA W2 hclk & xdma_w2_pclk enable */
	pdhsc_clken.imageclk_en.xdma_w3_clk_en = 1;/*!< XDMA W3 hclk & xdma_w3_pclk enable */
	pdhsc_clken.imageclk_en.xdma_r_clk_en = 1;/*!< XDMA R hclk & xdma_r_pclk enable */
	pdhsc_clken.imageclk_en.scclk_clk_en = 1;/*!< PD HSC SC clk enable */
	pdhsc_clken.imageclk_en.inp_clk_en = 1;/*!< PD HSC inp_cclk, inp_oclk clk enable */
	pdhsc_clken.imageclk_en.dp_clk_en = 1;/*!< PD HSC dp_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_2x2_clk_en = 1;/*!< PD HSC dp_2x2_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_5x5_clk_en = 1;/*!< PD HSC dp_5x5_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_cdm_clk_en = 1;/*!< PD HSC dp_cdm_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_jpeg_clk_en = 1;/*!< PD HSC dp_jpeg_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_tpg_clk_en = 1;/*!< PD HSC dp_tpg_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_edm_clk_en = 1;/*!< PD HSC dp_edm_pclk & dp_crc_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_rgb2yuv_pclk_en = 1;/*!< PD HSC dp_rgb2yuv_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_csc_pclk_en = 1;/*!< PD HSC dp_csc_pclk clk enable */
	pdhsc_clken.imageclk_en.mipirx_clk_en = 1;/*!< PD HSC mipirx_pclk clk enable */
	pdhsc_clken.imageclk_en.mipitx_clk_en = 1;/*!< PD HSC mipitx_pclk clk enable */

	hx_drv_scu_set_pdhsc_clken_cfg(pdhsc_clken);
}

//disable clock for boot with capture
static void app_pmu_bootwithcap_clk_disable(TIMER_ID_E sys_rtc_id)
{
	SCU_PDAON_CLKEN_CFG_T aonclken = {0};
	SCU_PDSB_CLKEN_CFG_T pdsb_clken = {0};
	SCU_PDLSC_CLKEN_CFG_T pdlsc_clken = {0};
	SCU_PDHSC_CLKEN_CFG_T pdhsc_clken = {0};

	aonclken.rtc0_clk_en = 0;/*!< RTC0 Clock enable */
	aonclken.rtc1_clk_en = 0;/*!< RTC1 Clock enable */
	aonclken.rtc2_clk_en = 0;/*!< RTC2 Clock enable */
	aonclken.pmu_clk_en = 1;/*!< PMU Clock enable */
	aonclken.aon_gpio_clk_en = 0;/*!< AON GPIO Clock enable */
	aonclken.aon_swreg_clk_en = 1;/*!< AON SW REG Clock enable */
	aonclken.antitamper_clk_en = 0;/*!< ANTI TAMPER Clock enable */
	hx_drv_scu_set_pdaon_clken_cfg(aonclken);

	pdsb_clken.apb1_ahb4_pclk_en = 1;/*!< APB_1_PCLK & AHB_4_HCLKclk enable */
	pdsb_clken.ts_clk_en = 0;/*!< TS CLK clk enable */
	pdsb_clken.adc_lp_hv_clk_en = 0;/*!< ADC_LP_CK_IN_HV clk enable */
	pdsb_clken.I2C2AHB_DBG_en = 0;/*!< I2C2AHB_DBG clk enable */
	pdsb_clken.WDT0_en = 0;/*!< WDT0 clk enable */
	pdsb_clken.WDT1_en = 0;/*!< WDT1 clk enable */

	switch (sys_rtc_id)
	{
		case TIMER_ID_0:
			pdsb_clken.TIMER0_en = 1;/*!< TIMER0 clk enable */
			break;
		case TIMER_ID_1:
			pdsb_clken.TIMER1_en = 1;/*!< TIMER1 clk enable */
			break;
		case TIMER_ID_2:
			pdsb_clken.TIMER2_en = 1;/*!< TIMER2 clk enable */
			break;
		case TIMER_ID_3:
			pdsb_clken.TIMER3_en = 1;/*!< TIMER3 clk enable */
			break;
		case TIMER_ID_4:
			pdsb_clken.TIMER4_en = 1;/*!< TIMER4 clk enable */
			break;
		case TIMER_ID_5:
			pdsb_clken.TIMER5_en = 1;/*!< TIMER5 clk enable */
			break;
		case TIMER_ID_6:
			pdsb_clken.TIMER6_en = 1;/*!< TIMER6 clk enable */
			break;
		case TIMER_ID_7:
			pdsb_clken.TIMER7_en = 1;/*!< TIMER7 clk enable */
			break;
		case TIMER_ID_8:
			pdsb_clken.TIMER8_en = 1;/*!< TIMER8 clk enable */
			break;
		default:
			break;
	}
	
	pdsb_clken.sb_gpio_en = 0;/*!< SB GPIO clk enable */
	pdsb_clken.hmxi2cm_en = 0;/*!< Himax I2C Master clk enable */
	hx_drv_scu_set_pdsb_clken_cfg(pdsb_clken);

	pdlsc_clken.cm55s_clk_en = 0;/*!< CM55S clk enable */
	pdlsc_clken.ahb_m_hclk_en = 1;/*!< AHB_M_HCLK clk enable */
	pdlsc_clken.ahb_2_hclk_en = 1;/*!< AHB_2_HCLK clk enable */
	pdlsc_clken.ahb_3_hclk_en = 1;/*!< AHB_3_HCLK clk enable */
	pdlsc_clken.apb_0_hclk_en = 1;/*!< APB_0_HCLK clk enable */
	pdlsc_clken.sram2_clk_en = 0;/*!< SRAM2 clk enable */
	pdlsc_clken.dma2_clk_en = 0;/*!< DMA2 clk enable */
	pdlsc_clken.dma3_clk_en = 0;/*!< DMA3 clk enable */
	pdlsc_clken.i2s_host_sclk_en = 0;/*!< i2s_host_sclk clk enable */
	pdlsc_clken.pdm_clk_en = 0;/*!< pdm_clk clk enable */
	pdlsc_clken.uart0_clk_en = 0;/*!< uart0_sclk clk enable */
	pdlsc_clken.uart1_clk_en = 0;/*!< uart1_sclk clk enable */
	pdlsc_clken.uart2_clk_en = 0;/*!< uart2_sclk clk enable */
	pdlsc_clken.i3c_slv0_sys_clk_en = 0;/*!< i3c_slv0_sys_clk clk enable */
	pdlsc_clken.i3c_slv1_sys_clk_en = 0;/*!< i3c_slv1_sys_clk clk enable */
	pdlsc_clken.pwm012_clk_en = 0;/*!< pwm012_clk clk enable */
	pdlsc_clken.i2s_slv_sclk_en = 0;/*!< i2s_slv_sclk clk enable */
	pdlsc_clken.ro_pd_clk_en = 0;/*!< RO_PD_CK_IN clk enable */
	pdlsc_clken.i2c_slv0_ic_clk_en = 0;/*!< i2c_slv0_ic_clk clk enable */
	pdlsc_clken.i2c_slv1_ic_clk_en = 0;/*!< i2c_slv1_ic_clk clk enable */
	pdlsc_clken.i2c_mst_ic_clk_en = 0;/*!< i2c_mst_ic_clk clk enable */
	pdlsc_clken.i2c_mst_sen_ic_clk_en = 1;/*!< i2c_mst_sen_ic_clk clk enable */
	pdlsc_clken.sw_clk_en = 0;/*!< sw_clk clk enable */
	pdlsc_clken.vad_d_clk_en = 0;/*!< VAD d clk enable */
	pdlsc_clken.adcck_en = 0;/*!< adcck clk enable */
	pdlsc_clken.gpio_en = 0;/*!< gpio clk enable */
	pdlsc_clken.sspim_en = 0;/*!< SSPI Master clk enable */
	pdlsc_clken.sspis_en = 0;/*!< SSPI Slave clk enable */
	pdlsc_clken.ckmon_en = 0;/*!< clock_monitor clock enable */
	pdlsc_clken.imageclk_en.sc_clk_lsc_en = 1;/*!< image clk enable */
	hx_drv_scu_set_pdlsc_clken_cfg(pdlsc_clken);

	pdhsc_clken.cm55m_clk_en = 1;/*!u55_clk_en< CM55M clk enable */
	pdhsc_clken.u55_clk_en = 0;/*!< U55 clk enable */
	pdhsc_clken.axi_clk_en = 1;/*!< AXI clk enable */
	pdhsc_clken.ahb0_clk_en = 1;/*!< AHB0 clk enable */
	pdhsc_clken.ahb5_clk_en = 1;/*!< AHB5 clk enable */
	pdhsc_clken.ahb1_clk_en = 1;/*!< AHB1 clk enable */
	pdhsc_clken.apb2_clk_en = 1;/*!< APB2 clk enable */
	pdhsc_clken.rom_clk_en = 1;/*!< ROM clk enable */
	pdhsc_clken.sram0_clk_en = 1;/*!< SRAM0 clk enable */
	pdhsc_clken.sram1_clk_en = 1;/*!< SRAM1 clk enable */
	pdhsc_clken.i3c_hc_clk_en = 0;/*!< I3C HC clk enable */
	pdhsc_clken.puf_clk_en = 1;/*!< puf clk enable */
	pdhsc_clken.dma0_clk_en = 0;/*!< DMA 0 clk enable */
	pdhsc_clken.dma1_clk_en = 0;/*!< DMA 1 clk enable */
	pdhsc_clken.sdio_clk_en = 0;/*!< SDIO clk enable */
	pdhsc_clken.i2c2ahb_flash_w_clk_en = 0;/*!< i2c2ahb flash write clk enable */
	pdhsc_clken.qspi_en = 0;/*!< QSPI clock enable */
	pdhsc_clken.ospi_en = 0;/*!< OSPI clock enable */
	pdhsc_clken.spi2ahb_en = 0;/*!< SPI2AHB clock enable */

	pdhsc_clken.imageclk_en.xdma_w1_clk_en = 0;/*!< XDMA W1 hclk & xdma_w1_pclk enable */
	pdhsc_clken.imageclk_en.xdma_w2_clk_en = 1;/*!< XDMA W2 hclk & xdma_w2_pclk enable */
	pdhsc_clken.imageclk_en.xdma_w3_clk_en = 1;/*!< XDMA W3 hclk & xdma_w3_pclk enable */
	pdhsc_clken.imageclk_en.xdma_r_clk_en = 0;/*!< XDMA R hclk & xdma_r_pclk enable */
	pdhsc_clken.imageclk_en.scclk_clk_en = 1;/*!< PD HSC SC clk enable */
	pdhsc_clken.imageclk_en.inp_clk_en = 1;/*!< PD HSC inp_cclk, inp_oclk clk enable */
	pdhsc_clken.imageclk_en.dp_clk_en = 1;/*!< PD HSC dp_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_2x2_clk_en = 0;/*!< PD HSC dp_2x2_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_5x5_clk_en = 1;/*!< PD HSC dp_5x5_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_cdm_clk_en = 0;/*!< PD HSC dp_cdm_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_jpeg_clk_en = 1;/*!< PD HSC dp_jpeg_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_tpg_clk_en = 0;/*!< PD HSC dp_tpg_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_edm_clk_en = 1;/*!< PD HSC dp_edm_pclk & dp_crc_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_rgb2yuv_pclk_en = 1;/*!< PD HSC dp_rgb2yuv_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_csc_pclk_en = 1;/*!< PD HSC dp_csc_pclk clk enable */
	
	#if defined(HM2170_BAYER) || defined(HM2170_MIPI)
	pdhsc_clken.imageclk_en.mipirx_clk_en = 1;/*!< PD HSC mipirx_pclk clk enable */
	#else
	pdhsc_clken.imageclk_en.mipirx_clk_en = 0;/*!< PD HSC mipirx_pclk clk enable */
	#endif

	pdhsc_clken.imageclk_en.mipitx_clk_en = 0;/*!< PD HSC mipitx_pclk clk enable */
	hx_drv_scu_set_pdhsc_clken_cfg(pdhsc_clken);
}

//disable clock for video preroll
static void app_pmu_vidpre_clk_disable(TIMER_ID_E sys_rtc_id)
{
	SCU_PDAON_CLKEN_CFG_T aonclken = {0};
	SCU_PDSB_CLKEN_CFG_T pdsb_clken = {0};
	SCU_PDLSC_CLKEN_CFG_T pdlsc_clken = {0};
	SCU_PDHSC_CLKEN_CFG_T pdhsc_clken = {0};
	
	aonclken.rtc0_clk_en = 0;/*!< RTC0 Clock enable */
	aonclken.rtc1_clk_en = 0;/*!< RTC1 Clock enable */
	aonclken.rtc2_clk_en = 0;/*!< RTC2 Clock enable */
	aonclken.pmu_clk_en = 1;/*!< PMU Clock enable */
	aonclken.aon_gpio_clk_en = 1;/*!< AON GPIO Clock enable */
	aonclken.aon_swreg_clk_en = 1;/*!< AON SW REG Clock enable */
	aonclken.antitamper_clk_en = 0;/*!< ANTI TAMPER Clock enable */
	hx_drv_scu_set_pdaon_clken_cfg(aonclken);
	
	pdsb_clken.apb1_ahb4_pclk_en = 1;/*!< APB_1_PCLK & AHB_4_HCLKclk enable */
	pdsb_clken.ts_clk_en = 0;/*!< TS CLK clk enable */
	pdsb_clken.adc_lp_hv_clk_en = 0;/*!< ADC_LP_CK_IN_HV clk enable */
	pdsb_clken.I2C2AHB_DBG_en = 0;/*!< I2C2AHB_DBG clk enable */
	pdsb_clken.WDT0_en = 0;/*!< WDT0 clk enable */
	pdsb_clken.WDT1_en = 0;/*!< WDT1 clk enable */
	pdsb_clken.TIMER0_en = 1;/*!< TIMER0 clk enable */
	
	switch (sys_rtc_id)
	{
		case TIMER_ID_0:
			pdsb_clken.TIMER0_en = 1;/*!< TIMER0 clk enable */
			break;
		case TIMER_ID_1:
			pdsb_clken.TIMER1_en = 1;/*!< TIMER1 clk enable */
			break;
		case TIMER_ID_2:
			pdsb_clken.TIMER2_en = 1;/*!< TIMER2 clk enable */
			break;
		case TIMER_ID_3:
			pdsb_clken.TIMER3_en = 1;/*!< TIMER3 clk enable */
			break;
		case TIMER_ID_4:
			pdsb_clken.TIMER4_en = 1;/*!< TIMER4 clk enable */
			break;
		case TIMER_ID_5:
			pdsb_clken.TIMER5_en = 1;/*!< TIMER5 clk enable */
			break;
		case TIMER_ID_6:
			pdsb_clken.TIMER6_en = 1;/*!< TIMER6 clk enable */
			break;
		case TIMER_ID_7:
			pdsb_clken.TIMER7_en = 1;/*!< TIMER7 clk enable */
			break;
		case TIMER_ID_8:
			pdsb_clken.TIMER8_en = 1;/*!< TIMER8 clk enable */
			break;
		default:
			break;
	}
	
	pdsb_clken.sb_gpio_en = 0;/*!< SB GPIO clk enable */
	pdsb_clken.hmxi2cm_en = 0;/*!< Himax I2C Master clk enable */
	hx_drv_scu_set_pdsb_clken_cfg(pdsb_clken);
	
	pdlsc_clken.cm55s_clk_en = 0;/*!< CM55S clk enable */
	pdlsc_clken.ahb_m_hclk_en = 1;/*!< AHB_M_HCLK clk enable */
	pdlsc_clken.ahb_2_hclk_en = 1;/*!< AHB_2_HCLK clk enable */
	pdlsc_clken.ahb_3_hclk_en = 1;/*!< AHB_3_HCLK clk enable */
	pdlsc_clken.apb_0_hclk_en = 1;/*!< APB_0_HCLK clk enable */
	pdlsc_clken.sram2_clk_en = 0;/*!< SRAM2 clk enable */
	pdlsc_clken.dma2_clk_en = 0;/*!< DMA2 clk enable */
	pdlsc_clken.dma3_clk_en = 0;/*!< DMA3 clk enable */
	pdlsc_clken.i2s_host_sclk_en = 0;/*!< i2s_host_sclk clk enable */
	pdlsc_clken.pdm_clk_en = 0;/*!< pdm_clk clk enable */
	pdlsc_clken.uart0_clk_en = 0;/*!< uart0_sclk clk enable */
	pdlsc_clken.uart1_clk_en = 0;/*!< uart1_sclk clk enable */
	pdlsc_clken.uart2_clk_en = 0;/*!< uart2_sclk clk enable */
	pdlsc_clken.i3c_slv0_sys_clk_en = 0;/*!< i3c_slv0_sys_clk clk enable */
	pdlsc_clken.i3c_slv1_sys_clk_en = 0;/*!< i3c_slv1_sys_clk clk enable */
	pdlsc_clken.pwm012_clk_en = 0;/*!< pwm012_clk clk enable */
	pdlsc_clken.i2s_slv_sclk_en = 0;/*!< i2s_slv_sclk clk enable */
	pdlsc_clken.ro_pd_clk_en = 0;/*!< RO_PD_CK_IN clk enable */
	pdlsc_clken.i2c_slv0_ic_clk_en = 0;/*!< i2c_slv0_ic_clk clk enable */
	pdlsc_clken.i2c_slv1_ic_clk_en = 0;/*!< i2c_slv1_ic_clk clk enable */
	pdlsc_clken.i2c_mst_ic_clk_en = 0;/*!< i2c_mst_ic_clk clk enable */
	pdlsc_clken.i2c_mst_sen_ic_clk_en = 0;/*!< i2c_mst_sen_ic_clk clk enable */
	pdlsc_clken.sw_clk_en = 0;/*!< sw_clk clk enable */
	pdlsc_clken.vad_d_clk_en = 0;/*!< VAD d clk enable */
	pdlsc_clken.adcck_en = 0;/*!< adcck clk enable */
	pdlsc_clken.gpio_en = 0;/*!< gpio clk enable */
	pdlsc_clken.sspim_en = 0;/*!< SSPI Master clk enable */
	pdlsc_clken.sspis_en = 0;/*!< SSPI Slave clk enable */
	pdlsc_clken.ckmon_en = 0;/*!< clock_monitor clock enable */
	pdlsc_clken.imageclk_en.sc_clk_lsc_en = 1;/*!< image clk enable */
	hx_drv_scu_set_pdlsc_clken_cfg(pdlsc_clken);

	pdhsc_clken.cm55m_clk_en = 1;/*!u55_clk_en< CM55M clk enable */
	pdhsc_clken.u55_clk_en = 0;/*!< U55 clk enable */
	pdhsc_clken.axi_clk_en = 1;/*!< AXI clk enable */
	pdhsc_clken.ahb0_clk_en = 1;/*!< AHB0 clk enable */
	pdhsc_clken.ahb5_clk_en = 1;/*!< AHB5 clk enable */
	pdhsc_clken.ahb1_clk_en = 1;/*!< AHB1 clk enable */
	pdhsc_clken.apb2_clk_en = 1;/*!< APB2 clk enable */
	pdhsc_clken.rom_clk_en = 1;/*!< ROM clk enable */
	pdhsc_clken.sram0_clk_en = 1;/*!< SRAM0 clk enable */
	pdhsc_clken.sram1_clk_en = 1;/*!< SRAM1 clk enable */
	pdhsc_clken.i3c_hc_clk_en = 0;/*!< I3C HC clk enable */
	pdhsc_clken.puf_clk_en = 1;/*!< puf clk enable */
	pdhsc_clken.dma0_clk_en = 0;/*!< DMA 0 clk enable */
	pdhsc_clken.dma1_clk_en = 0;/*!< DMA 1 clk enable */
	pdhsc_clken.sdio_clk_en = 0;/*!< SDIO clk enable */
	pdhsc_clken.i2c2ahb_flash_w_clk_en = 0;/*!< i2c2ahb flash write clk enable */
	pdhsc_clken.qspi_en = 0;/*!< QSPI clock enable */
	pdhsc_clken.ospi_en = 0;/*!< OSPI clock enable */
	pdhsc_clken.spi2ahb_en = 0;/*!< SPI2AHB clock enable */
	pdhsc_clken.imageclk_en.xdma_w1_clk_en = 1;/*!< XDMA W1 hclk & xdma_w1_pclk enable */
	pdhsc_clken.imageclk_en.xdma_w2_clk_en = 1;/*!< XDMA W2 hclk & xdma_w2_pclk enable */
	pdhsc_clken.imageclk_en.xdma_w3_clk_en = 1;/*!< XDMA W3 hclk & xdma_w3_pclk enable */
	pdhsc_clken.imageclk_en.xdma_r_clk_en = 1;/*!< XDMA R hclk & xdma_r_pclk enable */
	pdhsc_clken.imageclk_en.scclk_clk_en = 1;/*!< PD HSC SC clk enable */
	pdhsc_clken.imageclk_en.inp_clk_en = 1;/*!< PD HSC inp_cclk, inp_oclk clk enable */
	pdhsc_clken.imageclk_en.dp_clk_en = 1;/*!< PD HSC dp_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_2x2_clk_en = 1;/*!< PD HSC dp_2x2_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_5x5_clk_en = 1;/*!< PD HSC dp_5x5_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_cdm_clk_en = 1;/*!< PD HSC dp_cdm_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_jpeg_clk_en = 1;/*!< PD HSC dp_jpeg_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_tpg_clk_en = 1;/*!< PD HSC dp_tpg_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_edm_clk_en = 1;/*!< PD HSC dp_edm_pclk & dp_crc_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_rgb2yuv_pclk_en = 1;/*!< PD HSC dp_rgb2yuv_pclk clk enable */
	pdhsc_clken.imageclk_en.dp_csc_pclk_en = 1;/*!< PD HSC dp_csc_pclk clk enable */
	pdhsc_clken.imageclk_en.mipirx_clk_en = 0;/*!< PD HSC mipirx_pclk clk enable */
	pdhsc_clken.imageclk_en.mipitx_clk_en = 0;/*!< PD HSC mipitx_pclk clk enable */
	hx_drv_scu_set_pdhsc_clken_cfg(pdhsc_clken);
}
#endif

static void app_pmu_wakeup_gpio_cfg(AppCfgCustGpio_t *gpio_cfg)
{
	dbg_app_log("gpio_cfg->act_wakeupCPU_pin_cnt=%u\n", gpio_cfg->act_wakeupCPU_pin_cnt);

    for(uint32_t i = 0; i < gpio_cfg->act_wakeupCPU_pin_cnt; i++)
    {
        dbg_app_log("gpio_cfg->wakeupCPU_int_pin[%u]=0x%x\n", i, gpio_cfg->wakeupCPU_int_pin[i]);
        
        hx_drv_gpio_set_int_enable(gpio_cfg->wakeupCPU_int_pin[i], 0);
        hx_drv_gpio_set_int_type(gpio_cfg->wakeupCPU_int_pin[i], GPIO_IRQ_TRIG_TYPE_EDGE_RISING);
		//hx_drv_gpio_set_int_type(gpio_cfg->wakeupCPU_int_pin[i], GPIO_IRQ_TRIG_TYPE_EDGE_FALLING);
		#if 0 //test trunk, temporary removed
        hx_drv_gpio_set_irq_handle(gpio_cfg->wakeupCPU_int_pin[i], GPIO_IRQ_HANDLE_CLR_INT);
		#endif
        hx_drv_gpio_set_input(gpio_cfg->wakeupCPU_int_pin[i]);
        hx_drv_gpio_cb_register(gpio_cfg->wakeupCPU_int_pin[i], priv_pmu_gpio_cb);
        hx_drv_gpio_set_int_enable(gpio_cfg->wakeupCPU_int_pin[i], 1);
    }
}

static void app_pmu_sys_rtc_start(uint32_t sys_rtc_ms)
{
	TIMER_CFG_T timer_cfg = {0};

	timer_cfg.period = sys_rtc_ms;
	timer_cfg.mode = TIMER_MODE_ONESHOT;
	timer_cfg.ctrl = TIMER_CTRL_PMU;
	timer_cfg.state = TIMER_STATE_PMU;
	
	hx_drv_timer_hw_start(APP_PMU_SYS_RTC_ID, &timer_cfg, NULL);
}

void app_pmu_enter_sleep1_aos(AppCfgCustGpio_t gpio_cfg, uint32_t sensor_rtc_ms, uint32_t sys_rtc_ms, uint8_t quickboot)
{
    PM_PD_VIDPRE_CFG_T cfg;
    SCU_LSC_CLK_CFG_T lsc_cfg;
    SCU_PDHSC_HSCCLK_CFG_T hsc_cfg;

	uint32_t freq;
	SCU_PLL_FREQ_E pmuwakeup_pll_freq;
	SCU_HSCCLKDIV_E pmuwakeup_cm55m_div;
	SCU_LSCCLKDIV_E pmuwakeup_cm55s_div;

	/*Get System Current Clock*/
	hx_drv_swreg_aon_get_pmuwakeup_freq(&pmuwakeup_pll_freq, &pmuwakeup_cm55m_div, &pmuwakeup_cm55s_div);
	hx_drv_swreg_aon_get_pllfreq(&freq);
	dbg_app_log("pmuwakeup_freq_type=%d, pmuwakeup_cm55m_div=%d, pmuwakeup_cm55s_div=%d\n", pmuwakeup_pll_freq, pmuwakeup_cm55m_div, pmuwakeup_cm55s_div);
	dbg_app_log("pmuwakeup_run_freq=%d\n", freq);
    
    hx_lib_pm_get_defcfg_bymode(&cfg, PM_MODE_PS_VID_ONLY_PREROLLING);
    
	/*Setup bootrom clock speed when PMU Warm boot wakeup*/
	cfg.bootromspeed.bootromclkfreq = pmuwakeup_pll_freq;
	cfg.bootromspeed.pll_freq = freq;
	cfg.bootromspeed.cm55m_div = pmuwakeup_cm55m_div;
	cfg.bootromspeed.cm55s_div = pmuwakeup_cm55s_div;
	
    cfg.cm55s_reset = SWREG_AON_PMUWAKE_CM55S_RERESET_YES;
    cfg.pmu_senint_mask = PM_IP_INT_MASK_ALL_UNMASK;
    cfg.sensor_type = PM_SENSOR_TIMING_FVLDLVLD_SHIFT;

    //cfg.support_debugdump = 1;
    
    cfg.pmu_pad_pa01_mask = PM_IP_INT_MASK;
    cfg.pmu_pad_pa23_mask = PM_IP_INT_MASK;
    
    for(uint32_t i = 0; i < gpio_cfg.act_wakeupCPU_pin_cnt; i++)
    {
        if(gpio_cfg.wakeupCPU_int_pin[i] == HX_AON_GPIO_0 || gpio_cfg.wakeupCPU_int_pin[i] == HX_AON_GPIO_1)
            cfg.pmu_pad_pa01_mask = PM_IP_INT_MASK_ALL_UNMASK;
        else if(gpio_cfg.wakeupCPU_int_pin[i] == HX_SB_GPIO_0 || gpio_cfg.wakeupCPU_int_pin[i] == HX_SB_GPIO_1)
            cfg.pmu_pad_pa23_mask = PM_IP_INT_MASK_ALL_UNMASK;
    }
    
    cfg.sensor_timer = sensor_rtc_ms;
    cfg.wdt_timer = sensor_rtc_ms+1000;
	
	cfg.pmu_timer_mask = PM_TIMER_INT_MASK_TIMER_ALLMASK;
	cfg.pmu_timer_mask &= (~PM_TIMER_INT_MASK_TIMER0); /*TIMER_ID_0, sensor_rtc_ms*/

	if(sys_rtc_ms > 0) /*sys_rtc_ms, depends on APP_PMU_SYS_RTC_ID setting*/
		cfg.pmu_timer_mask &= (~(0x1 << APP_PMU_SYS_RTC_ID));

    if(quickboot == 0)
    {
        cfg.skip_bootflow.sec_mem_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.first_bl_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55m_s_app_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55m_ns_app_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55s_s_app_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55s_ns_app_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55m_model_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55s_model_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55m_appcfg_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55s_appcfg_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55m_s_app_rwdata_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55m_ns_app_rwdata_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55s_s_app_rwdata_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55s_ns_app_rwdata_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.secure_debug_flag = SWREG_AON_NO_RETENTION;
    }
    
    //priv_pmu_print_vidpre_cfg(&cfg);
	
    hx_lib_pm_cfg_set(&cfg, sensordplib_pmudpinit, PM_MODE_PS_VID_ONLY_PREROLLING);
    
    app_pmu_wakeup_gpio_cfg(&gpio_cfg);
	
	hx_drv_timer_hw_stop(APP_PMU_SYS_RTC_ID);
	
    if(sys_rtc_ms > 0)
        app_pmu_sys_rtc_start(sys_rtc_ms);
	
	#if defined(PMU_SLEEP_CLK_DISABLE)
	if(sys_rtc_ms > 0)
    	app_pmu_vidpre_clk_disable(APP_PMU_SYS_RTC_ID);
	else
    	app_pmu_vidpre_clk_disable(TIMER_ID_MAX); //none SYS_RTC used
    #endif

	#if defined(PMU_SIMO_VOLT)
	set_simo_0p8v();
	#endif
	
    /*Use PMU lib to control HSC_CLK and LSC_CLK so set thoes parameter to 0*/
    memset(&hsc_cfg, 0, sizeof(SCU_PDHSC_HSCCLK_CFG_T));
    memset(&lsc_cfg, 0, sizeof(SCU_LSC_CLK_CFG_T));
    
    /*Trigger to PMU mode*/
    #if 1
    hx_lib_pm_trigger(hsc_cfg, lsc_cfg, PM_CLK_PARA_CTRL_BYPMLIB);
    #else
    priv_pmu_set_clk_cfg(SCU_SYSCLKSRC_HW_XTAL24MRC32K, &lsc_cfg, &hsc_cfg);
	
    dbg_app_log("hx_lib_pm_trigger\n");
    hx_lib_pm_trigger(hsc_cfg, lsc_cfg, PM_CLK_PARA_CTRL_BYAPP);
    #endif
}

void app_pmu_enter_sleep1_bootwithcap(AppCfgCustGpio_t gpio_cfg, uint32_t sys_rtc_ms)
{
    PM_PD_NOVIDPRE_CFG_T cfg;
    SCU_LSC_CLK_CFG_T lsc_cfg;
    SCU_PDHSC_HSCCLK_CFG_T hsc_cfg;
	
	uint32_t freq;
	SCU_PLL_FREQ_E pmuwakeup_pll_freq;
	SCU_HSCCLKDIV_E pmuwakeup_cm55m_div;
	SCU_LSCCLKDIV_E pmuwakeup_cm55s_div;

	/*Get System Current Clock*/
	hx_drv_swreg_aon_get_pmuwakeup_freq(&pmuwakeup_pll_freq, &pmuwakeup_cm55m_div, &pmuwakeup_cm55s_div);
	hx_drv_swreg_aon_get_pllfreq(&freq);
	dbg_app_log("pmuwakeup_freq_type=%d, pmuwakeup_cm55m_div=%d, pmuwakeup_cm55s_div=%d\n", pmuwakeup_pll_freq, pmuwakeup_cm55m_div, pmuwakeup_cm55s_div);
	dbg_app_log("pmuwakeup_run_freq=%d\n", freq);

    hx_lib_pm_get_defcfg_bymode(&cfg, PM_MODE_PS_NOVID_PREROLLING);

	/*Setup bootrom clock speed when PMU Warm boot wakeup*/
	cfg.bootromspeed.bootromclkfreq = pmuwakeup_pll_freq;
	cfg.bootromspeed.pll_freq = freq;
	cfg.bootromspeed.cm55m_div = pmuwakeup_cm55m_div;
	cfg.bootromspeed.cm55s_div = pmuwakeup_cm55s_div;
	
    //cfg.support_debugdump = 1;

	cfg.pmu_i2cw_mask = PM_IP_INT_MASK;
	cfg.pmu_cmp_mask = PM_IP_INT_MASK;
	cfg.pmu_ts_mask = PM_IP_INT_MASK;
	cfg.pmu_anti_mask = PM_IP_INT_MASK;

	cfg.support_bootwithcap = PM_BOOTWITHCAP_YES;
	
    cfg.pmu_pad_pa01_mask = PM_IP_INT_MASK;
    cfg.pmu_pad_pa23_mask = PM_IP_INT_MASK;
    
    for(uint32_t i = 0; i < gpio_cfg.act_wakeupCPU_pin_cnt; i++)
    {
        if(gpio_cfg.wakeupCPU_int_pin[i] == HX_AON_GPIO_0 || gpio_cfg.wakeupCPU_int_pin[i] == HX_AON_GPIO_1)
            cfg.pmu_pad_pa01_mask = PM_IP_INT_MASK_ALL_UNMASK;
        else if(gpio_cfg.wakeupCPU_int_pin[i] == HX_SB_GPIO_0 || gpio_cfg.wakeupCPU_int_pin[i] == HX_SB_GPIO_1)
            cfg.pmu_pad_pa23_mask = PM_IP_INT_MASK_ALL_UNMASK;
    }
	
	cfg.pmu_timer_mask = PM_TIMER_INT_MASK_TIMER_ALLMASK;
	
	if(sys_rtc_ms > 0) /* sys_rtc_ms, depends on APP_PMU_SYS_RTC_ID setting*/
		cfg.pmu_timer_mask &= (~(0x1 << APP_PMU_SYS_RTC_ID));
	
	/*Setup Memory retention*/
	cfg.tcm_retention = PM_MEM_RET_YES;		/**< CM55M TCM Retention**/
	cfg.hscsram_retention[0] = PM_MEM_RET_YES;	/**< HSC SRAM Retention**/
	cfg.hscsram_retention[1] = PM_MEM_RET_YES;	/**< HSC SRAM Retention**/
	cfg.hscsram_retention[2] = PM_MEM_RET_YES;	/**< HSC SRAM Retention**/
	cfg.hscsram_retention[3] = PM_MEM_RET_YES;	/**< HSC SRAM Retention**/
	cfg.lscsram_retention = PM_MEM_RET_NO;		/**< LSC SRAM Retention**/
	cfg.skip_bootflow.sec_mem_flag = SWREG_AON_RETENTION;			/**< Skip Boot Flow**/
	cfg.skip_bootflow.first_bl_flag = SWREG_AON_RETENTION; /*!< First BL Retention */
	cfg.skip_bootflow.cm55m_s_app_flag = SWREG_AON_RETENTION; /*!< cm55m_s_app Retention */
	cfg.skip_bootflow.cm55m_ns_app_flag = SWREG_AON_RETENTION; /*!< cm55m_ns_app Retention */
	cfg.skip_bootflow.cm55s_s_app_flag = SWREG_AON_NO_RETENTION; /*!< cm55s_s_app Retention */
	cfg.skip_bootflow.cm55s_ns_app_flag = SWREG_AON_NO_RETENTION; /*!< cm55s_ns_app Retention */
	cfg.skip_bootflow.cm55m_model_flag = SWREG_AON_RETENTION; /*!< cm55m model Retention */
	cfg.skip_bootflow.cm55s_model_flag = SWREG_AON_NO_RETENTION; /*!< cm55s model Retention */
	cfg.skip_bootflow.cm55m_appcfg_flag = SWREG_AON_RETENTION; /*!< cm55m appcfg Retention */
	cfg.skip_bootflow.cm55s_appcfg_flag = SWREG_AON_NO_RETENTION; /*!< cm55s appcfg Retention */
	cfg.skip_bootflow.cm55m_s_app_rwdata_flag = SWREG_AON_NO_RETENTION;/*!< cm55m_s_app RW Data Retention */
	cfg.skip_bootflow.cm55m_ns_app_rwdata_flag = SWREG_AON_NO_RETENTION;/*!< cm55m_ns_app RW Data Retention */
	cfg.skip_bootflow.cm55s_s_app_rwdata_flag = SWREG_AON_NO_RETENTION;/*!< cm55s_s_app RW Data Retention */
	cfg.skip_bootflow.cm55s_ns_app_rwdata_flag = SWREG_AON_NO_RETENTION;/*!< cm55s_ns_app RW Data Retention */
	cfg.skip_bootflow.secure_debug_flag = SWREG_AON_RETENTION;

	#if defined(HM0360_MONO) || defined(HM0360_BAYER) || defined(HM11B1_MONO)
	/**< HW pin control to capture frame **/
	cfg.nframeend_ctrl = PMU_NFRAMEEND_CTRL_SC;
	/*No MIPI*/
	cfg.mipi_lane_en = PMU_MIPI_LANE_ALL_DISABLE;
	#endif
	
	#if defined(HM2170_BAYER) || defined(HM2170_MIPI)
	/**< Auto I2C control to capture frame **/
	cfg.nframeend_ctrl = PMU_NFRAMEEND_CTRL_I2C;
	/*MIPI 2lanes*/
	cfg.mipi_lane_en = PMU_MIPI_LANE_ALL_LANE_EN;
	#endif
	
	/*Not DCDC pin output*/
	cfg.pmu_dcdc_outpin = PM_CFG_DCDC_MODE_OFF;
	/** No Pre-capture when boot up**/
	cfg.ioret = PM_CFG_PD_IORET_ON;
	
	cfg.sensor_type = PM_SENSOR_TIMING_FVLDLVLD_CON;
	/*SIMO on in PD*/
	cfg.simo_pd_onoff = PM_SIMO_PD_ONOFF_ON;
	
	//priv_pmu_print_novidpre_cfg(&cfg);
	
    hx_lib_pm_cfg_set(&cfg, sensordplib_pmudpinit, PM_MODE_PS_NOVID_PREROLLING);
    
    app_pmu_wakeup_gpio_cfg(&gpio_cfg);
	
	hx_drv_timer_hw_stop(APP_PMU_SYS_RTC_ID);
	
    if(sys_rtc_ms > 0)
        app_pmu_sys_rtc_start(sys_rtc_ms);

	#if defined(PMU_SLEEP_CLK_DISABLE)
	if(sys_rtc_ms > 0)
    	app_pmu_bootwithcap_clk_disable(APP_PMU_SYS_RTC_ID);
	else
    	app_pmu_bootwithcap_clk_disable(TIMER_ID_MAX); //none SYS_RTC used
    #endif

	#if defined(PMU_SIMO_VOLT)
	set_simo_0p8v();
	#endif
	
    /*Use PMU lib to control HSC_CLK and LSC_CLK so set thoes parameter to 0*/
	memset(&hsc_cfg, 0, sizeof(SCU_PDHSC_HSCCLK_CFG_T));
	memset(&lsc_cfg, 0, sizeof(SCU_LSC_CLK_CFG_T));
    
	/*Trigger to PMU mode*/
    #if 1
    hx_lib_pm_trigger(hsc_cfg, lsc_cfg, PM_CLK_PARA_CTRL_BYPMLIB);
    #else
    priv_pmu_set_clk_cfg(SCU_SYSCLKSRC_HW_XTAL24MRC32K, &lsc_cfg, &hsc_cfg);
	
    dbg_app_log("hx_lib_pm_trigger\n");
    hx_lib_pm_trigger(hsc_cfg, lsc_cfg, PM_CLK_PARA_CTRL_BYAPP);
    #endif
}

static void priv_pmu_enter_novidpre(AppCfgCustGpio_t gpio_cfg, uint32_t sys_rtc_ms, uint8_t quickboot, uint8_t retention)
{
    PM_PD_NOVIDPRE_CFG_T cfg;
    SCU_LSC_CLK_CFG_T lsc_cfg;
    SCU_PDHSC_HSCCLK_CFG_T hsc_cfg;

	uint32_t freq;
	SCU_PLL_FREQ_E pmuwakeup_pll_freq;
	SCU_HSCCLKDIV_E pmuwakeup_cm55m_div;
	SCU_LSCCLKDIV_E pmuwakeup_cm55s_div;

	/*Get System Current Clock*/
	hx_drv_swreg_aon_get_pmuwakeup_freq(&pmuwakeup_pll_freq, &pmuwakeup_cm55m_div, &pmuwakeup_cm55s_div);
	hx_drv_swreg_aon_get_pllfreq(&freq);
	dbg_app_log("pmuwakeup_freq_type=%d, pmuwakeup_cm55m_div=%d, pmuwakeup_cm55s_div=%d\n", pmuwakeup_pll_freq, pmuwakeup_cm55m_div, pmuwakeup_cm55s_div);
	dbg_app_log("pmuwakeup_run_freq=%d\n", freq);

    hx_lib_pm_get_defcfg_bymode(&cfg, PM_MODE_PS_NOVID_PREROLLING);

	/*Setup bootrom clock speed when PMU Warm boot wakeup*/
	cfg.bootromspeed.bootromclkfreq = pmuwakeup_pll_freq;
	cfg.bootromspeed.pll_freq = freq;
	cfg.bootromspeed.cm55m_div = pmuwakeup_cm55m_div;
	cfg.bootromspeed.cm55s_div = pmuwakeup_cm55s_div;
	
    //cfg.support_debugdump = 1;
	
    cfg.pmu_pad_pa01_mask = PM_IP_INT_MASK;
    cfg.pmu_pad_pa23_mask = PM_IP_INT_MASK;
    
    for(uint32_t i = 0; i < gpio_cfg.act_wakeupCPU_pin_cnt; i++)
    {
        if(gpio_cfg.wakeupCPU_int_pin[i] == HX_AON_GPIO_0 || gpio_cfg.wakeupCPU_int_pin[i] == HX_AON_GPIO_1)
            cfg.pmu_pad_pa01_mask = PM_IP_INT_MASK_ALL_UNMASK;
        else if(gpio_cfg.wakeupCPU_int_pin[i] == HX_SB_GPIO_0 || gpio_cfg.wakeupCPU_int_pin[i] == HX_SB_GPIO_1)
            cfg.pmu_pad_pa23_mask = PM_IP_INT_MASK_ALL_UNMASK;
    }

	cfg.pmu_timer_mask = PM_TIMER_INT_MASK_TIMER_ALLMASK;

	if(sys_rtc_ms > 0) /* sys_rtc_ms, depends on APP_PMU_SYS_RTC_ID setting*/
		cfg.pmu_timer_mask &= (~(0x1 << APP_PMU_SYS_RTC_ID));
    
    if(retention == 0)
    {
        cfg.tcm_retention = PM_MEM_RET_NO;
        cfg.hscsram_retention[0] = PM_MEM_RET_NO;
        cfg.hscsram_retention[1] = PM_MEM_RET_NO;
        cfg.hscsram_retention[2] = PM_MEM_RET_NO;
        cfg.hscsram_retention[3] = PM_MEM_RET_NO;
        cfg.lscsram_retention = PM_MEM_RET_NO;
    }

    if(quickboot == 0)
    {
        cfg.skip_bootflow.sec_mem_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.first_bl_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55m_s_app_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55m_ns_app_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55s_s_app_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55s_ns_app_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55m_model_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55s_model_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55m_appcfg_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55s_appcfg_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55m_s_app_rwdata_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55m_ns_app_rwdata_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55s_s_app_rwdata_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.cm55s_ns_app_rwdata_flag = SWREG_AON_NO_RETENTION;
        cfg.skip_bootflow.secure_debug_flag = SWREG_AON_NO_RETENTION;
    }

	//priv_pmu_print_novidpre_cfg(&cfg);

    hx_lib_pm_cfg_set(&cfg, sensordplib_pmudpinit, PM_MODE_PS_NOVID_PREROLLING);
    
    app_pmu_wakeup_gpio_cfg(&gpio_cfg);
	
	hx_drv_timer_hw_stop(APP_PMU_SYS_RTC_ID);
	
    if(sys_rtc_ms > 0)
        app_pmu_sys_rtc_start(sys_rtc_ms);
	
    /*Use PMU lib to control HSC_CLK and LSC_CLK so set thoes parameter to 0*/
	memset(&hsc_cfg, 0, sizeof(SCU_PDHSC_HSCCLK_CFG_T));
	memset(&lsc_cfg, 0, sizeof(SCU_LSC_CLK_CFG_T));
    
	/*Trigger to PMU mode*/
    #if 1
    hx_lib_pm_trigger(hsc_cfg, lsc_cfg, PM_CLK_PARA_CTRL_BYPMLIB);
    #else
    priv_pmu_set_clk_cfg(SCU_SYSCLKSRC_HW_XTAL24MRC32K, &lsc_cfg, &hsc_cfg);
    
    dbg_app_log("hx_lib_pm_trigger\n");
    hx_lib_pm_trigger(hsc_cfg, lsc_cfg, PM_CLK_PARA_CTRL_BYAPP);
    #endif
}

void app_pmu_enter_sleep1(AppCfgCustGpio_t gpio_cfg, uint32_t sys_rtc_ms, uint8_t quickboot)
{
    priv_pmu_enter_novidpre(gpio_cfg, sys_rtc_ms, quickboot, 1);
}

void app_pmu_enter_sleep2(AppCfgCustGpio_t gpio_cfg, uint32_t sys_rtc_ms)
{
    priv_pmu_enter_novidpre(gpio_cfg, sys_rtc_ms, 0, 0);
}

void app_pmu_enter_shutdown(AppCfgCustGpio_t gpio_cfg)
{
    PM_DPD_CFG_T cfg;
    SCU_LSC_CLK_CFG_T lsc_cfg;
    SCU_PDHSC_HSCCLK_CFG_T hsc_cfg;
    uint32_t freq;
	SCU_PLL_FREQ_E pmuwakeup_pll_freq;
	SCU_HSCCLKDIV_E pmuwakeup_cm55m_div;
	SCU_LSCCLKDIV_E pmuwakeup_cm55s_div;

	/*Get System Current Clock*/
	hx_drv_swreg_aon_get_pmuwakeup_freq(&pmuwakeup_pll_freq, &pmuwakeup_cm55m_div, &pmuwakeup_cm55s_div);
	hx_drv_swreg_aon_get_pllfreq(&freq);
	dbg_app_log("pmuwakeup_freq_type=%d, pmuwakeup_cm55m_div=%d, pmuwakeup_cm55s_div=%d\n", pmuwakeup_pll_freq, pmuwakeup_cm55m_div, pmuwakeup_cm55s_div);
	dbg_app_log("pmuwakeup_run_freq=%d\n", freq);
	
    hx_lib_pm_get_defcfg_bymode(&cfg, PM_MODE_PS_DPD);
	
    /*Setup bootrom clock speed when PMU Warm boot wakeup*/
	cfg.bootromspeed.bootromclkfreq = pmuwakeup_pll_freq;
	cfg.bootromspeed.pll_freq = freq;
	cfg.bootromspeed.cm55m_div = pmuwakeup_cm55m_div;
	cfg.bootromspeed.cm55s_div = pmuwakeup_cm55s_div;
    
    //cfg.support_debugdump = 1;
    
    cfg.pmu_pad_pa0_mask = PM_IP_INT_MASK;
    cfg.pmu_pad_pa1_mask = PM_IP_INT_MASK;
	cfg.pmu_dcdc_outpin = PM_CFG_DCDC_MODE_VMUTE; //for EVB hard code

    dbg_app_log("gpio_cfg.act_wakeupCPU_pin_cnt=%u\n", gpio_cfg.act_wakeupCPU_pin_cnt);
    
    for(uint32_t i = 0; i < gpio_cfg.act_wakeupCPU_pin_cnt; i++)
    {
        dbg_app_log("gpio_cfg.wakeupCPU_int_pin[%u]=0x%x\n", i, gpio_cfg.wakeupCPU_int_pin[i]);
        
        if(gpio_cfg.wakeupCPU_int_pin[i] == HX_AON_GPIO_0)
            cfg.pmu_pad_pa0_mask = PM_IP_INT_MASK_ALL_UNMASK;
        else if(gpio_cfg.wakeupCPU_int_pin[i] == HX_AON_GPIO_1)
            cfg.pmu_pad_pa1_mask = PM_IP_INT_MASK_ALL_UNMASK;
    }
	
    //priv_pmu_print_dpd_cfg(&cfg);

    hx_lib_pm_cfg_set(&cfg, NULL, PM_MODE_PS_DPD);

    /*Use PMU lib to control HSC_CLK and LSC_CLK so set thoes parameter to 0*/
	memset(&hsc_cfg, 0, sizeof(SCU_PDHSC_HSCCLK_CFG_T));
	memset(&lsc_cfg, 0, sizeof(SCU_LSC_CLK_CFG_T));
    
	/*Trigger to PMU mode*/
    #if 1
    hx_lib_pm_trigger(hsc_cfg, lsc_cfg, PM_CLK_PARA_CTRL_BYPMLIB);
    #else
    priv_pmu_set_clk_cfg(SCU_SYSCLKSRC_HW_XTAL24MRC32K, &lsc_cfg, &hsc_cfg);

    dbg_app_log("hx_lib_pm_trigger\n");
    hx_lib_pm_trigger(hsc_cfg, lsc_cfg, PM_CLK_PARA_CTRL_BYAPP);
    #endif
}

/*
 *  CM55M all on    (set SIMO 0.9v)
 *  CM55M enter PMU (set SIMO 0.8v)
 */
#if defined(PMU_SIMO_VOLT)
void set_simo_0p9v(void)
{
	uint32_t simo_ctrl, simo_val;
	simo_ctrl = (*((volatile unsigned int*) 0x56101030));
	(*((volatile unsigned int*) 0x56101024)) = 0x0000250F;
	simo_val =  (*((volatile unsigned int*) 0x56101024));
	//xprintf("[%s] \n", __FUNCTION__);
	//xprintf("ctrl:0x%08x, val:0x%08x \n", simo_ctrl, simo_val);
}

void set_simo_0p8v(void)
{
	uint32_t simo_ctrl, simo_val;
	simo_ctrl = (*((volatile unsigned int*) 0x56101030));
	(*((volatile unsigned int*) 0x56101024)) = 0x0000250C;
	(*((volatile unsigned int*) 0x56101010)) = 0x0000030C;
	simo_val =  (*((volatile unsigned int*) 0x56101024));
	//xprintf("[%s] \n", __FUNCTION__);
	//xprintf("ctrl:0x%08x, val:0x%08x \n", simo_ctrl, simo_val);
}
#endif

