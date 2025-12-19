##
# platform (onchip ip) support feature
# Add all of supported ip list here
# The source code should be located in ~\drivers\{ip_name}\
##

DRIVERS_IP_LIST = 2x2 \
					5x5 \
					uart \
					spi \
					iic \
					mb \
					scu \
					timer \
					watchdog \
					rtc	\
					cdm \
					edm \
					jpeg \
					xdma \
					dp \
					inp \
					tpg \
					inp1bitparser \
					inpovparser \
					sensorctrl \
					gpio \
					swreg_aon \
					swreg_lsc \
					dma \
					ppc \
					pmu \
					mpc \
					hxautoi2c_mst \
					csirx \
					i2s \
					pdm \
					inpovparser \
					csitx \
                    adcc
ifneq ("$(TFLITE_ALGO)","")
DRIVERS_IP_LIST += u55
endif
				
DRIVERS_IP_INSTANCE = RTC0 \
						RTC1 \
						RTC2 \
						TIMER0 \
						TIMER1 \
						TIMER2 \
						TIMER3 \
                        TIMER4 \
						WDT0 \
                        WDT1 \
						DMA0 \
						DMA1 \
						UART0 \
						IIC_HOST_SENSOR \
                        IIC_HOST \
						IIC_HOST_MIPI \
						I2S_HOST \
                        QSPI_HOST \
						OSPI_HOST \
						GPIO_G0 \
						GPIO_G1 \
						GPIO_G2 \
						GPIO_G3 \
						GPIO_G4 \
						GPIO_G5 \
						SB_GPIO \
						AON_GPIO \
                        ADCC

						
DRIVERS_IP_NS_INSTANCE =
				