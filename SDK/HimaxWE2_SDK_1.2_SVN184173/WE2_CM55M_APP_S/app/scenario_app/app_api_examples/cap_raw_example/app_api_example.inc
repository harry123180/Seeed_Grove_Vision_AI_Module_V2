
APPL_APP_INCDIR_LIST = $(APP_API_EXAMPLE_NAME)/include
APPL_APP_CSRC_LIST = $(APP_API_EXAMPLE_NAME)/cap_raw_example.c

$(warning APPL_APP_CSRC_LIST=$(APPL_APP_CSRC_LIST))

##############################
## Transmit Protocol Option ##
## - UART					##
## - SPI_MASTER				##
## - SPI_SLAVE				##
##############################
TRANSMIT_PROTOCOL = SPI_MASTER

##
# middleware support feature
# Add new middleware here
# The source code should be loacted in ~\middleware\{mid_name}\
##
MID_SEL =


APPL_DEFINES += -DDBG_MORE
####################################
## Transmit Protocol APPL_DEFINES ##
####################################
APPL_DEFINES += -DI2C_COMM
ifeq ($(TRANSMIT_PROTOCOL), UART)
	APPL_DEFINES += -DUART_PROTOCOL
else ifeq ($(TRANSMIT_PROTOCOL), SPI_MASTER)
	APPL_DEFINES += -DSPI_MASTER_SEND
else ifeq ($(TRANSMIT_PROTOCOL), SPI_SLAVE)
	APPL_DEFINES += -DSPI_SLAVE_SEND
endif
