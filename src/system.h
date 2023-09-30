#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

#define SYSTEM_WINDOW_NAME "wipEout"
#define SYSTEM_WINDOW_WIDTH 1280
#define SYSTEM_WINDOW_HEIGHT 720

void system_init(void);
void system_update(void);
void system_cleanup(void);
void system_exit(void);
void system_resize(vec2i_t size);

fix16_t system_time(void);
fix16_t system_tick(void);
fix16_t system_cycle_time(void);
void system_reset_cycle_time(void);
fix16_t system_time_scale_get(void);
void system_time_scale_set(fix16_t ts);

#endif
