#ifndef __TYPE_H__
#define __TYPE_H__
#include <stdint.h>

#define rgba(R, G, B, A) ((rgba_t){.r = R, .g = G, .b = B, .a = A})

typedef uint8_t bool;
typedef int16_t fix16_t;
typedef uint16_t rgb1555_t;

#define true 1
#define false 0

typedef struct {
  uint16_t x,y;
} vec2i_t;

typedef struct rgba_t {
	uint8_t r, g, b, a;
} rgba_t;

typedef union vec3 {
    struct {
        int16_t x;
        int16_t y;
        int16_t z;
    };
    int16_t comp[3];
} vec3_t;

typedef union vec2 {
    struct {
        int16_t x;
        int16_t y;
    };
    int16_t comp[2];
} vec2_t;

typedef struct {
	vec3_t pos;
	vec2_t uv;
	rgba_t color;
} vertex_t;

typedef struct {
	vertex_t vertices[3];
} tris_t;

typedef struct {
	vertex_t vertices[4];
} quads_t;

static rgba_t rgba_from_u32(uint32_t v) {
	return rgba(
		((v >> 24) & 0xff),
		((v >> 16) & 0xff),
		((v >> 8) & 0xff),
		255
	);
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define die(...) \
	printf("Abort at " TOSTRING(__FILE__) " line " TOSTRING(__LINE__) ": " __VA_ARGS__); \
	printf("\n"); \
	exit(1)

#define error_if(TEST, ...) \
	if (TEST) { \
		die(__VA_ARGS__); \
	}

extern uint8_t get_u8(uint8_t *bytes, uint32_t *p);
extern uint16_t get_u16(uint8_t *bytes, uint32_t *p);
extern uint32_t get_u32(uint8_t *bytes, uint32_t *p);
extern uint16_t get_u16_le(uint8_t *bytes, uint32_t *p);
extern uint32_t get_u32_le(uint8_t *bytes, uint32_t *p);

#define get_i8(BYTES, P) ((int8_t)get_u8(BYTES, P))
#define get_i16(BYTES, P) ((int16_t)get_u16(BYTES, P))
#define get_i16_le(BYTES, P) ((int16_t)get_u16_le(BYTES, P))
#define get_i32(BYTES, P) ((int32_t)get_u32(BYTES, P))
#define get_i32_le(BYTES, P) ((int32_t)get_u32_le(BYTES, P))

#endif