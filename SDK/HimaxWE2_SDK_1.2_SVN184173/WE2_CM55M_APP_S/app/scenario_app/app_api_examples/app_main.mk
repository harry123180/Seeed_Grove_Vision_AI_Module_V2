override SCENARIO_APP_SUPPORT_LIST := $(APP_TYPE)
APPL_APP_ROOT = $(SCENARIO_APP_ROOT)/$(APP_TYPE)

APPL_DEFINES += -DDBG_MORE

########################
## Sensor Option      ##
## - HM11B1_MONO      ##
## - HM0360_BAYER     ##
## - HM0360_MONO      ##
## - HM0360_MIPI      ##
## - HM2170_BAYER     ##
## - HM2170_MIPI      ##
## - MIPI_CSI_RX_INTF ##
########################
CIS ?= HM0360_MONO

#############################
## Select Example          ##
## - psram_example         ##
## - pdm_example           ##
## - i2cm_example          ##
## - dpd_example           ##
## - pd_example            ##
## - adcc_example          ##
## - cap_raw_example       ##
## - cap_raw_yuv_example   ##
## - cdm_wake_example      ##
## - boot_cap_example      ##
## - i2s_speaker_example   ##
## - mipi_passthru_example ##
## - mipi_rx_example       ##
#############################
APP_API_EXAMPLE = cdm_wake_example
APP_API_EXAMPLE_NAME = $(strip $(APP_API_EXAMPLE))
APP_API_EXAMPLE_PATH = $(APPL_APP_ROOT)/$(APP_API_EXAMPLE_NAME)
include $(APP_API_EXAMPLE_PATH)/app_api_example.inc

####################
## Config APP_API ##
####################
include $(SCENARIO_APP_ROOT)/app_api/app_api.inc

##################
## Header files ##
##################


## append file path
APPL_APP_CSRCS = $(addprefix $(APPL_APP_ROOT)/, $(APPL_APP_CSRC_LIST))
APPL_APP_CCSRCS = $(addprefix $(APPL_APP_ROOT)/, $(APPL_APP_CCSRC_LIST))
APPL_APP_INCDIRS = $(addprefix $(APPL_APP_ROOT)/, $(APPL_APP_INCDIR_LIST))
APPL_APP_INCDIRS += $(APPL_APP_ROOT)/

##
# library support feature
# Add new library here
# The source code should be loacted in ~\library\{lib_name}\
##
#LIB_SEL = hxevent
LIB_SEL += pwrmgmt
LIB_SEL += sensordp
LIB_SEL += spi_ptl
#LIB_SEL += tflm 
#LIB_SEL += img_proc 
LIB_SEL += i2c_comm
LIB_SEL += audio
LIB_SEL += spi_eeprom





ifeq ($(FREERTOS_SUPPORT), 1)
override OS_SEL := freertos_10_5_1
override OS_HAL := n
override MPU := n
APPL_DEFINES += -DOS_FREERTOS
else
override undefine OS_SEL
endif
override TRUSTZONE := y
override TRUSTZONE_TYPE := security
override TRUSTZONE_FW_TYPE := 1
override CIS_SEL := HM_COMMON

ifeq ($(CORE_SETTING), DUAL_CORE)
APPL_DEFINES += -DWE2_DUAL_CORE
override EPII_USECASE_SEL := drv_dualcore_cm55m_s_only
else ifeq ($(CORE_SETTING), SINGLE_CORE)
APPL_DEFINES += -DWE2_SINGLE_CORE
override EPII_USECASE_SEL := drv_singlecore_cm55m_s_only
endif



ifeq ($(strip $(TOOLCHAIN)), arm)
override LINKER_SCRIPT_FILE := $(SCENARIO_APP_ROOT)/$(APP_TYPE)/TrustZone_S_ONLY.sct
else#TOOLChain
override LINKER_SCRIPT_FILE := $(SCENARIO_APP_ROOT)/$(APP_TYPE)/TrustZone_S_ONLY.ld
endif

##
# Add new external device here
# The source code should be located in ~\external\{device_name}\
##
#EXT_DEV_LIST += 


