#include "app_platform.h"
#include "app_api.h"

uint8_t *psram_ptr = (uint8_t *)0x3c000000;

int8_t app_psram_memcpy(void *str1, const void *str2, size_t n)
{
    uint32_t *mem_ptr = str1;
    uint32_t *aligned_addr;
    uint8_t tmp[4];
    uint8_t *byte_ptr;
    uint32_t *dw_ptr;
    uint32_t head_un_align_len = 0, tail_un_align_len = 0;
    uint32_t val = 0;
    int32_t i;
    int32_t aligned_idx, src_idx, copy_idx;

    if (0 == n)
        return (void *)str1;

    head_un_align_len = ((uint32_t)str1 % 4);
    tail_un_align_len = ((uint32_t)&((uint8_t*)str1)[n] % 4);
    //printf("head_un_align_len = %d\n", head_un_align_len);
    //printf("tail_un_align_len = %d\n", tail_un_align_len);
    aligned_idx = (uint32_t)&((uint8_t*)str1)[0] - (uint32_t)psram_ptr;
    src_idx = (uint32_t)&((uint8_t*)str2)[0] - (uint32_t)psram_ptr;
    copy_idx = src_idx;
    aligned_idx -= head_un_align_len;

    // copy the first 4 - head_un_align_len bytes value.
    memcpy(tmp, &psram_ptr[aligned_idx], 4);
    for (i = head_un_align_len; i < 4; i++) {
        tmp[i] = psram_ptr[copy_idx++];
        //printf("src_idx + i - head_un_align_len = %d\n", src_idx + i - head_un_align_len);
    }
    memcpy(&val, tmp, 4);
    //printf("val=%08x\n", val);
    dw_ptr = (uint32_t *)&psram_ptr[aligned_idx];
    hx_InvalidateDCache_by_Addr((volatile void *)dw_ptr, sizeof(uint8_t) * 4);
    *dw_ptr = val;
    //printf("*dw_ptr = %08x\n", *dw_ptr);

    /* set the memory by 4 bytes. */
    for ( i = 4; i < n - tail_un_align_len; i+=4 ) {
        //printf("i = %d, n - tail_un_align_len = %d\n", i, n - tail_un_align_len);
        dw_ptr = (uint32_t *)&psram_ptr[aligned_idx + i];
        memcpy(&val, &psram_ptr[copy_idx], 4);
        copy_idx += 4;
        hx_InvalidateDCache_by_Addr((volatile void *)dw_ptr, sizeof(uint8_t) * 4);
        *dw_ptr = val;
        //printf("set addr[%08x] to %08x\n", dw_ptr, val);
    }

    /* set the tail 4 bytes value. */
    //printf("tail idx = aligned_idx + i = %d\n", aligned_idx + i);
    dw_ptr = (uint32_t *)&psram_ptr[aligned_idx + i];
    memcpy(tmp, dw_ptr, 4);
    //printf("tail data = 0x%08x\n", *dw_ptr);
    for (i = 0; i < tail_un_align_len; i++) {
        tmp[i] = psram_ptr[copy_idx++];
        //printf("aligned_idx + i = %d\n", aligned_idx + i);
    }
    memcpy(&val, tmp, 4);
    hx_InvalidateDCache_by_Addr((volatile void *)dw_ptr, sizeof(uint8_t) * 4);
    *dw_ptr = val;

    return (void *)mem_ptr;
}

void *app_psram_memset(void *str, int32_t c, size_t n)
{
    uint32_t *mem_ptr = str;
    uint32_t val = 0;
    int32_t i;

    printf("app_psram_memset(addr=0x%08x, c=0x%08x, size=%d)\n", str, c, n);

    if ((uint32_t)str < 0x3c000000 || (uint32_t)str >= 0x3c800000) {
        printf("app_psram_memset: the address %08x is not PSRAM!\n", (uint32_t)str);
        return NULL;
    }

    uint32_t *aligned_addr;
    uint8_t tmp[4];
    uint8_t *byte_ptr;
    uint32_t *dw_ptr;
    uint32_t head_un_align_len = 0, tail_un_align_len = 0;
    int32_t aligned_idx;

    if (0 == n)
        return (void *)str;

    head_un_align_len = ((uint32_t)str % 4);
    tail_un_align_len = ((uint32_t)&((uint8_t*)str)[n] % 4);
    //printf("head_un_align_len = %d\n", head_un_align_len);
    //printf("tail_un_align_len = %d\n", tail_un_align_len);
    aligned_idx = (uint32_t)&((uint8_t*)str)[0] - (uint32_t)psram_ptr;
    //printf("idx = %d\n", aligned_idx);
    aligned_idx -= head_un_align_len;
    //printf("aligned_idx = %d\n", aligned_idx);

    // set the default 4 bytes value.
    val = (c & 0xff) | ((c<<8) & 0xff00) | ((c<<16) & 0xff0000) | ((c<<24) & 0xff000000);
    //printf("val = %08x\n", val);

    // set the first 4 bytes value.
    memcpy(tmp, &val, 4);
    for (i = 0; i < head_un_align_len; i++) {
        tmp[i] = psram_ptr[aligned_idx + i];
        //printf("aligned_idx + i = %d\n", aligned_idx + i);
    }
    memcpy(&val, tmp, 4);
    //printf("val=%08x\n", val);
    dw_ptr = (uint32_t *)&psram_ptr[aligned_idx];
    hx_InvalidateDCache_by_Addr((volatile void *)dw_ptr, sizeof(uint8_t) * 4);
    *dw_ptr = val;
    //printf("*dw_ptr = %08x\n", *dw_ptr);

    // set the default 4 bytes value.
    val = (c & 0xff) | ((c<<8) & 0xff00) | ((c<<16) & 0xff0000) | ((c<<24) & 0xff000000);
    //printf("val = %08x\n", val);

    /* set the memory by 4 bytes. */
    for ( i = 4; i < n - tail_un_align_len; i+=4 ) {
        //printf("i = %d, n - tail_un_align_len = %d\n", i, n - tail_un_align_len);
        dw_ptr = (uint32_t *)&psram_ptr[aligned_idx + i];
        hx_InvalidateDCache_by_Addr((volatile void *)dw_ptr, sizeof(uint8_t) * 4);
        *dw_ptr = val;
        //printf("set addr[%08x] to %08x\n", dw_ptr, val);
    }

    /* set the tail 4 bytes value. */
    //printf("tail idx = aligned_idx + i = %d\n", aligned_idx + i);
    dw_ptr = (uint32_t *)&psram_ptr[aligned_idx + i];
    memcpy(tmp, dw_ptr, 4);
    printf("tail data = 0x%08x\n", *dw_ptr);
    for (i = 0; i < tail_un_align_len; i++) {
        tmp[i] = c & 0xff;
        //printf("aligned_idx + i = %d\n", aligned_idx + i);
    }
    memcpy(&val, tmp, 4);
    hx_InvalidateDCache_by_Addr((volatile void *)dw_ptr, sizeof(uint8_t) * 4);
    *dw_ptr = val;

    //printf("\n");

    return (void *)mem_ptr;
}

int8_t app_psram_memchk(void *str, int32_t c, size_t n) //check the memset result
{
    uint32_t *mem_ptr = str;
    uint32_t val = 0;
    int32_t i;
    int8_t ret = 0;

    printf("app_psram_memchk(addr=0x%08x, c=0x%08x, size=%d)\n", str, c, n);

    val = (c & 0xff) | ((c<<8) & 0xff00) | ((c<<16) & 0xff0000) | ((c<<24) & 0xff000000);

    //hx_InvalidateDCache_by_Addr((volatile void *)str, sizeof(uint8_t) * n);

    /* check the dst memory is in the psram range. */
    /* verify if the start address and size is 4 bytes aligned. */
    /* if the start address or size is not 4 bytes aligned, handle the un-aligned bytes in the function. */
    /* set the memory by 4 bytes. */
    for ( i = 0; i < n/4; i += 1 ) {
        hx_InvalidateDCache_by_Addr((volatile void *)&mem_ptr[i], sizeof(uint8_t) * 4);
        if ( mem_ptr[i] != val ) {
            printf("mem_ptr[%d] = 0x%08x mis-matched! (addr = 0x%08x)\n", i, mem_ptr[i], &mem_ptr[i]);
            ret = -1;
        }
    }

    if (ret == 0)
        printf("app_psram_memchk complete.\n");
    else
        printf("app_psram_memchk failed!!!\n");
    return ret;
}

void app_init()
{
    /* PINMUX. */
    app_board_pinmux_settings();

    /* SPI Init. */
}


void app_main()
{
    uint32_t *psram_addr = 0x3c000000;
    uint8_t *byte_ptr;
    int32_t i;

    dbg_printf(DBG_LESS_INFO, "psram_example\n");

    /* Open PSRAM */
    hx_lib_spi_psram_open();
    /* PSRAM Enable XIP Mode */
    hx_lib_spi_psram_SetXIP(1);
 
    /* Dump 0x3c000000 */
    printf("Dump 0x3c000000\n");
    for ( i = 0; i < 10; i++ ) {
        printf("addr[%08x] = %08x\n", &psram_addr[i], psram_addr[i]);
    }

    /* Set value to 0x3c000004 */
    printf("\nSet 0x3c000004 value to 0x20231229\n");
    psram_addr[1] = 0x20231229;

    /* Dump 0x3c000000 */
    printf("\nDump 0x3c000000\n");
    for ( i = 0; i < 10; i++ ) {
        printf("addr[%08x] = %08x\n", &psram_addr[i], psram_addr[i]);
    }

    // set the first 4 MB to 0xaa.
    printf("set the first 4 MB to 0xaa...\n");
    app_psram_memset((void *)0x3c000004, 0xaa, 4*1024*1024 -4);
    app_psram_memchk((void *)0x3c000004, 0xaa, 4*1024*1024 -4);
    printf("done.\n");

    // copy the first 4 MB to last 4 MB.
    printf("copy the first 4 MB to last 4 MB...\n");
    app_psram_memcpy((void *)0x3c400000, (void *)0x3c000004, 4*1024*1024 -4);
    printf("done.\n");

    // check the last 4 MB contains.
    printf("check the last 4 MB contains...\n");
    app_psram_memchk((void *)0x3c400000, 0xaa, 4*1024*1024 -4);
    printf("done.\n");
}

