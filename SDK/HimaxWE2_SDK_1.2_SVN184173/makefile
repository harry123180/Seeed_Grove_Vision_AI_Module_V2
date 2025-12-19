########################
## Toolchain Settings ##
########################
## - arm              ##
## - gnu              ##
########################
TOOLCHAIN = gnu

###################
## CPU Setting   ##
###################
## - SINGLE_CORE ##
## - DUAL_CORE   ##
###################
CORE_SETTING = SINGLE_CORE

###################################
## EVK Settings                  ##
###################################
## - HX6538_NB_EVB_WLCSP65       ##
## - HX6538_AIoT_EVB_LQFP128_V10 ##
## - HX6538_AIoT_EVB_QFN88_V10   ##
## - HX6538_ISM028_03M           ##
###################################
EVK = HX6538_AIoT_EVB_QFN88_V10

################################
## Application Type
################################
## - sample_code_app          ##
## - sample_code_prerolling   ##
## - sample_code_app_dual     ##
## - app_api_examples         ##
################################
APP_TYPE = sample_code_app


# Toolchain Path Settings
ifeq ($(TOOLCHAIN), gnu)
GNU_TOOLPATH = "D:/WE2_toolchains/arm-gnu-toolchain-12.3.rel1-mingw-w64-i686-arm-none-eabi/bin"
else ifeq ($(TOOLCHAIN), arm)
ARMLMD_LICENSE_FILE = "C:/Users/$(USERNAME)/AppData/Roaming/arm/ds/licenses"
ARM_PRODUCT_DEF = "C:/Program Files/Arm/Development Studio 2020.1/sw/mappings/gold.elmap"
ARM_TOOLPATH = "C:/Program Files/Arm/Development Studio 2020.1/sw/ARMCompiler6.15/bin"
else
$(error TOOLCHAIN="$(TOOLCHAIN)" is NOT supported!)
endif

# Source codes and Utilities Path
SMALL_CORE_SRC_DIR = WE2_CM55S_APP_S
BIG_CORE_SRC_DIR = WE2_CM55M_APP_S
IMAGE_GEN_TOOL_DIR = we2_image_gen_local
OTA_TOOL_DIR = we2_ota_tool
MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MKFILE_DIR := $(dir $(MKFILE_PATH))

# EVK and package
ifeq ($(EVK), HX6538_AIoT_EVB_LQFP128_V10)
	IC_PACKAGE_SEL = LQFP128
else ifeq ($(EVK), HX6538_AIoT_EVB_QFN88_V10)
	IC_PACKAGE_SEL = QFN88
else ifeq ($(EVK), HX6538_ISM028_03M)
	IC_PACKAGE_SEL = QFN88
else ifeq ($(EVK), HX6538_NB_EVB_WLCSP65)
	IC_PACKAGE_SEL = WLCSP65
else
	$(error EVK="$(EVK)" is NOT supported yet!)
endif

## Variables for FW
BUILD_TARGETS = big_core_build
CLEAN_TARGETS = big_core_clean

ifeq ($(CORE_SETTING), DUAL_CORE)
	BUILD_TARGETS += small_core_build
	CLEAN_TARGETS += small_core_clean
endif

## Variables For Image Gen
BIG_CORE_OUTPUT_FILE = $(BIG_CORE_SRC_DIR)\obj_epii_evb_icv30_bdv10\$(TOOLCHAIN)_epii_evb_$(IC_PACKAGE_SEL)\WE2_CM55M_$(TOOLCHAIN)_epii_evb_$(IC_PACKAGE_SEL)_s
SMALL_CORE_OUTPUT_FILE = $(SMALL_CORE_SRC_DIR)\obj_epii_evb_icv30_bdv10\$(TOOLCHAIN)_epii_evb_$(IC_PACKAGE_SEL)\WE2_CM55S_$(TOOLCHAIN)_epii_evb_$(IC_PACKAGE_SEL)_s
BIG_CORE_IMG_GEN_INPUT_FILE = WE2_CM55M_we2_evb_s
SMALL_CORE_IMG_GEN_INPUT_FILE = WE2_CM55S_we2_evb_s

ifeq ($(CORE_SETTING), DUAL_CORE)
    IMAGE_GEN_INPUT_DIR = $(IMAGE_GEN_TOOL_DIR)\input_case3_secboot
    IMAGE_GEN_OUTPUT_DIR = $(IMAGE_GEN_TOOL_DIR)\output_case3_secboot_nodiv
else ifeq ($(CORE_SETTING), SINGLE_CORE)
    IMAGE_GEN_INPUT_DIR = $(IMAGE_GEN_TOOL_DIR)\input_case1_secboot
    IMAGE_GEN_OUTPUT_DIR = $(IMAGE_GEN_TOOL_DIR)\output_case1_secboot_nodiv
else
    $(error CORE_SETTING="$(CORE_SETTING)" is NOT supported yet!)
endif

## Compile Options
ifeq ($(TOOLCHAIN), gnu)
BUILD_OPT = CORE_SETTING=$(CORE_SETTING) EVK=$(EVK) IC_PACKAGE_SEL=$(IC_PACKAGE_SEL) APP_TYPE=$(APP_TYPE) TOOLCHAIN=$(TOOLCHAIN) GNU_TOOLPATH=$(GNU_TOOLPATH)
else
BUILD_OPT = CORE_SETTING=$(CORE_SETTING) EVK=$(EVK) IC_PACKAGE_SEL=$(IC_PACKAGE_SEL) APP_TYPE=$(APP_TYPE) TOOLCHAIN=$(TOOLCHAIN) ARM_TOOLPATH=$(ARM_TOOLPATH) ARMLMD_LICENSE_FILE=$(ARMLMD_LICENSE_FILE) ARM_PRODUCT_DEF=$(ARM_PRODUCT_DEF)
endif

MAKE = make
MULTI_CORE_OPT = -j4

## Image Gen setting JSON file
ifeq ($(CORE_SETTING), DUAL_CORE)
    IMG_GEN_JSON=project_case3_blp_nodiv.json
else ifeq ($(CORE_SETTING), SINGLE_CORE)
    IMG_GEN_JSON=project_case1_blp_nodiv.json
else
    $(error CORE_SETTING="$(CORE_SETTING)" is NOT supported yet!)
endif

all: $(BUILD_TARGETS)
	@echo build complete.

clean: $(CLEAN_TARGETS)
	@echo clean complete.

small_core_build:
	$(MAKE) -C $(SMALL_CORE_SRC_DIR) $(BUILD_OPT) APPL=WE2_CM55S BOARD=epii_evb $(MULTI_CORE_OPT)

big_core_build:
	$(MAKE) -C $(BIG_CORE_SRC_DIR) $(BUILD_OPT) APPL=WE2_CM55M BOARD=epii_evb $(MULTI_CORE_OPT)

small_core_clean:
	$(MAKE) -C $(SMALL_CORE_SRC_DIR) $(BUILD_OPT) BOARD=epii_evb clean

big_core_clean:
	$(MAKE) -C $(BIG_CORE_SRC_DIR) $(BUILD_OPT) BOARD=epii_evb clean

flash:
	@echo generate output.img for GNU toolchain...
	@echo using json setting file = $(IMG_GEN_JSON)
	copy $(BIG_CORE_OUTPUT_FILE).elf $(IMAGE_GEN_INPUT_DIR)\$(BIG_CORE_IMG_GEN_INPUT_FILE).elf
	copy $(BIG_CORE_OUTPUT_FILE).map $(IMAGE_GEN_INPUT_DIR)\$(BIG_CORE_IMG_GEN_INPUT_FILE).map
ifeq ($(CORE_SETTING), DUAL_CORE)
	copy $(SMALL_CORE_OUTPUT_FILE).elf $(IMAGE_GEN_INPUT_DIR)\$(SMALL_CORE_IMG_GEN_INPUT_FILE).elf
	copy $(SMALL_CORE_OUTPUT_FILE).map $(IMAGE_GEN_INPUT_DIR)\$(SMALL_CORE_IMG_GEN_INPUT_FILE).map
endif
	cd $(IMAGE_GEN_TOOL_DIR) && .\we2_local_image_gen.exe $(IMG_GEN_JSON)
	copy $(IMAGE_GEN_OUTPUT_DIR)\output.img $(OTA_TOOL_DIR)\img

menuconfig:
	cd $(IMAGE_GEN_TOOL_DIR) && .\menuconfig_mw.bat

info:
	@echo USERNAME = $(USERNAME)
	@echo APP_TYPE = $(APP_TYPE)
	@echo TOOLCHAIN = $(TOOLCHAIN)
	@echo SMALL_CORE_SRC_DIR = $(SMALL_CORE_SRC_DIR)
	@echo BIG_CORE_SRC_DIR = $(BIG_CORE_SRC_DIR)
	@echo BUILD_TARGETS = $(BUILD_TARGETS)
	@echo BUILD_OPT = "$(BUILD_OPT)"
	@echo MULTI_CORE_OPT = $(MULTI_CORE_OPT)
	@echo IMAGE_GEN_TOOL_DIR = $(IMAGE_GEN_TOOL_DIR)
	@echo OTA_TOOL_DIR = $(OTA_TOOL_DIR)
	@echo IMG_GEN_JSON = $(IMG_GEN_JSON)
ifeq ($(TOOLCHAIN), gnu)
	@echo GNU_TOOLPATH = $(GNU_TOOLPATH)
else ifeq ($(TOOLCHAIN), arm)
	@echo ARM_TOOLPATH = $(ARM_TOOLPATH)
	@echo ARMLMD_LICENSE_FILE = $(ARMLMD_LICENSE_FILE)
	@echo ARM_PRODUCT_DEF = $(ARM_PRODUCT_DEF)
endif

help:
	@echo make all: compile all the sources
	@echo make all MULTI_CORE_OPT=-j4: use 4 CPU cores to compile the sources
	@echo make clean: clean all the sources
	@echo make flash: generate the output.img
	@echo make menuconfig: update loader config setting
	@echo make small_core_build : compile small core source only
	@echo make big_core_build : compile big core source only
	@echo make small_core_clean : clean small core source
	@echo make big_core_clean : clean big core source
