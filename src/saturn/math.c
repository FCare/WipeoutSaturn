#include "fixed_math.h"

#include <gamemath/fix16.h>

fix16_t sqrt(fix16_t x) {
  return (fix16_t)fix16_sqrt((fix16_t)x);
}

fix16_t sin(fix16_t x) {
  return (fix16_t)fix16_sin((angle_t)RAD2ANGLE(x))/65536.0f;
}

fix16_t cos(fix16_t x) {
  return (fix16_t)fix16_cos((angle_t)RAD2ANGLE(x))/65536.0f;
}

fix16_t tan(fix16_t x) {
  return (fix16_t)fix16_tan((angle_t)RAD2ANGLE(x))/65536.0f;
}

fix16_t acos(fix16_t x) {
  //to be developed
  return 0.0f;
}

fix16_t fmod(fix16_t x, fix16_t y) {
	while (fix16_min(x, y) == y) x-=y;
  return x;
}

fix16_t atan2(fix16_t x, fix16_t y) {
  return (fix16_t)fix16_atan2((angle_t)RAD2ANGLE(x), (angle_t)RAD2ANGLE(y));
}

fix16_t fabsf(fix16_t x) {
  if (x<0.0) return -x;
  else return x;
}

fix16_t floor(fix16_t x) {
  return (fix16_t)((int)x);
}

fix16_t ceil(fix16_t x) {
  return (fix16_t)((int)(x+0.5));
}
