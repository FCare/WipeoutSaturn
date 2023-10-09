#ifndef MEM_H
#define MEM_H

#include "type.h"

#ifndef MEM_TEMP_OBJECTS_MAX
#define MEM_TEMP_OBJECTS_MAX 8
#endif

#ifndef MEM_HUNK_BYTES
#define MEM_HUNK_BYTES (4 * 1024 * 1024)
#endif

void *mem_bump(uint32_t size);
void *mem_mark(void);
void mem_reset(void *p);

void *mem_temp_alloc(uint32_t size);
void mem_temp_free(void *p);
void mem_temp_check(void);

#endif
