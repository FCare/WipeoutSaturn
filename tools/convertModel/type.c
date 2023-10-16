#include "type.h"

uint8_t get_u8(uint8_t *bytes, uint32_t *p) {
	return bytes[(*p)++];
}

uint16_t get_u16(uint8_t *bytes, uint32_t *p) {
	uint16_t v = 0;
	v |= bytes[(*p)++] << 8;
	v |= bytes[(*p)++] << 0;
	return v;
}

uint32_t get_u32(uint8_t *bytes, uint32_t *p) {
	uint32_t v = 0;
	v |= bytes[(*p)++] << 24;
	v |= bytes[(*p)++] << 16;
	v |= bytes[(*p)++] <<  8;
	v |= bytes[(*p)++] <<  0;
	return v;
}

uint16_t get_u16_le(uint8_t *bytes, uint32_t *p) {
	uint16_t v = 0;
	v |= bytes[(*p)++] << 0;
	v |= bytes[(*p)++] << 8;
	return v;
}

uint32_t get_u32_le(uint8_t *bytes, uint32_t *p) {
	uint32_t v = 0;
	v |= bytes[(*p)++] <<  0;
	v |= bytes[(*p)++] <<  8;
	v |= bytes[(*p)++] << 16;
	v |= bytes[(*p)++] << 24;
	return v;
}

rgb1555_t rgb155_from_u32(uint32_t v) {
	rgba_t val;
	val.a = v & 0xFF;
	val.b = (v>>8) & 0xFF;
	val.g = (v>>16) & 0xFF;
	val.r = (v>24) & 0xFF;
	printf("%x %x %x %x\n", val.a, val.b, val.g, val.r);
  return convert_to_rgb(val);
}