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

fix16_t rand_fix16_t(fix16_t min, fix16_t max) {
	return min + ((fix16_t)rand() * (max - min))/ (fix16_t)RAND_MAX;
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

static void swapArray(vdp1_cmdt_t *a, vdp1_cmdt_t *b) {
	vdp1_cmdt_t tmp = *a;
	*a = *b;
	*b = tmp;
}

static void swapMap(fix16_t *a, fix16_t *b) {
	fix16_t tmp = *a;
	*a = *b;
	*b = tmp;
}

// function to find the partition position
static int partition(vdp1_cmdt_t array[], uint16_t low, uint16_t high, fix16_t *map) {

  // select the rightmost element as pivot
  fix16_t pivot = map[high];

  // pointer for greater element
  int16_t i = (low - 1);

  // traverse each element of the array
  // compare them with the pivot
  for (int16_t j = low; j < high; j++) {
    if (map[j] <= pivot) {

      // if element smaller than pivot is found
      // swap it with the greater element pointed by i
      i++;

      // swap element at i with element at j
			swapArray(&array[i], &array[j]);
      swapMap(&map[i], &map[j]);
    }
  }

  // swap the pivot element with the greater element at i
	swapArray(&array[i+1], &array[high]);
  swapMap(&map[i+1], &map[high]);

  // return the partition point
  return (i + 1);
}

void quickSort_Z(vdp1_cmdt_t array[], uint16_t low, uint16_t high, fix16_t* map) {
  if (low < high) {

    // find the pivot element such that
    // elements smaller than pivot are on left of pivot
    // elements greater than pivot are on right of pivot
    int16_t pi = partition(array, low, high, map);

    // recursive call on the left of pivot
    if (low < (pi-1))
      quickSort_Z(array, low, pi - 1, map);

    // recursive call on the right of pivot
    if ((pi + 1) < high)
      quickSort_Z(array, pi + 1, high, map);
  }
}
