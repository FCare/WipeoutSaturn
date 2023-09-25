#include <stdio.h>

#include "utils.h"

#define RAND_MAX 0xFFFFFFFF

// Overwrite stdout and stderr
#ifdef DEBUG_PRINT
#define CS1(x)                  (0x24000000UL + (x))
static size_t debug_write(FILE *f __attribute__((unused)), const unsigned char * buf, size_t size) {
  volatile unsigned char *addr = (volatile unsigned char *)CS1(0x1000);
  for (size_t i = 0; i < size; i++) {
    unsigned char val = *buf++;
    if (val != 0) *addr = val;
  }
  return size;
}
#else
static size_t debug_write(FILE *f, const unsigned char * buf, size_t size) {
  return size;
}
#endif


FILE __stdout_FILE = {
    .write = debug_write,
    .buf_size = 0,
};

FILE __stderr_FILE = {
    .write = debug_write,
    .buf_size = 0,
};

float rand_float(float min, float max) {
	return min + ((float)rand() * (max - min))/ (float)RAND_MAX;
}

int32_t rand_int(int32_t min, int32_t max) {
	return min + rand() % (max - min);
}

char temp_path[64];
char *get_path(const char *dir, const char *file) {
	strcpy(temp_path, dir);
	strcpy(temp_path + strlen(dir), file);
	return temp_path;
}

bool str_starts_with(const char *haystack, const char *needle) {
	return (strncmp(haystack, needle, strlen(needle)) == 0);
}