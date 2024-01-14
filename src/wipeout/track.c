#include "../mem.h"
#include "../utils.h"
#include "../render.h"
#include "../system.h"
#include "../platform.h"

#include "object.h"
#include "track.h"
#include "camera.h"
#include "object.h"
#include "game.h"

void track_load(const char *base_path) {
	// Load and assemble high res track tiles

	g.track.textures.start = render_textures_len();
	g.track.textures.len = 0;

	ttf_t *ttf = track_load_tile_format(get_path(base_path, "library.ttf"));
	cmp_t *cmp = image_load_compressed(get_path(base_path, "library.cmp"));

	image_t *temp_tile = image_alloc(128, 128);
	for (uint32_t i = 0; i < ttf->len; i++) {
		for (int tx = 0; tx < 4; tx++) {
			for (int ty = 0; ty < 4; ty++) {
				uint32_t sub_tile_index = ttf->tiles[i].near[ty * 4 + tx];
				image_t *sub_tile = image_load_from_bytes(cmp->entries[sub_tile_index], false);
				image_copy(sub_tile, temp_tile, 0, 0, 32, 32, tx * 32, ty * 32);
				mem_temp_free(sub_tile);
			}
		}
		render_texture_create(temp_tile->width, temp_tile->height, temp_tile->pixels);
		g.track.textures.len++;
	}

	mem_temp_free(temp_tile);
	mem_temp_free(cmp);
	mem_temp_free(ttf);

	vec3_t *vertices = track_load_vertices(get_path(base_path, "track.trv"));
	track_load_faces(get_path(base_path, "track.trf"), vertices);
	mem_temp_free(vertices);

	track_load_sections(get_path(base_path, "track.trs"));

	g.track.pickups_len = 0;
	section_t *s = g.track.sections;
	section_t *j = NULL;

	// Nummerate all sections; take care to give both stretches at a junction
	// the same numbers.
	int num = 0;
	do {
		s->num = num++;
		if (s->junction) { // start junction
			j = s->junction;
			do {
				j->num = num++;
				j = j->next;
			} while (!j->junction); // end junction
			num = s->num;
		}
		s = s->next;
	} while (s != g.track.sections);
	g.track.total_section_nums = num;

	g.track.pickups = mem_mark();
	for (int i = 0; i < g.track.section_count; i++) {
		track_face_t *face = track_section_get_base_face(&g.track.sections[i]);

		for (int f = 0; f < 2; f++) {
			if (flags_any(face->flags, FACE_PICKUP_RIGHT | FACE_PICKUP_LEFT)) {
				mem_bump(sizeof(track_pickup_t));
				g.track.pickups[g.track.pickups_len].face = face;
				g.track.pickups[g.track.pickups_len].cooldown_timer = 0;
				g.track.pickups_len++;
			}

			if (flags_is(face->flags, FACE_BOOST)) {
				track_face_set_color(face, rgba(0, 0, 255, 255));
			}
			face++;
		}
	}
}

ttf_t *track_load_tile_format(char *ttf_name) {
	uint32_t ttf_size;
	uint8_t *ttf_bytes = platform_load_asset(ttf_name, &ttf_size);

	uint32_t p = 0;
	uint32_t num_tiles = ttf_size / 42;

	ttf_t *ttf = mem_temp_alloc(sizeof(ttf_t) + sizeof(ttf_tile_t) * num_tiles);
	ttf->len = num_tiles;

	for (uint32_t t = 0; t < num_tiles; t++) {
		for (int i = 0; i < 16; i++) {
			ttf->tiles[t].near[i] = get_i16(ttf_bytes, &p);
		}
		for (int i = 0; i < 4; i++) {
			ttf->tiles[t].med[i] = get_i16(ttf_bytes, &p);
		}
		ttf->tiles[t].far = get_i16(ttf_bytes, &p);
	}
	mem_temp_free(ttf_bytes);

	return ttf;
}

bool track_collect_pickups(track_face_t *face) {
	if (flags_is(face->flags, FACE_PICKUP_ACTIVE)) {
		flags_rm(face->flags, FACE_PICKUP_ACTIVE);
		flags_add(face->flags, FACE_PICKUP_COLLECTED);
		track_face_set_color(face, rgba(255, 255, 255, 255));
		return true;
	}
	else {
		return false;
	}
}

vec3_t *track_load_vertices(char *file_name) {
	uint32_t size;
	uint8_t *bytes = platform_load_asset(file_name, &size);

	g.track.vertex_count = size / 16; // VECTOR_SIZE
	vec3_t *vertices = mem_temp_alloc(sizeof(vec3_t) * g.track.vertex_count);

	uint32_t p = 0;
	int div = 0;
	for (int i = 0; i < g.track.vertex_count; i++) {
		int val[3] = {get_i32(bytes, &p),get_i32(bytes, &p),get_i32(bytes, &p)};
		for (int k=0; k<3; k++) {
			if ((val[k]>>div) > 32768) div++;
			if ((val[k]>>div) < -32768) div++;
		}
		vertices[i].x = val[0];
		vertices[i].y = val[1];
		vertices[i].z = val[2];
		p += 4; // padding
	}
	for (int i = 0; i < g.track.vertex_count; i++) {
		vertices[i].x = FIX16(vertices[i].x>>div);
		vertices[i].y = FIX16(vertices[i].y>>div);
		vertices[i].z = FIX16(vertices[i].z>>div);
	}

	mem_temp_free(bytes);
	return vertices;
}

static const vec2_t track_uv[2][4] = {
	{{.x=128, .y=0}, {.x=0, .y=0}, {.x=0, .y=128}, {.x=128, .y=128}},
	{{.x=0, .y=0}, {.x=128, .y=0}, {.x=128, .y=128}, {.x=0, .y=128}}
};

void track_load_faces(char *file_name, vec3_t *vertices) {
	uint32_t size;
	uint8_t *bytes = platform_load_asset(file_name, &size);

	g.track.face_count = size / 20; // TRACK_FACE_DATA_SIZE
	g.track.faces = mem_bump(sizeof(track_face_t) * g.track.face_count);

	uint32_t p = 0;
	track_face_t *tf = g.track.faces;


	for (int i = 0; i < g.track.face_count; i++) {

		vec3_t v0 = vertices[get_i16(bytes, &p)];
		vec3_t v1 = vertices[get_i16(bytes, &p)];
		vec3_t v2 = vertices[get_i16(bytes, &p)];
		vec3_t v3 = vertices[get_i16(bytes, &p)];
		uint16_t val = get_i16(bytes, &p);
		tf->normal.x = FIX16(val) >> 12; // val / 4096.0
		val = get_i16(bytes, &p);
		tf->normal.y = FIX16(val) >> 12; // val / 4096.0
		val = get_i16(bytes, &p);
		tf->normal.z = FIX16(val) >> 12; // val / 4096.0

		tf->texture = get_i8(bytes, &p);
		tf->flags = get_i8(bytes, &p);

		rgba_t color = rgba_from_u32(get_u32(bytes, &p));
		const vec2_t *uv = track_uv[flags_is(tf->flags, FACE_FLIP_TEXTURE) ? 1 : 0];

		tf->quad = (quads_t){
			.vertices = {
				{.pos = v0, .uv = uv[0], .color = color},
				{.pos = v1, .uv = uv[1], .color = color},
				{.pos = v2, .uv = uv[2], .color = color},
				{.pos = v3, .uv = uv[3], .color = color},
			}
		};

		tf++;
	}

	mem_temp_free(bytes);
}


void track_load_sections(char *file_name) {
	uint32_t size;
	uint8_t *bytes = platform_load_asset(file_name, &size);

	g.track.section_count = size / 156; // SECTION_DATA_SIZE
	g.track.sections = mem_bump(sizeof(section_t) * g.track.section_count);

	uint32_t p = 0;
	section_t *ts = g.track.sections;
	for (int i = 0; i < g.track.section_count; i++) {
		int32_t junction_index = get_i32(bytes, &p);
		if (junction_index != -1) {
			ts->junction = g.track.sections + junction_index;
		}
		else {
			ts->junction = NULL;
		}

		ts->prev = g.track.sections + get_i32(bytes, &p);
		ts->next = g.track.sections + get_i32(bytes, &p);

		ts->center.x = get_i32(bytes, &p);
		ts->center.y = get_i32(bytes, &p);
		ts->center.z = get_i32(bytes, &p);

		int16_t version = get_i16(bytes, &p);
		error_if(version != TRACK_VERSION, "Convert track with track10: section: %d Track: %d\n", version, TRACK_VERSION);
		p += 2; // padding

		p += 4 + 4; // objects pointer, objectCount
		p += 5 * 3 * 4; // view section pointers
		p += 5 * 3 * 2; // view section counts

		p += 4 * 2; // high list
		p += 4 * 2; // med list

		ts->face_start = get_i16(bytes, &p);
		ts->face_count = get_i16(bytes, &p);

		p += 2 * 2; // global/local radius

		ts->flags = get_i16(bytes, &p);
		ts->num = get_i16(bytes, &p);
		p += 2; // padding
		ts++;
	}

	mem_temp_free(bytes);
}




void track_draw_section(section_t *section) {
	track_face_t *face = g.track.faces + section->face_start;
	int16_t face_count = section->face_count;

	for (int16_t j = 0; j < face_count; j++) {
		uint16_t tex_index = texture_from_list(g.track.textures, face->texture);
		render_push_quads(&(face->quad), tex_index);
		face++;
	}
}

void track_draw(camera_t *camera) {
	render_set_model_mat(&mat4_identity());

	// Calculate the camera forward vector, so we can cull everything that's
	// behind. Ideally we'd want to do a full frustum culling here. FIXME.
	vec3_t cam_pos = camera->position;
	vec3_t cam_dir = camera_forward(camera);

	int drawn = 0;
	int skipped = 0;
	for(int32_t i = 0; i < g.track.section_count; i++) {
		section_t *s = &g.track.sections[i];
		vec3_t diff = vec3_sub(cam_pos, s->center);
		fix16_t cam_dot = vec3_dot(diff, cam_dir);
		fix16_t dist_sq = vec3_dot(diff, diff);
		if (
			(cam_dot < FIX16(2048)) && // FIXME: should use the bounding radius of the section
			(dist_sq < (RENDER_FADEOUT_FAR * RENDER_FADEOUT_FAR))
		) {
			track_draw_section(s);
		}
	}
}

void track_cycle_pickups(void) {
	fix16_t pickup_cycle_time = 1.5 * system_cycle_time();

	for (int i = 0; i < g.track.pickups_len; i++) {
		if (flags_is(g.track.pickups[i].face->flags, FACE_PICKUP_COLLECTED)) {
			flags_rm(g.track.pickups[i].face->flags, FACE_PICKUP_COLLECTED);
			g.track.pickups[i].cooldown_timer = TRACK_PICKUP_COOLDOWN_TIME;
		}
		else if (g.track.pickups[i].cooldown_timer <= 0) {
			flags_add(g.track.pickups[i].face->flags, FACE_PICKUP_ACTIVE);
			track_face_set_color(g.track.pickups[i].face, rgba(
				sin( pickup_cycle_time + i) * 127 + 128,
				cos( pickup_cycle_time + i) * 127 + 128,
				sin(-pickup_cycle_time - i) * 127 + 128,
				255
			));
		}
		else{
			g.track.pickups[i].cooldown_timer -= system_tick();
		}
	}
}

void track_face_set_color(track_face_t *face, rgba_t color) {
	face->quad.vertices[0].color = color;
	face->quad.vertices[1].color = color;
	face->quad.vertices[2].color = color;
	face->quad.vertices[3].color = color;
}

track_face_t *track_section_get_base_face(section_t *section) {
	track_face_t *face = g.track.faces +section->face_start;
	while(flags_not(face->flags, FACE_TRACK_BASE)) {
		face++;
	}
	return face;
}

section_t *track_nearest_section(vec3_t pos, section_t *section, fix16_t *distance) {
	// Start search several sections before current section

	for (int i = 0; i < TRACK_SEARCH_LOOK_BACK; i++) {
		section = section->prev;
	}

	// Find vector from ship center to track section under
	// consideration
	fix16_t shortest_distance = 1000000000.0;
	section_t *nearest_section = section;
	section_t *junction = NULL;
	for (int i = 0; i < TRACK_SEARCH_LOOK_AHEAD; i++) {
		if (section->junction) {
			junction = section->junction;
		}

		fix16_t d = vec3_len(vec3_sub(pos, section->center));
		if (d < shortest_distance) {
			shortest_distance = d;
			nearest_section = section;
		}

		section = section->next;
	}

	if (junction) {
		section = junction;
		for (int i = 0; i < TRACK_SEARCH_LOOK_AHEAD; i++) {
			fix16_t d = vec3_len(vec3_sub(pos, section->center));
			if (d < shortest_distance) {
				shortest_distance = d;
				nearest_section = section;
			}

			if (flags_is(junction->flags, SECTION_JUNCTION_START)) {
				section = section->next;
			}
			else {
				section = section->prev;
			}
		}
	}

	if (distance != NULL) {
		*distance = shortest_distance;
	}
	return nearest_section;
}
