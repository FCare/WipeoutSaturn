#include "../mem.h"
#include "../utils.h"
#include "../system.h"

#include "object.h"
#include "scene.h"
#include "track.h"
#include "weapon.h"
#include "camera.h"
#include "image.h"
#include "object.h"
#include "ship.h"
#include "ship_ai.h"
#include "ship_player.h"
#include "game.h"
#include "race.h"
#include "sfx.h"

void ships_load(void) {
	Object_Saturn_list *ship_models = objects_saturn_load("wipeout/common/allsh.smf");
	LOGD("!!!!!!!!!!!!!!!!!!!!Load objects\n");
	// all_object_dump_saturn(ship_models);
	Object_Saturn_list *collision_models = objects_saturn_load("wipeout/common/alcol.smf");

	error_if(ship_models->length != NUM_PILOTS, "Expected %ld ship models, got %d", NUM_PILOTS, ship_models->length);
	error_if(collision_models->length != NUM_PILOTS, "Expected %ld collison models, got %d", NUM_PILOTS, collision_models->length);

	for (int object_index = 0; object_index < NUM_PILOTS; object_index++) {
		int ship_index = def.ship_model_to_pilot[object_index];
		g.ships[ship_index].model = ship_models->objects[object_index];
		g.ships[ship_index].collision_model = collision_models->objects[object_index];
		ship_init_exhaust_plume(&g.ships[ship_index]);
	}

/*
	uint16_t shadow_textures_start = render_textures_len();
	image_get_texture_semi_trans("wipeout/textures/shad1.tim");
	image_get_texture_semi_trans("wipeout/textures/shad2.tim");
	image_get_texture_semi_trans("wipeout/textures/shad3.tim");
	image_get_texture_semi_trans("wipeout/textures/shad4.tim");

	for (uint32_t i = 0; i < len(g.ships); i++) {
		g.ships[i].shadow_texture = shadow_textures_start + (i >> 1);
	}
*/
}


void ships_init(section_t *section) {
	section_t *start_sections[len(g.ships)];

	int ranks_to_pilots[NUM_PILOTS];

	// Initialize ranks with all pilots in order
	for (uint32_t i = 0; i < len(g.ships); i++) {
		ranks_to_pilots[i] = i;
	}

	// Randomize order for single race or new championship
	if (g.race_type != RACE_TYPE_CHAMPIONSHIP || g.circut == CIRCUT_ALTIMA_VII) {
		shuffle(ranks_to_pilots, len(ranks_to_pilots));
	}

	// Randomize some tiers in an ongoing championship
	else if (g.race_type == RACE_TYPE_CHAMPIONSHIP) {
		// Initialize with current championship order
		for (uint32_t i = 0; i < len(g.ships); i++) {
			ranks_to_pilots[i] = g.championship_ranks[i].pilot;
		}
		shuffle(ranks_to_pilots, 2); // shuffle 0..1
		shuffle(ranks_to_pilots + 4, len(ranks_to_pilots)-5); // shuffle 4..len-1
	}

	// player is always last
	for (uint32_t i = 0; i < len(ranks_to_pilots)-1; i++) {
		if (ranks_to_pilots[i] == g.pilot) {
			swap(ranks_to_pilots[i], ranks_to_pilots[i+1]);
		}
	}


	int start_line_pos = def.circuts[g.circut].settings[g.race_class].start_line_pos;
	for (int i = 0; i < start_line_pos - 15; i++) {
		section = section->next;
	}
	for (uint32_t i = 0; i < len(g.ships); i++) {
		start_sections[i] = section;
		section = section->next;
		if ((i % 2) == 0) {
			section = section->next;
		}
	}
printf("Rank to pilot is %d\n", len(ranks_to_pilots));
	for (uint32_t i = 0; i < len(ranks_to_pilots); i++) {
		int rank_inv = (len(g.ships)-1) - i;
		int pilot = ranks_to_pilots[i];
		ship_init(&g.ships[pilot], start_sections[rank_inv], pilot, rank_inv);
	}
}

static inline bool sort_rank_compare(pilot_points_t *pa, pilot_points_t *pb) {
	ship_t *a = &g.ships[pa->pilot];
	ship_t *b = &g.ships[pb->pilot];
	if (a->total_section_num == b->total_section_num) {
		vec3_t c0 = a->section->center;
		vec3_t c1 = a->section->next->center;
		vec3_t dir = vec3_sub(c1, c0);
		fix16_t pos_a = vec3_dot(vec3_sub(a->position, c0), dir);
		fix16_t pos_b = vec3_dot(vec3_sub(b->position, c0), dir);
		return (pos_a < pos_b);
	}
	else {
		return a->total_section_num < b->total_section_num;
	}
}

void ships_update(void) {
	if (g.race_type == RACE_TYPE_TIME_TRIAL) {
		printf("%d\n", __LINE__);
		ship_update(&g.ships[g.pilot]);
		printf("%d\n", __LINE__);
	}
	else {
		for (uint32_t i = 0; i < len(g.ships); i++) {
			printf("%d\n", __LINE__);
			ship_update(&g.ships[i]);
			printf("%d\n", __LINE__);
		}
		for (uint32_t j = 0; j < (len(g.ships) - 1); j++) {
			for (uint32_t i = j + 1; i < len(g.ships); i++) {
				printf("%d\n", __LINE__);
				ship_collide_with_ship(&g.ships[i], &g.ships[j]);
				printf("%d\n", __LINE__);
			}
		}

		if (flags_is(g.ships[g.pilot].flags, SHIP_RACING)) {
			printf("%d\n", __LINE__);
			sort(g.race_ranks, len(g.race_ranks), sort_rank_compare);
			printf("%d\n", __LINE__);
			for (uint32_t i = 0; i < len(g.ships); i++) {
				g.ships[g.race_ranks[i].pilot].position_rank = i + 1;
			}
		}
	}
	printf("%d\n", __LINE__);
}

void ships_reset_exhaust_plumes(void) {
	for (uint32_t i = 0; i < len(g.ships); i++) {
		ship_reset_exhaust_plume(&g.ships[i]);
	}
}


void ships_draw(void) {
	// Ship models
	for (uint32_t i = 0; i < len(g.ships); i++) {
		if (
			flags_is(g.ships[i].flags, SHIP_VIEW_INTERNAL) ||
			(g.race_type == RACE_TYPE_TIME_TRIAL && i != (uint32_t)g.pilot)
		) {
			continue;
		}

		ship_draw(&g.ships[i]);
	}


	// Shadows
	render_set_model_mat(&mat4_identity());

	render_set_depth_write(false);
	render_set_depth_offset(-32.0);

	for (uint32_t i = 0; i < len(g.ships); i++) {
		if (
			(g.race_type == RACE_TYPE_TIME_TRIAL && i != (uint32_t)g.pilot) ||
			flags_not(g.ships[i].flags, SHIP_VISIBLE) ||
			flags_is(g.ships[i].flags, SHIP_FLYING)
		) {
			continue;
		}

		ship_draw_shadow(&g.ships[i]);
	}

	render_set_depth_offset(0.0);
	render_set_depth_write(true);
}







void ship_init(ship_t *self, section_t *section, int pilot, int inv_start_rank) {
	self->pilot = pilot;
	self->velocity = vec3_fix16(FIX16_ZERO, FIX16_ZERO, FIX16_ZERO);
	self->acceleration = vec3_fix16(FIX16_ZERO, FIX16_ZERO, FIX16_ZERO);
	self->angle = vec3_fix16(FIX16_ZERO, FIX16_ZERO, FIX16_ZERO);
	self->angular_velocity = vec3_fix16(FIX16_ZERO, FIX16_ZERO, FIX16_ZERO);
	self->turn_rate = 0;
	self->thrust_mag = 0;
	self->current_thrust_max = 0;
	self->turn_rate_from_hit = 0;
	self->brake_right = 0;
	self->brake_left = 0;
	self->flags = SHIP_RACING | SHIP_VISIBLE | SHIP_DIRECTION_FORWARD;
	self->weapon_type = WEAPON_TYPE_NONE;
	self->lap = -1;
	self->max_lap = -1;
	self->speed = 0;
	self->ebolt_timer = 0;
	self->revcon_timer = 0;
	self->special_timer = 0;
	self->mat = mat4_identity();

	self->update_timer = 0;
	self->last_impact_time = 0;

	int team = def.pilots[pilot].team;
	self->mass =          def.teams[team].attributes[g.race_class].mass;
	self->thrust_max =    def.teams[team].attributes[g.race_class].thrust_max;
	self->skid =          def.teams[team].attributes[g.race_class].skid;
	self->turn_rate =     def.teams[team].attributes[g.race_class].turn_rate;
	self->turn_rate_max = def.teams[team].attributes[g.race_class].turn_rate_max;
	self->resistance =    def.teams[team].attributes[g.race_class].resistance;
	self->lap_time = 0;

	self->update_timer = UPDATE_TIME_INITIAL;
	self->position_rank = NUM_PILOTS - inv_start_rank;

	if (pilot == g.pilot) {
		self->update_func = ship_player_update_intro;
		self->remote_thrust_max = 2900;
		self->remote_thrust_mag = 46;
		self->fight_back = 0;
	}
	else {
		self->update_func = ship_ai_update_intro;
		self->remote_thrust_max = def.ai_settings[g.race_class][inv_start_rank-1].thrust_max;
		self->remote_thrust_mag = def.ai_settings[g.race_class][inv_start_rank-1].thrust_magnitude;
		self->fight_back = def.ai_settings[g.race_class][inv_start_rank-1].fight_back;
	}

	printf("UPDATE_FUNC is %x\n",self->update_func);

	self->section = section;
	self->prev_section = section;
	fix16_t spread_base = def.circuts[g.circut].settings[g.race_class].spread_base;
	fix16_t spread_factor = def.circuts[g.circut].settings[g.race_class].spread_factor;
	int p = inv_start_rank - 1;
	self->start_accelerate_timer = p * (spread_base + (p * spread_factor)) * (1.0/30.0);

	track_face_t *face = g.track.faces + section->face_start;
	face++;
	if ((inv_start_rank % 2) != 0) {
		face++;
	}

	vec3_t face_point = vec3_mulf(vec3_add(face->quad.vertices[0].pos, face->quad.vertices[2].pos), 0.5);
	self->position = vec3_add(face_point, vec3_mulf(face->normal, 200));

	self->section_num = section->num;
	self->prev_section_num = section->num;
	self->total_section_num = section->num;

	section_t *next = section->next;
	vec3_t direction = vec3_sub(next->center, section->center);
	self->angle.y = -atan2(direction.x, direction.z);
}

void ship_init_exhaust_plume(ship_t *self) {
	int16_t indices[64];
	int16_t indices_len = 0;

	for (int i = 0; i < self->model->info->primitives_len; i++) {
		PRM_saturn *prm = self->model->primitives[i];
		switch (prm->type) {
		case PRM_TYPE_F3 :
			if (flags_is(prm->f3.flag, PRM_SHIP_ENGINE)) {
				die("F3 ::SE marked polys should be ft3's");
			}
			break;

		case PRM_TYPE_F4 :
			if (flags_is(prm->f4.flag, PRM_SHIP_ENGINE)) {
				die("F4 ::SE marked polys should be ft3's");
			}
			break;

		case PRM_TYPE_FT3 :
			if (flags_is(prm->ft3.flag, PRM_SHIP_ENGINE)) {
				indices[indices_len++] = prm->ft3.coords[0];
				indices[indices_len++] = prm->ft3.coords[1];
				indices[indices_len++] = prm->ft3.coords[2];

				flags_add(prm->ft3.flag, PRM_TRANSLUCENT);
				prm->ft3.color.r = 180>>3;
				prm->ft3.color.g = 97>>3 ;
				prm->ft3.color.b = 120>>3;
				prm->ft3.color.msb = 1; //140; //to be review here. Shall use mesh
			}
			break;

		case PRM_TYPE_FT4 :
			if (flags_is(prm->ft4.flag, PRM_SHIP_ENGINE)) {
				die("FT4 ::SE marked polys should be ft3's");
			}
			break;

		case PRM_TYPE_G3 :
			if (flags_is(prm->g3.flag, PRM_SHIP_ENGINE)) {
				die("G3 ::SE marked polys should be ft3's");
			}
			break;

		case PRM_TYPE_G4 :
			if (flags_is(prm->g4.flag, PRM_SHIP_ENGINE)) {
				die("G4 ::SE marked polys should be ft3's");
			}
			break;

		case PRM_TYPE_GT3 :
			if (flags_is(prm->gt3.flag, PRM_SHIP_ENGINE)) {
				indices[indices_len++] = prm->gt3.coords[0];
				indices[indices_len++] = prm->gt3.coords[1];
				indices[indices_len++] = prm->gt3.coords[2];

				flags_add(prm->gt3.flag, PRM_TRANSLUCENT);
				for (int j = 0; j < 3; j++) {
					prm->gt3.color[j].r = 180>>3;
					prm->gt3.color[j].g = 97>>3 ;
					prm->gt3.color[j].b = 120>>3;
					prm->gt3.color[j].msb = 1; //140; //to be review here. Shall use mesh
				}
			}
			break;

		case PRM_TYPE_GT4 :
			if (flags_is(prm->gt4.flag, PRM_SHIP_ENGINE)) {
				die("GT4 ::SE marked polys should be ft3's");
			}
			break;

		default :
			die("cone.c::InitCone:Bad primitive type %x\n", prm->f3.type);
			break;
		}
	}


	// get out the center vertex

	self->exhaust_plume[0].v = NULL;
	self->exhaust_plume[1].v = NULL;
	self->exhaust_plume[2].v = NULL;

	int shared[3] = {-1, -1, -1};
	int booster = 0;
	for (int i = 0; (i < indices_len) && (booster < 3); i++) {
		int similar = 0;
		for (int j = 0; j < indices_len; j++) {
			if (indices[i] == indices[j]) {
				similar++;
				if (similar > 3) {
					int found = 0;
					for (int k = 0; k < 3; k++) {
						if (shared[k] == indices[i]) {
							found = 1;
						}
					}

					if (!found) {
						shared[booster] = indices[i];
						booster++;
					}
				}
			}
		}
	}

	for (int j = 0; j < 3; j++) {
		if (shared[j] != -1) {
			self->exhaust_plume[j].v = &self->model->vertices[shared[j]];
			self->exhaust_plume[j].initial = self->model->vertices[shared[j]];
		}
	}
}

void ship_reset_exhaust_plume(ship_t* self)
{
	for (int i = 0; i < 3; i++) {
		if (self->exhaust_plume[i].v != NULL) {
			self->exhaust_plume[i].v->z = self->exhaust_plume[i].initial.z;
			self->exhaust_plume[i].v->x = self->exhaust_plume[i].initial.x;
			self->exhaust_plume[i].v->y = self->exhaust_plume[i].initial.y;
		}
	}
}


void ship_draw(ship_t *self) {
	object_saturn_draw(self->model, &self->mat);
}

void ship_draw_shadow(ship_t *self) {
	track_face_t *face = track_section_get_base_face(self->section);

	vec3_t face_point = face->quad.vertices[0].pos;
	vec3_t nose = vec3_add(self->position, vec3_mulf(self->dir_forward, 384));
	vec3_t wngl = vec3_sub(vec3_sub(self->position, vec3_mulf(self->dir_right, 256)), vec3_mulf(self->dir_forward, 384));
	vec3_t wngr = vec3_sub(vec3_add(self->position, vec3_mulf(self->dir_right, 256)), vec3_mulf(self->dir_forward, 384));

	nose = vec3_sub(nose, vec3_mulf(face->normal, vec3_distance_to_plane(nose, face_point, face->normal)));
	wngl = vec3_sub(wngl, vec3_mulf(face->normal, vec3_distance_to_plane(wngl, face_point, face->normal)));
	wngr = vec3_sub(wngr, vec3_mulf(face->normal, vec3_distance_to_plane(wngr, face_point, face->normal)));

	rgba_t color = rgba(0 , 0 , 0, 128);
	render_push_tris((tris_t) {
		.vertices = {
			{
				.pos = {.x=wngl.x, .y=wngl.y, .z=wngl.z},
				.uv = {.x=0, .y=256},
				.color = color,
			},
			{
				.pos = {.x=wngr.x, .y=wngr.y, .z=wngr.z},
				.uv = {.x=128, .y=256},
				.color = color
			},
			{
				.pos = {.x=nose.x, .y=nose.y, .z=nose.z},
				.uv = {.x=64, .y=0},
				.color = color
			},
		}
	}, self->shadow_texture);
}

void ship_update(ship_t *self) {
printf("%d\n", __LINE__);
	// Set Unit vectors of this ship
	fix16_t sx = sin(self->angle.x);
	fix16_t cx = cos(self->angle.x);
	fix16_t sy = sin(self->angle.y);
	fix16_t cy = cos(self->angle.y);
	fix16_t sz = sin(self->angle.z);
	fix16_t cz = cos(self->angle.z);
printf("%d\n", __LINE__);
	self->dir_forward.x = -(sy * cx);
	self->dir_forward.y = - sx;
	self->dir_forward.z =  (cy * cx);
printf("%d\n", __LINE__);
	self->dir_right.x =  (cy * cz) + (sy * sz * sx);
	self->dir_right.y = -(sz * cx);
	self->dir_right.z =  (sy * cz) - (cy * sx * sz);
printf("%d\n", __LINE__);
	self->dir_up.x = (cy * sz) - (sy * sx * cz);
	self->dir_up.y = -(cx * cz);
	self->dir_up.z = (sy * sz) + (cy * sx * cz);
printf("%d\n", __LINE__);
	self->prev_section = self->section;
	fix16_t distance;
	printf("%d\n", __LINE__);
	self->section = track_nearest_section(self->position, self->section, &distance);
	printf("%d\n", __LINE__);
	if (distance > 3700) {
		flags_add(self->flags, SHIP_FLYING);
	}
	else {
		flags_rm(self->flags, SHIP_FLYING);
	}
printf("%d\n", __LINE__);
	self->prev_section_num = self->prev_section->num;
	self->section_num = self->section->num;

printf("%d\n", __LINE__);
	// Figure out which side of the track the ship is on
	track_face_t *face = track_section_get_base_face(self->section);
printf("%d\n", __LINE__);
	vec3_t to_face_vector = vec3_sub(
		face->quad.vertices[0].pos,
		face->quad.vertices[1].pos
	);
printf("%d\n", __LINE__);
	vec3_t direction = vec3_sub(self->section->center, self->position);
printf("%d\n", __LINE__);
	if (vec3_dot(direction, to_face_vector) > 0) {
		flags_add(self->flags, SHIP_LEFT_SIDE);
	}
	else {
		flags_rm(self->flags, SHIP_LEFT_SIDE);
		face++;
	}
printf("%d\n", __LINE__);
	// Collect powerup
	if (
		flags_is(face->flags, FACE_PICKUP_ACTIVE) &&
		flags_not(self->flags, SHIP_SPECIALED) &&
		self->weapon_type == WEAPON_TYPE_NONE &&
		track_collect_pickups(face)
	) {
		printf("%d\n", __LINE__);
		if (self->pilot == g.pilot) {
			printf("%d\n", __LINE__);
			sfx_play(SFX_POWERUP);
			printf("%d\n", __LINE__);
			if (flags_is(self->flags, SHIP_SHIELDED)) {
				printf("%d\n", __LINE__);
				self->weapon_type = weapon_get_random_type(WEAPON_CLASS_PROJECTILE);
				printf("%d\n", __LINE__);
			}
			else {
				printf("%d\n", __LINE__);
				self->weapon_type = weapon_get_random_type(WEAPON_CLASS_ANY);
				printf("%d\n", __LINE__);
			}
		}
		else {
			printf("%d\n", __LINE__);
			self->weapon_type = 1;
			printf("%d\n", __LINE__);
		}
		printf("%d\n", __LINE__);
	}

printf("%d\n", __LINE__);
	self->last_impact_time += system_tick();
printf("%d %x\n", __LINE__, self->update_func);	// Call the active player/ai update function
	(self->update_func)(self);
printf("%d\n", __LINE__);

	// Animate the exhaust plume

	int exhaust_len;

	if (self->pilot == g.pilot) {
		// get the z exhaust_len related to speed or thrust
		printf("%d\n", __LINE__);
		exhaust_len = self->thrust_mag * 0.0625;
		printf("%d\n", __LINE__);
		exhaust_len += self->speed * 0.00390625;
		printf("%d\n", __LINE__);
	}
	else {
		// for remote ships the z exhaust_len is a constant
		printf("%d\n", __LINE__);
		exhaust_len = 150;
		printf("%d\n", __LINE__);
	}
printf("%d\n", __LINE__);
	for (int i = 0; i < 3; i++) {
		printf("%d\n", __LINE__);
		if (self->exhaust_plume[i].v != NULL) {
			printf("%d\n", __LINE__);
			self->exhaust_plume[i].v->z = self->exhaust_plume[i].initial.z - exhaust_len + (rand_int(-16383, 16383) >> 9);
			printf("%d\n", __LINE__);
			self->exhaust_plume[i].v->x = self->exhaust_plume[i].initial.x + (rand_int(-16383, 16383) >> 11);
			printf("%d\n", __LINE__);
			self->exhaust_plume[i].v->y = self->exhaust_plume[i].initial.y + (rand_int(-16383, 16383) >> 11);
			printf("%d\n", __LINE__);
		}
		printf("%d\n", __LINE__);
	}
printf("%d\n", __LINE__);
	mat4_set_translation(&self->mat, self->position);
	mat4_set_yaw_pitch_roll(&self->mat, self->angle);

printf("%d\n", __LINE__);

	// Race position and lap times

	self->lap_time += system_tick();
printf("%d\n", __LINE__);
	int start_line_pos = def.circuts[g.circut].settings[g.race_class].start_line_pos;

	// Crossed line backwards
	if (self->prev_section_num == start_line_pos + 1 && self->section_num <= start_line_pos) {
		self->lap--;
	}
	// Crossed line forwards
	else if (self->prev_section_num == start_line_pos && self->section_num > start_line_pos) {
		self->lap++;
printf("%d\n", __LINE__);
		// Is it the first time we're crossing the line for this lap?
		if (self->lap > self->max_lap) {
			printf("%d\n", __LINE__);
			self->max_lap = self->lap;

			if (self->lap > 0 && self->lap <= NUM_LAPS) {
				printf("%d\n", __LINE__);
				g.lap_times[self->pilot][self->lap-1] = self->lap_time;
printf("%d\n", __LINE__);
			}
			self->lap_time = 0;
printf("%d\n", __LINE__);
			if (g.race_type == RACE_TYPE_TIME_TRIAL) {
				printf("%d\n", __LINE__);
				self->weapon_type = WEAPON_TYPE_TURBO;
				printf("%d\n", __LINE__);
			}
printf("%d\n", __LINE__);
			if (self->lap == NUM_LAPS && self->pilot == g.pilot) {
				printf("%d\n", __LINE__);
				race_end();
				printf("%d\n", __LINE__);
			}
			printf("%d\n", __LINE__);
		}
		printf("%d\n", __LINE__);
	}
printf("%d\n", __LINE__);
	int section_num_from_line = self->section_num - (start_line_pos + 1);
	printf("%d\n", __LINE__);
	if (section_num_from_line < 0) {
		printf("%d\n", __LINE__);
		section_num_from_line += g.track.section_count;
		printf("%d\n", __LINE__);
	}
	printf("%d\n", __LINE__);
	self->total_section_num = self->lap * g.track.section_count + section_num_from_line;
	printf("%d\n", __LINE__);
}

vec3_t ship_cockpit(ship_t *self) {
	return vec3_add(self->position, vec3_mulf(self->dir_up, 128));
}

vec3_t ship_nose(ship_t *self) {
	return vec3_add(self->position, vec3_mulf(self->dir_forward, 512));

}

vec3_t ship_wing_left(ship_t *self) {
	return vec3_sub(vec3_sub(self->position, vec3_mulf(self->dir_right, 256)), vec3_mulf(self->dir_forward, 256));
}

vec3_t ship_wing_right(ship_t *self) {
	return vec3_sub(vec3_add(self->position, vec3_mulf(self->dir_right, 256)), vec3_mulf(self->dir_forward, 256));
}

static bool vec3_is_on_face(vec3_t pos, track_face_t *face, fix16_t alpha) {
	vec3_t plane_point = vec3_sub(pos, vec3_mulf(face->normal, alpha));
	vec3_t vec0 = vec3_sub(plane_point, face->quad.vertices[1].pos);
	vec3_t vec1 = vec3_sub(plane_point, face->quad.vertices[2].pos);
	vec3_t vec2 = vec3_sub(plane_point, face->quad.vertices[0].pos);
	vec3_t vec3 = vec3_sub(plane_point, face->quad.vertices[3].pos);

	fix16_t angle =
		vec3_angle(vec0, vec2) +
		vec3_angle(vec2, vec3) +
		vec3_angle(vec3, vec1) +
		vec3_angle(vec1, vec0);

	return (angle > PLATFORM_PI * 2 - 0.01);
}

void ship_resolve_wing_collision(ship_t *self, track_face_t *face, fix16_t direction) {
	vec3_t collision_vector = vec3_sub(self->section->center, face->quad.vertices[2].pos);
	fix16_t angle = vec3_angle(collision_vector, self->dir_forward);
	self->velocity = vec3_reflect(self->velocity, face->normal, 2);
	self->position = vec3_sub(self->position, vec3_mulf(self->velocity, 0.015625)); // system_tick?
	self->velocity = vec3_sub(self->velocity, vec3_mulf(self->velocity, 0.5));
	self->velocity = vec3_add(self->velocity, vec3_mulf(face->normal, 4096)); // div by 4096?

	fix16_t magnitude = (fabsf(angle) * self->speed) * PLATFORM_PI / (4096 * 16.0); // (6 velocity shift, 12 angle shift?)

	vec3_t wing_pos;
	if (direction > 0) {
		self->angular_velocity.z += magnitude;
		wing_pos = vec3_add(self->position, vec3_mulf(vec3_sub(self->dir_right, self->dir_forward), 256)); // >> 4??
	}
	else {
		self->angular_velocity.z -= magnitude;
		wing_pos = vec3_sub(self->position, vec3_mulf(vec3_sub(self->dir_right, self->dir_forward), 256)); // >> 4??
	}

	if (self->last_impact_time > 0.2) {
		self->last_impact_time = 0;
		sfx_play_at(SFX_IMPACT, wing_pos, vec3_fix16(0, 0, 0), 1);
	}
}


void ship_resolve_nose_collision(ship_t *self, track_face_t *face, fix16_t direction) {
	vec3_t collision_vector = vec3_sub(self->section->center, face->quad.vertices[2].pos);
	fix16_t angle = vec3_angle(collision_vector, self->dir_forward);
	self->velocity = vec3_reflect(self->velocity, face->normal, 2);
	self->position = vec3_sub(self->position, vec3_mulf(self->velocity, 0.015625)); // system_tick?
	self->velocity = vec3_sub(self->velocity, vec3_mulf(self->velocity, 0.5));
	self->velocity = vec3_add(self->velocity, vec3_mulf(face->normal, 4096)); // div by 4096?

	fix16_t magnitude = ((self->speed * 0.0625) + 400) * PLATFORM_PI / (4096.0 * 64.0);
	if (direction > 0) {
		self->angular_velocity.y += magnitude;
	}
	else {
		self->angular_velocity.y -= magnitude;
	}

	if (self->last_impact_time > 0.2) {
		self->last_impact_time = 0;
		sfx_play_at(SFX_IMPACT, ship_nose(self), vec3_fix16(0, 0, 0), 1);
	}
}


void ship_collide_with_track(ship_t *self, track_face_t *face) {
	fix16_t alpha;
	section_t 	*trackPtr;
	bool collide;
	track_face_t *face2;

	trackPtr = self->section->next;
	vec3_t direction = vec3_sub(trackPtr->center, self->section->center);
	fix16_t down_track = vec3_dot(direction, self->dir_forward);

	if (down_track < 0) {
		flags_rm(self->flags, SHIP_DIRECTION_FORWARD);
	}
	else {
		flags_add(self->flags, SHIP_DIRECTION_FORWARD);
	}

	vec3_t to_face_vector = vec3_sub(face->quad.vertices[0].pos, face->quad.vertices[1].pos);
	direction = vec3_sub(self->section->center, self->position);
	fix16_t to_face = vec3_dot(direction, to_face_vector);

	face--;

	// Check against left hand side of track

	// FIXME: the collision checks in junctions are very flakey and often select
	// the wrong face to test for a collision.
	// Instead of this whole mess here, there should just be a function
	// `track_get_nearest_face(section, pos)` that we call with the nose and
	// wing positions and then just resolve against this face.

	if (to_face > 0) {
		flags_add(self->flags, SHIP_LEFT_SIDE);

		vec3_t face_point = face->quad.vertices[0].pos;

		alpha = vec3_distance_to_plane(ship_nose(self), face_point, face->normal);
		if (alpha <= 0) {
			if (flags_is(self->section->flags, SECTION_JUNCTION_START)) {
				collide = vec3_is_on_face(ship_nose(self), face, alpha);
				if (collide) {
					ship_resolve_nose_collision(self, face, -down_track);
				}
				else {
					face2 = g.track.faces + self->section->next->face_start;
					collide = vec3_is_on_face(ship_nose(self), face2, alpha);
					if (collide) {
						ship_resolve_nose_collision(self, face, -down_track);
					}
				}
			}
			else if (flags_is(self->section->flags, SECTION_JUNCTION_END)) {
				collide = vec3_is_on_face(ship_nose(self), face, alpha);
				if (collide) {
					ship_resolve_nose_collision(self, face, -down_track);
				}
				else {
					face2 = g.track.faces + self->section->prev->face_start;
					collide = vec3_is_on_face(ship_nose(self), face2, alpha);
					if (collide) {
						ship_resolve_nose_collision(self, face, -down_track);
					}
				}
			}
			else {
				ship_resolve_nose_collision(self, face, -down_track);
			}
			return;
		}

		alpha = vec3_distance_to_plane(ship_wing_left(self), face_point, face->normal);
		if (alpha <= 0) {
			if (
				flags_is(self->section->flags, SECTION_JUNCTION_START) ||
				flags_is(self->section->flags, SECTION_JUNCTION_END)
			) {
				collide = vec3_is_on_face(ship_wing_left(self), face, alpha);
				if (collide) {
					ship_resolve_nose_collision(self, face, -down_track);
				}
			}
			else {
				ship_resolve_wing_collision(self, face, -down_track);
			}
			return;
		}

		alpha = vec3_distance_to_plane(ship_wing_right(self), face_point, face->normal);
		if (alpha <= 0) {
			if (
				flags_is(self->section->flags, SECTION_JUNCTION_START) ||
				flags_is(self->section->flags, SECTION_JUNCTION_END)
			) {
				collide = vec3_is_on_face(ship_wing_right(self), face, alpha);
				if (collide) {
					ship_resolve_nose_collision(self, face, -down_track);
				}
			}
			else {
				ship_resolve_wing_collision(self, face, -down_track);
			}
			return;
		}
	}


	// Collision check against 2nd wall
	else {
		flags_rm(self->flags, SHIP_LEFT_SIDE);

		face++;
		while (face->flags & FACE_TRACK_BASE) {
			face++;
		}

		vec3_t face_point = face->quad.vertices[0].pos;

		alpha = vec3_distance_to_plane(ship_nose(self), face_point, face->normal);
		if (alpha <= 0) {
			if (flags_is(self->section->flags, SECTION_JUNCTION_START)) {
				collide = vec3_is_on_face(ship_nose(self), face, alpha);
				if (collide) {
					ship_resolve_nose_collision(self, face, down_track);
				}
				else {
					face2 = g.track.faces + self->section->next->face_start;
					face2 += 3;
					collide = vec3_is_on_face(ship_nose(self), face2, alpha);
					if (collide) {
						ship_resolve_nose_collision(self, face, -down_track);
					}
				}
			}
			else if (flags_is(self->section->flags, SECTION_JUNCTION_END)) {
				collide = vec3_is_on_face(ship_nose(self), face, alpha);
				if (collide) {
					ship_resolve_nose_collision(self, face, -down_track);
				}
				else {
					face2 = g.track.faces + self->section->prev->face_start;
					face2 += 3;
					collide = vec3_is_on_face(ship_nose(self), face2, alpha);
					if (collide) {
						ship_resolve_nose_collision(self, face2, -down_track);
					}
				}
			}
			else {
				ship_resolve_nose_collision(self, face, down_track);
			}
			return;
		}

		alpha = vec3_distance_to_plane(ship_wing_left(self), face_point, face->normal);
		if (alpha <= 0) {
			if (
				flags_is(self->section->flags, SECTION_JUNCTION_START) ||
				flags_is(self->section->flags, SECTION_JUNCTION_END)
			) {
				collide = vec3_is_on_face(ship_wing_left(self), face, alpha);
				if (collide) {
					ship_resolve_nose_collision(self, face, down_track);
				}
			}
			else {
				ship_resolve_wing_collision(self, face, down_track);
			}
			return;
		}

		alpha = vec3_distance_to_plane(ship_wing_right(self), face_point, face->normal);
		if (alpha <= 0) {
			if (
				flags_is(self->section->flags, SECTION_JUNCTION_START) ||
				flags_is(self->section->flags, SECTION_JUNCTION_END)
			) {
				collide = vec3_is_on_face(ship_wing_right(self), face, alpha);
				if (collide) {
					ship_resolve_nose_collision(self, face, down_track);
				}
			}
			else {
				ship_resolve_wing_collision(self, face, down_track);
			}
			return;
		}
	}
}


bool ship_intersects_ship(ship_t *self, ship_t *other) {
	// Get 4 points of collision model relative to the
	// camera
	vec3_t a = vec3_transform(other->collision_model->vertices[0], &other->mat);
	vec3_t b = vec3_transform(other->collision_model->vertices[1], &other->mat);
	vec3_t c = vec3_transform(other->collision_model->vertices[2], &other->mat);
	vec3_t d = vec3_transform(other->collision_model->vertices[3], &other->mat);

	vec3_t other_points[6] = {b, a, d, a, a, b};
	vec3_t other_lines[6] = {
		vec3_sub(c, b),
		vec3_sub(c, a),
		vec3_sub(c, d),
		vec3_sub(b, a),
		vec3_sub(d, a),
		vec3_sub(d, b)
	};


	int primitives_len = other->collision_model->info->primitives_len;

	vec3_t p1, p2, p3;

	// for all 4 planes of the enemy ship
	for (int pi = 0; pi < other->collision_model->info->primitives_len; pi++) {
		PRM_saturn *poly = other->collision_model->primitives[pi];
		int16_t *indices;
		switch (poly->type) {
			case PRM_TYPE_F3:
				indices = poly->f3.coords;
				p1 =  vec3_transform(self->collision_model->vertices[indices[0]], &self->mat);
				p2 =  vec3_transform(self->collision_model->vertices[indices[1]], &self->mat);
				p3 =  vec3_transform(self->collision_model->vertices[indices[2]], &self->mat);
				break;
			case PRM_TYPE_G3:
				indices = poly->g3.coords;
				p1 =  vec3_transform(self->collision_model->vertices[indices[0]], &self->mat);
				p2 =  vec3_transform(self->collision_model->vertices[indices[1]], &self->mat);
				p3 =  vec3_transform(self->collision_model->vertices[indices[2]], &self->mat);
				break;
			case PRM_TYPE_FT3:
				indices = poly->ft3.coords;
				p1 =  vec3_transform(self->collision_model->vertices[indices[0]], &self->mat);
				p2 =  vec3_transform(self->collision_model->vertices[indices[1]], &self->mat);
				p3 =  vec3_transform(self->collision_model->vertices[indices[2]], &self->mat);
				break;
			case PRM_TYPE_GT3:
				indices = poly->gt3.coords;
				p1 =  vec3_transform(self->collision_model->vertices[indices[0]], &self->mat);
				p2 =  vec3_transform(self->collision_model->vertices[indices[1]], &self->mat);
				p3 =  vec3_transform(self->collision_model->vertices[indices[2]], &self->mat);
				break;
			default:
				break;
		}

		// Find polyGon line vectors
		vec3_t p1p2 = vec3_sub(p2, p1);
		vec3_t p1p3 = vec3_sub(p3, p1);

		// Find plane equations
		vec3_t plane1 = vec3_cross(p1p2, p1p3);

		for (int vi = 0; vi < 6; vi++) {
			fix16_t dp1 = vec3_dot(vec3_sub(p1, other_points[vi]), plane1);
			fix16_t dp2 = vec3_dot(other_lines[vi], plane1);

			if (dp2 != 0) {
				fix16_t norm = dp1 / dp2;

				if ((norm >= 0) && (norm <= 1)) {
					vec3_t term = vec3_mulf(other_lines[vi], norm);
					vec3_t res = vec3_add(term, other_points[vi]);

					vec3_t v0 = vec3_sub(p1, res);
					vec3_t v1 = vec3_sub(p2, res);
					vec3_t v2 = vec3_sub(p3, res);

					fix16_t angle =
						vec3_angle(v0, v1) +
						vec3_angle(v1, v2) +
						vec3_angle(v2, v0);

					if ((angle >= PLATFORM_PI * 2 - PLATFORM_PI * 0.1)) {
						return true;
					}
				}
			}
		}
	}
	return false;
}

void ship_collide_with_ship(ship_t *self, ship_t *other) {
	fix16_t distance = vec3_len(vec3_sub(self->position, other->position));

	// Do a quick distance check; if ships are far apart, remove the collision flag
	// and early out.
	if (distance > 960) {
		flags_rm(self->flags, SHIP_COLL);
		flags_rm(other->flags, SHIP_COLL);
		return;
	}

	// Ships are close, do a real collision test
	if (!ship_intersects_ship(self, other)) {
		return;
	}

	// Ships did collide, resolve

	vec3_t vc = vec3_divf(
		vec3_add(
			vec3_mulf(self->velocity, self->mass),
			vec3_mulf(other->velocity, other->mass)
		),
		self->mass + other->mass
	);

	vec3_t ship_react = vec3_mulf(vec3_sub(vc, self->velocity), 0.25);
	vec3_t other_react = vec3_mulf(vec3_sub(vc, other->velocity), 0.25);
	self->position = vec3_sub(self->position, vec3_mulf(self->velocity, 0.015625)); // >> 6
	other->position = vec3_sub(other->position, vec3_mulf(other->velocity, 0.015625)); // >> 6

	self->velocity = vec3_add(vc, ship_react);
	other->velocity = vec3_add(vc, other_react);

	vec3_t res = vec3_sub(self->position, other->position);

	self->velocity = vec3_add(self->velocity, vec3_mulf(res, 2));  // << 2
	self->position = vec3_add(self->position, vec3_mulf(self->velocity, 0.015625)); // >> 6

	other->velocity = vec3_sub(other->velocity, vec3_mulf(res, 2)); // << 2
	other->position = vec3_add(other->position, vec3_mulf(other->velocity, 0.015625)); // >> 6

	if (
		flags_not(self->flags, SHIP_COLL) &&
		flags_not(other->flags, SHIP_COLL) &&
		self->last_impact_time > 0.2
	) {
		self->last_impact_time = 0;
		vec3_t sound_pos = vec3_mulf(vec3_add(self->position, other->position), 0.5);
		sfx_play_at(SFX_CRUNCH, sound_pos, vec3_fix16(0, 0, 0), 1);
	}
	flags_add(self->flags, SHIP_COLL);
	flags_add(other->flags, SHIP_COLL);
}
