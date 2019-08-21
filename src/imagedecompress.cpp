//
// Created by deano on 8/1/2019.
//
#include "al2o3_platform/platform.h"

void GetCompressedAlphaRamp(uint8_t alpha[8]) {
	if (alpha[0] > alpha[1]) {
		// 8-alpha block:  derive the other six alphas.
		// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
		alpha[2] = static_cast<uint8_t>((6 * alpha[0] + 1 * alpha[1] + 3) / 7);    // bit code 010
		alpha[3] = static_cast<uint8_t>((5 * alpha[0] + 2 * alpha[1] + 3) / 7);    // bit code 011
		alpha[4] = static_cast<uint8_t>((4 * alpha[0] + 3 * alpha[1] + 3) / 7);    // bit code 100
		alpha[5] = static_cast<uint8_t>((3 * alpha[0] + 4 * alpha[1] + 3) / 7);    // bit code 101
		alpha[6] = static_cast<uint8_t>((2 * alpha[0] + 5 * alpha[1] + 3) / 7);    // bit code 110
		alpha[7] = static_cast<uint8_t>((1 * alpha[0] + 6 * alpha[1] + 3) / 7);    // bit code 111
	} else {
		// 6-alpha block.
		// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
		alpha[2] = static_cast<uint8_t>((4 * alpha[0] + 1 * alpha[1] + 2) / 5);  // Bit code 010
		alpha[3] = static_cast<uint8_t>((3 * alpha[0] + 2 * alpha[1] + 2) / 5);  // Bit code 011
		alpha[4] = static_cast<uint8_t>((2 * alpha[0] + 3 * alpha[1] + 2) / 5);  // Bit code 100
		alpha[5] = static_cast<uint8_t>((1 * alpha[0] + 4 * alpha[1] + 2) / 5);  // Bit code 101
		alpha[6] = 0;                                      // Bit code 110
		alpha[7] = 255;                                    // Bit code 111
	}
}
#define BLOCK_ALPHA_PIXEL_MASK 0x7
#define BLOCK_ALPHA_PIXEL_BPP 3
#define EXPLICIT_ALPHA_PIXEL_MASK 0xf
#define EXPLICIT_ALPHA_PIXEL_BPP 4

void DecompressDXTCAlphaBlock(uint64_t const compressedBlock, uint8_t *outRGBA, uint32_t pixelPitch) {
	uint8_t alpha[8];

	alpha[0] = (uint8_t) (compressedBlock & 0xff);
	alpha[1] = (uint8_t) ((compressedBlock >> 8) & 0xff);
	GetCompressedAlphaRamp(alpha);

	for (int i = 0; i < 4 * 4; i++) {
		uint32_t const index = (compressedBlock >> (16 + (i * BLOCK_ALPHA_PIXEL_BPP))) & BLOCK_ALPHA_PIXEL_MASK;
		*outRGBA = alpha[index];
		outRGBA += pixelPitch;
	}
}

void DecompressExplicitAlphaBlock(uint64_t const compressedBlock, uint8_t *outRGBA, uint32_t pixelPitch) {
	for (int i = 0; i < 4 * 4; i++) {
		uint8_t cAlpha = (uint8_t) ((compressedBlock >> (i * EXPLICIT_ALPHA_PIXEL_BPP)) & EXPLICIT_ALPHA_PIXEL_MASK);
		*outRGBA = (uint8_t) ((cAlpha << EXPLICIT_ALPHA_PIXEL_BPP) | cAlpha);
		outRGBA += pixelPitch;
	}
}

// This function decompresses a DXT colour block
// The block is decompressed to 8 bits per channel
void DecompressRGBBlock(uint64_t const compressedBlock, uint32_t *outRGBA, bool bBC1) {
	// 2 565 colours are in the 1st 32 bits
	uint32_t n0 = compressedBlock & 0xffff;
	uint32_t n1 = (compressedBlock >> 16) & 0xffff;
	uint32_t r0 = ((n0 & 0xf800) >> 8);
	uint32_t g0 = ((n0 & 0x07e0) >> 3);
	uint32_t b0 = ((n0 & 0x001f) << 3);
	uint32_t r1 = ((n1 & 0xf800) >> 8);
	uint32_t g1 = ((n1 & 0x07e0) >> 3);
	uint32_t b1 = ((n1 & 0x001f) << 3);

	// Apply the lower bit replication to give full dynamic range
	r0 += (r0 >> 5);
	g0 += (g0 >> 6);
	b0 += (b0 >> 5);
	r1 += (r1 >> 5);
	g1 += (g1 >> 6);
	b1 += (b1 >> 5);

	// compute the 4 colours to interpolate between
	uint32_t c[4];
	c[0] = 0xff000000 | (r0 << 16) | (g0 << 8) | b0;
	c[1] = 0xff000000 | (r1 << 16) | (g1 << 8) | b1;
	if (!bBC1 || n0 > n1) {
		c[2] = 0xff000000 | (((2 * r0 + r1 + 1) / 3) << 16) | (((2 * g0 + g1 + 1) / 3) << 8) | (((2 * b0 + b1 + 1) / 3));
		c[3] = 0xff000000 | (((2 * r1 + r0 + 1) / 3) << 16) | (((2 * g1 + g0 + 1) / 3) << 8) | (((2 * b1 + b0 + 1) / 3));
	} else {
		c[2] = 0xff000000 | (((r0 + r1) / 2) << 16) | (((g0 + g1) / 2) << 8) | (((b0 + b1) / 2));
		c[3] = 0x00000000;
	}

	for (int i = 0; i < 16; i++) {
		outRGBA[i] = c[(compressedBlock >> (32 + (2 * i))) & 3];
	}
}

void DecompressBC1Block(uint64_t const compressedBlock, uint32_t *outRGBA) {
	DecompressRGBBlock(compressedBlock, outRGBA, true);
}

void DecompressBC2Block(uint64_t const compressedBlock[2], uint32_t *outRGBA) {
	DecompressRGBBlock(compressedBlock[1], outRGBA, false);
	DecompressExplicitAlphaBlock(compressedBlock[0], ((uint8_t *) outRGBA) + 3, 4);
}

void DecompressBC3Block(uint64_t const compressedBlock[2], uint32_t *outRGBA) {
	DecompressRGBBlock(compressedBlock[1], outRGBA, false);
	DecompressDXTCAlphaBlock(compressedBlock[0], ((uint8_t *) outRGBA) + 3, 4);
}

void DecompressBC4Block(uint64_t const compressedBlock, uint8_t *outRGBA) {
	DecompressDXTCAlphaBlock(compressedBlock, outRGBA, 1);
}
void DecompressBC5Block(uint64_t const compressedBlock[2], uint8_t *outRGBA) {
	DecompressDXTCAlphaBlock(compressedBlock[0], outRGBA, 2);
	DecompressDXTCAlphaBlock(compressedBlock[1], outRGBA + 1, 2);
}

uint8_t interpolate(uint8_t e0, uint8_t e1, uint8_t index, uint8_t indexprecision) {

	const int32_t BC67_WEIGHT_MAX = 64;
	const uint32_t BC67_WEIGHT_SHIFT = 6;
	const int32_t BC67_WEIGHT_ROUND = 32;

	uint8_t const aWeight[4 + 8 + 16] = {
			0, 21, 43, 64,
			0, 9, 18, 27, 37, 46, 55, 64,
			0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64
	};
	uint8_t offset = 0;
	switch (indexprecision - 2) {
	default:
	case 0: offset = 0;
		ASSERT(index < 4);
		break;
	case 1: offset = 4;
		ASSERT(index < 8)
		break;
	case 2: offset = 12;
		ASSERT(index < 16)
		break;
	}
	auto w = (uint16_t const) aWeight[offset + index];
	return (uint8_t) ((uint32_t(e0) * uint32_t(BC67_WEIGHT_MAX - w) + uint32_t(e1) * uint32_t(w) + BC67_WEIGHT_ROUND)
			>> BC67_WEIGHT_SHIFT);
}

uint8_t GetAndShift128(uint64_t& v, uint64_t const v2, uint8_t const bitCount) {
	static uint8_t accum = 0;
	if(accum + bitCount <= 63) {
		uint8_t val = v & ((1 << bitCount)-1);
		v >>= bitCount;
		accum += bitCount;
		return v & ((1 << bitCount) - 1);
	} else {
		uint8_t const secondPart = (accum + bitCount) - 63;
		uint8_t const firstPart = bitCount - secondPart;
		uint8_t val1 = v & ((1 << firstPart)-1);
		uint8_t val2 = v2 & ((1 << secondPart)-1);
		v = v2 >> secondPart;
		accum = secondPart;
		return (val1 | (val2 << firstPart)) & ((1 << bitCount) - 1);
	}
}

void BC7Decode(uint64_t const compressedBlock[2], uint32_t *outRGBA) {

#define BC7_PART(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15) \
      (v0 << 0)  | (v1 << 2)  | (v2 << 4)  | (v3 << 6)  | \
      (v4 << 8)  | (v5 << 10)  | (v6 << 12)  | (v7 << 14)  | \
      (v8 << 16)  | (v9 << 18)  | (v10 << 20) | (v11 << 22)  | \
      (v12 << 24)  | (v13 << 26)  | (v14 << 28)  | (v15 << 30)

#define BC7_EXTRACT(v, s) (((v) >> (s *2)) & 0x3)

// 2 subset partitions
	uint32_t BC7_PartitionTable2[64] = {
			BC7_PART(0, 0, 1, 1, 0, 0, 1, 1,
							 0, 0, 1, 1, 0, 0, 1, 1),
			BC7_PART(0, 0, 0, 1, 0, 0, 0, 1,
							 0, 0, 0, 1, 0, 0, 0, 1),
			BC7_PART(0, 1, 1, 1, 0, 1, 1, 1,
							 0, 1, 1, 1, 0, 1, 1, 1),
			BC7_PART(0, 0, 0, 1, 0, 0, 1, 1,
							 0, 0, 1, 1, 0, 1, 1, 1),
			BC7_PART(0, 0, 0, 0, 0, 0, 0, 1,
							 0, 0, 0, 1, 0, 0, 1, 1),
			BC7_PART(0, 0, 1, 1, 0, 1, 1, 1,
							 0, 1, 1, 1, 1, 1, 1, 1),
			BC7_PART(0, 0, 0, 1, 0, 0, 1, 1,
							 0, 1, 1, 1, 1, 1, 1, 1),
			BC7_PART(0, 0, 0, 0, 0, 0, 0, 1,
							 0, 0, 1, 1, 0, 1, 1, 1),
			BC7_PART(0, 0, 0, 0, 0, 0, 0, 0,
							 0, 0, 0, 1, 0, 0, 1, 1),
			BC7_PART(0, 0, 1, 1, 0, 1, 1, 1,
							 1, 1, 1, 1, 1, 1, 1, 1),
			BC7_PART(0, 0, 0, 0, 0, 0, 0, 1,
							 0, 1, 1, 1, 1, 1, 1, 1),
			BC7_PART(0, 0, 0, 0, 0, 0, 0, 0,
							 0, 0, 0, 1, 0, 1, 1, 1),
			BC7_PART(0, 0, 0, 1, 0, 1, 1, 1,
							 1, 1, 1, 1, 1, 1, 1, 1),
			BC7_PART(0, 0, 0, 0, 0, 0, 0, 0,
							 1, 1, 1, 1, 1, 1, 1, 1),
			BC7_PART(0, 0, 0, 0, 1, 1, 1, 1,
							 1, 1, 1, 1, 1, 1, 1, 1),
			BC7_PART(0, 0, 0, 0, 0, 0, 0, 0,
							 0, 0, 0, 0, 1, 1, 1, 1),
			BC7_PART(0, 0, 0, 0, 1, 0, 0, 0,
							 1, 1, 1, 0, 1, 1, 1, 1),
			BC7_PART(0, 1, 1, 1, 0, 0, 0, 1,
							 0, 0, 0, 0, 0, 0, 0, 0),
			BC7_PART(0, 0, 0, 0, 0, 0, 0, 0,
							 1, 0, 0, 0, 1, 1, 1, 0),
			BC7_PART(0, 1, 1, 1, 0, 0, 1, 1,
							 0, 0, 0, 1, 0, 0, 0, 0),
			BC7_PART(0, 0, 1, 1, 0, 0, 0, 1,
							 0, 0, 0, 0, 0, 0, 0, 0),
			BC7_PART(0, 0, 0, 0, 1, 0, 0, 0,
							 1, 1, 0, 0, 1, 1, 1, 0),
			BC7_PART(0, 0, 0, 0, 0, 0, 0, 0,
							 1, 0, 0, 0, 1, 1, 0, 0),
			BC7_PART(0, 1, 1, 1, 0, 0, 1, 1,
							 0, 0, 1, 1, 0, 0, 0, 1),
			BC7_PART(0, 0, 1, 1, 0, 0, 0, 1,
							 0, 0, 0, 1, 0, 0, 0, 0),
			BC7_PART(0, 0, 0, 0, 1, 0, 0, 0,
							 1, 0, 0, 0, 1, 1, 0, 0),
			BC7_PART(0, 1, 1, 0, 0, 1, 1, 0,
							 0, 1, 1, 0, 0, 1, 1, 0),
			BC7_PART(0, 0, 1, 1, 0, 1, 1, 0,
							 0, 1, 1, 0, 1, 1, 0, 0),
			BC7_PART(0, 0, 0, 1, 0, 1, 1, 1,
							 1, 1, 1, 0, 1, 0, 0, 0),
			BC7_PART(0, 0, 0, 0, 1, 1, 1, 1,
							 1, 1, 1, 1, 0, 0, 0, 0),
			BC7_PART(0, 1, 1, 1, 0, 0, 0, 1,
							 1, 0, 0, 0, 1, 1, 1, 0),
			BC7_PART(0, 0, 1, 1, 1, 0, 0, 1,
							 1, 0, 0, 1, 1, 1, 0, 0),
			// -----------  BC7 only shapes from here on -------------
			BC7_PART(0, 1, 0, 1, 0, 1, 0, 1,
							 0, 1, 0, 1, 0, 1, 0, 1),
			BC7_PART(0, 0, 0, 0, 1, 1, 1, 1,
							 0, 0, 0, 0, 1, 1, 1, 1),
			BC7_PART(0, 1, 0, 1, 1, 0, 1, 0,
							 0, 1, 0, 1, 1, 0, 1, 0),
			BC7_PART(0, 0, 1, 1, 0, 0, 1, 1,
							 1, 1, 0, 0, 1, 1, 0, 0),
			BC7_PART(0, 0, 1, 1, 1, 1, 0, 0,
							 0, 0, 1, 1, 1, 1, 0, 0),
			BC7_PART(0, 1, 0, 1, 0, 1, 0, 1,
							 1, 0, 1, 0, 1, 0, 1, 0),
			BC7_PART(0, 1, 1, 0, 1, 0, 0, 1,
							 0, 1, 1, 0, 1, 0, 0, 1),
			BC7_PART(0, 1, 0, 1, 1, 0, 1, 0,
							 1, 0, 1, 0, 0, 1, 0, 1),
			BC7_PART(0, 1, 1, 1, 0, 0, 1, 1,
							 1, 1, 0, 0, 1, 1, 1, 0),
			BC7_PART(0, 0, 0, 1, 0, 0, 1, 1,
							 1, 1, 0, 0, 1, 0, 0, 0),
			BC7_PART(0, 0, 1, 1, 0, 0, 1, 0,
							 0, 1, 0, 0, 1, 1, 0, 0),
			BC7_PART(0, 0, 1, 1, 1, 0, 1, 1,
							 1, 1, 0, 1, 1, 1, 0, 0),
			BC7_PART(0, 1, 1, 0, 1, 0, 0, 1,
							 1, 0, 0, 1, 0, 1, 1, 0),
			BC7_PART(0, 0, 1, 1, 1, 1, 0, 0,
							 1, 1, 0, 0, 0, 0, 1, 1),
			BC7_PART(0, 1, 1, 0, 0, 1, 1, 0,
							 1, 0, 0, 1, 1, 0, 0, 1),
			BC7_PART(0, 0, 0, 0, 0, 1, 1, 0,
							 0, 1, 1, 0, 0, 0, 0, 0),
			BC7_PART(0, 1, 0, 0, 1, 1, 1, 0,
							 0, 1, 0, 0, 0, 0, 0, 0),
			BC7_PART(0, 0, 1, 0, 0, 1, 1, 1,
							 0, 0, 1, 0, 0, 0, 0, 0),
			BC7_PART(0, 0, 0, 0, 0, 0, 1, 0,
							 0, 1, 1, 1, 0, 0, 1, 0),
			BC7_PART(0, 0, 0, 0, 0, 1, 0, 0,
							 1, 1, 1, 0, 0, 1, 0, 0),
			BC7_PART(0, 1, 1, 0, 1, 1, 0, 0,
							 1, 0, 0, 1, 0, 0, 1, 1),
			BC7_PART(0, 0, 1, 1, 0, 1, 1, 0,
							 1, 1, 0, 0, 1, 0, 0, 1),
			BC7_PART(0, 1, 1, 0, 0, 0, 1, 1,
							 1, 0, 0, 1, 1, 1, 0, 0),
			BC7_PART(0, 0, 1, 1, 1, 0, 0, 1,
							 1, 1, 0, 0, 0, 1, 1, 0),
			BC7_PART(0, 1, 1, 0, 1, 1, 0, 0,
							 1, 1, 0, 0, 1, 0, 0, 1),
			BC7_PART(0, 1, 1, 0, 0, 0, 1, 1,
							 0, 0, 1, 1, 1, 0, 0, 1),
			BC7_PART(0, 1, 1, 1, 1, 1, 1, 0,
							 1, 0, 0, 0, 0, 0, 0, 1),
			BC7_PART(0, 0, 0, 1, 1, 0, 0, 0,
							 1, 1, 1, 0, 0, 1, 1, 1),
			BC7_PART(0, 0, 0, 0, 1, 1, 1, 1,
							 0, 0, 1, 1, 0, 0, 1, 1),
			BC7_PART(0, 0, 1, 1, 0, 0, 1, 1,
							 1, 1, 1, 1, 0, 0, 0, 0),
			BC7_PART(0, 0, 1, 0, 0, 0, 1, 0,
							 1, 1, 1, 0, 1, 1, 1, 0),
			BC7_PART(0, 1, 0, 0, 0, 1, 0, 0,
							 0, 1, 1, 1, 0, 1, 1, 1)
	};
	// Table.P3 - only for BC7
	uint32_t BC7_PartitionTable3[64] = {
			BC7_PART(0, 0, 1, 1, 0, 0, 1, 1,
							 0, 2, 2, 1, 2, 2, 2, 2U),
			BC7_PART(0, 0, 0, 1, 0, 0, 1, 1,
							 2, 2, 1, 1, 2, 2, 2, 1),
			BC7_PART(0, 0, 0, 0, 2, 0, 0, 1,
							 2, 2, 1, 1, 2, 2, 1, 1),
			BC7_PART(0, 2, 2, 2, 0, 0, 2, 2,
							 0, 0, 1, 1, 0, 1, 1, 1),
			BC7_PART(0, 0, 0, 0, 0, 0, 0, 0,
							 1, 1, 2, 2, 1, 1, 2, 2U),
			BC7_PART(0, 0, 1, 1, 0, 0, 1, 1,
							 0, 0, 2, 2, 0, 0, 2, 2U),
			BC7_PART(0, 0, 2, 2, 0, 0, 2, 2,
							 1, 1, 1, 1, 1, 1, 1, 1),
			BC7_PART(0, 0, 1, 1, 0, 0, 1, 1,
							 2, 2, 1, 1, 2, 2, 1, 1),
			BC7_PART(0, 0, 0, 0, 0, 0, 0, 0,
							 1, 1, 1, 1, 2, 2, 2, 2U),
			BC7_PART(0, 0, 0, 0, 1, 1, 1, 1,
							 1, 1, 1, 1, 2, 2, 2, 2U),
			BC7_PART(0, 0, 0, 0, 1, 1, 1, 1,
							 2, 2, 2, 2, 2, 2, 2, 2U),
			BC7_PART(0, 0, 1, 2, 0, 0, 1, 2,
							 0, 0, 1, 2, 0, 0, 1, 2U),
			BC7_PART(0, 1, 1, 2, 0, 1, 1, 2,
							 0, 1, 1, 2, 0, 1, 1, 2U),
			BC7_PART(0, 1, 2, 2, 0, 1, 2, 2,
							 0, 1, 2, 2, 0, 1, 2, 2U),
			BC7_PART(0, 0, 1, 1, 0, 1, 1, 2,
							 1, 1, 2, 2, 1, 2, 2, 2U),
			BC7_PART(0, 0, 1, 1, 2, 0, 0, 1,
							 2, 2, 0, 0, 2, 2, 2, 0),
			BC7_PART(0, 0, 0, 1, 0, 0, 1, 1,
							 0, 1, 1, 2, 1, 1, 2, 2U),
			BC7_PART(0, 1, 1, 1, 0, 0, 1, 1,
							 2, 0, 0, 1, 2, 2, 0, 0),
			BC7_PART(0, 0, 0, 0, 1, 1, 2, 2,
							 1, 1, 2, 2, 1, 1, 2, 2U),
			BC7_PART(0, 0, 2, 2, 0, 0, 2, 2,
							 0, 0, 2, 2, 1, 1, 1, 1U),
			BC7_PART(0, 1, 1, 1, 0, 1, 1, 1,
							 0, 2, 2, 2, 0, 2, 2, 2U),
			BC7_PART(0, 0, 0, 1, 0, 0, 0, 1,
							 2, 2, 2, 1, 2, 2, 2, 1),
			BC7_PART(0, 0, 0, 0, 0, 0, 1, 1,
							 0, 1, 2, 2, 0, 1, 2, 2U),
			BC7_PART(0, 0, 0, 0, 1, 1, 0, 0,
							 2, 2, 1, 0, 2, 2, 1, 0),
			BC7_PART(0, 1, 2, 2, 0, 1, 2, 2,
							 0, 0, 1, 1, 0, 0, 0, 0),
			BC7_PART(0, 0, 1, 2, 0, 0, 1, 2,
							 1, 1, 2, 2, 2, 2, 2, 2U),
			BC7_PART(0, 1, 1, 0, 1, 2, 2, 1,
							 1, 2, 2, 1, 0, 1, 1, 0),
			BC7_PART(0, 0, 0, 0, 0, 1, 1, 0,
							 1, 2, 2, 1, 1, 2, 2, 1),
			BC7_PART(0, 0, 2, 2, 1, 1, 0, 2,
							 1, 1, 0, 2, 0, 0, 2, 2U),
			BC7_PART(0, 1, 1, 0, 0, 1, 1, 0,
							 2, 0, 0, 2, 2, 2, 2, 2U),
			BC7_PART(0, 0, 1, 1, 0, 1, 2, 2,
							 0, 1, 2, 2, 0, 0, 1, 1),
			BC7_PART(0, 0, 0, 0, 2, 0, 0, 0,
							 2, 2, 1, 1, 2, 2, 2, 1),
			BC7_PART(0, 0, 0, 0, 0, 0, 0, 2,
							 1, 1, 2, 2, 1, 2, 2, 2U),
			BC7_PART(0, 2, 2, 2, 0, 0, 2, 2,
							 0, 0, 1, 2, 0, 0, 1, 1),
			BC7_PART(0, 0, 1, 1, 0, 0, 1, 2,
							 0, 0, 2, 2, 0, 2, 2, 2U),
			BC7_PART(0, 1, 2, 0, 0, 1, 2, 0,
							 0, 1, 2, 0, 0, 1, 2, 0),
			BC7_PART(0, 0, 0, 0, 1, 1, 1, 1,
							 2, 2, 2, 2, 0, 0, 0, 0),
			BC7_PART(0, 1, 2, 0, 1, 2, 0, 1,
							 2, 0, 1, 2, 0, 1, 2, 0),
			BC7_PART(0, 1, 2, 0, 2, 0, 1, 2,
							 1, 2, 0, 1, 0, 1, 2, 0),
			BC7_PART(0, 0, 1, 1, 2, 2, 0, 0,
							 1, 1, 2, 2, 0, 0, 1, 1),
			BC7_PART(0, 0, 1, 1, 1, 1, 2, 2,
							 2, 2, 0, 0, 0, 0, 1, 1),
			BC7_PART(0, 1, 0, 1, 0, 1, 0, 1,
							 2, 2, 2, 2, 2, 2, 2, 2U),
			BC7_PART(0, 0, 0, 0, 0, 0, 0, 0,
							 2, 1, 2, 1, 2, 1, 2, 1),
			BC7_PART(0, 0, 2, 2, 1, 1, 2, 2,
							 0, 0, 2, 2, 1, 1, 2, 2U),
			BC7_PART(0, 0, 2, 2, 0, 0, 1, 1,
							 0, 0, 2, 2, 0, 0, 1, 1),
			BC7_PART(0, 2, 2, 0, 1, 2, 2, 1,
							 0, 2, 2, 0, 1, 2, 2, 1),
			BC7_PART(0, 1, 0, 1, 2, 2, 2, 2,
							 2, 2, 2, 2, 0, 1, 0, 1),
			BC7_PART(0, 0, 0, 0, 2, 1, 2, 1,
							 2, 1, 2, 1, 2, 1, 2, 1),
			BC7_PART(0, 1, 0, 1, 0, 1, 0, 1,
							 0, 1, 0, 1, 2, 2, 2, 2U),
			BC7_PART(0, 2, 2, 2, 0, 1, 1, 1,
							 0, 2, 2, 2, 0, 1, 1, 1),
			BC7_PART(0, 0, 0, 2, 1, 1, 1, 2,
							 0, 0, 0, 2, 1, 1, 1, 2U),
			BC7_PART(0, 0, 0, 0, 2, 1, 1, 2,
							 2, 1, 1, 2, 2, 1, 1, 2U),
			BC7_PART(0, 2, 2, 2, 0, 1, 1, 1,
							 0, 1, 1, 1, 0, 2, 2, 2U),
			BC7_PART(0, 0, 0, 2, 1, 1, 1, 2,
							 1, 1, 1, 2, 0, 0, 0, 2U),
			BC7_PART(0, 1, 1, 0, 0, 1, 1, 0,
							 0, 1, 1, 0, 2, 2, 2, 2U),
			BC7_PART(0, 0, 0, 0, 0, 0, 0, 0,
							 2, 1, 1, 2, 2, 1, 1, 2U),
			BC7_PART(0, 1, 1, 0, 0, 1, 1, 0,
							 2, 2, 2, 2, 2, 2, 2, 2U),
			BC7_PART(0, 0, 2, 2, 0, 0, 1, 1,
							 0, 0, 1, 1, 0, 0, 2, 2U),
			BC7_PART(0, 0, 2, 2, 1, 1, 2, 2,
							 1, 1, 2, 2, 0, 0, 2, 2U),
			BC7_PART(0, 0, 0, 0, 0, 0, 0, 0,
							 0, 0, 0, 0, 2, 1, 1, 2U),
			BC7_PART(0, 0, 0, 2, 0, 0, 0, 1,
							 0, 0, 0, 2, 0, 0, 0, 1),
			BC7_PART(0, 2, 2, 2, 1, 2, 2, 2,
							 0, 2, 2, 2, 1, 2, 2, 2U),
			BC7_PART(0, 1, 0, 1, 2, 2, 2, 2,
							 2, 2, 2, 2, 2, 2, 2, 2U),
			BC7_PART(0, 1, 1, 1, 2, 0, 1, 1,
							 2, 2, 0, 1, 2, 2, 2, 0)
	};

	// two subsets (first always zero)
	uint8_t BC7_FIXUPINDICES2[64] = {
			15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
			15, 2, 8, 2, 2, 8, 8, 15, 2, 8, 2, 2, 8, 8, 2, 2,
			15, 15, 6, 8, 2, 8, 15, 15, 2, 8, 2, 2, 2, 15, 15, 6,
			6, 2, 6, 8, 15, 15, 2, 2, 15, 15, 15, 15, 15, 2, 2, 15,
	};

	// Three subsets (first always 0)
	uint8_t BC7_FIXUPINDICES3[64 * 2] = {
			3, 15, 3, 8, 15, 8, 15, 3, 8, 15, 3, 15, 15, 3, 15, 8,
			8, 15, 8, 15, 6, 15, 6, 15, 6, 15, 5, 15, 3, 15, 3, 8,
			3, 15, 3, 8, 8, 15, 15, 3, 3, 15, 3, 8, 6, 15, 10, 8,
			5, 3, 8, 15, 8, 6, 6, 10, 8, 15, 5, 15, 15, 10, 15, 8,
			8, 15, 15, 3, 3, 15, 5, 10, 6, 10, 10, 8, 8, 9, 15, 10,
			15, 6, 3, 15, 15, 8, 5, 15, 15, 3, 15, 6, 15, 6, 15, 8,
			3, 15, 15, 3, 5, 15, 5, 15, 5, 15, 8, 15, 5, 15, 10, 15,
			5, 15, 10, 15, 8, 15, 13, 15, 15, 3, 12, 15, 3, 15, 3, 8
	};

	// Descriptor structure for block encodings
	typedef struct ModeInfo {

		uint8_t partition;            // which partition is this mode in
		uint8_t partitionBits;       // Number of bits for partition data
		uint8_t rotBits;            // Number of bits for component rotation
		uint8_t iModeBits;       // Number of bits for index selection

		uint8_t pBits;               // p bits

		uint8_t vIndexBits;        // Number of bits per index for vector sets
		uint8_t sIndexBits;        // Number of bits per index in scalar sets

		uint8_t precBits[2];      // bits pre p-bits for rgb, a
		uint8_t pprecBits[2];      // bits post p-bits rgb, a
	} ModeInfo;

	// BC7 mode table
	// Block encoding information for all block types
	// {
	// 		partition, PartitionBits, RotationBits, indexSwapBits,
	//  	total scalarBits, total vectorBits, pBitType, subsetCount,
	//  	vIndexBits, sIndexBits
	//  	precBits[4]
	//  }
	//
	ModeInfo const bti[8] = {
		{2, 4, 0, 0, 6, 3, 0, {4, 0}, {5, 0}},
		{1, 6, 0, 0, 2, 3, 0, {6, 0}, {7, 0}},
		{2, 6, 0, 0, 0, 2, 0, {5, 0}, {5, 0}},
		{1, 6, 0, 0, 4, 2, 0, {7, 0}, {8, 0}},
		{0, 0, 2, 1, 0, 2, 3, {5, 6}, {5, 6}},
		{0, 0, 2, 0, 0, 2, 2, {7, 8}, {7, 8}},
		{0, 0, 0, 0, 2, 4, 0, {7, 7}, {8, 8}},
		{1, 6, 0, 0, 4, 2, 0, {5, 5}, {6, 6}}
	};

	uint8_t mode = 0;
	uint64_t v = compressedBlock[0];
	uint64_t v2 = compressedBlock[1];
	uint8_t shiftCount = 0;
	while (!GetAndShift128(v, v2, 1)) {
		mode++;
	}

	if (mode == 0 || mode > 7) {
		memset(outRGBA, 0, sizeof(uint32_t) * 16);
		return;
	}

	ModeInfo const &mi = bti[mode];
	uint8_t const partition = mi.partition;
	ASSERT(partition < 3);
	uint8_t const numEndPts = (partition + 1) * 2;
	ASSERT(numEndPts <= (3 * 2));

	uint8_t const shape = partition > 0 ? GetAndShift128(v, v2, mi.partitionBits) : 0;
	ASSERT(shape < 32);
	uint8_t const rotation = partition == 0 ? GetAndShift128(v, v2, mi.rotBits) : 0;
	ASSERT(rotation < 4);
	uint8_t const indexMode = partition == 0 ? GetAndShift128(v, v2, mi.iModeBits) : 0;
	ASSERT(indexMode < 2);

	// max 3 subset, 2 endpoints, 4 components
	uint8_t eps[3 * 2][4];

	// end point extraction
	for (int s = 0; s < numEndPts; ++s) {
		eps[s][0] = GetAndShift128(v, v2, mi.precBits[0]);
		eps[s][1] = GetAndShift128(v, v2, mi.precBits[0]);
		eps[s][2] = GetAndShift128(v, v2, mi.precBits[0]);

		if (mi.precBits[1] > 0) {
			eps[s][3] = GetAndShift128(v, v2, mi.precBits[1]);
		} else {
			eps[s][3] = 0;
		}
	}

	// parity bit extraction and extend endpoints
	if (mi.pBits > 0) {
		uint8_t p = GetAndShift128(v, v2, mi.pBits);

		uint8_t const bfp1 = (mi.pBits * 2) / numEndPts;
		uint8_t ifp1 = 0;
		for (int s = 0; s < numEndPts; ++s) {
			if (mi.precBits[0] != mi.pprecBits[0]) {
				eps[s][0] = (eps[s][0] << 1) | (p & (1 << (ifp1 >> 1)));
				eps[s][1] = (eps[s][1] << 1) | (p & (1 << (ifp1 >> 1)));
				eps[s][2] = (eps[s][2] << 1) | (p & (1 << (ifp1 >> 1)));
			}
			if (mi.precBits[1] != mi.pprecBits[1]) {
				eps[s][3] = (eps[s][3] << 1) | (p & (1 << (ifp1 >> 1)));
			}
			ifp1 += bfp1;
		}
	}

	// get weights using the correct fixup table and shape
	// partition 0 and scalar index bitsis very differnt so we handle it seperately
	if(partition == 0 && mi.sIndexBits) {
		uint8_t w[2][16];
		w[0][0] = GetAndShift128(v, v2, mi.vIndexBits - 1);
		for (int i = 1; i < 16; ++i) {
			w[0][i] = GetAndShift128(v, v2, mi.vIndexBits);
		}

		w[1][0] = GetAndShift128(v, v2, mi.sIndexBits - 1);
		for (int i = 1; i < 16; ++i) {
			w[1][i] = GetAndShift128(v, v2, mi.sIndexBits);
		}

		uint8_t cw[2];
		uint8_t const bts[2] = {mi.vIndexBits, mi.sIndexBits};
		if (indexMode == 0) {
			cw[0] = 0;
			cw[1] = 1;
		} else {
			cw[0] = 1;
			cw[1] = 0;
		}

		for (int i = 0; i < 16; ++i) {
			uint8_t col[4];
			col[0] = interpolate(eps[0][0], eps[1][0], w[cw[0]][i], bts[cw[0]]);
			col[1] = interpolate(eps[0][1], eps[1][1], w[cw[0]][i], bts[cw[0]]);
			col[2] = interpolate(eps[0][2], eps[1][2], w[cw[0]][i], bts[cw[0]]);
			col[3] = interpolate(eps[0][3], eps[1][3], w[cw[1]][i], bts[cw[1]]);
			if(rotation > 0) {
				uint8_t const tmp = col[rotation-1];
				col[rotation-1] = col[3];
				col[3] = tmp;
			}
			outRGBA[i] = (col[0] << 24) | col[1] << 16 | col[2] << 8 | col[3];
		}
	} else {
		if(partition == 0) {
			for (int i = 0; i < 16; ++i) {
				uint8_t const w = GetAndShift128(v, v2, (i== 0) ? mi.vIndexBits -1 : mi.vIndexBits) ;
				uint8_t col[4];
				col[0] = interpolate(eps[0][0], eps[1][0], w, mi.vIndexBits);
				col[1] = interpolate(eps[0][1], eps[1][1], w, mi.vIndexBits);
				col[2] = interpolate(eps[0][2], eps[1][2], w, mi.vIndexBits);
				col[3] = interpolate(eps[0][3], eps[1][3], w, mi.vIndexBits);
				outRGBA[i] = (col[0] << 24) | col[1] << 16 | col[2] << 8 | col[3];
			}
		} else if(partition == 1) {
			for (int i = 0; i < 16; ++i) {
				uint8_t const bitsToRead = (i == 0) ||
																		(i == BC7_FIXUPINDICES2[shape]) ?
																		mi.vIndexBits - 1 : mi.vIndexBits;
				uint8_t const w = GetAndShift128(v, v2, bitsToRead);

				uint8_t col[4];
				uint8_t const epi = BC7_EXTRACT(BC7_PartitionTable2[shape], i) * 2;
				col[0] = interpolate(eps[epi][0], eps[epi + 1][0], w, mi.vIndexBits);
				col[1] = interpolate(eps[epi][1], eps[epi + 1][1], w, mi.vIndexBits);
				col[2] = interpolate(eps[epi][2], eps[epi + 1][2], w, mi.vIndexBits);
				col[3] = interpolate(eps[epi][3], eps[epi + 1][3], w, mi.vIndexBits);
				outRGBA[i] = (col[0] << 24) | col[1] << 16 | col[2] << 8 | col[3];
			}
		} else {
			for (int i = 0; i < 16; ++i) {
				uint8_t const bitsToRead =	(i == 0) ||
																		(i == BC7_FIXUPINDICES3[(shape * 2) + 0]) ||
																		(i == BC7_FIXUPINDICES3[(shape * 2) + 1]) ?
																		mi.vIndexBits - 1 : mi.vIndexBits;
				uint8_t const w = GetAndShift128(v, v2, bitsToRead);

				uint8_t col[4];
				uint8_t const epi = BC7_EXTRACT(BC7_PartitionTable3[shape], i) * 2;
				col[0] = interpolate(eps[epi][0], eps[epi + 1][0], w, mi.vIndexBits);
				col[1] = interpolate(eps[epi][1], eps[epi + 1][1], w, mi.vIndexBits);
				col[2] = interpolate(eps[epi][2], eps[epi + 1][2], w, mi.vIndexBits);
				col[3] = interpolate(eps[epi][3], eps[epi + 1][3], w, mi.vIndexBits);
				outRGBA[i] = (col[0] << 24) | col[1] << 16 | col[2] << 8 | col[3];
			}
		}

	}
}