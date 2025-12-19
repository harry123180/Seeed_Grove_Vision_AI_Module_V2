##
# platform (onchip ip) support feature
# Add all of supported ip list here
# The source code should be located in ~\drivers\{ip_name}\
##

DRIVERS_IP_LIST =  uart \
					spi \
                    iic \
                    mb \
                    scu \
                    timer \
                    gpio \
					swreg_aon \
					swreg_lsc \
                    dma \
                    ppc \
                    pmu \
					mpc \

                    
DRIVERS_IP_INSTANCE =  TIMER5 \
						TIMER6 \
						TIMER7 \
                        TIMER8 \
                        DMA2 \
						DMA3 \
                        UART1 \
                        UART2 \
                        IIC_SLAVE0 \
                        SSPI_HOST \
                        SSPI_SLAVE \
                        GPIO_G0 \
						GPIO_G1 \
						GPIO_G2 \
						GPIO_G3 \
						GPIO_G4 \
						GPIO_G5 \
						SB_GPIO \
						AON_GPIO \
                        


DRIVERS_IP_NS_INSTANCE =
