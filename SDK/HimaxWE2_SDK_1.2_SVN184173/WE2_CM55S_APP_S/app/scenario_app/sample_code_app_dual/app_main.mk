override SCENARIO_APP_SUPPORT_LIST := $(APP_TYPE)

####################
## Sensor Option  ##
## - HM11B1_MONO  ##
## - HM0360_BAYER ##
## - HM0360_MONO  ##
## - HM2170_BAYER ##
####################
CIS = 

##############################
## Transmit Protocol Option ##
## - UART					##
## - SPI_MASTER				##
## - SPI_SLAVE				##
##############################
TRANSMIT_PROTOCOL = SPI_MASTER

################
## Config APP ##
################
include $(SCENARIO_APP_ROOT)/app_api/app_api.inc
APPL_APP_ROOT = $(SCENARIO_APP_ROOT)/$(APP_TYPE)

####################
## C source files ##
####################
APPL_APP_CSRC_LIST = src/app_main.c

APPL_APP_CSRC_LIST += src/app_i2c_cmd.c

ifeq ($(TRANSMIT_PROTOCOL), UART)
APPL_APP_CSRC_LIST += src/app_uart_cmd.c
endif

#####################
## CC source files ##
#####################
APPL_APP_CCSRC_LIST =

##################
## Header files ##
##################
APPL_APP_INCDIR_LIST = include

## append file path
APPL_APP_CSRCS = $(addprefix $(APPL_APP_ROOT)/, $(APPL_APP_CSRC_LIST))
APPL_APP_CCSRCS = $(addprefix $(APPL_APP_ROOT)/, $(APPL_APP_CCSRC_LIST))
APPL_APP_INCDIRS = $(addprefix $(APPL_APP_ROOT)/, $(APPL_APP_INCDIR_LIST))

##
# library support feature
# Add new library here
# The source code should be loacted in ~\library\{lib_name}\
##
LIB_SEL =
#LIB_SEL += sensordp
#LIB_SEL += pwrmgmt
LIB_SEL += spi_ptl
LIB_SEL += i2c_comm
#LIB_SEL += img_proc 
#LIB_SEL += audio
##
# middleware support feature
# Add new middleware here
# The source code should be loacted in ~\middleware\{mid_name}\
##
MID_SEL =

override undefine OS_SEL
override TRUSTZONE := y
override TRUSTZONE_TYPE := security
override TRUSTZONE_FW_TYPE := 1
override EPII_USECASE_SEL := drv_daulcore_cm55s_s_only

override CIS_SEL := HM_COMMON

ifeq ($(strip $(TOOLCHAIN)), arm)
override LINKER_SCRIPT_FILE := $(SCENARIO_APP_ROOT)/$(APP_TYPE)/TrustZone_S_ONLY.sct
else#TOOLChain
override LINKER_SCRIPT_FILE := $(SCENARIO_APP_ROOT)/$(APP_TYPE)/TrustZone_S_ONLY.ld
endif

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
