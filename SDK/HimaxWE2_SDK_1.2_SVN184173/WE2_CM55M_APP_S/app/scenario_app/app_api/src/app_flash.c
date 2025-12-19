#include "app_flash.h"
#include "app_memory.h"
#include "com_flash_boot.h"
#include "WE2_debug.h"

//#define PRINT_FLASH
#if defined(IP_INST_QSPI_HOST) || defined(IP_INST_QSPI_SLAVE)
//----------------------------------------------
#define MEMORY_DESCRIPTOR_SECTION_SIZE			(0x1000)
#define SECURE_HEADER_2K_SIZE					(0x4ac)
#define SECURE_HEADER_SIZE 						(0x6ac)

#define XIP_BASE_ADDR		FLASH1_BASE

//----------------------------------------------
//
unsigned char flash_init_flag = 0;
FLASH_SECTIONS Flash_sections = {
	.num_sbs = 0,
	.pitem = NULL
};

#ifdef OS_FREERTOS
#include "FreeRTOS.h"
#include "semphr.h"
SemaphoreHandle_t mutex_flash = NULL;
#endif

char *pat_type_string(unsigned int pat_type)
{
	char *p;

	switch(pat_type)
	{
		case SEC_IMAGE_TYPE_BOOTLOADER:
			p = "bootloader";
			break;
		case SEC_IMAGE_TYPE_SECOND_BOOTLOADER:
			p = "2nd bootloader";
			break;
		case SEC_IMAGE_TYPE_LAYOUT_DESCRIPTOR:
			p = "memory descriptor";
			break;
		case SEC_IMAGE_TYPE_CM55M_S_APP:
			p = "big Seucre app";
			break;
		case SEC_IMAGE_TYPE_CM55M_NS_APP:
			p = "big Non Seucre app";
			break;
		case SEC_IMAGE_TYPE_CM55S_S_APP:
			p = "LITTLE Seucre app";
			break;
		case SEC_IMAGE_TYPE_CM55S_NS_APP:
			p = "LITTLE Non Seucre app";
			break;
		case SEC_IMAGE_TYPE_CM55M_MODEL:
			p = "big Model";
			break;
		case SEC_IMAGE_TYPE_CM55S_MODEL:
			p = "LITTLE Model";
			break;
		case SEC_IMAGE_TYPE_CM55M_APPCFG:
			p = "big app config";
			break;
		case SEC_IMAGE_TYPE_CM55S_APPCFG:
			p = "LITTLE app config";
			break;
		case SEC_IMAGE_TYPE_SEC_MEM_LAYOUT:
			p = "Secure memory layout";
			break;
		case SEC_IMAGE_TYPE_SEC_DEBUG:
			p = "secure debug";
			break;
		case SEC_IMAGE_MAX:
			p = "max";
			break;

		case CUST_FLASH_SECTION_AREA_CONFIG:
			p = "cust_area_config";
			break;
		case CUST_FLASH_SECTION_FR_ALBUM:
			p = "cust_fr_album";
			break;
		case CUST_FLASH_SECTION_SCENE_ALBUM:
			p = "cust_scene_album";
			break;
		case CUST_FLASH_SECTION_MODEL_AUDIO:
			p = "cust_model_audio";
			break;
		case CUST_FLASH_SECTION_MODEL_FD:
			p = "cust_model_fd";
			break;
		case CUST_FLASH_SECTION_MODEL_FR:
			p = "cust_model_fr";
			break;
		case CUST_FLASH_SECTION_MODEL_FO_HEAD_POSE:
			p = "cust_model_head_pose";
			break;
		case CUST_FLASH_SECTION_MODEL_GESTURE_HD:
			p = "cust_model_gesture_hd";
			break;
		case CUST_FLASH_SECTION_MODEL_GESTURE_HL:
			p = "cust_model_gesture_hl";
			break;
		case CUST_FLASH_SECTION_MODEL_SCENE:
			p = "cust_model_scene";
			break;
		default:
			p = "undefined";
			break;
	}

	return p;
}

int flash_read_memory_descriptor(unsigned char **ppData, unsigned int *pSize, unsigned char *pFlag)
{
#ifdef FLASH_AS_SRAM
	*pFlag = 0;
	*ppData = XIP_BASE_ADDR + 0;
	*pSize = MEMORY_DESCRIPTOR_SECTION_SIZE;

	return FLASH_OK;
#else
	unsigned char *p;
	unsigned char flash_info[3];
	int ret;
	int i;
	//xprintf("\t\033[33m[%s]\033[0m, \033[33mline = %d\033[0m\n",
	//		__FUNCTION__,
	//		__LINE__);
    p = (unsigned char *)app_mem_alloc(MEMORY_DESCRIPTOR_SECTION_SIZE,32);
	if(p == NULL)
	{
		return FLASH_MALLOC_FAIL;
	}
    
	memset(p, 0x00, MEMORY_DESCRIPTOR_SECTION_SIZE);
	ret = hx_lib_spi_eeprom_read_ID(USE_DW_SPI_MST_Q, flash_info);
	if(ret == 0)
	{
		xprintf("flash info data[%x][%x][%x]\n", flash_info[0], flash_info[1], flash_info[2]);
	}

	ret = hx_lib_spi_eeprom_4read(USE_DW_SPI_MST_Q, 0, p, MEMORY_DESCRIPTOR_SECTION_SIZE);
	if(ret != 0)
	{
		xprintf("ret = %d, read memory descriptor from flash fail!!!\n", ret);
		app_mem_free((void *)p);
		return FLASH_READ_FAIL;
	}

	for(i=0; i<(MEMORY_DESCRIPTOR_SECTION_SIZE>>2); i++)
	{
		unsigned char value[4];

		value[0] = p[(i<<2) + 0];
		value[1] = p[(i<<2) + 1];
		value[2] = p[(i<<2) + 2];
		value[3] = p[(i<<2) + 3];

		p[(i<<2) + 0] = value[3];
		p[(i<<2) + 1] = value[2];
		p[(i<<2) + 2] = value[1];
		p[(i<<2) + 3] = value[0];
	}

	*pFlag = 1;
	*ppData = p;
	*pSize = MEMORY_DESCRIPTOR_SECTION_SIZE;

	return FLASH_OK;
#endif
}

int flash_parse_memory_desriptor_sections(FLASH_SECTIONS *pFlash)
{
	//memory_descriptor_section =
	//		secure header +
	//		secure memory layout +
	//		memory descriptor

	secure_memory_layout_t *pSecureMemoryLayout;
	memory_dsp_t *pMemoryDescriptor;
	MEMORY_DESCRIPTOR_ATTR *pMemoryDescriptorAttribute;
	unsigned int offset = 0;
	unsigned char num_sbs;
	int i;
	unsigned char *pData = NULL;
	unsigned int Size = 0;
	unsigned char Flag = 0;
	int ret;

	ret = flash_read_memory_descriptor(&pData, &Size, &Flag);
	if(ret != FLASH_OK)
	{
		xprintf("\t\033[33m[%s]\033[0m, ret = %d, \033[33mline = %d\033[0m\n",
				__FUNCTION__,
				ret,
				__LINE__);
		ret = FLASH_READ_MEMORY_DESCRIPTOR_FAIL;
		return ret;
	}

	offset = SECURE_HEADER_2K_SIZE;
	#ifdef PRINT_FLASH
	xprintf("\t\033[33m[%s]\033[0m, offset = 0x%x, %d, \033[33mline = %d\033[0m\n",
			__FUNCTION__,
			offset, offset,
			__LINE__);
	//print_data(pData + offset, 64);
	#endif

	pSecureMemoryLayout = (secure_memory_layout_t *) (pData + offset);
	num_sbs = pSecureMemoryLayout->num_sbs;
	offset += 4;
	offset += (num_sbs * sizeof(epii_secure_memory_item_t));
	pMemoryDescriptor = (memory_dsp_t *)(pData + offset);

	#ifdef PRINT_FLASH
	for(i=0; i<num_sbs; i++)
	{
		xprintf("\t\033[33m[%s]\033[0m, [%d], (0x%x, 0x%x) \033[33mline = %d\033[0m\n",
				__FUNCTION__,
				i, pSecureMemoryLayout->item[i].start_addr, pSecureMemoryLayout->item[i].end_addr,
				__LINE__);
	}
	xprintf("\t\033[33m[%s]\033[0m, type = 0x%x, \033[33mline = %d\033[0m\n",
			__FUNCTION__,
			pMemoryDescriptor->info.type,
			__LINE__);
	xprintf("\t\033[33m[%s]\033[0m, version = 0x%x, \033[33mline = %d\033[0m\n",
			__FUNCTION__,
			pMemoryDescriptor->info.version,
			__LINE__);
	xprintf("\t\033[33m[%s]\033[0m, crc = 0x%x, \033[33mline = %d\033[0m\n",
			__FUNCTION__,
			pMemoryDescriptor->info.crc,
			__LINE__);
	xprintf("\t\033[33m[%s]\033[0m, pid = 0x%x, \033[33mline = %d\033[0m\n",
			__FUNCTION__,
			pMemoryDescriptor->info.pid,
			__LINE__);
	xprintf("\t\033[33m[%s]\033[0m, size = %d, \033[33mline = %d\033[0m\n",
			__FUNCTION__,
			pMemoryDescriptor->info.size,
			__LINE__);
	xprintf("\t\033[33m[%s]\033[0m, fw_image_version = 0x%x, \033[33mline = %d\033[0m\n",
			__FUNCTION__,
			pMemoryDescriptor->info.fw_image_version,
			__LINE__);
	xprintf("\t\033[33m[%s]\033[0m, num_sbs = %d, \033[33mline = %d\033[0m\n",
			__FUNCTION__,
			pMemoryDescriptor->info.num_sbs,
			__LINE__);
	xprintf("\t\033[33m[%s]\033[0m, reserved1 = %d, \033[33mline = %d\033[0m\n",
			__FUNCTION__,
			pMemoryDescriptor->info.reserved1,
			__LINE__);
	xprintf("\t\033[33m[%s]\033[0m, reserved2 = %d, \033[33mline = %d\033[0m\n",
			__FUNCTION__,
			pMemoryDescriptor->info.reserved2,
			__LINE__);
	xprintf("\t\033[33m[%s]\033[0m, flash_maxsize = %d, \033[33mline = %d\033[0m\n",
			__FUNCTION__,
			pMemoryDescriptor->info.flash_maxsize,
			__LINE__);
	#endif

	num_sbs = pMemoryDescriptor->info.num_sbs;
    pFlash->pitem = (FLASH_SECTION_ITEM *)app_mem_alloc(num_sbs * sizeof(FLASH_SECTION_ITEM),32);
	if(pFlash->pitem == NULL)
	{
		if(Flag == 1)
		{
			app_mem_free((void *)pData);
		}
		return FLASH_MALLOC_FAIL;
	}
	pFlash->num_sbs = num_sbs;

	for(i=0; i<num_sbs; i++)
	{
		pMemoryDescriptorAttribute = &pMemoryDescriptor->item[i].pat_att;

		#ifdef PRINT_FLASH
		xprintf("\n\t\033[33m[%s]\033[0m, [%d], pat_type = %d (%s), \033[33mline = %d\033[0m\n",
				__FUNCTION__,
				i, pMemoryDescriptor->item[i].pat_type, pat_type_string(pMemoryDescriptor->item[i].pat_type),
				__LINE__);
		xprintf("\t\033[33m[%s]\033[0m, [%d], pat_att = 0x%x, \033[33mline = %d\033[0m\n",
				__FUNCTION__,
				i, pMemoryDescriptor->item[i].pat_att,
				__LINE__);
		xprintf("\t\t\033[33m[%s]\033[0m, [%d], pat_att->format = 0x%x, \033[33mline = %d\033[0m\n",
				__FUNCTION__,
				i, pMemoryDescriptorAttribute->format,
				__LINE__);
		xprintf("\t\t\033[33m[%s]\033[0m, [%d], pat_att->type = 0x%x, \033[33mline = %d\033[0m\n",
				__FUNCTION__,
				i, pMemoryDescriptorAttribute->type,
				__LINE__);
		xprintf("\t\t\033[33m[%s]\033[0m, [%d], pat_att->secure_header_size = 0x%x, \033[33mline = %d\033[0m\n",
				__FUNCTION__,
				i, pMemoryDescriptorAttribute->secure_header_size,
				__LINE__);
		xprintf("\t\033[33m[%s]\033[0m, [%d], pat_addr = 0x%x, \033[33mline = %d\033[0m\n",
				__FUNCTION__,
				i, pMemoryDescriptor->item[i].pat_addr,
				__LINE__);
		xprintf("\t\033[33m[%s]\033[0m, [%d], pat_size = %d, \033[33mline = %d\033[0m\n",
				__FUNCTION__,
				i, pMemoryDescriptor->item[i].pat_size,
				__LINE__);
		xprintf("\t\033[33m[%s]\033[0m, [%d], pat_exec_addr = 0x%x, \033[33mline = %d\033[0m\n",
				__FUNCTION__,
				i, pMemoryDescriptor->item[i].pat_exec_addr,
				__LINE__);
		xprintf("\t\033[33m[%s]\033[0m, [%d], pat_ccitt_crc16 = 0x%x, \033[33mline = %d\033[0m\n",
				__FUNCTION__,
				i, pMemoryDescriptor->item[i].pat_ccitt_crc16,
				__LINE__);
		#endif      
		pFlash->pitem[i].type = pMemoryDescriptor->item[i].pat_type;
		pFlash->pitem[i].info.base_addr = pMemoryDescriptor->item[i].pat_addr;
		pFlash->pitem[i].info.size = pMemoryDescriptor->item[i].pat_size;
		if(pMemoryDescriptorAttribute->format != SB_SECURE_TYPE_RAW)
		{
			pFlash->pitem[i].info.offset = pMemoryDescriptorAttribute->secure_header_size;
		}
		else
		{
			pFlash->pitem[i].info.offset = 0;
		}
	}

	if(Flag == 1)
	{
		app_mem_free((void *)pData);
	}

	return FLASH_OK;
}

int get_flash_info_index(unsigned int type)
{
	int i;

	for(i=0; i<Flash_sections.num_sbs; i++)
	{
		if(type == Flash_sections.pitem[i].type)
		{
			#ifdef PRINT_FLASH
			xprintf("\t\033[1;31m[%s]\033[0m, [%d] (%d, 0x%x, %d) \033[33mline = %d\033[0m\n",
					__FUNCTION__,
					i,
					Flash_sections.pitem[i].type,
					Flash_sections.pitem[i].info.base_addr,
					Flash_sections.pitem[i].info.size,
					__LINE__);
			#endif
			return i;
		}
	}
	return FLASH_UNDEFINED_SECTION;
}

//----------------------------------------------
void Flash_Lock()
{
	#ifdef OS_FREERTOS
		if(mutex_flash)
		{
			xSemaphoreTake(mutex_flash, portMAX_DELAY);
		}
	#endif
}

void Flash_UnLock()
{
	#ifdef OS_FREERTOS
		if(mutex_flash)
		{
			xSemaphoreGive(mutex_flash);
		}
	#endif
}

#ifdef FLASH_AS_SRAM
void flash_enable_XIP()
{
	int ret;
	
	//hx_lib_spi_eeprom_open(USE_DW_SPI_MST_Q);
	
	ret = hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, true, FLASH_QUAD, true);
	
	if (ret != 0) {
       dbg_printf(DBG_LESS_INFO, "hx_lib_spi_eeprom_enable_XIP failed!\n");
    } else {
        dbg_printf(DBG_LESS_INFO, "hx_lib_spi_eeprom_enable_XIP OK.\n");
    }
}

void flash_disable_XIP()
{
	int ret;
	
	//hx_lib_spi_eeprom_open(USE_DW_SPI_MST_Q);
	
	hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, false, FLASH_QUAD, false);

	if (ret != 0) {
       dbg_printf(DBG_LESS_INFO, "hx_lib_spi_eeprom_enable_XIP failed!\n");
    } else {
        dbg_printf(DBG_LESS_INFO, "hx_lib_spi_eeprom_enable_XIP OK.\n");
    }
}
#endif

int flash_read(unsigned char Type, unsigned char **ppData, int *pDataSize, unsigned char *pFlag)
{
	unsigned char *pData = NULL;
	int ret = 0;
	int index;
	FLASH_SECTION_INFO *pInfo;
	int i;
	unsigned char *paddr;
	unsigned int size;

	*pFlag = 0;
	index = get_flash_info_index(Type);
	if(index != FLASH_UNDEFINED_SECTION)
	{
		pInfo = &Flash_sections.pitem[index].info;
		paddr = pInfo->base_addr + pInfo->offset;
		size = pInfo->size - pInfo->offset;

		#ifdef FLASH_AS_SRAM
			*ppData = (unsigned char *)paddr + XIP_BASE_ADDR;
			*pDataSize = (int) size;
			*pFlag = 0;
		#else
			flash_disable_XIP();
            pData = (unsigned char *)app_mem_alloc(size,32);
			if(pData == NULL)
			{
				*ppData = pData;
				*pDataSize = 0;
				ret = FLASH_MALLOC_FAIL;
			}
			else
			{
				*pFlag = 1;
				if(0!=hx_lib_spi_eeprom_4read(USE_DW_SPI_MST_Q, paddr, (uint32_t)pData, size))
				{
					xprintf("read from flash fail!!!\n");
					ret = FLASH_READ_FAIL;
				}
				else
				{
					for(i=0; i<(size>>2); i++)
					{
						unsigned char value[4];

						value[0] = pData[(i<<2) + 0];
						value[1] = pData[(i<<2) + 1];
						value[2] = pData[(i<<2) + 2];
						value[3] = pData[(i<<2) + 3];

						pData[(i<<2) + 0] = value[3];
						pData[(i<<2) + 1] = value[2];
						pData[(i<<2) + 2] = value[1];
						pData[(i<<2) + 3] = value[0];
					}
					//xprintf("read from flash successful\n");
					//xprintf("pInfo->base_addr = 0x%08x\n", pInfo->base_addr);
					//xprintf("pInfo->offset = 0x%x\n", pInfo->offset);
					//xprintf("size = %d\n", size);
					*ppData = pData;
					*pDataSize = size;
					ret = FLASH_OK;
				}
			}
			flash_enable_XIP();
		#endif
	}
	else
	{
		xprintf("access illegal flash section\n");
		ret = index;
	}

	return ret;
}

int flash_read_secure_cert(unsigned char Type, unsigned char **ppData, unsigned int *pDataSize, unsigned char *pFlag)
{
	unsigned char *pData = NULL;
	int ret = 0;
	unsigned char index;
	FLASH_SECTION_INFO *pInfo;
	int i;
	unsigned char *paddr;
	unsigned int size;

	*pFlag = 0;
	index = get_flash_info_index(Type);
	if(index != FLASH_UNDEFINED_SECTION)
	{
		pInfo = &Flash_sections.pitem[index].info;
		paddr = pInfo->base_addr;
		size = pInfo->offset;
		#ifdef FLASH_AS_SRAM
			*ppData = (unsigned char *)paddr + XIP_BASE_ADDR;
			*pDataSize = (int) size;
			*pFlag = 0;
		#else
			flash_disable_XIP();
			if(size == 0)
			{
				*pDataSize = 0;
				return 0;
			}

            pData = (unsigned char *)app_mem_alloc(size,32);
            
			if(pData == NULL)
			{
				*ppData = pData;
				*pDataSize = 0;
				ret = FLASH_MALLOC_FAIL;
			}
			else
			{
				*pFlag = 1;
				if(0!=hx_lib_spi_eeprom_4read(USE_DW_SPI_MST_Q, paddr, (uint32_t)pData, size))
				{
					xprintf("read from flash fail!!!\n");
					ret = FLASH_READ_FAIL;
				}
				else
				{
					for(i=0; i<(size>>2); i++)
					{
						unsigned char value[4];

						value[0] = pData[(i<<2) + 0];
						value[1] = pData[(i<<2) + 1];
						value[2] = pData[(i<<2) + 2];
						value[3] = pData[(i<<2) + 3];

						pData[(i<<2) + 0] = value[3];
						pData[(i<<2) + 1] = value[2];
						pData[(i<<2) + 2] = value[1];
						pData[(i<<2) + 3] = value[0];
					}
					//xprintf("read from flash successful\n");
					//xprintf("pInfo->base_addr = 0x%x\n", paddr);
					//xprintf("pInfo->offset = 0x%x\n", pInfo->offset);
					//xprintf("pInfo->size = %d\n", size);
					*ppData = pData;
					*pDataSize = size;
					ret = FLASH_OK;
				}
			}
			flash_enable_XIP();
		#endif
	}
	else
	{
		xprintf("access illegal flash section\n");
		ret = FLASH_UNDEFINED_SECTION;
	}

	return ret;
}

int flash_erase(unsigned char Type, int EraseSize)		//integrate into flash_write
{
	int ret = 0;
	int i;
	int EraseSector;
	int index;
	FLASH_SECTION_INFO *pInfo;

	index = get_flash_info_index(Type);
	if(index != FLASH_UNDEFINED_SECTION)
	{
		pInfo = &Flash_sections.pitem[index].info;

		flash_disable_XIP();
		if(EraseSize == -1)
		{
			EraseSector = (pInfo->size + 0x1000 - 1) / 0x1000;
		}
		else
		{
			EraseSector = (EraseSize + 0x1000 - 1) / 0x1000;
		}
		for(i=0; i<EraseSector; i++)
    	{
			if(hx_lib_spi_eeprom_erase_sector(USE_DW_SPI_MST_Q, (unsigned int)pInfo->base_addr + (i * 0x1000), FLASH_SECTOR) != 0)
			{
				xprintf( "hx_drv_spi_flash_protocol_erase_sector fail \n");
				ret = FLASH_ERASE_SECTOR_FAIL;
			}
		}
		flash_enable_XIP();
	}
	else
	{
		xprintf("access illegal flash section\n");
		ret = index;
	}

	return ret;
}

int flash_write(unsigned char Type, unsigned char *pData, int DataSize)
{
	int ret = 0;
	int index;
	FLASH_SECTION_INFO *pInfo;
	unsigned char *paddr;
	unsigned int size;

	unsigned char *pCert = NULL;
	unsigned int CertSize = 0;
	unsigned char Flag = 0;
	unsigned char *p;

	ret = flash_read_secure_cert(Type, &pCert, &CertSize, &Flag);
	if(ret < 0)
	{
		return ret;
	}
	if(CertSize)
	{
		if(Flag == 0)
		{
            p = (unsigned char *)app_mem_alloc(CertSize,32);
			if(p == NULL)
			{
				return FLASH_MALLOC_FAIL;
			}
			memcpy(p, pCert, CertSize);
		}
		else
		{
			p = pCert;
		}
	}

	ret = flash_erase(Type, DataSize);
	if(ret != 0)
	{
		return ret;
	}

	index = get_flash_info_index(Type);
	if(index != FLASH_UNDEFINED_SECTION)
	{
		pInfo = &Flash_sections.pitem[index].info;
		paddr = pInfo->base_addr + pInfo->offset;
		size = pInfo->size - pInfo->offset;

		flash_disable_XIP();

		//write secure cert
		if(CertSize)
		{
			if(hx_lib_spi_eeprom_word_write(USE_DW_SPI_MST_Q, pInfo->base_addr, p, CertSize) != 0)
			{
				xprintf( "write flash fail flash_addr=0x%x, unSize=%d\n", paddr, size);
				ret = FLASH_WRITE_FAIL;
			}
		}

		//write data
		if(hx_lib_spi_eeprom_word_write(USE_DW_SPI_MST_Q, paddr, pData, DataSize) != 0)
		{
			xprintf( "write flash fail flash_addr=0x%x, unSize=%d\n", paddr, size);
			ret = FLASH_WRITE_FAIL;
		}        
		flash_enable_XIP();
	}
	else
	{
		xprintf("access illegal flash section\n");
		ret = index;
	}

    if(p!=NULL)
    {
    	app_mem_free((void *)p);
    }

	return ret;
}

void flash_init()
{
	int ret;

	#ifdef OS_FREERTOS
		if(mutex_flash == NULL)
		{
			mutex_flash = xSemaphoreCreateMutex();
		}
	#endif
	
	dbg_printf(DBG_LESS_INFO, "flash_init...\n");
	if(flash_init_flag == 0)
	{
		#ifdef FLASH_AS_SRAM
            unsigned char flash_info[3];
			ret = hx_lib_spi_eeprom_open(USE_DW_SPI_MST_Q);
			if (ret != 0) {
		        dbg_printf(DBG_LESS_INFO, "hx_lib_spi_eeprom_open failed!\n");
		    } else {
		        dbg_printf(DBG_LESS_INFO, "hx_lib_spi_eeprom_open OK.\n");
		    }
        	ret = hx_lib_spi_eeprom_read_ID(USE_DW_SPI_MST_Q, flash_info);
        	if(ret == 0)
        	{
        		xprintf("flash info data[%x][%x][%x]\n", flash_info[0], flash_info[1], flash_info[2]);
        	}        
			xprintf("XIP mode\n");
			flash_enable_XIP();
		#else
			ret = hx_lib_spi_eeprom_open(USE_DW_SPI_MST_Q);
		#endif

		ret = flash_parse_memory_desriptor_sections(&Flash_sections);
		if(ret != FLASH_OK)
		{
			xprintf("\t\033[33m[%s]\033[0m, ret = %d, \033[33mline = %d\033[0m\n",
					__FUNCTION__,
					ret,
					__LINE__);
		}

		flash_init_flag = 1;
	}
}

void flash_deinit()
{
	DEV_SPI_PTR dev_spi_m;

	#ifdef OS_FREERTOS
		if(mutex_flash)
		{
			vSemaphoreDelete(mutex_flash);
			mutex_flash = NULL;
		}
	#endif

    #ifdef FLASH_AS_SRAM
    flash_disable_XIP();
    #endif

	if(Flash_sections.pitem != NULL)
	{
		app_mem_free((void *)Flash_sections.pitem);
		Flash_sections.pitem = NULL;
	}
	dev_spi_m = hx_drv_spi_mst_get_dev(USE_DW_SPI_MST_Q);
	if(dev_spi_m)
	{
		dev_spi_m->spi_close();
	}
    
	flash_init_flag = 0;
}
#endif

#if defined(HMX_FR_ENABLE)
int app_flash_restore_album_data(unsigned char **ppAddr, int *pSize, unsigned char *pFlag)
{
	unsigned char *pBuffer = NULL;
	int Size = 0;
	int DataSize = 0;
	int nRet;

	Flash_Lock();
	nRet = flash_read(CUST_FLASH_SECTION_FR_ALBUM, &pBuffer, &Size, pFlag);
	Flash_UnLock();

	if(nRet < 0)
	{
		if(*pFlag == 1)
		{
			app_mem_free((void *)pBuffer);
		}
		pBuffer = NULL;
		Size = 0;
		return nRet;
	}
	DataSize =  (pBuffer[0] << 24) |
				(pBuffer[1] << 16) |
				(pBuffer[2] << 8)  |
				(pBuffer[3]);

	if(DataSize < Size)
	{
		Size = DataSize + 4;	//4: sizeof(AlbumSize)
	}

	*ppAddr = pBuffer;
	*pSize = Size;

	return nRet;
}

int app_flash_save_album_data(unsigned char *pAddr, int Size)
{
	int nRet = -1;

	Flash_Lock();
	nRet = flash_write(CUST_FLASH_SECTION_FR_ALBUM, pAddr, Size);
	Flash_UnLock();

	return nRet;
}
#endif

