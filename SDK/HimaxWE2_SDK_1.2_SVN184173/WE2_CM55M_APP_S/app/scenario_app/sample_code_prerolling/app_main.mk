override SCENARIO_APP_SUPPORT_LIST := $(APP_TYPE)

####################
## Sensor Option  ##
## - HM11B1_MONO  ##
## - HM0360_BAYER ##
## - HM0360_MONO  ##
## - HM2170_BAYER ##
####################
CIS ?= HM0360_BAYER

APPL_DEFINES += -DCIS_AOS_MODE

#####################
## Function Enable ##
#####################
I2S_SPEAKER = n
PDM_REC = n
AUTO_PREROLL = y

#############################################
## Function Enable for Pmu Power Reduction ##
#############################################
PMU_SLEEP_CLK_DISABLE = n
PMU_SIMO_VOLT = n

APPL_DEFINES += -DDBG_MORE

#For PMU dump register
#APPL_DEFINES += -DLIBPWRMGMT_PMUDUMP
#APPL_DEFINES += -DAPP_PMU_REG_EXP

##############################
## Transmit Protocol Option ##
## - UART					##
## - SPI_MASTER				##
## - SPI_SLAVE				##
##############################
TRANSMIT_PROTOCOL ?= SPI_SLAVE

##################################
## Algorithm Config Option      ##
## - YOLO_FASTEST               ##
## - TFLITE_MICRO_GOOGLE_PERSON ##
## - HMX_ALGO                   ##
##################################
TFLITE_ALGO ?= 
FLASH_AS_SRAM_ENABLE ?= 0

ifneq ("$(TFLITE_ALGO)","")
    APPL_DEFINES += -DTFLITE_ALGO_ENABLED
endif

ifeq ($(FLASH_AS_SRAM_ENABLE), 1)
    APPL_DEFINES += -DFLASH_AS_SRAM
endif

##############################
## Preroll Option           ##
## - AUTO_PREROLL_JPEG		##
## - AUTO_PREROLL_RAW		##
## - AUTO_PREROLL_CDM		##
##############################
ifeq ($(AUTO_PREROLL), y)
	AUTO_PREROLL_TYPE = AUTO_PREROLL_JPEG
#	AUTO_PREROLL_TYPE = AUTO_PREROLL_RAW
#	AUTO_PREROLL_TYPE = AUTO_PREROLL_CDM
#	AUTO_PREROLL_TYPE = AUTO_PREROLL_JPEG_BD
endif

################
## Config APP ##
################
include $(SCENARIO_APP_ROOT)/app_api/app_api.inc
APPL_APP_ROOT = $(SCENARIO_APP_ROOT)/$(APP_TYPE)

####################
## C source files ##
####################
APPL_APP_CSRC_LIST = src/app_main.c \
                     src/app_i2c_cmd.c \
                     src/app_debug_print.c

#####################
## CC source files ##
#####################
ifeq ($(TFLITE_ALGO), TFLITE_MICRO_GOOGLE_PERSON)
APPL_APP_CCSRC_LIST = src/google_person/app_algo.cc \
                      src/google_person/person_detect_model_data_vela.cc
else ifeq ($(TFLITE_ALGO), YOLO_FASTEST)
APPL_APP_CCSRC_LIST = src/yolo_fastest/app_algo.cc \
					  src/yolo_fastest/yolo_coco_vela.cc
else ifeq ($(TFLITE_ALGO), HMX_ALGO)
include $(APPL_APP_ROOT)/hmx_algo.inc
endif

##################
## Header files ##
##################
APPL_APP_INCDIR_LIST = include
ifeq ($(TFLITE_ALGO), HMX_ALGO)
APPL_APP_INCDIR_LIST += include/hmx_algo
endif

## append file path
APPL_APP_CSRCS = $(addprefix $(APPL_APP_ROOT)/, $(APPL_APP_CSRC_LIST))
APPL_APP_CCSRCS = $(addprefix $(APPL_APP_ROOT)/, $(APPL_APP_CCSRC_LIST))
APPL_APP_INCDIRS = $(addprefix $(APPL_APP_ROOT)/, $(APPL_APP_INCDIR_LIST))


##
# library support feature
# Add new library here
# The source code should be loacted in ~\library\{lib_name}\
##
LIB_SEL = sensordp pwrmgmt spi_ptl i2c_comm

ifneq ("$(TFLITE_ALGO)","")
LIB_SEL += img_proc 
LIB_SEL += tflmtag2209_u55tag2205
ifeq ($(TFLITE_ALGO), HMX_ALGO)
LIB_SEL += hx_vip_algo
endif
endif

LIB_SEL += spi_eeprom

override undefine OS_SEL
override TRUSTZONE := y
override TRUSTZONE_TYPE := security
override TRUSTZONE_FW_TYPE := 1

ifeq ($(CORE_SETTING), DUAL_CORE)
override EPII_USECASE_SEL := drv_dualcore_cm55m_s_only
APPL_DEFINES += -DWE2_DUAL_CORE
else ifeq ($(CORE_SETTING), SINGLE_CORE)
APPL_DEFINES += -DWE2_SINGLE_CORE
override EPII_USECASE_SEL := drv_singlecore_cm55m_s_only
endif

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

################################
## AUTO PREROLL APPL_DEFINES ##
################################
ifeq ($(AUTO_PREROLL), y)
	APPL_DEFINES += -DAUTO_PREROLL
ifeq ($(AUTO_PREROLL_TYPE), AUTO_PREROLL_JPEG)
	APPL_DEFINES += -DAUTO_PREROLL_JPEG
else ifeq ($(AUTO_PREROLL_TYPE), AUTO_PREROLL_RAW)
	APPL_DEFINES += -DAUTO_PREROLL_RAW
else ifeq ($(AUTO_PREROLL_TYPE), AUTO_PREROLL_CDM)
	APPL_DEFINES += -DAUTO_PREROLL_CDM
	APPL_DEFINES += -DCDM_WAKEUP
else ifeq ($(AUTO_PREROLL_TYPE), AUTO_PREROLL_JPEG_BD)
	APPL_DEFINES += -DAUTO_PREROLL_JPEG
endif
endif

ifeq ($(PMU_SLEEP_CLK_DISABLE), y)
	APPL_DEFINES += -DPMU_SLEEP_CLK_DISABLE
endif

ifeq ($(PMU_SIMO_VOLT), y)
	APPL_DEFINES += -DPMU_SIMO_VOLT
endif
