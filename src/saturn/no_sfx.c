#include "../utils.h"
#include "../mem.h"
#include "../platform.h"

#include "../wipeout/sfx.h"


void sfx_load(void) {
}

void sfx_reset(void) {
}

void sfx_unpause(void) {
}

void sfx_pause(void) {
}

// Sound effects
static sfx_t no_sfx;
sfx_t *sfx_get_node(sfx_source_t source_index __unused) {
	return &no_sfx;
}

sfx_t *sfx_play(sfx_source_t source_index __unused) {
	return &no_sfx;
}

sfx_t *sfx_play_at(sfx_source_t source_index __unused, vec3_t pos __unused, vec3_t vel __unused, fix16_t volume __unused) {
	return &no_sfx;
}

sfx_t *sfx_reserve_loop(sfx_source_t source_index __unused) {
	return &no_sfx;
}

void sfx_set_position(sfx_t *sfx __unused, vec3_t pos __unused, vec3_t vel __unused, fix16_t volume __unused) {
}

// Music

uint32_t sfx_music_decode_frame(void) {
	return 0;
}

void sfx_music_rewind(void) {
}

void sfx_music_open(char *path __unused) {
}

void sfx_music_play(uint32_t index __unused) {
}

void sfx_music_mode(sfx_music_mode_t mode __unused) {
}


// Mixing

void sfx_set_external_mix_cb(void (*cb)(fix16_t *, uint32_t len) __unused) {
}

void sfx_stero_mix(fix16_t *buffer __unused, uint32_t len __unused) {
}
