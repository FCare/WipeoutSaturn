#include "../render.h"
#include "../utils.h"

#include "mem.h"
#include "platform.h"

#include "ui.h"
#include "image.h"

int ui_scale = 2;

typedef struct {
	saturn_font_t *image;
	uint16_t *tex;
} char_set_t;

char_set_t char_set[UI_SIZE_MAX];

static const char letter[38] = {
  'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
  '0','1','2','3','4','5','6','7','8','9',':','.'
};

static uint16_t getTexIndex(char c) {
	switch(c) {
		case 'A': return 0;
		case 'B': return 1;
		case 'C': return 2;
		case 'D': return 3;
		case 'E': return 4;
		case 'F': return 5;
		case 'G': return 6;
		case 'H': return 7;
		case 'I': return 8;
		case 'J': return 9;
		case 'K': return 10;
		case 'L': return 11;
		case 'M': return 12;
		case 'N': return 13;
		case 'O': return 14;
		case 'P': return 15;
		case 'Q': return 16;
		case 'R': return 17;
		case 'S': return 18;
		case 'T': return 19;
		case 'U': return 20;
		case 'V': return 21;
		case 'W': return 22;
		case 'X': return 23;
		case 'Y': return 24;
		case 'Z': return 25;
		case '0': return 26;
		case '1': return 27;
		case '2': return 28;
		case '3': return 29;
		case '4': return 30;
		case '5': return 31;
		case '6': return 32;
		case '7': return 33;
		case '8': return 34;
		case '9': return 35;
		case ':': return 36;
		case '.': return 37;
		default : return 0;
	}
}


uint16_t icon_textures[UI_ICON_MAX];


void ui_load(void) {
	uint16_t texture;
	char_set[UI_SIZE_16].image = (saturn_font_t*) platform_load_saturn_asset("wipeout/textures/fonts/fonts_16.stf", &texture);
	char_set[UI_SIZE_16].tex = mem_bump(sizeof(uint16_t) * char_set[UI_SIZE_16].image->nbQuads);
	for (int i =0; i<char_set[UI_SIZE_16].image->nbQuads; i++) {
		font_character_t * char_glyph = &char_set[UI_SIZE_16].image->character[i];
		char_set[UI_SIZE_16].tex[i] = create_sub_texture(char_glyph->offset/2, char_glyph->stride, char_glyph->height, texture);
	}

	char_set[UI_SIZE_12].image = (saturn_font_t*) platform_load_saturn_asset("wipeout/textures/fonts/fonts_12.stf", &texture);
	char_set[UI_SIZE_12].tex = mem_bump(sizeof(uint16_t) * char_set[UI_SIZE_12].image->nbQuads);
	for (int i =0; i<char_set[UI_SIZE_12].image->nbQuads; i++) {
		font_character_t * char_glyph = &char_set[UI_SIZE_12].image->character[i];
		char_set[UI_SIZE_12].tex[i] = create_sub_texture(char_glyph->offset/2, char_glyph->stride, char_glyph->height, texture);
	}

	char_set[UI_SIZE_8].image = (saturn_font_t*) platform_load_saturn_asset("wipeout/textures/fonts/fonts_8.stf", &texture);
	char_set[UI_SIZE_8].tex = mem_bump(sizeof(uint16_t) * char_set[UI_SIZE_8].image->nbQuads);
	for (int i =0; i<char_set[UI_SIZE_8].image->nbQuads; i++) {
		font_character_t * char_glyph = &char_set[UI_SIZE_8].image->character[i];
		char_set[UI_SIZE_8].tex[i] = create_sub_texture(char_glyph->offset/2, char_glyph->stride, char_glyph->height, texture);
	}

	// icon_textures[UI_ICON_HAND]    = texture_from_list(tl, 3);
	// icon_textures[UI_ICON_CONFIRM] = texture_from_list(tl, 5);
	// icon_textures[UI_ICON_CANCEL]  = texture_from_list(tl, 6);
	// icon_textures[UI_ICON_END]     = texture_from_list(tl, 7);
	// icon_textures[UI_ICON_DEL]     = texture_from_list(tl, 8);
	// icon_textures[UI_ICON_STAR]    = texture_from_list(tl, 9);
}

int ui_get_scale(void) {
	return ui_scale;
}

void ui_set_scale(int scale) {
	ui_scale = scale;
}


vec2i_t ui_scaled(vec2i_t v) {
	return vec2i(v.x * ui_scale, v.y * ui_scale);
}

vec2i_t ui_scaled_screen(void) {
	return vec2i_mulf(render_size(), ui_scale);
}

vec2i_t ui_scaled_pos(ui_pos_t anchor, vec2i_t offset) {
	vec2i_t pos;
	vec2i_t screen_size = render_size();

	if (flags_is(anchor, UI_POS_LEFT)) {
		pos.x = offset.x * ui_scale;
	}
	else if (flags_is(anchor, UI_POS_CENTER)) {
		pos.x = (screen_size.x >> 1) + offset.x * ui_scale;
	}
	else if (flags_is(anchor, UI_POS_RIGHT)) {
		pos.x = screen_size.x + offset.x * ui_scale;
	}

	if (flags_is(anchor, UI_POS_TOP)) {
		pos.y = offset.y * ui_scale;
	}
	else if (flags_is(anchor, UI_POS_MIDDLE)) {
		pos.y = (screen_size.y >> 1) + offset.y * ui_scale;
	}
	else if (flags_is(anchor, UI_POS_BOTTOM)) {
		pos.y = screen_size.y + offset.y * ui_scale;
	}

	return pos;
}

#define char_to_glyph_index(C) (C >= '0' && C <= '9' ? (C - '0' + 26) : C - 'A')

int ui_char_width(char c, ui_text_size_t size) {
	if (c == ' ') {
		return 8;
	}
	return char_set[size].image->character[char_to_glyph_index(c)].width;
}

int ui_text_width(const char *text, ui_text_size_t size) {
	int width = 0;
	saturn_font_t *cs = char_set[size].image;

	for (int i = 0; text[i] != 0; i++) {
		width += text[i] != ' '
			? cs->character[char_to_glyph_index(text[i])].width
			: 8;
	}

	return width;
}

int ui_number_width(int num, ui_text_size_t size) {
	char text_buffer[16];
	text_buffer[15] = '\0';

	int i;
	for (i = 14; i > 0; i--) {
		text_buffer[i] = '0' + (num % 10);
		num = num / 10;
		if (num == 0) {
			break;
		}
	}
	return ui_text_width(text_buffer + i, size);
}

void ui_draw_time(fix16_t time, vec2i_t pos, ui_text_size_t size, rgba_t color) {
	int msec = time * 1000;
	int tenths = (msec / 100) % 10;
	int secs = (msec / 1000) % 60;
	int mins = msec / (60 * 1000);

	char text_buffer[8];
	text_buffer[0] = '0' + (mins / 10) % 10;
	text_buffer[1] = '0' + mins % 10;
	text_buffer[2] = 'e'; // ":"
	text_buffer[3] = '0' + secs / 10;
	text_buffer[4] = '0' + secs % 10;
	text_buffer[5] = 'f'; // "."
	text_buffer[6] = '0' + tenths;
	text_buffer[7] = '\0';
	ui_draw_text(text_buffer, pos, size, color);
}

void ui_draw_number(int num, vec2i_t pos, ui_text_size_t size, rgba_t color) {
	char text_buffer[16];
	text_buffer[15] = '\0';

	int i;
	for (i = 14; i > 0; i--) {
		text_buffer[i] = '0' + (num % 10);
		num = num / 10;
		if (num == 0) {
			break;
		}
	}
	ui_draw_text(text_buffer + i, pos, size, color);
}

void ui_draw_text(const char *text, vec2i_t pos, ui_text_size_t size, rgba_t color) {
	char_set_t *cs = &char_set[size];

	for (int i = 0; text[i] != 0; i++) {
		if (text[i] != ' ') {
			uint16_t glyphIndex = getTexIndex(text[i]);
			font_character_t *glyph = &cs->image->character[glyphIndex];
			vec2i_t size = vec2i(glyph->width, glyph->height);
			render_push_2d_tile(pos, vec2i(0,0), size, ui_scaled(size), color, cs->tex[glyphIndex]);
			pos.x += glyph->width * ui_scale;
		}
		else {
			pos.x += 8 * ui_scale;
		}
	}
}

void ui_draw_image(vec2i_t pos, uint16_t texture) {
	vec2i_t scaled_size = ui_scaled(render_texture_size(texture));
	render_push_2d(pos, scaled_size, rgba(128, 128, 128, 255), texture);
}

void ui_draw_icon(ui_icon_type_t icon, vec2i_t pos, rgba_t color) {
	render_push_2d(pos, ui_scaled(render_texture_size(icon_textures[icon])), color, icon_textures[icon]);
}

void ui_draw_text_centered(const char *text, vec2i_t pos, ui_text_size_t size, rgba_t color) {
	pos.x -= (ui_text_width(text, size) * ui_scale) >> 1;
	ui_draw_text(text, pos, size, color);
}
