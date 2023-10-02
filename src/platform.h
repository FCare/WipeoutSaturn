#ifndef PLATFORM_H
#define PLATFORM_H

#include "types.h"

void platform_exit(void);
vec2i_t platform_screen_size(void);
fix16_t platform_now(void);
bool platform_get_fullscreen(void);
void platform_set_fullscreen(bool fullscreen);
void platform_set_audio_mix_cb(void (*cb)(fix16_t *buffer, uint32_t len));

uint8_t *platform_load_asset(const char *name, uint32_t *bytes_read);
uint8_t *platform_load_userdata(const char *name, uint32_t *bytes_read);
uint32_t platform_store_userdata(const char *name, void *bytes, int32_t len);
uint16_t render_texture_create_555(uint32_t width, uint32_t height, rgb1555_t *pixels);
uint8_t *platform_load_saturn_asset(const char *name);

#if defined(RENDERER_SOFTWARE)
	rgba_t *platform_get_screenbuffer(int32_t *pitch);
#endif

#endif
