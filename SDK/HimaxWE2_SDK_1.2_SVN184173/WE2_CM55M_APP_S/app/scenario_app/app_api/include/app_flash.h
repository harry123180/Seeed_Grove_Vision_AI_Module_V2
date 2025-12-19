#ifndef _APP_FLASH_H
#define _APP_FLASH_H

#include "stdio.h"
#include "stdlib.h"
#include "hx_drv_basic_def.h"

#ifdef IP_spi
	#include "spi_eeprom_comm.h"
#endif

#pragma pack(1)
typedef struct _FLASH_SECTION_INFO
{
	unsigned int base_addr;
	unsigned int offset;
	unsigned int size;
}FLASH_SECTION_INFO;

typedef struct _FLASH_SECTION_ITEM
{
	unsigned int type;
	FLASH_SECTION_INFO info;
}FLASH_SECTION_ITEM;

typedef struct _FLASH_SECTIONS
{
	unsigned char num_sbs;
	FLASH_SECTION_ITEM *pitem;
}FLASH_SECTIONS;

typedef struct _MEMORY_DESCRIPTOR_ATTR_
{
	unsigned char format;
	unsigned char type;
	unsigned short secure_header_size;
}MEMORY_DESCRIPTOR_ATTR;
#pragma pack()

enum _FLASH_STATUS_
{
	FLASH_OK = 0,
	FLASH_MALLOC_FAIL = -1,
	FLASH_READ_FAIL = -2,
	FLASH_WRITE_FAIL = -3,
	FLASH_ERASE_SECTOR_FAIL = -4,
	FLASH_UNDEFINED_SECTION = -5,
	FLASH_READ_MEMORY_DESCRIPTOR_FAIL = -6,
};

//---------------------------------------------
enum _CUST_FLASH_SECTION_TYPE_
{
	CUST_FLASH_SECTION_AREA_CONFIG = 30,
	CUST_FLASH_SECTION_FR_ALBUM,
	CUST_FLASH_SECTION_SCENE_ALBUM,

	CUST_FLASH_SECTION_MODEL_AUDIO = 40,

	CUST_FLASH_SECTION_MODEL_FD = 50,
	CUST_FLASH_SECTION_MODEL_FR,
	CUST_FLASH_SECTION_MODEL_FO_HEAD_POSE,
	CUST_FLASH_SECTION_MODEL_GESTURE_HD,
	CUST_FLASH_SECTION_MODEL_GESTURE_HL,
	CUST_FLASH_SECTION_MODEL_SCENE,
	CUST_FLASH_SECTION_MODEL_FACIAL,

    CUST_FLASH_SECTION_POWER_ON_CONFIG = 60,
};


#ifdef __cplusplus
extern "C" {
#endif

void Flash_Lock();
void Flash_UnLock();
void flash_enable_XIP();
void flash_disable_XIP();
int flash_read(unsigned char Type, unsigned char **ppData, int *pDataSize, unsigned char *pFlag);
int flash_write(unsigned char Type, unsigned char *pData, int DataSize);
void flash_init();
void flash_deinit();
#if defined(HMX_FR_ENABLE)
int app_flash_restore_album_data(unsigned char **ppAddr, int *pSize, unsigned char *pFlag);
int app_flash_save_album_data(unsigned char *pAddr, int Size);
#endif
#ifdef __cplusplus
}
#endif

#endif
