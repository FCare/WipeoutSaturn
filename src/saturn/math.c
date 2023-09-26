#include "fixed_math.h"

#include <gamemath/fix16.h>

float sqrt(float x) {
  return (float)fix16_sqrt((fix16_t)x);
}

float sin(float x) {
  return (float)fix16_sin((angle_t)RAD2ANGLE(x))/65536.0f;
}

float cos(float x) {
  return (float)fix16_cos((angle_t)RAD2ANGLE(x))/65536.0f;
}

float tan(float x) {
  return (float)fix16_tan((angle_t)RAD2ANGLE(x))/65536.0f;
}

float acos(float x) {
  //to be developed
  return 0.0f;
}

float fmod(float x, float y) {
  while(x >= y) x-=y;
  return x;
}

float atan2(float x, float y) {
  return (float)fix16_atan2((angle_t)RAD2ANGLE(x), (angle_t)RAD2ANGLE(y));
}

float fabsf(float x) {
  if (x<0.0) return -x;
  else return x;
}

float floor(float x) {
  return (float)((int)x);
}

float ceil(float x) {
  return (float)((int)(x+0.5));
}
