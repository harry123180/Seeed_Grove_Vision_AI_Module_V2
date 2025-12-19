#ifndef _APP_MEMORY_H
#define _APP_MEMORY_H

#include "stdio.h"
#include "stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void* app_mem_alloc(size_t bytes,size_t alignment);
void app_mem_free(void *p);

#ifdef __cplusplus
}
#endif

#endif

