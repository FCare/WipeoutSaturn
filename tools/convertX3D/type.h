#ifndef __TYPE_H__
#define __TYPE_H__

#define FIX16(x) ((uint32_t)(((x) >= 0)                                         \
    ? ((x) * 65536.0f + 0.5f)                                                  \
    : ((x) * 65536.0f - 0.5f)))

#define true 1
#define false 0

typedef int (*step_func)(void);

typedef uint16_t rgb1555_t;

typedef struct rgba_t {
	uint8_t r, g, b, a;
} rgba_t;

typedef struct {
  //Only palette 4bpp for the moment
	int16_t width;
  int16_t height;
	rgb1555_t *pixels;
} render_texture_t;

typedef struct vec2 {
  int16_t x;
  int16_t y;
} vec2_t;

typedef struct {
	vec2_t uv[4];
} quads_t;

typedef struct {
  char name[32];
  uint32_t flag;
  uint32_t nbFaces;
  uint32_t facesOffset;
  uint32_t textureOffset;
}geometry;

typedef struct {
  char name[32];
  uint32_t vertexNb;
  uint32_t vertexOffset;
  uint32_t normalsOffset;
  uint32_t nbGeometry;
  uint32_t totalSectorNb;
  geometry geometry[32];
} model;

typedef struct{
  uint32_t vertex_id[4]; //A,B,C,D
  uint16_t RGB;
  uint16_t padding;
} face;

typedef struct{
  uint16_t w; //Shall be a multiple of 8
  uint16_t h;
  uint32_t offset; //Address in the full texture.
  uint32_t format;
} texture;


typedef struct {
	int16_t width;
  int16_t height;
	uint8_t *pixels;
} texture_t;

#endif