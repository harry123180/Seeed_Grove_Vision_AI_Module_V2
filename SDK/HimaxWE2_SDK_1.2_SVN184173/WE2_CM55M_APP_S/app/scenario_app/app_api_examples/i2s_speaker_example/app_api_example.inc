
APPL_APP_INCDIR_LIST = $(APP_API_EXAMPLE_NAME)/include
APPL_APP_CSRC_LIST = $(APP_API_EXAMPLE_NAME)/i2s_speaker_example.c

APPL_DEFINES += -DSPI_MASTER_SEND

I2S_SPEAKER = y

ifneq ($(EVK), HX6538_ISM028_03M)
$(error "The i2s_speaker_example is for HX6538_ISM028_03M.")
endif

