#include "WE2_ARMCM55.h"

#include "WE2_device_addr.h"
#include "WE2_debug.h"

#include "app_cisdp_cfg.h"
#include "app_mipi.h"

#include "hx_drv_scu_export.h"

#define MIPI_DEBUG_MSG

#ifdef MIPI_DEBUG_MSG
void priv_mipi_csirx_print_info(void)
{
    volatile uint32_t *dphy_reg = CSIRX_DPHY_REG;
    volatile uint32_t *csi_static_cfg_reg = (CSIRX_REGS_BASE+0x08);
    volatile uint32_t *csi_dphy_lane_control_reg = (CSIRX_REGS_BASE+0x40);
    volatile uint32_t *csi_stream0_control_reg = (CSIRX_REGS_BASE+0x100);
    volatile uint32_t *csi_stream0_data_cfg = (CSIRX_REGS_BASE+0x108);
    volatile uint32_t *csi_stream0_cfg_reg = (CSIRX_REGS_BASE+0x10C);

    dbg_printf(DBG_LESS_INFO, "0x%08X = 0x%08X\n", dphy_reg, *dphy_reg);
    dbg_printf(DBG_LESS_INFO, "0x%08X = 0x%08X\n", csi_static_cfg_reg, *csi_static_cfg_reg);
    dbg_printf(DBG_LESS_INFO, "0x%08X = 0x%08X\n", csi_dphy_lane_control_reg, *csi_dphy_lane_control_reg);
    dbg_printf(DBG_LESS_INFO, "0x%08X = 0x%08X\n", csi_stream0_data_cfg, *csi_stream0_data_cfg);
    dbg_printf(DBG_LESS_INFO, "0x%08X = 0x%08X\n", csi_stream0_control_reg, *csi_stream0_control_reg);
}

void priv_mipi_csitx_print_info(void)
{
	dbg_printf(DBG_LESS_INFO, "VMUTE: 0x%08X\n", *(uint32_t*)(0x52001408));
	dbg_printf(DBG_LESS_INFO, "0x53061000: 0x%08X\n", *(uint32_t*)(CSITX_REGS_BASE+0x1000));
	dbg_printf(DBG_LESS_INFO, "0x53061004: 0x%08X\n", *(uint32_t*)(CSITX_REGS_BASE+0x1004));
	dbg_printf(DBG_LESS_INFO, "0x53061008: 0x%08X\n", *(uint32_t*)(CSITX_REGS_BASE+0x1008));
	dbg_printf(DBG_LESS_INFO, "0x5306100C: 0x%08X\n", *(uint32_t*)(CSITX_REGS_BASE+0x100C));
	dbg_printf(DBG_LESS_INFO, "0x53061010: 0x%08X\n", *(uint32_t*)(CSITX_REGS_BASE+0x1010));
}
#endif

void app_mipi_set_csirx_enable(app_mipi_rx_config_t *cfg)
{
	uint32_t mipi_pixel_clk = 96;
	
	hx_drv_scu_get_freq(SCU_CLK_FREQ_TYPE_HSC_MIPI_RXCLK, &mipi_pixel_clk);
	mipi_pixel_clk = mipi_pixel_clk / 1000000;

	dbg_printf(DBG_LESS_INFO, "app_mipi_set_csirx_enable\n");

	#ifdef MIPI_DEBUG_MSG
	dbg_printf(DBG_LESS_INFO, "MIPI DATA LANE: %d\n", cfg->mipi_lnno);
	dbg_printf(DBG_LESS_INFO, "MIPI PIXEL DEPTH: %d\n", cfg->pixel_dpp);
	#endif
	
	/*
	* Reset CSI RX
	*/
    #ifdef MIPI_DEBUG_MSG
    dbg_printf(DBG_LESS_INFO, "RESET MIPI CSI RX\n");
    #endif
	
    SCU_DP_SWRESET_T dp_swrst = {0};
    drv_interface_get_dp_swreset(&dp_swrst);
    dp_swrst.HSC_MIPIRX = 0;

    hx_drv_scu_set_DP_SWReset(dp_swrst);
    hx_drv_timer_cm55x_delay_us(50, TIMER_STATE_DC);

    dp_swrst.HSC_MIPIRX = 1;
    hx_drv_scu_set_DP_SWReset(dp_swrst);

	/*RX*/
    MIPIRX_DPHYHSCNT_CFG_T hscnt_cfg;
    hscnt_cfg.mipirx_dphy_hscnt_clk_en = 0;
    hscnt_cfg.mipirx_dphy_hscnt_ln0_en = 1;
    hscnt_cfg.mipirx_dphy_hscnt_ln1_en = 1;

	#if(IC_VERSION >= 30)
	if(mipi_pixel_clk == 200) //pll200
	{
		hscnt_cfg.mipirx_dphy_hscnt_clk_val = 0x03;
		hscnt_cfg.mipirx_dphy_hscnt_ln0_val = 0x10;
		hscnt_cfg.mipirx_dphy_hscnt_ln1_val = 0x10;
	}
	else if(mipi_pixel_clk == 300) //pll300
	{
		hscnt_cfg.mipirx_dphy_hscnt_clk_val = 0x03;
		hscnt_cfg.mipirx_dphy_hscnt_ln0_val = 0x13;
		hscnt_cfg.mipirx_dphy_hscnt_ln1_val = 0x13;
	}
	else if(mipi_pixel_clk == 350) //pll300
	{
		hscnt_cfg.mipirx_dphy_hscnt_clk_val = 0x03;
		hscnt_cfg.mipirx_dphy_hscnt_ln0_val = 0x16;
		hscnt_cfg.mipirx_dphy_hscnt_ln1_val = 0x16;
	}
	else //rc96
	{
		hscnt_cfg.mipirx_dphy_hscnt_clk_val = 0x03;
		hscnt_cfg.mipirx_dphy_hscnt_ln0_val = 0x06;
		hscnt_cfg.mipirx_dphy_hscnt_ln1_val = 0x06;
	}
	#else //VerB
	hscnt_cfg.mipirx_dphy_hscnt_clk_val = 0x03;
	hscnt_cfg.mipirx_dphy_hscnt_ln0_val = 0x10;
	hscnt_cfg.mipirx_dphy_hscnt_ln1_val = 0x10;
	#endif
	
	#ifdef MIPI_DEBUG_MSG
	dbg_printf(DBG_LESS_INFO, "hscnt_cfg.mipirx_dphy_hscnt_clk_val 0x%02X\n", hscnt_cfg.mipirx_dphy_hscnt_clk_val);
	dbg_printf(DBG_LESS_INFO, "hscnt_cfg.mipirx_dphy_hscnt_ln0_val 0x%02X\n", hscnt_cfg.mipirx_dphy_hscnt_ln0_val);
	dbg_printf(DBG_LESS_INFO, "hscnt_cfg.mipirx_dphy_hscnt_ln1_val 0x%02X\n", hscnt_cfg.mipirx_dphy_hscnt_ln1_val);
	#endif
	
	sensordplib_csirx_set_hscnt(hscnt_cfg);

	if(cfg->pixel_dpp == 10 || cfg->pixel_dpp == 8)
    {
    	sensordplib_csirx_set_pixel_depth(cfg->pixel_dpp);
    }
    else
    {
    	dbg_printf(DBG_LESS_INFO, "PIXEL DEPTH fail %d\n", cfg->pixel_dpp);
        return;
    }
	
	//sensordplib_csirx_set_vcnum(1);
	sensordplib_csirx_enable(cfg->mipi_lnno);
	
	#ifdef MIPI_DEBUG_MSG
	priv_mipi_csirx_print_info();
	#endif
}

void app_mipi_set_csirx_disable(void)
{
	dbg_printf(DBG_LESS_INFO, "app_mipi_set_csirx_disable\n");

    sensordplib_csirx_disable();

	#ifdef MIPI_DEBUG_MSG
	priv_mipi_csirx_print_info();
	#endif
}

void app_mipi_passthrough_init(app_mipi_passthrough_config_t *cfg)
{
	uint32_t mipi_pixel_clk = 96;
	uint32_t byte_clk = cfg->bitrate_1lane/8;
	
	hx_drv_scu_get_freq(SCU_CLK_FREQ_TYPE_HSC_MIPI_RXCLK, &mipi_pixel_clk);
	mipi_pixel_clk = mipi_pixel_clk / 1000000;

	dbg_printf(DBG_LESS_INFO, "app_mipi_passthrough_init\n");

	#ifdef MIPI_DEBUG_MSG
	dbg_printf(DBG_LESS_INFO, "MIPI TX CLK: %dM\n", mipi_pixel_clk);
	dbg_printf(DBG_LESS_INFO, "MIPI BITRATE 1LANE: %dM\n", cfg->bitrate_1lane);
	dbg_printf(DBG_LESS_INFO, "MIPI DATA LANE: %d\n", cfg->mipi_lnno);
	dbg_printf(DBG_LESS_INFO, "MIPI PIXEL DEPTH: %d\n", cfg->pixel_dpp);
	dbg_printf(DBG_LESS_INFO, "MIPI LINE LENGTH: %d\n", cfg->line_length);
	dbg_printf(DBG_LESS_INFO, "MIPI FRAME LENGTH: %d\n", cfg->frame_length);
	dbg_printf(DBG_LESS_INFO, "MIPI CONTINUOUSOUT: %d\n", cfg->continuousout);
	dbg_printf(DBG_LESS_INFO, "MIPI DESKEW: %d\n", cfg->deskew_en);
	#endif

	uint32_t n_preload = 15;
	uint32_t l_header = 4;
	uint32_t l_footer = 2;

	double t_input = (double)(l_header+cfg->line_length*cfg->pixel_dpp/8+l_footer)/(cfg->mipi_lnno*byte_clk)+0.06;
	double t_output = (double)cfg->line_length/mipi_pixel_clk;
	double t_preload = (double)(7+(n_preload*4)/cfg->mipi_lnno)/mipi_pixel_clk;
	
	double delta_t = t_input - t_output - t_preload;
	
	#ifdef MIPI_DEBUG_MSG
	dbg_printf(DBG_LESS_INFO, "t_input: %dns\n", (uint32_t)(t_input*1000));
	dbg_printf(DBG_LESS_INFO, "t_output: %dns\n", (uint32_t)(t_output*1000));
	dbg_printf(DBG_LESS_INFO, "t_preload: %dns\n", (uint32_t)(t_preload*1000));
	#endif
	
	uint16_t rx_fifo_fill = 0;
	uint16_t tx_fifo_fill = 0;

	if(delta_t <= 0)
	{
		delta_t = 0 - delta_t;
		tx_fifo_fill = ceil(delta_t*byte_clk*cfg->mipi_lnno/4/(cfg->pixel_dpp/2))*(cfg->pixel_dpp/2);
		rx_fifo_fill = 0;
	}
	else
	{
		rx_fifo_fill = ceil(delta_t*byte_clk*cfg->mipi_lnno/4/(cfg->pixel_dpp/2))*(cfg->pixel_dpp/2);
		tx_fifo_fill = 0;
	}
	
	#ifdef MIPI_DEBUG_MSG
	dbg_printf(DBG_LESS_INFO, "MIPI RX FIFO FILL: %d\n", rx_fifo_fill);
	dbg_printf(DBG_LESS_INFO, "MIPI TX FIFO FILL: %d\n", tx_fifo_fill);
	#endif

	/*
	 * Reset CSI RX/TX
	 */
	#ifdef MIPI_DEBUG_MSG
	dbg_printf(DBG_LESS_INFO, "RESET MIPI CSI RX/TX\n");
	#endif
	SCU_DP_SWRESET_T dp_swrst = {0};
	drv_interface_get_dp_swreset(&dp_swrst);
	dp_swrst.HSC_MIPIRX = 0;
	dp_swrst.HSC_MIPITX = 0;

	hx_drv_scu_set_DP_SWReset(dp_swrst);
	hx_drv_timer_cm55x_delay_us(50, TIMER_STATE_DC);

	dp_swrst.HSC_MIPIRX = 1;
	dp_swrst.HSC_MIPITX = 1;
	hx_drv_scu_set_DP_SWReset(dp_swrst);

	MIPIRX_DPHYHSCNT_CFG_T hscnt_cfg;
	hscnt_cfg.mipirx_dphy_hscnt_clk_en = 0;
	hscnt_cfg.mipirx_dphy_hscnt_ln0_en = 1;
	hscnt_cfg.mipirx_dphy_hscnt_ln1_en = 1;
	
	#if(IC_VERSION >= 30)
	if(mipi_pixel_clk == 200) //pll200
	{
		hscnt_cfg.mipirx_dphy_hscnt_clk_val = 0x03;
		hscnt_cfg.mipirx_dphy_hscnt_ln0_val = 0x10;
		hscnt_cfg.mipirx_dphy_hscnt_ln1_val = 0x10;
	}
	else if(mipi_pixel_clk == 300) //pll300
	{
		hscnt_cfg.mipirx_dphy_hscnt_clk_val = 0x03;
		hscnt_cfg.mipirx_dphy_hscnt_ln0_val = 0x13;
		hscnt_cfg.mipirx_dphy_hscnt_ln1_val = 0x13;
	}
	else if(mipi_pixel_clk == 330) //pll330
	{
		hscnt_cfg.mipirx_dphy_hscnt_clk_val = 0x03;
		hscnt_cfg.mipirx_dphy_hscnt_ln0_val = 0x15;
		hscnt_cfg.mipirx_dphy_hscnt_ln1_val = 0x15;
	}
	else if(mipi_pixel_clk == 350) //pll350
	{
		hscnt_cfg.mipirx_dphy_hscnt_clk_val = 0x03;
		hscnt_cfg.mipirx_dphy_hscnt_ln0_val = 0x16;
		hscnt_cfg.mipirx_dphy_hscnt_ln1_val = 0x16;
	}
	else //rc96
	{
		hscnt_cfg.mipirx_dphy_hscnt_clk_val = 0x03;
		hscnt_cfg.mipirx_dphy_hscnt_ln0_val = 0x06;
		hscnt_cfg.mipirx_dphy_hscnt_ln1_val = 0x06;
	}
	#else //VerB
	hscnt_cfg.mipirx_dphy_hscnt_clk_val = 0x03;
	hscnt_cfg.mipirx_dphy_hscnt_ln0_val = 0x10;
	hscnt_cfg.mipirx_dphy_hscnt_ln1_val = 0x10;
	#endif
	
	#ifdef MIPI_DEBUG_MSG
	dbg_printf(DBG_LESS_INFO, "hscnt_cfg.mipirx_dphy_hscnt_clk_val 0x%02X\n", hscnt_cfg.mipirx_dphy_hscnt_clk_val);
	dbg_printf(DBG_LESS_INFO, "hscnt_cfg.mipirx_dphy_hscnt_ln0_val 0x%02X\n", hscnt_cfg.mipirx_dphy_hscnt_ln0_val);
	dbg_printf(DBG_LESS_INFO, "hscnt_cfg.mipirx_dphy_hscnt_ln1_val 0x%02X\n", hscnt_cfg.mipirx_dphy_hscnt_ln1_val);
	#endif
	
	sensordplib_csirx_set_hscnt(hscnt_cfg);
	
	if(cfg->pixel_dpp == 10 || cfg->pixel_dpp == 8)
	{
		sensordplib_csirx_set_pixel_depth(cfg->pixel_dpp);
	}
	else
	{
		dbg_printf(DBG_LESS_INFO, "PIXEL DEPTH fail %d\n", cfg->pixel_dpp);
		return;
	}
	
	//	sensordplib_csirx_set_vcnum(1);
	sensordplib_csirx_set_deskew(cfg->deskew_en);
	sensordplib_csirx_set_fifo_fill(rx_fifo_fill);
	sensordplib_csirx_enable(cfg->mipi_lnno);

	CSITX_DPHYCLKMODE_E clkmode;
	if(cfg->continuousout)
	{
		clkmode = CSITX_DPHYCLOCK_CONT;
	}
	else
	{
		clkmode = CSITX_DPHYCLOCK_NON_CONT;
	}
	sensordplib_csitx_set_dphy_clkmode(clkmode);
	
	if(cfg->pixel_dpp == 10 || cfg->pixel_dpp == 8)
	{
		sensordplib_csitx_set_pixel_depth(cfg->pixel_dpp);
	}
	else
	{
		dbg_printf(DBG_LESS_INFO, "PIXEL DEPTH fail %d\n", cfg->pixel_dpp);
		return;
	}
	
	sensordplib_csitx_set_deskew(cfg->deskew_en);
	sensordplib_csitx_set_fifo_fill(tx_fifo_fill);
	sensordplib_csitx_enable(cfg->mipi_lnno, cfg->bitrate_1lane, cfg->line_length, cfg->frame_length);

	app_mipi_set_paththrough_off();
}


void app_mipi_passthrough_deinit(void)
{
    dbg_printf(DBG_LESS_INFO, "app_mipi_passthrough_deinit\n");
	
	sensordplib_csirx_disable();
	sensordplib_csitx_disable();
	
	#ifdef MIPI_DEBUG_MSG
	priv_mipi_csirx_print_info();
	#endif
}

void app_mipi_set_paththrough_on(void)
{
	/*
	 * polling CSIRX packet state
	 */
	uint32_t packet_state = 0x00;
	do
	{
		hx_drv_csirx_get_infoirq_state(&packet_state);
		dbg_printf(DBG_LESS_INFO, "packet_state: 0x%08X\n", packet_state);
	}while(packet_state == 0x00);

	hx_drv_timer_cm55x_delay_ms(1, TIMER_STATE_DC);

    /*
     * //VMUTE setting: SW VNUTE OFF
     * W:0x52001408:0x00000005:4:4
     */
    SCU_VMUTE_CFG_T ctrl;
    ctrl.timingsrc = SCU_VMUTE_CTRL_TIMING_SRC_VMUTE;
    ctrl.txphypwr = SCU_VMUTE_CTRL_TXPHY_PWR_DISABLE;
    ctrl.ctrlsrc = SCU_VMUTE_CTRL_SRC_SW;
    ctrl.swctrl = SCU_VMUTE_CTRL_SW_DISABLE;
    hx_drv_scu_set_vmute(&ctrl);
    
	#ifdef MIPI_DEBUG_MSG
	priv_mipi_csitx_print_info();
	#endif
}

void app_mipi_set_paththrough_off(void)
{
    /*
     * //VMUTE setting: SW VNUTE ON
     * W:0x52001408:0x0000000D:4:4
     */
    SCU_VMUTE_CFG_T ctrl;
    ctrl.timingsrc = SCU_VMUTE_CTRL_TIMING_SRC_VMUTE;
    ctrl.txphypwr = SCU_VMUTE_CTRL_TXPHY_PWR_DISABLE;
    ctrl.ctrlsrc = SCU_VMUTE_CTRL_SRC_SW;
    ctrl.swctrl = SCU_VMUTE_CTRL_SW_ENABLE;
    hx_drv_scu_set_vmute(&ctrl);

	#ifdef MIPI_DEBUG_MSG
	priv_mipi_csitx_print_info();
	#endif
}

void app_mipi_change_to_pll(void)
{
    SCU_PDHSC_DPCLK_CFG_T cfg;

    hx_drv_scu_get_pdhsc_dpclk_cfg(&cfg);

    uint32_t pllfreq;
    hx_drv_swreg_aon_get_pllfreq(&pllfreq);

    if(pllfreq == 400000000)
    {
        cfg.mipiclk.hscmipiclksrc = SCU_HSCMIPICLKSRC_PLL;
        cfg.mipiclk.hscmipiclkdiv = 1;
    }
    else
    {
        cfg.mipiclk.hscmipiclksrc = SCU_HSCMIPICLKSRC_PLL;
        cfg.mipiclk.hscmipiclkdiv = 0;
    }

    hx_drv_scu_set_pdhsc_dpclk_cfg(cfg, 0, 1);
}

