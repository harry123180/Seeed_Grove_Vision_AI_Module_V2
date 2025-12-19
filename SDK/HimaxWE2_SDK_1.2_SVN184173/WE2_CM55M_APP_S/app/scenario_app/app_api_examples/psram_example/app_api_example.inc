
APPL_APP_INCDIR_LIST = $(APP_API_EXAMPLE_NAME)/include
APPL_APP_CSRC_LIST = $(APP_API_EXAMPLE_NAME)/psram_example.c

LIB_SEL = spi_psram

ifneq ($(EVK), HX6538_AIoT_EVB_LQFP128_V10)
$(error "The psram function is only availabe on LQFP128 A07")
endif