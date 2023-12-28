#include "system.h"
#include "input.h"
#include "render.h"
#include "platform.h"
#include "mem.h"
#include "utils.h"

#include "wipeout/game.h"

static fix16_t time_real;
static fix16_t time_scaled;
static fix16_t time_scale = FIX16_ONE;
static fix16_t tick_last;
static fix16_t cycle_time = 0;
static fix16_t pi_3600 = 0;

void system_init(void) {
	pi_3600 = fix16_int16_mul(PLATFORM_PI,3600);
	time_real = platform_now();
	input_init();
	render_init(platform_screen_size());
	game_init();
}

void system_cleanup(void) {
	render_cleanup();
	input_cleanup();
}

void system_exit(void) {
	platform_exit();
}

void system_update(void) {
	fix16_t time_real_now = platform_now();
	fix16_t real_delta = time_real_now - time_real;
	time_real = time_real_now;
	tick_last = fix16_mul(fix16_min(real_delta, FIX16(0.1)), time_scale);

	time_scaled += tick_last;

	// FIXME: come up with a better way to wrap the cycle_time, so that it
	// doesn't lose precission, but also doesn't jump upon reset.
	cycle_time = time_scaled;
	if (cycle_time > pi_3600) {
		cycle_time -= pi_3600;
	}

	render_frame_prepare();

	game_update();
printf("%d\n", __LINE__);
	render_frame_end();
	printf("%d\n", __LINE__);
	input_clear();
	printf("%d\n", __LINE__);
	mem_temp_check();
	printf("%d\n", __LINE__);
}

void system_reset_cycle_time(void) {
	cycle_time = 0;
}

void system_resize(vec2i_t size) {
	render_set_screen_size(size);
}

fix16_t system_time_scale_get(void) {
	return time_scale;
}

void system_time_scale_set(fix16_t scale) {
	time_scale = scale;
}

fix16_t system_tick(void) {
	return tick_last;
}

fix16_t system_time(void) {
	return time_scaled;
}

fix16_t system_cycle_time(void) {
	return cycle_time;
}
