#include "fixed_math.h"

#include <limits.h>
#include <gamemath/fix16.h>

static const fix16_t THREE_PI_DIV_4 = 0x00025B2F;       /*!< Fix16 value of 3PI/4 */

fix16_t sqrt(fix16_t x) {
  return (fix16_t)fix16_sqrt((fix16_t)x);
}

fix16_t sin(fix16_t inAngle) {
  fix16_t tempAngle = inAngle % (PLATFORM_PI << 1);

  if(tempAngle > PLATFORM_PI)
		tempAngle -= (PLATFORM_PI << 1);
	else if(tempAngle < -PLATFORM_PI)
		tempAngle += (PLATFORM_PI << 1);

	fix16_t tempAngleSq = fix16_mul(tempAngle, tempAngle);

printf("sin %d %d %d => ", inAngle, tempAngle, tempAngleSq);

  fix16_t tempOut;
	tempOut = fix16_mul(-13, tempAngleSq) + 546;
	tempOut = fix16_mul(tempOut, tempAngleSq) - 10923;
	tempOut = fix16_mul(tempOut, tempAngleSq) + 65536;
	tempOut = fix16_mul(tempOut, tempAngle);

printf("%d\n", tempOut);

  return tempOut;
}

fix16_t cos(fix16_t inAngle) {
  return sin(inAngle + PLATFORM_PI_2);
}

fix16_t tan(fix16_t inAngle) {
  	return fix16_sdiv(fix16_sin(inAngle), fix16_cos(inAngle));
}

fix16_t asin(fix16_t x)
{
	if((x > FIX16_ONE)
		|| (x < -FIX16_ONE))
		return 0;

	fix16_t out;
	out = (FIX16_ONE - fix16_mul(x, x));
	out = fix16_div(x, fix16_sqrt(out));
	out = atan(out);
	return out;
}

fix16_t acos(fix16_t x)
{
	return ((PLATFORM_PI >> 1) - asin(x));
}

fix16_t fmod(fix16_t x, fix16_t y) {
	while (fix16_min(x, y) == y) x-=y;
  return x;
}

fix16_t atan2(fix16_t inY , fix16_t inX)
{
	fix16_t abs_inY, mask, angle, r, r_3;

	/* Absolute inY */
	mask = (inY >> (sizeof(fix16_t)*CHAR_BIT-1));
	abs_inY = (inY + mask) ^ mask;

	if (inX >= 0)
	{
		r = fix16_div( (inX - abs_inY), (inX + abs_inY));
		r_3 = fix16_mul(fix16_mul(r, r),r);
		angle = fix16_mul(0x00003240 , r_3) - fix16_mul(0x0000FB50,r) + PLATFORM_PI_4;
	} else {
		r = fix16_div( (inX + abs_inY), (abs_inY - inX));
		r_3 = fix16_mul(fix16_mul(r, r),r);
		angle = fix16_mul(0x00003240 , r_3)
			- fix16_mul(0x0000FB50,r)
			+ THREE_PI_DIV_4;
	}
	if (inY < 0)
	{
		angle = -angle;
	}

	return angle;
}

fix16_t atan(fix16_t x)
{
	return atan2(x, FIX16_ONE);
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
