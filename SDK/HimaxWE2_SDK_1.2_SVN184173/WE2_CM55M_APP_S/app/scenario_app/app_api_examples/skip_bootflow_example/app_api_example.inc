##############################
## Transmit Protocol Option ##
## - UART					##
## - SPI_MASTER				##
## - SPI_SLAVE				##
##############################
TRANSMIT_PROTOCOL = SPI_MASTER
#TRANSMIT_PROTOCOL = SPI_SLAVE
#TRANSMIT_PROTOCOL = UART

##################################
## Algorithm Config Option      ##
## - YOLO_FASTEST               ##
## - TFLITE_MICRO_GOOGLE_PERSON ##
## - HMX_ALGO                   ##
##################################
TFLITE_ALGO ?= YOLO_FASTEST
FLASH_AS_SRAM_ENABLE ?= 1
FREERTOS_SUPPORT ?= 0

#APPL_DEFINES += -DDBG_MORE

ifneq ("$(TFLITE_ALGO)","")
    APPL_DEFINES += -DTFLITE_ALGO_ENABLED
endif

ifeq ($(FLASH_AS_SRAM_ENABLE), 1)
    APPL_DEFINES += -DFLASH_AS_SRAM
endif


####################
## C source files ##
####################
ifeq ($(FREERTOS_SUPPORT), 0)
APPL_APP_CSRC_LIST = $(APP_API_EXAMPLE_NAME)/src/skip_bootflow_example.c
else ifeq ($(FREERTOS_SUPPORT), 1)
APPL_APP_CSRC_LIST = $(APP_API_EXAMPLE_NAME)/src/skip_bootflow_example_freertos.c
endif
                     
APPL_APP_CSRC_LIST += $(APP_API_EXAMPLE_NAME)/src/app_i2c_cmd.c
                     
ifeq ($(TRANSMIT_PROTOCOL), UART)
APPL_APP_CSRC_LIST += $(APP_API_EXAMPLE_NAME)/src/app_uart_cmd.c
endif

$(warning APPL_APP_CSRC_LIST=$(APPL_APP_CSRC_LIST))
#####################
## CC source files ##
#####################
ifeq ($(TFLITE_ALGO), TFLITE_MICRO_GOOGLE_PERSON)
APPL_APP_CCSRC_LIST = $(APP_API_EXAMPLE_NAME)/src/google_person/app_algo.cc \
                      $(APP_API_EXAMPLE_NAME)/src/google_person/person_detect_model_data_vela.cc
else ifeq ($(TFLITE_ALGO), YOLO_FASTEST)
APPL_APP_CCSRC_LIST = $(APP_API_EXAMPLE_NAME)/src/yolo_fastest/app_algo.cc \
					  $(APP_API_EXAMPLE_NAME)/src/yolo_fastest/yolo_coco_vela.cc
else ifeq ($(TFLITE_ALGO), HMX_ALGO)
include $(APP_API_EXAMPLE_PATH)/hmx_algo.inc
endif
$(warning APPL_APP_CCSRC_LIST=$(APPL_APP_CCSRC_LIST))
##################
## Header files ##
##################
APPL_APP_INCDIR_LIST = $(APP_API_EXAMPLE_NAME)/include
ifeq ($(TFLITE_ALGO), HMX_ALGO)
APPL_APP_INCDIR_LIST += $(APP_API_EXAMPLE_NAME)/include/hmx_algo
endif


ifneq ("$(TFLITE_ALGO)","")
LIB_SEL += img_proc 
LIB_SEL += tflmtag2209_u55tag2205
ifeq ($(TFLITE_ALGO), HMX_ALGO)
LIB_SEL += hx_vip_algo
endif
endif


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
