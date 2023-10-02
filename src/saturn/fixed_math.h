#ifndef __FIXED_MATH_H__
#define __FIXED_MATH_H__

#include <yaul.h>
#include "fix16_mat44.h"

#define PLATFORM_2PI      FIX16_2PI /* pi */
#define PLATFORM_PI       FIX16_PI /* pi */
#define PLATFORM_PI_2     FIX16_PI_2 /* pi/2 */
#define PLATFORM_PI_4     FIX16_PI_4 /* pi/4 */

extern fix16_t sqrt(fix16_t x);
extern fix16_t sin(fix16_t x);
extern fix16_t cos(fix16_t x);
extern fix16_t tan(fix16_t x);
extern fix16_t acos(fix16_t x);
extern fix16_t fmod(fix16_t x, fix16_t y);
extern fix16_t atan2(fix16_t x, fix16_t y);
extern fix16_t fabsf(fix16_t x);
extern fix16_t floor(fix16_t x);
extern fix16_t ceil(fix16_t x);

#undef min
#undef max
#undef near
#undef far
#undef clamp

#endif