#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "mem.h"
#include "utils.h"

#define TEMP_MEM_HUNK_BYTES (0xFC000) //984Kb but 512KB looks enough

static uint8_t* hunk = (uint8_t*)MAP_CS1(0); //2MB on RAM cartridge
static uint8_t* temp_hunk = (uint8_t*)MAP_LWRAM(0); //1MB in highWram
static uint32_t bump_len = 0;
static uint32_t temp_len = 0;

static uint32_t max_mem = 0;

static uint32_t temp_objects[MEM_TEMP_OBJECTS_MAX] = {};
static uint32_t temp_objects_len = 0;


// Bump allocator - returns bytes from the front of the hunk

// These allocations persist for many frames. The allocator level is reset
// whenever we load a new race track or menu in game_set_scene()

void mem_init(void) {
}

void *mem_mark(void) {
	LOGD("Mem mark 0x%x\n", &hunk[bump_len]);
	return &hunk[bump_len];
}

void *mem_bump(uint32_t size) {
	error_if(bump_len + size >= MEM_HUNK_BYTES, "Failed to allocate %d bytes (0x%x)", size, bump_len);
	uint8_t *p = &hunk[bump_len];
	bump_len += size;
	LOGD("bump_len is 0x%x (0x%x)\n", bump_len, &bump_len);
	memset(p, 0, size);
  if (max_mem < bump_len) {
    max_mem = bump_len;
    LOGD("%d Persistent Mem => %d kB\n",__LINE__, max_mem/1024);
  }
	return p;
}

void mem_reset(void *p) {
	uint32_t offset = (uint8_t *)p - (uint8_t *)hunk;
	error_if(offset > bump_len || offset > MEM_HUNK_BYTES, "Invalid mem reset");
	bump_len = offset;
  if (max_mem < bump_len) {
    max_mem = bump_len;
    LOGD("%d Persistent Mem => %d kB\n",__LINE__, max_mem/1024);
  }
}

// Temp allocator - returns bytes from the back of the hunk

// Temporary allocated bytes are not allowed to persist for multiple frames. You
// need to explicitly free them when you are done. Temp allocated bytes don't
// have be freed in reverse allocation order. I.e. you can allocate A then B,
// and aftewards free A then B.

void *mem_temp_alloc(uint32_t size) {
	size = (size + 0x7)&~0x7; // allign to 8 bytes

  error_if(temp_len + size >= TEMP_MEM_HUNK_BYTES, "Failed to allocate %d bytes in temp mem (%d)", size, temp_len);
	error_if(temp_objects_len >= MEM_TEMP_OBJECTS_MAX, "MEM_TEMP_OBJECTS_MAX reached");

	temp_len += size;
	void *p = &temp_hunk[TEMP_MEM_HUNK_BYTES - temp_len];
	temp_objects[temp_objects_len++] = temp_len;
  if (max_mem < temp_len) {
    max_mem = temp_len;
    LOGD("%d temp Mem => %d kB\n",__LINE__, max_mem/1024);
  }
	return p;
}

void mem_temp_free(void *p) {
	uint32_t offset = (uint8_t *)&temp_hunk[TEMP_MEM_HUNK_BYTES] - (uint8_t *)p;
	error_if(offset > TEMP_MEM_HUNK_BYTES, "Object 0x%p not in temp hunk (%x %x)", p, offset, TEMP_MEM_HUNK_BYTES);

	bool found = false;
	uint32_t remaining_max = 0;
	for (uint32_t i = 0; i < temp_objects_len; i++) {
		if (temp_objects[i] == offset) {
			temp_objects[i--] = temp_objects[--temp_objects_len];
			found = true;
		}
		else if (temp_objects[i] > remaining_max) {
			remaining_max = temp_objects[i];
		}
	}
	error_if(!found, "Object 0x%p not in temp hunk", p);
	temp_len = remaining_max;
  if (max_mem < temp_len) {
    max_mem = temp_len;
    LOGD("%d temp Mem => %d kB\n",__LINE__, max_mem/1024);
  }
}

void mem_temp_check(void) {
	error_if(temp_len != 0, "Temp memory not free: %d object(s)", temp_objects_len);
}
