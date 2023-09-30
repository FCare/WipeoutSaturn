#ifdef SATURN
#include "fixed_math.h"
#else
#define fix16_t float
#define FIX16(A) A
#include <math.h>
#endif