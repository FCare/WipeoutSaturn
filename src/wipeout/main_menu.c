#include "../utils.h"
#include "../system.h"
#include "../mem.h"
#include "../platform.h"
#include "../input.h"

#include "menu.h"
#include "main_menu.h"
#include "game.h"
#include "image.h"
#include "ui.h"

#ifdef __EMSCRIPTEN__
#undef NO_QUIT
#endif

static void page_main_init(menu_t *menu);
static void page_options_init(menu_t *menu);
static void page_race_class_init(menu_t *menu);
static void page_race_type_init(menu_t *menu);
static void page_team_init(menu_t *menu);
static void page_pilot_init(menu_t *menu);
static void page_circut_init(menu_t *menu);

#ifndef SATURN
static void page_options_controls_init(menu_t *menu);
#endif
static void page_options_video_init(menu_t *menu);
static void page_options_audio_init(menu_t *menu);

static uint16_t background;
static texture_list_t track_images;
static menu_t *main_menu;

static struct {
	Object_Saturn *race_classes[2];
	Object_Saturn *teams[4];
	Object_Saturn *pilots[8];
	struct { Object_Saturn *stopwatch, *save, *load, *headphones, *video; } options;
	struct { Object_Saturn *championship, *msdos, *single_race, *options; } misc;
	Object_Saturn *rescue;
	Object_Saturn *controller;
} models;


static void draw_saturn_model(Object_Saturn *model, vec2_t offset, vec3_t pos, fix16_t rotation, light_t* lights, uint8_t nbLights) {
	render_set_view(vec3_fix16(FIX16_ZERO,FIX16_ZERO,FIX16_ZERO), vec3_fix16(FIX16_ZERO, -PLATFORM_PI, -PLATFORM_PI));
	render_set_screen_position(offset);
	mat4_t transform = mat4_identity();
	mat4_set_yaw_pitch_roll(&transform, vec3_fix16(FIX16_ZERO, rotation, PLATFORM_PI));
	mat4_t translate = mat4_identity();
	mat4_set_translation(&translate, pos);
	applyTransform(&translate, &transform);
	object_saturn_draw(model, &transform, lights, nbLights);
	render_set_screen_position(vec2_fix16(FIX16_ZERO, FIX16_ZERO));
}

// -----------------------------------------------------------------------------
// Main Menu

static void button_start_game(menu_t *menu, int data __unused) {
	page_race_class_init(menu);
}

static void button_options(menu_t *menu, int data __unused) {
	page_options_init(menu);
}

#ifndef NO_QUIT
static void button_quit_confirm(menu_t *menu, int data) {
	if (data) {
		system_exit();
	}
	else {
		menu_pop(menu);
	}
}

static void button_quit(menu_t *menu, int data __unused) {
	menu_confirm(menu, "ARE YOU SURE YOU", "WANT TO QUIT", "YES", "NO", button_quit_confirm);
}
#endif

static void page_main_draw(menu_t *menu __unused, int data) {
	switch (data) {
		case 0: draw_saturn_model(g.ships[0].model, vec2(0, -0.1), vec3(0, 0, -700), system_cycle_time(), NULL, 0); break;
		case 1: draw_saturn_model(models.misc.options, vec2(0, -0.2), vec3(0, 0, -700), system_cycle_time(), NULL, 0); break;
		case 2: draw_saturn_model(models.misc.msdos, vec2(0, -0.2), vec3(0, 0, -700), system_cycle_time(), NULL, 0); break;
	}
}

static void page_main_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "OPTIONS", page_main_draw);
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, -110);
	page->items_anchor = UI_POS_BOTTOM | UI_POS_CENTER;

	menu_page_add_button(page, 0, "START GAME", button_start_game);
	menu_page_add_button(page, 1, "OPTIONS", button_options);

	#ifndef NO_QUIT
		menu_page_add_button(page, 2, "QUIT", button_quit);
	#endif
}



// -----------------------------------------------------------------------------
// Options

#ifndef SATURN
static void button_controls(menu_t *menu, int data __unused) {
	page_options_controls_init(menu);
}
#endif

static void button_video(menu_t *menu, int data __unused) {
	page_options_video_init(menu);
}

static void button_audio(menu_t *menu, int data __unused) {
	page_options_audio_init(menu);
}

static void page_options_draw(menu_t *menu __unused, int data) {
	switch (data) {
		case 0: draw_saturn_model(models.options.video, vec2(0, -0.2), vec3(0, 0, -700), system_cycle_time(), NULL, 0); break; // TODO: needs better model
		case 1: draw_saturn_model(models.options.headphones, vec2(0, -0.2), vec3(0, 0, -300), system_cycle_time(), NULL, 0); break;
		case 2: draw_saturn_model(models.controller, vec2(0, -0.1), vec3(0, 0, -6000), system_cycle_time(), NULL, 0); break;
	}
}

static void page_options_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "OPTIONS", page_options_draw);
	int button_nb = 0;
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, -110);
	page->items_anchor = UI_POS_BOTTOM | UI_POS_CENTER;
	menu_page_add_button(page, button_nb++, "VIDEO", button_video);
	menu_page_add_button(page, button_nb++, "AUDIO", button_audio);
#ifndef SATURN
	menu_page_add_button(page, button_nb++, "CONTROLS", button_controls);
#endif
}


// -----------------------------------------------------------------------------
// Options Controls

static const char *button_names[NUM_GAME_ACTIONS][2] = {};
static int control_current_action;
static fix16_t await_input_deadline;

void button_capture(void *user, button_t button, int32_t ascii_char __unused) {
	if (button == INPUT_INVALID) {
		return;
	}

	menu_t *menu = (menu_t *)user;
	if (button == INPUT_KEY_ESCAPE) {
		input_capture(NULL, NULL);
		menu_pop(menu);
		return;
	}

	int index = button < INPUT_KEY_MAX ? 0 : 1; // joypad or keyboard

	// unbind this button if it's bound anywhere
	for (uint32_t i = 0; i < len(save.buttons); i++) {
		if (save.buttons[i][index] == button) {
			save.buttons[i][index] = INPUT_INVALID;
		}
	}
	input_capture(NULL, NULL);
	input_bind(INPUT_LAYER_USER, button, control_current_action);
	save.buttons[control_current_action][index] = button;
	save.is_dirty = true;
	menu_pop(menu);
}

#ifndef SATURN
static void page_options_control_set_draw(menu_t *menu, int data __unused) {
	fix16_t remaining = await_input_deadline - platform_now();

	menu_page_t *page = &menu->pages[menu->index];
	char remaining_text[2] = { '0' + (uint8_t)clamp(remaining + 1, 0, 3), '\0'};
	vec2i_t pos = vec2i(page->items_pos.x, page->items_pos.y + 24);
	ui_draw_text_centered(remaining_text, ui_scaled_pos(page->items_anchor, pos), UI_SIZE_16, UI_COLOR_DEFAULT);

	if (remaining <= 0) {
		input_capture(NULL, NULL);
		menu_pop(menu);
		return;
	}
}

static void page_options_controls_set_init(menu_t *menu, int data) {
	control_current_action = data;
	await_input_deadline = platform_now() + 3;

	menu_page_t *page = menu_push(menu, "AWAITING INPUT", page_options_control_set_draw);
	input_capture(button_capture, menu);
}

static void page_options_control_draw(menu_t *menu, int data __unused) {
	menu_page_t *page = &menu->pages[menu->index];

	int left = page->items_pos.x + page->block_width - 100;
	int right = page->items_pos.x + page->block_width;
	int line_y = page->items_pos.y - 20;

	vec2i_t left_head_pos = vec2i(left - ui_text_width("KEYBOARD", UI_SIZE_8), line_y);
	ui_draw_text("KEYBOARD", ui_scaled_pos(page->items_anchor, left_head_pos), UI_SIZE_8, UI_COLOR_DEFAULT);

	vec2i_t right_head_pos = vec2i(right - ui_text_width("JOYSTICK", UI_SIZE_8), line_y);
	ui_draw_text("JOYSTICK", ui_scaled_pos(page->items_anchor, right_head_pos), UI_SIZE_8, UI_COLOR_DEFAULT);
	line_y += 20;

	for (int action = 0; action < NUM_GAME_ACTIONS; action++) {
		rgba_t text_color = UI_COLOR_DEFAULT;
		if (action == data) {
			text_color = UI_COLOR_ACCENT;
		}

		if (save.buttons[action][0] != INPUT_INVALID) {
			const char *name = input_button_to_name(save.buttons[action][0]);
			if (!name) {
				name = "UNKNWN";
			}
			vec2i_t pos = vec2i(left - ui_text_width(name, UI_SIZE_8), line_y);
			ui_draw_text(name, ui_scaled_pos(page->items_anchor, pos), UI_SIZE_8, text_color);
		}
		if (save.buttons[action][1] != INPUT_INVALID) {
			const char *name = input_button_to_name(save.buttons[action][1]);
			if (!name) {
				name = "UNKNWN";
			}
			vec2i_t pos = vec2i(right - ui_text_width(name, UI_SIZE_8), line_y);
			ui_draw_text(name, ui_scaled_pos(page->items_anchor, pos), UI_SIZE_8, text_color);
		}
		line_y += 12;
	}
}

static void page_options_controls_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "CONTROLS", page_options_control_draw);
	flags_set(page->layout_flags, MENU_VERTICAL | MENU_FIXED);
	page->title_pos = vec2i(-160, -100);
	page->title_anchor = UI_POS_MIDDLE | UI_POS_CENTER;
	page->items_pos = vec2i(-160, -50);
	page->block_width = 320;
	page->items_anchor = UI_POS_MIDDLE | UI_POS_CENTER;

	// const char *thrust_name = button_name(A_THRUST);
	// LOGD("thrust: %s\n", thrust_name);
	menu_page_add_button(page, A_UP, "UP", page_options_controls_set_init);
	menu_page_add_button(page, A_DOWN, "DOWN", page_options_controls_set_init);
	menu_page_add_button(page, A_LEFT, "LEFT", page_options_controls_set_init);
	menu_page_add_button(page, A_RIGHT, "RIGHT", page_options_controls_set_init);
	menu_page_add_button(page, A_BRAKE_LEFT, "BRAKE L", page_options_controls_set_init);
	menu_page_add_button(page, A_BRAKE_RIGHT, "BRAKE R", page_options_controls_set_init);
	menu_page_add_button(page, A_THRUST, "THRUST", page_options_controls_set_init);
	menu_page_add_button(page, A_FIRE, "FIRE", page_options_controls_set_init);
	menu_page_add_button(page, A_CHANGE_VIEW, "VIEW", page_options_controls_set_init);
}
#endif

// -----------------------------------------------------------------------------
// Options Video

static void toggle_show_fps(menu_t *menu __unused, int data) {
	save.show_fps = data;
	save.is_dirty = true;
}

#ifndef SATURN
static void toggle_fullscreen(menu_t *menu __unused, int data __unused) {
	save.fullscreen = data;
	save.is_dirty = true;
	platform_set_fullscreen(save.fullscreen);
}

static void toggle_ui_scale(menu_t *menu __unused, int data) {
	save.ui_scale = data;
	save.is_dirty = true;
}

static void toggle_res(menu_t *menu __unused, int data) {
	render_set_resolution(data);
	save.screen_res = data;
	save.is_dirty = true;
}
static void toggle_post(menu_t *menu __unused, int data) {
	render_set_post_effect(data);
	save.post_effect = data;
	save.is_dirty = true;
}
#endif

static const char *opts_off_on[] = {"OFF", "ON"};
static const char *opts_ui_sizes[] = {"AUTO", "1X", "2X", "3X", "4X"};
static const char *opts_res[] = {"NATIVE", "240P", "480P"};
static const char *opts_post[] = {"NONE", "CRT EFFECT"};

static void page_options_video_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "VIDEO", NULL);
	flags_set(page->layout_flags, MENU_VERTICAL | MENU_FIXED);
	page->title_pos = vec2i(-160, -100);
	page->title_anchor = UI_POS_MIDDLE | UI_POS_CENTER;
	page->items_pos = vec2i(-160, -60);
	page->block_width = 320;
	page->items_anchor = UI_POS_MIDDLE | UI_POS_CENTER;

#ifndef SATURN
	#ifndef __EMSCRIPTEN__
		menu_page_add_toggle(page, save.fullscreen, "FULLSCREEN", opts_off_on, len(opts_off_on), toggle_fullscreen);
	#endif
	menu_page_add_toggle(page, save.ui_scale, "UI SCALE", opts_ui_sizes, len(opts_ui_sizes), toggle_ui_scale);
	menu_page_add_toggle(page, save.post_effect, "POST PROCESSING", opts_post, len(opts_post), toggle_post);
	menu_page_add_toggle(page, save.screen_res, "SCREEN RESOLUTION", opts_res, len(opts_res), toggle_res);
#endif
	menu_page_add_toggle(page, save.show_fps, "SHOW FPS", opts_off_on, len(opts_off_on), toggle_show_fps);
}

// -----------------------------------------------------------------------------
// Options Audio

static void toggle_music_volume(menu_t *menu __unused, int data) {
	save.music_volume = (fix16_t)data * 0.1;
	save.is_dirty = true;
}

static void toggle_sfx_volume(menu_t *menu __unused, int data) {
	save.sfx_volume = (fix16_t)data * 0.1;
	save.is_dirty = true;
}

static const char *opts_volume[] = {"0", "10", "20", "30", "40", "50", "60", "70", "80", "90", "100"};

static void page_options_audio_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "AUDIO", NULL);

	flags_set(page->layout_flags, MENU_VERTICAL | MENU_FIXED);
	page->title_pos = vec2i(-160, -100);
	page->title_anchor = UI_POS_MIDDLE | UI_POS_CENTER;
	page->items_pos = vec2i(-160, -80);
	page->block_width = 320;
	page->items_anchor = UI_POS_MIDDLE | UI_POS_CENTER;

	menu_page_add_toggle(page, save.music_volume * 10, "MUSIC VOLUME", opts_volume, len(opts_volume), toggle_music_volume);
	menu_page_add_toggle(page, save.sfx_volume * 10, "SFX VOLUME", opts_volume, len(opts_volume), toggle_sfx_volume);
}








// -----------------------------------------------------------------------------
// Racing class

static void button_race_class_select(menu_t *menu, int data) {
	if (!save.has_rapier_class && data == RACE_CLASS_RAPIER) {
		return;
	}
	g.race_class = data;
	page_race_type_init(menu);
}

static void page_race_class_draw(menu_t *menu, int data) {
	menu_page_t *page = &menu->pages[menu->index];
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, -110);
	page->items_anchor = UI_POS_BOTTOM | UI_POS_CENTER;
	draw_saturn_model(models.race_classes[data], vec2(0, -0.2), vec3(0, 0, -350), system_cycle_time(), NULL, 0);

	if (!save.has_rapier_class && data == RACE_CLASS_RAPIER) {
		render_set_view_2d();
		vec2i_t pos = vec2i(page->items_pos.x, page->items_pos.y + 32);
		ui_draw_text_centered("NOT AVAILABLE", ui_scaled_pos(page->items_anchor, pos), UI_SIZE_12, UI_COLOR_ACCENT);
	}
}

static void page_race_class_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "RACING CLASS", page_race_class_draw);
	for (uint32_t i = 0; i < len(def.race_classes); i++) {
		menu_page_add_button(page, i, def.race_classes[i].name, button_race_class_select);
	}
}



// -----------------------------------------------------------------------------
// Race Type

static void button_race_type_select(menu_t *menu, int data) {
	g.race_type = data;
	g.highscore_tab = g.race_type == RACE_TYPE_TIME_TRIAL ? HIGHSCORE_TAB_TIME_TRIAL : HIGHSCORE_TAB_RACE;
	page_team_init(menu);
}

static void page_race_type_draw(menu_t *menu __unused, int data) {
	switch (data) {
		case 0: draw_saturn_model(models.misc.championship, vec2(0, -0.2), vec3(0, 0, -400), system_cycle_time(), NULL, 0); break;
		case 1: draw_saturn_model(models.misc.single_race, vec2(0, -0.2), vec3(0, 0, -400), system_cycle_time(), NULL, 0); break;
		case 2: draw_saturn_model(models.options.stopwatch, vec2(0, -0.2), vec3(0, 0, -400), system_cycle_time(), NULL, 0); break;
	}
}

static void page_race_type_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "SELECT RACE TYPE", page_race_type_draw);
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, -110);
	page->items_anchor = UI_POS_BOTTOM | UI_POS_CENTER;
	for (uint32_t i = 0; i < len(def.race_types); i++) {
		menu_page_add_button(page, i, def.race_types[i].name, button_race_type_select);
	}
}



// -----------------------------------------------------------------------------
// Team

static void button_team_select(menu_t *menu, int data) {
	g.team = data;
	page_pilot_init(menu);
}

static void page_team_draw(menu_t *menu __unused, int data) {
	draw_saturn_model(models.teams[data], vec2(0, -0.2), vec3(0, 0, -10000), system_cycle_time(), NULL, 0);
	draw_saturn_model(g.ships[def.teams[data].pilots[0]].model, vec2(0, -0.3), vec3(1, 0, -1300), fix16_mul(system_cycle_time(), FIX16(1.1)), NULL, 0);
	draw_saturn_model(g.ships[def.teams[data].pilots[1]].model, vec2(0, -0.3), vec3( -1, 0, -1300), fix16_mul(system_cycle_time(), FIX16(1.2)), NULL, 0);
}

static void page_team_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "SELECT YOUR TEAM", page_team_draw);
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, -110);
	page->items_anchor = UI_POS_BOTTOM | UI_POS_CENTER;
	for (uint32_t i = 0; i < len(def.teams); i++) {
		menu_page_add_button(page, i, def.teams[i].name, button_team_select);
	}
}



// -----------------------------------------------------------------------------
// Pilot

static void button_pilot_select(menu_t *menu, int data) {
	g.pilot = data;
	if (g.race_type != RACE_TYPE_CHAMPIONSHIP) {
		page_circut_init(menu);
	}
	else {
		g.circut = 0;
		game_reset_championship();
		game_set_scene(GAME_SCENE_RACE);
	}
}

static void page_pilot_draw(menu_t *menu __unused, int data) {
	draw_saturn_model(models.pilots[data], vec2(0, -0.2), vec3(0, 0, -10000), system_cycle_time(), NULL, 0);
}

static void page_pilot_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "CHOOSE YOUR PILOT", page_pilot_draw);
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, -110);
	page->items_anchor = UI_POS_BOTTOM | UI_POS_CENTER;
	for (uint32_t i = 0; i < len(def.teams[g.team].pilots); i++) {
		menu_page_add_button(page, def.teams[g.team].pilots[i], def.pilots[def.teams[g.team].pilots[i]].name, button_pilot_select);
	}
}


// -----------------------------------------------------------------------------
// Circut

static void button_circut_select(menu_t *menu __unused, int data) {
	g.circut = data;
	game_set_scene(GAME_SCENE_RACE);
}

static void page_circut_draw(menu_t *menu __unused, int data) {
	vec2i_t pos = vec2i(0, -25);
	vec2i_t size = vec2i(128, 74);
	vec2i_t scaled_size = ui_scaled(size);
	vec2i_t scaled_pos = ui_scaled_pos(UI_POS_MIDDLE | UI_POS_CENTER, vec2i(pos.x - size.x/2, pos.y - size.y/2));
	render_push_2d(scaled_pos, scaled_size, rgba(128, 128, 128, 255), texture_from_list(track_images, data));
}

static void page_circut_init(menu_t *menu) {
	menu_page_t *page = menu_push(menu, "SELECT RACING CIRCUT", page_circut_draw);
	flags_add(page->layout_flags, MENU_FIXED);
	page->title_pos = vec2i(0, 30);
	page->title_anchor = UI_POS_TOP | UI_POS_CENTER;
	page->items_pos = vec2i(0, -100);
	page->items_anchor = UI_POS_BOTTOM | UI_POS_CENTER;
	for (uint32_t i = 0; i < len(def.circuts); i++) {
		if (!def.circuts[i].is_bonus_circut || save.has_bonus_circuts) {
			menu_page_add_button(page, i, def.circuts[i].name, button_circut_select);
		}
	}
}

#define objects_unpack_saturn(DEST, SRC) \
	objects_unpack_saturn_imp((Object_Saturn **)&DEST,  SRC)

static void objects_unpack_saturn_imp(Object_Saturn **dest_array, Object_Saturn_list *src) {
	for (int i = 0; src && i < src->length; i++) {
		dest_array[i] = src->objects[i];
	}
}


void main_menu_init(void) {

	g.is_attract_mode = false;

	ships_reset_exhaust_plumes();

	main_menu = mem_bump(sizeof(menu_t));

//See if it can be optimized
	background = image_get_texture("wipeout/textures/wipeout1.tim");
	// track_images = image_get_compressed_textures("wipeout/textures/track.cmp");

	models.misc.options = object_saturn_load("wipeout/common/options.smf");
	models.controller  = object_saturn_load("wipeout/common/pad1.smf");
	models.options.headphones = object_saturn_load("wipeout/common/hp.smf");
	models.options.video = object_saturn_load("wipeout/common/video.smf");
	for (int i = 0; i<2; i++)
	{
		models.race_classes[i] = object_saturn_load(def.race_classes[i].model);
	}
	models.misc.championship = object_saturn_load("wipeout/common/champion.smf");
	models.misc.single_race = object_saturn_load("wipeout/common/single.smf");
	models.options.stopwatch = object_saturn_load("wipeout/common/watch.smf");

	// objects_unpack_saturn(models.race_classes, objects_saturn_load("wipeout/common/leeg.smf"));
	// objects_unpack_saturn(models.misc, objects_saturn_load("wipeout/common/msdos.smf"));
//To be converted
	for (int i = 0; i<NUM_TEAMS; i++)
	{
		models.teams[i] = object_saturn_load(def.teams[i].model);
	}
	// objects_unpack_saturn(models.pilots, objects_saturn_load("wipeout/common/pilot.smf"));
	// objects_unpack_saturn(models.options, objects_saturn_load("wipeout/common/alopt.smf"));

	menu_reset(main_menu);
	page_main_init(main_menu);
}

void main_menu_update(void) {
	render_set_view_2d();
	render_push_2d(vec2i(0, 0), render_size(), rgba(128, 128, 128, 255), background);
	menu_update(main_menu);
}

