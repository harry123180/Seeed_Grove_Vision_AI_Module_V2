#ifdef MIPI_CSI_RX_INTF

#include "app_datapath.h"
#include "app_cisdp_cfg.h"
#include "app_mipi.h"

#include "WE2_debug.h"


int app_cisdp_datapath_init(const app_dp_cfg_t* dp_init_cfg)
{
	sensordplib_set_sensorctrl_inp(dp_init_cfg->sensor_type, dp_init_cfg->stream_type, 
        dp_init_cfg->sensor_width, dp_init_cfg->sensor_height, dp_init_cfg->inp_subsample_type);

    return 0;
}

int app_cisdp_sensor_init(const app_dp_cfg_t* dp_init_cfg)
{
    dbg_printf(DBG_LESS_INFO, "mipi_csi_rx_intf_init \r\n");

    return 0;
}

void app_cisdp_sensor_start(void)
{
}

void app_cisdp_sensor_start_AOS(void)
{
}

void app_cisdp_sensor_stop()
{
    sensordplib_stop_capture();
    sensordplib_start_swreset();
    sensordplib_stop_swreset_WoSensorCtrl();

	app_mipi_set_csirx_disable();
}
#endif //MIPI_CSI_RX_INTF

