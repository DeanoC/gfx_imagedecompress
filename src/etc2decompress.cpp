

/*

Copyright (c) 2015 Harm Hanemaaijer <fgenfb@yahoo.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*/

#include "al2o3_platform/platform.h"

extern const uint8_t detex_clamp0to255_table[767];

/* Clamp an integer value in the range -255 to 511 to the the range 0 to 255. */
static AL2O3_FORCE_INLINE uint8_t detexClamp0To255(int x) {
	return detex_clamp0to255_table[x + 255];
}

static AL2O3_FORCE_INLINE uint32_t detexPack32B8(int b) {
	return (uint32_t) b;
}

static AL2O3_FORCE_INLINE uint32_t detexPack32G8(int g) {
	return (uint32_t) g << 8;
}

static AL2O3_FORCE_INLINE uint32_t detexPack32R8(int r) {
	return (uint32_t) r << 16;
}

static AL2O3_FORCE_INLINE uint32_t detexPack32A8(int a) {
	return (uint32_t) a << 24;
}

static AL2O3_FORCE_INLINE uint32_t detexPack32RGBA8(int r, int g, int b, int a) {
	return detexPack32R8(r) | detexPack32G8(g) | detexPack32B8(b) | detexPack32A8(a);
}

static AL2O3_FORCE_INLINE uint32_t detexPack32RGB8Alpha0xFF(int r, int g, int b) {
	return detexPack32RGBA8(r, g, b, 0xFF);
}

static const int complement3bitshifted_table[8] = {
		0, 8, 16, 24, -32, -24, -16, -8
};

static const int modifier_table[8][4] = {
		{ 2, 8, -2, -8 },
		{ 5, 17, -5, -17 },
		{ 9, 29, -9, -29 },
		{ 13, 42, -13, -42 },
		{ 18, 60, -18, -60 },
		{ 24, 80, -24, -80 },
		{ 33, 106, -33, -106 },
		{ 47, 183, -47, -183 }
};


// This function calculates the 3-bit complement value in the range -4 to 3 of a three bit
// representation. The result is arithmetically shifted 3 places to the left before returning.
static AL2O3_FORCE_INLINE int complement3bitshifted(int x) {
	return complement3bitshifted_table[x];
}


// Define inline function to speed up ETC1 decoding.

static AL2O3_FORCE_INLINE void ProcessPixelETC1(uint8_t i, uint32_t pixel_index_word,
																							 uint32_t table_codeword, int * AL2O3_RESTRICT base_color_subblock,
																							 uint8_t * AL2O3_RESTRICT pixel_buffer) {
	int pixel_index = ((pixel_index_word & (1 << i)) >> i)
			| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1));
	int r, g, b;
	int modifier = modifier_table[table_codeword][pixel_index];
	r = detexClamp0To255(base_color_subblock[0] + modifier);
	g = detexClamp0To255(base_color_subblock[1] + modifier);
	b = detexClamp0To255(base_color_subblock[2] + modifier);
	uint32_t *buffer = (uint32_t *)pixel_buffer;
	buffer[(i & 3) * 4 + ((i & 12) >> 2)] =
			detexPack32RGB8Alpha0xFF(r, g, b);
}

/* Decompress a 64-bit 4x4 pixel texture block compressed using the ETC1 */
/* format. */
bool detexDecompressBlockETC1(const uint8_t * AL2O3_RESTRICT bitstring, uint8_t * AL2O3_RESTRICT pixel_buffer) {
	int differential_mode = bitstring[3] & 2;
	int flipbit = bitstring[3] & 1;
	int base_color_subblock1[3];
	int base_color_subblock2[3];
	if (differential_mode) {
		base_color_subblock1[0] = (bitstring[0] & 0xF8);
		base_color_subblock1[0] |= ((base_color_subblock1[0] & 224) >> 5);
		base_color_subblock1[1] = (bitstring[1] & 0xF8);
		base_color_subblock1[1] |= (base_color_subblock1[1] & 224) >> 5;
		base_color_subblock1[2] = (bitstring[2] & 0xF8);
		base_color_subblock1[2] |= (base_color_subblock1[2] & 224) >> 5;
		base_color_subblock2[0] = (bitstring[0] & 0xF8);			// 5 highest order bits.
		base_color_subblock2[0] += complement3bitshifted(bitstring[0] & 7);	// Add difference.
		if (base_color_subblock2[0] & 0xFF07)					// Check for overflow.
			return false;
		base_color_subblock2[0] |= (base_color_subblock2[0] & 224) >> 5;	// Replicate.
		base_color_subblock2[1] = (bitstring[1] & 0xF8);
		base_color_subblock2[1] += complement3bitshifted(bitstring[1] & 7);
		if (base_color_subblock2[1] & 0xFF07)
			return false;
		base_color_subblock2[1] |= (base_color_subblock2[1] & 224) >> 5;
		base_color_subblock2[2] = (bitstring[2] & 0xF8);
		base_color_subblock2[2] += complement3bitshifted(bitstring[2] & 7);
		if (base_color_subblock2[2] & 0xFF07)
			return false;
		base_color_subblock2[2] |= (base_color_subblock2[2] & 224) >> 5;
	}
	else {
		base_color_subblock1[0] = (bitstring[0] & 0xF0);
		base_color_subblock1[0] |= base_color_subblock1[0] >> 4;
		base_color_subblock1[1] = (bitstring[1] & 0xF0);
		base_color_subblock1[1] |= base_color_subblock1[1] >> 4;
		base_color_subblock1[2] = (bitstring[2] & 0xF0);
		base_color_subblock1[2] |= base_color_subblock1[2] >> 4;
		base_color_subblock2[0] = (bitstring[0] & 0x0F);
		base_color_subblock2[0] |= base_color_subblock2[0] << 4;
		base_color_subblock2[1] = (bitstring[1] & 0x0F);
		base_color_subblock2[1] |= base_color_subblock2[1] << 4;
		base_color_subblock2[2] = (bitstring[2] & 0x0F);
		base_color_subblock2[2] |= base_color_subblock2[2] << 4;
	}
	uint32_t table_codeword1 = (bitstring[3] & 224) >> 5;
	uint32_t table_codeword2 = (bitstring[3] & 28) >> 2;
	uint32_t pixel_index_word = ((uint32_t)bitstring[4] << 24) | ((uint32_t)bitstring[5] << 16) |
			((uint32_t)bitstring[6] << 8) | bitstring[7];
	if (flipbit == 0) {
		ProcessPixelETC1(0, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(1, pixel_index_word, table_codeword1,base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(2, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(3, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(4, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(5, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(6, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(7, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(8, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC1(9, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC1(10, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC1(11, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC1(12, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC1(13, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC1(14, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC1(15, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
	}
	else {
		ProcessPixelETC1(0, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(1, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(2, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC1(3, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC1(4, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(5, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(6, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC1(7, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC1(8, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(9, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(10, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC1(11, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC1(12, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(13, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC1(14, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC1(15, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
	}
	return true;
}

static const int etc2_distance_table[8] = { 3, 6, 11, 16, 23, 32, 41, 64 };

static void ProcessBlockETC2TMode(const uint8_t * AL2O3_RESTRICT bitstring,
																		 uint8_t * AL2O3_RESTRICT pixel_buffer) {
	int base_color1_R, base_color1_G, base_color1_B;
	int base_color2_R, base_color2_G, base_color2_B;
	int paint_color_R[4], paint_color_G[4], paint_color_B[4];
	int distance;
	// T mode.
	base_color1_R = ((bitstring[0] & 0x18) >> 1) | (bitstring[0] & 0x3);
	base_color1_R |= base_color1_R << 4;
	base_color1_G = bitstring[1] & 0xF0;
	base_color1_G |= base_color1_G >> 4;
	base_color1_B = bitstring[1] & 0x0F;
	base_color1_B |= base_color1_B << 4;
	base_color2_R = bitstring[2] & 0xF0;
	base_color2_R |= base_color2_R >> 4;
	base_color2_G = bitstring[2] & 0x0F;
	base_color2_G |= base_color2_G << 4;
	base_color2_B = bitstring[3] & 0xF0;
	base_color2_B |= base_color2_B >> 4;
	// index = (da << 1) | db
	distance = etc2_distance_table[((bitstring[3] & 0x0C) >> 1) | (bitstring[3] & 0x1)];
	paint_color_R[0] = base_color1_R;
	paint_color_G[0] = base_color1_G;
	paint_color_B[0] = base_color1_B;
	paint_color_R[2] = base_color2_R;
	paint_color_G[2] = base_color2_G;
	paint_color_B[2] = base_color2_B;
	paint_color_R[1] = detexClamp0To255(base_color2_R + distance);
	paint_color_G[1] = detexClamp0To255(base_color2_G + distance);
	paint_color_B[1] = detexClamp0To255(base_color2_B + distance);
	paint_color_R[3] = detexClamp0To255(base_color2_R - distance);
	paint_color_G[3] = detexClamp0To255(base_color2_G - distance);
	paint_color_B[3] = detexClamp0To255(base_color2_B - distance);

	uint32_t pixel_index_word = ((uint32_t)bitstring[4] << 24) | ((uint32_t)bitstring[5] << 16) |
			((uint32_t)bitstring[6] << 8) | bitstring[7];
	uint32_t *buffer = (uint32_t *)pixel_buffer;
	for (int i = 0; i < 16; i++) {
		int pixel_index = ((pixel_index_word & (1 << i)) >> i)			// Least significant bit.
				| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1));	// Most significant bit.
		int r = paint_color_R[pixel_index];
		int g = paint_color_G[pixel_index];
		int b = paint_color_B[pixel_index];
		buffer[(i & 3) * 4 + ((i & 12) >> 2)] = detexPack32RGB8Alpha0xFF(r, g, b);
	}
}

static void ProcessBlockETC2HMode(const uint8_t * AL2O3_RESTRICT bitstring,
																		 uint8_t * AL2O3_RESTRICT pixel_buffer) {
	int base_color1_R, base_color1_G, base_color1_B;
	int base_color2_R, base_color2_G, base_color2_B;
	int paint_color_R[4], paint_color_G[4], paint_color_B[4];
	int distance;
	// H mode.
	base_color1_R = (bitstring[0] & 0x78) >> 3;
	base_color1_R |= base_color1_R << 4;
	base_color1_G = ((bitstring[0] & 0x07) << 1) | ((bitstring[1] & 0x10) >> 4);
	base_color1_G |= base_color1_G << 4;
	base_color1_B = (bitstring[1] & 0x08) | ((bitstring[1] & 0x03) << 1) | ((bitstring[2] & 0x80) >> 7);
	base_color1_B |= base_color1_B << 4;
	base_color2_R = (bitstring[2] & 0x78) >> 3;
	base_color2_R |= base_color2_R << 4;
	base_color2_G = ((bitstring[2] & 0x07) << 1) | ((bitstring[3] & 0x80) >> 7);
	base_color2_G |= base_color2_G << 4;
	base_color2_B = (bitstring[3] & 0x78) >> 3;
	base_color2_B |= base_color2_B << 4;
	// da is most significant bit, db is middle bit, least significant bit is
	// (base_color1 value >= base_color2 value).
	int base_color1_value = (base_color1_R << 16) + (base_color1_G << 8) + base_color1_B;
	int base_color2_value = (base_color2_R << 16) + (base_color2_G << 8) + base_color2_B;
	int bit;
	if (base_color1_value >= base_color2_value)
		bit = 1;
	else
		bit = 0;
	distance = etc2_distance_table[(bitstring[3] & 0x04) | ((bitstring[3] & 0x01) << 1) | bit];
	paint_color_R[0] = detexClamp0To255(base_color1_R + distance);
	paint_color_G[0] = detexClamp0To255(base_color1_G + distance);
	paint_color_B[0] = detexClamp0To255(base_color1_B + distance);
	paint_color_R[1] = detexClamp0To255(base_color1_R - distance);
	paint_color_G[1] = detexClamp0To255(base_color1_G - distance);
	paint_color_B[1] = detexClamp0To255(base_color1_B - distance);
	paint_color_R[2] = detexClamp0To255(base_color2_R + distance);
	paint_color_G[2] = detexClamp0To255(base_color2_G + distance);
	paint_color_B[2] = detexClamp0To255(base_color2_B + distance);
	paint_color_R[3] = detexClamp0To255(base_color2_R - distance);
	paint_color_G[3] = detexClamp0To255(base_color2_G - distance);
	paint_color_B[3] = detexClamp0To255(base_color2_B - distance);

	uint32_t pixel_index_word = ((uint32_t)bitstring[4] << 24) | ((uint32_t)bitstring[5] << 16) |
			((uint32_t)bitstring[6] << 8) | bitstring[7];
	uint32_t *buffer = (uint32_t *)pixel_buffer;
	for (int i = 0; i < 16; i++) {
		int pixel_index = ((pixel_index_word & (1 << i)) >> i)			// Least significant bit.
				| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1));	// Most significant bit.
		int r = paint_color_R[pixel_index];
		int g = paint_color_G[pixel_index];
		int b = paint_color_B[pixel_index];
		buffer[(i & 3) * 4 + ((i & 12) >> 2)] = detexPack32RGB8Alpha0xFF(r, g, b);
	}
}

static void ProcessBlockETC2PlanarMode(const uint8_t * AL2O3_RESTRICT bitstring,
																			 uint8_t * AL2O3_RESTRICT pixel_buffer) {
	// Each color O, H and V is in 6-7-6 format.
	int RO = (bitstring[0] & 0x7E) >> 1;
	int GO = ((bitstring[0] & 0x1) << 6) | ((bitstring[1] & 0x7E) >> 1);
	int BO = ((bitstring[1] & 0x1) << 5) | (bitstring[2] & 0x18) | ((bitstring[2] & 0x03) << 1) |
			((bitstring[3] & 0x80) >> 7);
	int RH = ((bitstring[3] & 0x7C) >> 1) | (bitstring[3] & 0x1);
	int GH = (bitstring[4] & 0xFE) >> 1;
	int BH = ((bitstring[4] & 0x1) << 5) | ((bitstring[5] & 0xF8) >> 3);
	int RV = ((bitstring[5] & 0x7) << 3) | ((bitstring[6] & 0xE0) >> 5);
	int GV = ((bitstring[6] & 0x1F) << 2) | ((bitstring[7] & 0xC0) >> 6);
	int BV = bitstring[7] & 0x3F;
	RO = (RO << 2) | ((RO & 0x30) >> 4);	// Replicate bits.
	GO = (GO << 1) | ((GO & 0x40) >> 6);
	BO = (BO << 2) | ((BO & 0x30) >> 4);
	RH = (RH << 2) | ((RH & 0x30) >> 4);
	GH = (GH << 1) | ((GH & 0x40) >> 6);
	BH = (BH << 2) | ((BH & 0x30) >> 4);
	RV = (RV << 2) | ((RV & 0x30) >> 4);
	GV = (GV << 1) | ((GV & 0x40) >> 6);
	BV = (BV << 2) | ((BV & 0x30) >> 4);
	uint32_t *buffer = (uint32_t *)pixel_buffer;
	for (int y = 0; y < 4; y++)
		for (int x = 0; x < 4; x++) {
			int r = detexClamp0To255((x * (RH - RO) + y * (RV - RO) + 4 * RO + 2) >> 2);
			int g = detexClamp0To255((x * (GH - GO) + y * (GV - GO) + 4 * GO + 2) >> 2);
			int b = detexClamp0To255((x * (BH - BO) + y * (BV - BO) + 4 * BO + 2) >> 2);
			buffer[y * 4 + x] = detexPack32RGB8Alpha0xFF(r, g, b);
		}
}

/* Decompress a 64-bit 4x4 pixel texture block compressed using the ETC2 */
/* format. */
bool detexDecompressBlockETC2(const uint8_t * AL2O3_RESTRICT bitstring, uint8_t * AL2O3_RESTRICT pixel_buffer) {
	// Figure out the mode.
	if ((bitstring[3] & 2) == 0) {
		// Individual mode.
		return detexDecompressBlockETC1(bitstring, pixel_buffer);
	}
	int R = (bitstring[0] & 0xF8);
	R += complement3bitshifted(bitstring[0] & 7);
	int G = (bitstring[1] & 0xF8);
	G += complement3bitshifted(bitstring[1] & 7);
	int B = (bitstring[2] & 0xF8);
	B += complement3bitshifted(bitstring[2] & 7);
	if (R & 0xFF07) {
		// T mode.
		ProcessBlockETC2TMode(bitstring, pixel_buffer);
		return true;
	}
	else
	if (G & 0xFF07) {
		// H mode.
		ProcessBlockETC2HMode(bitstring, pixel_buffer);
		return true;
	}
	else
	if (B & 0xFF07) {
		// Planar mode.
		ProcessBlockETC2PlanarMode(bitstring, pixel_buffer);
		return true;
	}
	else {
		// Differential mode.
		return detexDecompressBlockETC1(bitstring,	pixel_buffer);
	}
}

static const int punchthrough_modifier_table[8][4] = {
		{ 0, 8, 0, -8 },
		{ 0, 17, 0, -17 },
		{ 0, 29, 0, -29 },
		{ 0, 42, 0, -42 },
		{ 0, 60, 0, -60 },
		{ 0, 80, 0, -80 },
		{ 0, 106, 0, -106 },
		{ 0, 183, 0, -183 }
};

static const uint32_t punchthrough_mask_table[4] = {
		0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF };

static AL2O3_FORCE_INLINE void ProcessPixelETC2Punchthrough(uint8_t i,
																													 uint32_t pixel_index_word, uint32_t table_codeword,
																													 int * AL2O3_RESTRICT base_color_subblock, uint8_t * AL2O3_RESTRICT pixel_buffer) {
	int pixel_index = ((pixel_index_word & (1 << i)) >> i)
			| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1));
	int r, g, b;
	int modifier = punchthrough_modifier_table[table_codeword][pixel_index];
	r = detexClamp0To255(base_color_subblock[0] + modifier);
	g = detexClamp0To255(base_color_subblock[1] + modifier);
	b = detexClamp0To255(base_color_subblock[2] + modifier);
	uint32_t mask = punchthrough_mask_table[pixel_index];
	uint32_t *buffer = (uint32_t *)pixel_buffer;
	buffer[(i & 3) * 4 + ((i & 12) >> 2)] =
			detexPack32RGB8Alpha0xFF(r, g, b) & mask;
}


void ProcessBlockETC2PunchthroughDifferentialMode(const uint8_t * AL2O3_RESTRICT bitstring,
																									uint8_t * AL2O3_RESTRICT pixel_buffer) {
	int flipbit = bitstring[3] & 1;
	int base_color_subblock1[3];
	int base_color_subblock2[3];
	base_color_subblock1[0] = (bitstring[0] & 0xF8);
	base_color_subblock1[0] |= ((base_color_subblock1[0] & 224) >> 5);
	base_color_subblock1[1] = (bitstring[1] & 0xF8);
	base_color_subblock1[1] |= (base_color_subblock1[1] & 224) >> 5;
	base_color_subblock1[2] = (bitstring[2] & 0xF8);
	base_color_subblock1[2] |= (base_color_subblock1[2] & 224) >> 5;
	base_color_subblock2[0] = (bitstring[0] & 0xF8);				// 5 highest order bits.
	base_color_subblock2[0] += complement3bitshifted(bitstring[0] & 7);	// Add difference.
	base_color_subblock2[0] |= (base_color_subblock2[0] & 224) >> 5;		// Replicate.
	base_color_subblock2[1] = (bitstring[1] & 0xF8);
	base_color_subblock2[1] += complement3bitshifted(bitstring[1] & 7);
	base_color_subblock2[1] |= (base_color_subblock2[1] & 224) >> 5;
	base_color_subblock2[2] = (bitstring[2] & 0xF8);
	base_color_subblock2[2] += complement3bitshifted(bitstring[2] & 7);
	base_color_subblock2[2] |= (base_color_subblock2[2] & 224) >> 5;
	uint32_t table_codeword1 = (bitstring[3] & 224) >> 5;
	uint32_t table_codeword2 = (bitstring[3] & 28) >> 2;
	uint32_t pixel_index_word = ((uint32_t)bitstring[4] << 24) | ((uint32_t)bitstring[5] << 16) |
			((uint32_t)bitstring[6] << 8) | bitstring[7];
	if (flipbit == 0) {
		ProcessPixelETC2Punchthrough(0, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(1, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(2, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(3, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(4, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(5, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(6, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(7, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(8, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC2Punchthrough(9, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC2Punchthrough(10, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC2Punchthrough(11, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC2Punchthrough(12, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC2Punchthrough(13, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC2Punchthrough(14, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC2Punchthrough(15, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
	}
	else {
		ProcessPixelETC2Punchthrough(0, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(1, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(2, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC2Punchthrough(3, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC2Punchthrough(4, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(5, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(6, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC2Punchthrough(7, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC2Punchthrough(8, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(9, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(10, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC2Punchthrough(11, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC2Punchthrough(12, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(13, pixel_index_word, table_codeword1, base_color_subblock1, pixel_buffer);
		ProcessPixelETC2Punchthrough(14, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
		ProcessPixelETC2Punchthrough(15, pixel_index_word, table_codeword2, base_color_subblock2, pixel_buffer);
	}
}

static void ProcessBlockETC2PunchthroughTMode(const uint8_t * AL2O3_RESTRICT bitstring,
																								 uint8_t * AL2O3_RESTRICT pixel_buffer) {
	int base_color1_R, base_color1_G, base_color1_B;
	int base_color2_R, base_color2_G, base_color2_B;
	int paint_color_R[4], paint_color_G[4], paint_color_B[4];
	int distance;
	// T mode.
	base_color1_R = ((bitstring[0] & 0x18) >> 1) | (bitstring[0] & 0x3);
	base_color1_R |= base_color1_R << 4;
	base_color1_G = bitstring[1] & 0xF0;
	base_color1_G |= base_color1_G >> 4;
	base_color1_B = bitstring[1] & 0x0F;
	base_color1_B |= base_color1_B << 4;
	base_color2_R = bitstring[2] & 0xF0;
	base_color2_R |= base_color2_R >> 4;
	base_color2_G = bitstring[2] & 0x0F;
	base_color2_G |= base_color2_G << 4;
	base_color2_B = bitstring[3] & 0xF0;
	base_color2_B |= base_color2_B >> 4;
	// index = (da << 1) | db
	distance = etc2_distance_table[((bitstring[3] & 0x0C) >> 1) | (bitstring[3] & 0x1)];
	paint_color_R[0] = base_color1_R;
	paint_color_G[0] = base_color1_G;
	paint_color_B[0] = base_color1_B;
	paint_color_R[2] = base_color2_R;
	paint_color_G[2] = base_color2_G;
	paint_color_B[2] = base_color2_B;
	paint_color_R[1] = detexClamp0To255(base_color2_R + distance);
	paint_color_G[1] = detexClamp0To255(base_color2_G + distance);
	paint_color_B[1] = detexClamp0To255(base_color2_B + distance);
	paint_color_R[3] = detexClamp0To255(base_color2_R - distance);
	paint_color_G[3] = detexClamp0To255(base_color2_G - distance);
	paint_color_B[3] = detexClamp0To255(base_color2_B - distance);
	uint32_t pixel_index_word = ((uint32_t)bitstring[4] << 24) | ((uint32_t)bitstring[5] << 16) |
			((uint32_t)bitstring[6] << 8) | bitstring[7];
	uint32_t *buffer = (uint32_t *)pixel_buffer;
	for (int i = 0; i < 16; i++) {
		int pixel_index = ((pixel_index_word & (1 << i)) >> i)			// Least significant bit.
				| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1));	// Most significant bit.
		int r = paint_color_R[pixel_index];
		int g = paint_color_G[pixel_index];
		int b = paint_color_B[pixel_index];
		uint32_t mask = punchthrough_mask_table[pixel_index];
		buffer[(i & 3) * 4 + ((i & 12) >> 2)] = (detexPack32RGB8Alpha0xFF(r, g, b)) & mask;
	}
}
static void ProcessBlockETC2PunchthroughHMode(const uint8_t * AL2O3_RESTRICT bitstring,
																								 uint8_t * AL2O3_RESTRICT pixel_buffer) {
	int base_color1_R, base_color1_G, base_color1_B;
	int base_color2_R, base_color2_G, base_color2_B;
	int paint_color_R[4], paint_color_G[4], paint_color_B[4];
	int distance;
	// H mode.
	base_color1_R = (bitstring[0] & 0x78) >> 3;
	base_color1_R |= base_color1_R << 4;
	base_color1_G = ((bitstring[0] & 0x07) << 1) | ((bitstring[1] & 0x10) >> 4);
	base_color1_G |= base_color1_G << 4;
	base_color1_B = (bitstring[1] & 0x08) | ((bitstring[1] & 0x03) << 1) | ((bitstring[2] & 0x80) >> 7);
	base_color1_B |= base_color1_B << 4;
	base_color2_R = (bitstring[2] & 0x78) >> 3;
	base_color2_R |= base_color2_R << 4;
	base_color2_G = ((bitstring[2] & 0x07) << 1) | ((bitstring[3] & 0x80) >> 7);
	base_color2_G |= base_color2_G << 4;
	base_color2_B = (bitstring[3] & 0x78) >> 3;
	base_color2_B |= base_color2_B << 4;
	// da is most significant bit, db is middle bit, least significant bit is
	// (base_color1 value >= base_color2 value).
	int base_color1_value = (base_color1_R << 16) + (base_color1_G << 8) + base_color1_B;
	int base_color2_value = (base_color2_R << 16) + (base_color2_G << 8) + base_color2_B;
	int bit;
	if (base_color1_value >= base_color2_value)
		bit = 1;
	else
		bit = 0;
	distance = etc2_distance_table[(bitstring[3] & 0x04) | ((bitstring[3] & 0x01) << 1) | bit];
	paint_color_R[0] = detexClamp0To255(base_color1_R + distance);
	paint_color_G[0] = detexClamp0To255(base_color1_G + distance);
	paint_color_B[0] = detexClamp0To255(base_color1_B + distance);
	paint_color_R[1] = detexClamp0To255(base_color1_R - distance);
	paint_color_G[1] = detexClamp0To255(base_color1_G - distance);
	paint_color_B[1] = detexClamp0To255(base_color1_B - distance);
	paint_color_R[2] = detexClamp0To255(base_color2_R + distance);
	paint_color_G[2] = detexClamp0To255(base_color2_G + distance);
	paint_color_B[2] = detexClamp0To255(base_color2_B + distance);
	paint_color_R[3] = detexClamp0To255(base_color2_R - distance);
	paint_color_G[3] = detexClamp0To255(base_color2_G - distance);
	paint_color_B[3] = detexClamp0To255(base_color2_B - distance);
	uint32_t pixel_index_word = ((uint32_t)bitstring[4] << 24) | ((uint32_t)bitstring[5] << 16) |
			((uint32_t)bitstring[6] << 8) | bitstring[7];
	uint32_t *buffer = (uint32_t *)pixel_buffer;
	for (int i = 0; i < 16; i++) {
		int pixel_index = ((pixel_index_word & (1 << i)) >> i)			// Least significant bit.
				| ((pixel_index_word & (0x10000 << i)) >> (16 + i - 1));	// Most significant bit.
		int r = paint_color_R[pixel_index];
		int g = paint_color_G[pixel_index];
		int b = paint_color_B[pixel_index];
		uint32_t mask = punchthrough_mask_table[pixel_index];
		buffer[(i & 3) * 4 + ((i & 12) >> 2)] = (detexPack32RGB8Alpha0xFF(r, g, b)) & mask;
	}
}

/* Decompress a 64-bit 4x4 pixel texture block compressed using the */
/* ETC2_PUNCHTROUGH format. */
bool detexDecompressBlockETC2_PUNCHTHROUGH(const uint8_t * AL2O3_RESTRICT bitstring, uint8_t * AL2O3_RESTRICT pixel_buffer) {
	int R = (bitstring[0] & 0xF8);
	R += complement3bitshifted(bitstring[0] & 7);
	int G = (bitstring[1] & 0xF8);
	G += complement3bitshifted(bitstring[1] & 7);
	int B = (bitstring[2] & 0xF8);
	B += complement3bitshifted(bitstring[2] & 7);
	int opaque = bitstring[3] & 2;
	if (R & 0xFF07) {
		// T mode.
		if (opaque) {
			ProcessBlockETC2TMode(bitstring, pixel_buffer);
			return true;
		}
		// T mode with punchthrough alpha.
		ProcessBlockETC2PunchthroughTMode(bitstring, pixel_buffer);
		return true;
	}
	else
	if (G & 0xFF07) {
		// H mode.
		if (opaque) {
			ProcessBlockETC2HMode(bitstring, pixel_buffer);
			return true;
		}
		// H mode with punchthrough alpha.
		ProcessBlockETC2PunchthroughHMode(bitstring, pixel_buffer);
		return true;
	}
	else
	if (B & 0xFF07) {
		// Planar mode.
		ProcessBlockETC2PlanarMode(bitstring, pixel_buffer);
		return true;
	}
	else {
		// Differential mode.
		if (opaque)
			return detexDecompressBlockETC1(bitstring, pixel_buffer);
		// Differential mode with punchthrough alpha.
		ProcessBlockETC2PunchthroughDifferentialMode(bitstring, pixel_buffer);
		return true;
	}
}

AL2O3_EXTERN_C void Image_DecompressETC2PunchThroughBlock(void const AL2O3_RESTRICT* input, uint8_t output[4 * 4 * sizeof(uint32_t)]) {
	detexDecompressBlockETC2_PUNCHTHROUGH((uint8_t const*) input, output);
}

AL2O3_EXTERN_C void Image_DecompressETC2Block(void const AL2O3_RESTRICT* input, uint8_t output[4 * 4 * sizeof(uint32_t)]) {
	detexDecompressBlockETC2((uint8_t const*)input, output);
}