#include "fixed_math.h"

#include <limits.h>
#include <gamemath/fix16.h>

static const fix16_t THREE_PI_DIV_4 = 0x00025B2F;       /*!< Fix16 value of 3PI/4 */

#define TABLE_SIZE 201
#define PRECISION_BITS 9
static const fix16_t sin_lut[TABLE_SIZE+1]={
0x00000000,0x00000200,0x00000400,0x00000600,0x00000800,0x000009ff,0x00000bff,0x00000dfe,
0x00000ffd,0x000011fc,0x000013fb,0x000015f9,0x000017f7,0x000019f5,0x00001bf2,0x00001dee,
0x00001feb,0x000021e6,0x000023e2,0x000025dc,0x000027d6,0x000029d0,0x00002bc9,0x00002dc1,
0x00002fb8,0x000031af,0x000033a5,0x0000359a,0x0000378e,0x00003981,0x00003b74,0x00003d65,
0x00003f56,0x00004145,0x00004334,0x00004522,0x0000470e,0x000048f9,0x00004ae3,0x00004ccc,
0x00004eb4,0x0000509b,0x00005280,0x00005464,0x00005647,0x00005828,0x00005a08,0x00005be7,
0x00005dc4,0x00005fa0,0x0000617a,0x00006353,0x0000652a,0x000066ff,0x000068d3,0x00006aa5,
0x00006c76,0x00006e45,0x00007012,0x000071de,0x000073a7,0x0000756f,0x00007735,0x000078f9,
0x00007abc,0x00007c7c,0x00007e3a,0x00007ff7,0x000081b1,0x0000836a,0x00008520,0x000086d4,
0x00008887,0x00008a37,0x00008be4,0x00008d90,0x00008f3a,0x000090e1,0x00009286,0x00009429,
0x000095c9,0x00009767,0x00009903,0x00009a9c,0x00009c33,0x00009dc7,0x00009f59,0x0000a0e9,
0x0000a276,0x0000a400,0x0000a588,0x0000a70d,0x0000a890,0x0000aa10,0x0000ab8d,0x0000ad08,
0x0000ae80,0x0000aff5,0x0000b168,0x0000b2d7,0x0000b444,0x0000b5ae,0x0000b716,0x0000b87a,
0x0000b9dc,0x0000bb3a,0x0000bc96,0x0000bdef,0x0000bf45,0x0000c097,0x0000c1e7,0x0000c334,
0x0000c47e,0x0000c5c4,0x0000c708,0x0000c848,0x0000c986,0x0000cac0,0x0000cbf7,0x0000cd2b,
0x0000ce5b,0x0000cf89,0x0000d0b3,0x0000d1da,0x0000d2fd,0x0000d41e,0x0000d53b,0x0000d654,
0x0000d76b,0x0000d87e,0x0000d98d,0x0000da99,0x0000dba2,0x0000dca7,0x0000dda9,0x0000dea8,
0x0000dfa3,0x0000e09a,0x0000e18e,0x0000e27e,0x0000e36b,0x0000e455,0x0000e53a,0x0000e61c,
0x0000e6fb,0x0000e7d6,0x0000e8ad,0x0000e981,0x0000ea51,0x0000eb1d,0x0000ebe6,0x0000ecab,
0x0000ed6d,0x0000ee2a,0x0000eee4,0x0000ef9a,0x0000f04d,0x0000f0fb,0x0000f1a6,0x0000f24d,
0x0000f2f1,0x0000f390,0x0000f42c,0x0000f4c4,0x0000f558,0x0000f5e8,0x0000f675,0x0000f6fd,
0x0000f782,0x0000f803,0x0000f880,0x0000f8f9,0x0000f96e,0x0000f9df,0x0000fa4d,0x0000fab6,
0x0000fb1c,0x0000fb7d,0x0000fbdb,0x0000fc35,0x0000fc8b,0x0000fcdd,0x0000fd2b,0x0000fd75,
0x0000fdbb,0x0000fdfd,0x0000fe3b,0x0000fe75,0x0000feab,0x0000fedd,0x0000ff0b,0x0000ff36,
0x0000ff5c,0x0000ff7e,0x0000ff9c,0x0000ffb7,0x0000ffcd,0x0000ffdf,0x0000ffed,0x0000fff7,
0x0000fffe,0x00010000
};

static const fix16_t fix16_maximum  = 0x7FFFFFFF; /*!< the maximum value of fix16_t */
static const fix16_t fix16_minimum  = 0x80000000; /*!< the minimum value of fix16_t */
static const fix16_t fix16_overflow = 0x80000000; /*!< the value used to indicate overflows when FIXMATH_NO_OVERFLOW is not specified */

/* Wrapper around fix16_div to add saturating arithmetic. */
fix16_t fix16_sdiv(fix16_t inArg0, fix16_t inArg1)
{
	fix16_t result = fix16_div(inArg0, inArg1);

	if (result == fix16_overflow)
	{
		if ((inArg0 >= 0) == (inArg1 >= 0))
			return fix16_maximum;
		else
			return fix16_minimum;
	}

	return result;
}

fix16_t sqrt(fix16_t x) {
  return (fix16_t)fix16_sqrt((fix16_t)x);
}

fix16_t sin(fix16_t phase)
{
	while(phase < FIX16_ZERO) {
		phase += PLATFORM_2PI;
	}
  int quadrant = (phase/PLATFORM_PI_2)%4;
  int index = 0;
  switch(quadrant){
    case 0:
      index = (phase%PLATFORM_PI_2)>>PRECISION_BITS;
      return sin_lut[index];
    case 1:
      index = TABLE_SIZE-((phase%PLATFORM_PI_2)>>PRECISION_BITS);
      return sin_lut[index];
    case 2:
      index = (phase%PLATFORM_PI_2)>>PRECISION_BITS;
      return -sin_lut[index];
      break;
    case 3:
      index = TABLE_SIZE-((phase%PLATFORM_PI_2)>>PRECISION_BITS);
      return -sin_lut[index];
      break;
    default:
      break;
  }
  return 0;
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
