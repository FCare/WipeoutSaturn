#ifndef __FIXED_MATH_H__
#define __FIXED_MATH_H__

#define M_PI       (3.14159265358979323846f) /* pi */
#define M_PI_2     (1.57079632679489661923f) /* pi/2 */
#define M_PI_4     (0.78539816339744830962f) /* pi/4 */

extern float sqrt(float x);
extern float sin(float x);
extern float cos(float x);
extern float acos(float x);
extern float fmod(float x, float y);
extern float atan2(float x, float y);
extern float fabsf(float x);
extern float floor(float x);
extern float ceil(float x);

#undef min
#undef max
#undef near
#undef far
#undef clamp

#endif