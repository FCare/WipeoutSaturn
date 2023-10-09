#include "lzss.h"

#define LZSS_INDEX_BIT_COUNT  13
#define LZSS_LENGTH_BIT_COUNT 4
#define LZSS_WINDOW_SIZE      (1 << LZSS_INDEX_BIT_COUNT)
#define LZSS_BREAK_EVEN       ((1 + LZSS_INDEX_BIT_COUNT + LZSS_LENGTH_BIT_COUNT) / 9)
#define LZSS_END_OF_STREAM    0
#define LZSS_MOD_WINDOW(a)    ((a) & (LZSS_WINDOW_SIZE - 1))

void lzss_decompress(uint8_t *in_data, uint8_t *out_data) {
	int16_t i;
	int16_t current_position;
	uint8_t cc;
	int16_t match_length;
	int16_t match_position;
	uint32_t mask;
	uint32_t return_value;
	uint8_t in_bfile_mask;
	int16_t in_bfile_rack;
	int16_t value;
	uint8_t window[LZSS_WINDOW_SIZE];

	in_bfile_rack = 0;
	in_bfile_mask = 0x80;

	current_position = 1;
	while (true) {
		if (in_bfile_mask == 0x80) {
			in_bfile_rack = (int16_t) * in_data++;
		}

		value = in_bfile_rack & in_bfile_mask;
		in_bfile_mask >>= 1;
		if (in_bfile_mask == 0) {
			in_bfile_mask = 0x80;
		}

		if (value) {
			mask = 1L << (8 - 1);
			return_value = 0;
			while (mask != 0) {
				if (in_bfile_mask == 0x80) {
					in_bfile_rack = (int16_t) * in_data++;
				}

				if (in_bfile_rack & in_bfile_mask) {
					return_value |= mask;
				}
				mask >>= 1;
				in_bfile_mask >>= 1;

				if (in_bfile_mask == 0) {
					in_bfile_mask = 0x80;
				}
			}
			cc = (uint8_t) return_value;
			*out_data++ = cc;
			window[ current_position ] = cc;
			current_position = LZSS_MOD_WINDOW(current_position + 1);
		}
		else {
			mask = 1L << (LZSS_INDEX_BIT_COUNT - 1);
			return_value = 0;
			while (mask != 0) {
				if (in_bfile_mask == 0x80) {
					in_bfile_rack = (int16_t) * in_data++;
				}

				if (in_bfile_rack & in_bfile_mask) {
					return_value |= mask;
				}
				mask >>= 1;
				in_bfile_mask >>= 1;

				if (in_bfile_mask == 0) {
					in_bfile_mask = 0x80;
				}
			}
			match_position = (int16_t) return_value;

			if (match_position == LZSS_END_OF_STREAM) {
				break;
			}

			mask = 1L << (LZSS_LENGTH_BIT_COUNT - 1);
			return_value = 0;
			while (mask != 0) {
				if (in_bfile_mask == 0x80) {
					in_bfile_rack = (int16_t) * in_data++;
				}

				if (in_bfile_rack & in_bfile_mask) {
					return_value |= mask;
				}
				mask >>= 1;
				in_bfile_mask >>= 1;

				if (in_bfile_mask == 0) {
					in_bfile_mask = 0x80;
				}
			}
			match_length = (int16_t) return_value;

			match_length += LZSS_BREAK_EVEN;

			for (i = 0 ; i <= match_length ; i++) {
				cc = window[LZSS_MOD_WINDOW(match_position + i)];
				*out_data++ = cc;
				window[current_position] = cc;
				current_position = LZSS_MOD_WINDOW(current_position + 1);
			}
		}
	}
}