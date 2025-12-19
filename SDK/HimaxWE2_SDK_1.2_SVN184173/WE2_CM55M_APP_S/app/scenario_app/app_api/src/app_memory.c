#include "app_memory.h"
#include "xprintf.h"

//Use this API to malloc memory need use app_alignment_free to free
void* app_mem_alloc(size_t bytes,size_t alignment)
{
    void *p1 ,*p2;

    if((p1 = (void*)malloc(bytes + alignment + sizeof(size_t))) == NULL)
    {
        xprintf("app_mem_alloc fail:0x%X alignment:%u\r\n",bytes,alignment);
        return NULL;
    }

    //xprintf("app_mem_alloc:0x%08X bytes:%u %u\r\n",p1,bytes,bytes + alignment + sizeof(size_t));
    
    size_t addr = (size_t) p1 + alignment + sizeof(size_t);
    p2 = (void*)(addr - (addr % alignment));

    *((size_t*)p2 - 1) = (size_t)p1;

    return p2;
}

void app_mem_free(void *p)
{
    void* pp=(void*)(*((size_t*) p - 1));
    //xprintf("app_mem_free:0x%08X \r\n",pp);
    free(pp);
}

