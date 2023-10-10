#ifndef OBJECT_SATURN_H
#define OBJECT_SATURN_H

#include "../types.h"
#include "../render.h"
#include "../utils.h"
#include "image.h"

// Primitive Structure Stub ( Structure varies with primitive type )

typedef struct F3_S {
	int16_t type; // Type of primitive
	fix16_t coords[3]; // Indices of the coords
	rgba_t color;
} F3_S;

typedef struct FT3_S {
	int16_t type; // Type of primitive
	fix16_t coords[3]; // Indices of the coords
	int16_t texture;
	rgba_t color;
} FT3_S;

typedef struct F4_S {
	int16_t type; // Type of primitive
	fix16_t coords[4]; // Indices of the coords
	rgba_t color;
} F4_S;

typedef struct FT4_S {
	int16_t type; // Type of primitive
	fix16_t coords[4]; // Indices of the coords
	int16_t texture;
	rgba_t color;
} FT4_S;

typedef struct G3_S {
	int16_t type; // Type of primitive
	fix16_t coords[3]; // Indices of the coords
	rgba_t color[3];
} G3_S;

typedef struct GT3_S {
	int16_t type; // Type of primitive
	fix16_t coords[3]; // Indices of the coords
	int16_t texture;
	rgba_t color[3];
} GT3_S;

typedef struct G4_S {
	int16_t type; // Type of primitive
	fix16_t coords[4]; // Indices of the coords
	rgba_t color[4];
} G4_S;

typedef struct GT4_S {
	int16_t type; // Type of primitive
	fix16_t coords[4]; // Indices of the coords
	int16_t texture;
	rgba_t color[4];
} GT4_S;

/* LIGHT SOURCED POLYGONS
*/

typedef struct LSF3_S {
	int16_t type; // Type of primitive
	fix16_t coords[3]; // Indices of the coords
	fix16_t normal; // Indices of the normals
	rgba_t color;
} LSF3_S;

typedef struct LSFT3_S {
	int16_t type; // Type of primitive
	fix16_t coords[3]; // Indices of the coords
	fix16_t normal; // Indices of the normals
	int16_t texture;
	rgba_t color;
} LSFT3_S;

typedef struct LSF4_S {
	int16_t type; // Type of primitive
	fix16_t coords[4]; // Indices of the coords
	fix16_t normal; // Indices of the normals
	rgba_t color;
} LSF4;

typedef struct LSFT4_S {
	int16_t type; // Type of primitive
	fix16_t coords[4]; // Indices of the coords
	fix16_t normal; // Indices of the normals
	int16_t texture;
	rgba_t color;
} LSFT4_S;

typedef struct LSG3_S {
	int16_t type; // Type of primitive
	fix16_t coords[3]; // Indices of the coords
	fix16_t normals[3]; // Indices of the normals
	rgba_t color[3];
} LSG3_S;

typedef struct LSGT3_S {
	int16_t type; // Type of primitive
	fix16_t coords[3]; // Indices of the coords
	fix16_t normals[3]; // Indices of the normals
	int16_t texture;
	rgba_t color[3];
} LSGT3_S;

typedef struct LSG4_S {
	int16_t type; // Type of primitive
	fix16_t coords[4]; // Indices of the coords
	fix16_t normals[4]; // Indices of the normals
	rgba_t color[4];
} LSG4_S;

typedef struct LSGT4_S {
	int16_t type; // Type of primitive
	fix16_t coords[4]; // Indices of the coords
	fix16_t normals[4]; // Indices of the normals
	int16_t texture;
	rgba_t color[4];
} LSGT4_S;

/* OTHER PRIMITIVE TYPES
*/
typedef struct SPR_S {
	int16_t type;
	fix16_t coord;
	int16_t width;
	int16_t height;
	int16_t texture;
	rgba_t color;
} SPR;


typedef struct Spline_S {
	int16_t type; // Type of primitive
	fix16_vec3_t control1;
	fix16_vec3_t position;
	fix16_vec3_t control2;
	rgba_t color;
} Spline_S;


typedef struct PointLight_S {
	int16_t type;
	fix16_vec3_t position;
	rgba_t color;
	int16_t startFalloff;
	int16_t endFalloff;
} PointLight_S;


typedef struct SpotLight_S {
	int16_t type;
	fix16_vec3_t position;
	fix16_vec3_t direction;
	rgba_t color;
	int16_t startFalloff;
	int16_t endFalloff;
	int16_t coneAngle;
	int16_t spreadAngle;
} SpotLight_S;


typedef struct InfiniteLight_S {
	int16_t type;
	fix16_vec3_t direction;
	rgba_t color;
} InfiniteLight_S;


#define PRM_TYPE_F3               1
#define PRM_TYPE_FT3              2
#define PRM_TYPE_F4               3
#define PRM_TYPE_FT4              4
#define PRM_TYPE_G3               5
#define PRM_TYPE_GT3              6
#define PRM_TYPE_G4               7
#define PRM_TYPE_GT4              8

#define PRM_TYPE_LF2              9
#define PRM_TYPE_TSPR             10
#define PRM_TYPE_BSPR             11

#define PRM_TYPE_LSF3             12
#define PRM_TYPE_LSFT3            13
#define PRM_TYPE_LSF4             14
#define PRM_TYPE_LSFT4            15
#define PRM_TYPE_LSG3             16
#define PRM_TYPE_LSGT3            17
#define PRM_TYPE_LSG4             18
#define PRM_TYPE_LSGT4            19

#define PRM_TYPE_SPLINE           20

#define PRM_TYPE_INFINITE_LIGHT    21
#define PRM_TYPE_POINT_LIGHT       22
#define PRM_TYPE_SPOT_LIGHT        23


typedef union Prm {
	F3_S               *f3;
	FT3_S              *ft3;
	F4_S               *f4;
	FT4_S              *ft4;
	G3_S               *g3;
	GT3_S              *gt3;
	G4_S               *g4;
	GT4_S              *gt4;
	SPR_S              *spr;
	Spline_S           *spline;
	PointLight_S       *pointLight;
	SpotLight_S        *spotLight;
	InfiniteLight_S    *infiniteLight;

	LSF3_S             *lsf3;
	LSFT3_S            *lsft3;
	LSF4_S             *lsf4;
	LSFT4_S            *lsft4;
	LSG3_S             *lsg3;
	LSGT3_S            *lsgt3;
	LSG4_S             *lsg4;
	LSGT4_S            *lsgt4;
} PRM_saturn;

typedef struct {
	char name[16];
	uint16_t vertices_len;
	uint16_t normals_len;
	uint16_t primitives_len;
	uint16_t flags;
	fix16_vec3_t origin;
	PRM_saturn *primitives
	fix16_t *vertices;
	fix16_t *normals;
} Object_Saturn;


#endif