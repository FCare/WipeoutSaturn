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

sfx_t *sfx_get_node(sfx_source_t source_index) {
	return NULL;
}

sfx_t *sfx_play(sfx_source_t source_index) {
	return NULL;
}

sfx_t *sfx_play_at(sfx_source_t source_index, vec3_t pos, vec3_t vel, float volume) {
	return NULL;
}

sfx_t *sfx_reserve_loop(sfx_source_t source_index) {
	return NULL;
}

void sfx_set_position(sfx_t *sfx, vec3_t pos, vec3_t vel, float volume) {
}

// Music

uint32_t sfx_music_decode_frame(void) {
	return 0;
}

void sfx_music_rewind(void) {
}

void sfx_music_open(char *path) {
}

void sfx_music_play(uint32_t index) {
}

void sfx_music_mode(sfx_music_mode_t mode) {
}


// Mixing

void sfx_set_external_mix_cb(void (*cb)(float *, uint32_t len)) {
}

void sfx_stero_mix(float *buffer, uint32_t len) {
}
