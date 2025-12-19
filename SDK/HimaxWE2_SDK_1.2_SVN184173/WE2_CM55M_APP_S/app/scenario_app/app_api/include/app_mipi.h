#ifndef APP_MIPI_CSIRX_H_
#define APP_MIPI_CSIRX_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct app_mipi_rx_config {
	uint32_t mipi_lnno;						//MIPI 1 Lane / 2 Lane
	uint32_t pixel_dpp;					    //RAW 8 / RAW 10
} app_mipi_rx_config_t;

typedef struct app_mipi_passthrough_config {
	uint32_t bitrate_1lane; 				//MIPI CLK FREQ x2
	uint32_t mipi_lnno;						//MIPI 1 Lane / 2 Lane
	uint32_t pixel_dpp;					    //RAW 8 / RAW 10
	uint32_t line_length;				    //active pixel 
	uint32_t frame_length;
	uint32_t continuousout;
	uint32_t deskew_en;
} app_mipi_passthrough_config_t;

void app_mipi_set_csirx_enable(app_mipi_rx_config_t *cfg);
void app_mipi_set_csirx_disable(void);

void app_mipi_passthrough_init(app_mipi_passthrough_config_t *cfg);
void app_mipi_passthrough_deinit(void);

void app_mipi_set_paththrough_on(void);
void app_mipi_set_paththrough_off(void);

void app_mipi_change_to_pll(void);

#ifdef __cplusplus
}
#endif

#endif /*APP_MIPI_CSIRX_H_*/