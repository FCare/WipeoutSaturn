#ifndef __FIXED_MATH_H__
#define __FIXED_MATH_H__

#include <yaul.h>
#include "fix16_mat44.h"

#define PLATFORM_2PI      0x0006487F /* pi */
#define PLATFORM_PI       0x0003243F /* pi */
#define PLATFORM_PI_2     0x00019220 /* pi/2 */
#define PLATFORM_PI_4     0x0000C90F /* pi/4 */

extern fix16_t sqrt(fix16_t x);
extern fix16_t sin(fix16_t x);
extern fix16_t cos(fix16_t x);
extern fix16_t tan(fix16_t x);
extern fix16_t acos(fix16_t x);
extern fix16_t fmod(fix16_t x, fix16_t y);
extern fix16_t atan(fix16_t x);
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