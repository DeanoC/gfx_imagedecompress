//
// Created by deano on 8/1/2019.
//
#include "al2o3_platform/platform.h"
#include "tiny_imageformat/tinyimageformat_decode.h"
#include "gfx_image/image.h"

extern bool detexDecompressBlockBPTC(const uint8_t *bitstring, uint32_t mode_mask,
																		 uint32_t flags, uint8_t *pixel_buffer);

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

void DecompressDXTCAlphaBlock(uint64_t const compressedBlock, uint8_t *out, uint32_t pixelPitch) {
	uint8_t alpha[8];

	alpha[0] = (uint8_t) (compressedBlock & 0xff);
	alpha[1] = (uint8_t) ((compressedBlock >> 8) & 0xff);
	GetCompressedAlphaRamp(alpha);

	for (int i = 0; i < 4 * 4; i++) {
		uint32_t const index = (compressedBlock >> (16 + (i * BLOCK_ALPHA_PIXEL_BPP))) & BLOCK_ALPHA_PIXEL_MASK;
		*out = alpha[index];
		out += pixelPitch;
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

AL2O3_EXTERN_C void Image_DecompressDXBCRGBSingleModeBlock(void const *input, uint32_t output[4 * 4]) {
	DecompressRGBBlock(*(uint64_t const *) input, output, false);
}

AL2O3_EXTERN_C void Image_DecompressDXBCExplictAlphaSingleModeBlock(void const *input,
																																		uint8_t *output,
																																		uint32_t pixelPitch) {
	DecompressExplicitAlphaBlock(*(uint64_t const *) input, output, pixelPitch);
}

AL2O3_EXTERN_C void Image_DecompressDXBCAlphaSingleModeBlock(void const *input, uint8_t *output, uint32_t pixelPitch) {
	DecompressDXTCAlphaBlock(*(uint64_t const *) input, output, pixelPitch);
}

AL2O3_EXTERN_C void Image_DecompressDXBCMultiModeLDRBlock(void const *input, uint32_t output[4 * 4]) {
	detexDecompressBlockBPTC((uint8_t const *) input, 0xFF, 0, (uint8_t *) output);
}


AL2O3_EXTERN_C void Image_DecompressDXBC1Block(void const *input, uint8_t output[4 * 4 * sizeof(uint32_t)]) {
	DecompressRGBBlock(*(uint64_t const *) input, (uint32_t *) output, true);
}

AL2O3_EXTERN_C void Image_DecompressDXBC2Block(void const *input, uint8_t output[4 * 4 * sizeof(uint32_t)]) {
	DecompressRGBBlock(((uint64_t const *) input)[1], (uint32_t *) output, false);
	DecompressExplicitAlphaBlock(((uint64_t const *) input)[0], ((uint8_t *) output) + 3, 4);
}

AL2O3_EXTERN_C void Image_DecompressDXBC3Block(void const *input, uint8_t output[4 * 4 * sizeof(uint32_t)]) {
	DecompressRGBBlock(((uint64_t const *) input)[1], (uint32_t *) output, false);
	DecompressDXTCAlphaBlock(((uint64_t const *) input)[0], ((uint8_t *) output) + 3, 4);
}

AL2O3_EXTERN_C void Image_DecompressDXBC4Block(void const *input, uint8_t output[4 * 4]) {
	DecompressDXTCAlphaBlock(*((uint64_t const *) input), output, 1);
}

AL2O3_EXTERN_C void Image_DecompressDXBC5Block(void const *input, uint8_t output[4 * 4 * 2]) {
	DecompressDXTCAlphaBlock(((uint64_t const *) input)[0], ((uint8_t *) output) + 0, 2);
	DecompressDXTCAlphaBlock(((uint64_t const *) input)[1], ((uint8_t *) output) + 1, 2);
}

AL2O3_EXTERN_C void Image_DecompressDXBC7Block(void const *input, uint8_t output[4 * 4 * sizeof(uint32_t)]) {
	Image_DecompressDXBCMultiModeLDRBlock((uint64_t const *) input, (uint32_t *) output);
}

AL2O3_EXTERN_C void Image_DecompressDXBC1BlockF(void const *input, float output[4 * 4 * 4]) {
	uint8_t packed[4 * 4 * sizeof(uint32_t)];
	Image_DecompressDXBC1Block(input, packed);
	TinyImageFormat_DecodeInput in = {};
	in.pixel = packed;
	TinyImageFormat_DecodeLogicalPixelsF(TinyImageFormat_B8G8R8A8_UNORM, &in, 16, output);
}

AL2O3_EXTERN_C void Image_DecompressDXBC2BlockF(void const *input, float output[4 * 4 * 4]) {
	uint8_t packed[4 * 4 * sizeof(uint32_t)];
	Image_DecompressDXBC2Block(input, packed);
	TinyImageFormat_DecodeInput in = {};
	in.pixel = packed;
	TinyImageFormat_DecodeLogicalPixelsF(TinyImageFormat_B8G8R8A8_UNORM, &in, 16, output);
}


AL2O3_EXTERN_C void Image_DecompressDXBC3BlockF(void const *input, float output[4 * 4 * 4]) {
	uint8_t packed[4 * 4 * sizeof(uint32_t)];
	Image_DecompressDXBC3Block(input, packed);
	TinyImageFormat_DecodeInput in = {};
	in.pixel = packed;
	TinyImageFormat_DecodeLogicalPixelsF(TinyImageFormat_B8G8R8A8_UNORM, &in, 16, output);
}


AL2O3_EXTERN_C void Image_DecompressDXBC4BlockF(void const *input, float output[4 * 4]) {
	uint8_t packed[4 * 4];
	Image_DecompressDXBC4Block(input, packed);
	for (uint32_t i = 0; i < 4 * 4; ++i) {
		output[i] = ((float) packed[i]) / 255.0f;
	}
}

AL2O3_EXTERN_C void Image_DecompressDXBC5BlockF(void const *input, float output[4 * 4 * 2]) {
	uint8_t packed[4 * 4 * 2];
	Image_DecompressDXBC5Block(input, packed);
	for (uint32_t i = 0; i < 4 * 4 * 2; ++i) {
		output[i] = ((float) packed[i]) / 255.0f;
	}
}

AL2O3_EXTERN_C void Image_DecompressDXBC7BlockF(void const *input, float output[4 * 4 * 4]) {
	uint8_t packed[4 * 4 * sizeof(uint32_t)];

	Image_DecompressDXBC7Block(input, packed);
	TinyImageFormat_DecodeInput in = {};
	in.pixel = packed;
	TinyImageFormat_DecodeLogicalPixelsF(TinyImageFormat_B8G8R8A8_UNORM, &in, 16, output);
}

static void ReadNxNBlock(Image_ImageHeader const *src,
												 uint32_t x,
												 uint32_t y,
												 uint32_t w,
												 uint32_t maxBlockByteCount,
												 void *dstBlockData) {
	uint32_t const blockSize = TinyImageFormat_BitSizeOfBlock(src->format) / 8;
	ASSERT(blockSize < maxBlockByteCount);
	uint8_t *srcData = (uint8_t *) Image_RawDataPtr(src);

	size_t const blockIndex = Image_GetBlockIndex(src, x, y, 0, w);
	uint8_t *const srcPtr = srcData + (blockIndex * blockSize);
	memcpy(dstBlockData, srcPtr, blockSize);
}

AL2O3_EXTERN_C Image_ImageHeader const *Image_Decompress(Image_ImageHeader const *src) {
	if (src->depth > 1)
		return nullptr;

	if (!TinyImageFormat_IsCompressed(src->format))
		return src;

	bool const sRGB = TinyImageFormat_IsSRGB(src->format);

	TinyImageFormat dstFormat = TinyImageFormat_UNDEFINED;

	switch (src->format) {
	case TinyImageFormat_DXBC1_RGB_UNORM:
	case TinyImageFormat_DXBC1_RGBA_UNORM:
	case TinyImageFormat_DXBC2_UNORM:
	case TinyImageFormat_DXBC3_UNORM:
	case TinyImageFormat_DXBC7_UNORM: dstFormat = TinyImageFormat_B8G8R8A8_UNORM;
		break;

	case TinyImageFormat_DXBC1_RGB_SRGB:
	case TinyImageFormat_DXBC1_RGBA_SRGB:
	case TinyImageFormat_DXBC2_SRGB:
	case TinyImageFormat_DXBC3_SRGB:
	case TinyImageFormat_DXBC7_SRGB: dstFormat = TinyImageFormat_B8G8R8A8_SRGB;
		break;

	case TinyImageFormat_DXBC4_UNORM: dstFormat = TinyImageFormat_R8_UNORM;
		break;
	case TinyImageFormat_DXBC4_SNORM: dstFormat = TinyImageFormat_R8_SNORM;
		break;

	case TinyImageFormat_DXBC5_UNORM: dstFormat = TinyImageFormat_R8G8_UNORM;
		break;
	case TinyImageFormat_DXBC5_SNORM: dstFormat = TinyImageFormat_R8G8_SNORM;
		break;

	case TinyImageFormat_ETC2_EAC_R11_UNORM:
	case TinyImageFormat_ETC2_EAC_R11_SNORM: return nullptr;

	case TinyImageFormat_ETC2_EAC_R11G11_UNORM:
	case TinyImageFormat_ETC2_EAC_R11G11_SNORM: return nullptr;

	case TinyImageFormat_DXBC6H_UFLOAT:
	case TinyImageFormat_DXBC6H_SFLOAT: return nullptr;

	case TinyImageFormat_PVRTC1_2BPP_UNORM:
	case TinyImageFormat_PVRTC1_4BPP_UNORM:
	case TinyImageFormat_PVRTC2_2BPP_UNORM:
	case TinyImageFormat_PVRTC2_4BPP_UNORM:
	case TinyImageFormat_PVRTC1_2BPP_SRGB:
	case TinyImageFormat_PVRTC1_4BPP_SRGB:
	case TinyImageFormat_PVRTC2_2BPP_SRGB:
	case TinyImageFormat_PVRTC2_4BPP_SRGB:
	case TinyImageFormat_ETC2_R8G8B8_UNORM:
	case TinyImageFormat_ETC2_R8G8B8_SRGB:
	case TinyImageFormat_ETC2_R8G8B8A1_UNORM:
	case TinyImageFormat_ETC2_R8G8B8A1_SRGB:
	case TinyImageFormat_ETC2_R8G8B8A8_UNORM:
	case TinyImageFormat_ETC2_R8G8B8A8_SRGB:
	case TinyImageFormat_ASTC_4x4_UNORM:
	case TinyImageFormat_ASTC_4x4_SRGB:
	case TinyImageFormat_ASTC_5x4_UNORM:
	case TinyImageFormat_ASTC_5x4_SRGB:
	case TinyImageFormat_ASTC_5x5_UNORM:
	case TinyImageFormat_ASTC_5x5_SRGB:
	case TinyImageFormat_ASTC_6x5_UNORM:
	case TinyImageFormat_ASTC_6x5_SRGB:
	case TinyImageFormat_ASTC_6x6_UNORM:
	case TinyImageFormat_ASTC_6x6_SRGB:
	case TinyImageFormat_ASTC_8x5_UNORM:
	case TinyImageFormat_ASTC_8x5_SRGB:
	case TinyImageFormat_ASTC_8x6_UNORM:
	case TinyImageFormat_ASTC_8x6_SRGB:
	case TinyImageFormat_ASTC_8x8_UNORM:
	case TinyImageFormat_ASTC_8x8_SRGB:
	case TinyImageFormat_ASTC_10x5_UNORM:
	case TinyImageFormat_ASTC_10x5_SRGB:
	case TinyImageFormat_ASTC_10x6_UNORM:
	case TinyImageFormat_ASTC_10x6_SRGB:
	case TinyImageFormat_ASTC_10x8_UNORM:
	case TinyImageFormat_ASTC_10x8_SRGB:
	case TinyImageFormat_ASTC_10x10_UNORM:
	case TinyImageFormat_ASTC_10x10_SRGB:
	case TinyImageFormat_ASTC_12x10_UNORM:
	case TinyImageFormat_ASTC_12x10_SRGB:
	case TinyImageFormat_ASTC_12x12_UNORM:
	case TinyImageFormat_ASTC_12x12_SRGB: return nullptr;
	default: ASSERT(false);
	}
	Image_ImageHeader const *dst = Image_CreateNoClear(src->width, src->height, 1, src->slices, dstFormat);
	if (!dst)
		return nullptr;

	// get block size round up to 4
	size_t const blocksX = (src->width + TinyImageFormat_WidthOfBlock(src->format)-1) / TinyImageFormat_WidthOfBlock(src->format);
	size_t const blocksY = (src->height + TinyImageFormat_HeightOfBlock(src->format)-1) / TinyImageFormat_HeightOfBlock(src->format);

	uint64_t compressedBlock[4]; // upto 256 bit block size
	uint8_t uncompressedBlock[TinyImageFormat_MaxPixelCountOfBlock * 4 * 4 * sizeof(float)];

	auto func = &Image_DecompressDXBC1Block;

	switch (src->format) {
	case TinyImageFormat_DXBC1_RGB_UNORM:
	case TinyImageFormat_DXBC1_RGBA_UNORM:
	case TinyImageFormat_DXBC1_RGB_SRGB:
	case TinyImageFormat_DXBC1_RGBA_SRGB: func = Image_DecompressDXBC1Block;
		break;
	case TinyImageFormat_DXBC2_UNORM:
	case TinyImageFormat_DXBC2_SRGB: func = Image_DecompressDXBC2Block;
		break;
	case TinyImageFormat_DXBC3_UNORM:
	case TinyImageFormat_DXBC3_SRGB: func = Image_DecompressDXBC3Block;
		break;
	case TinyImageFormat_DXBC4_UNORM:
	case TinyImageFormat_DXBC4_SNORM: func = Image_DecompressDXBC4Block;
		break;
	case TinyImageFormat_DXBC5_UNORM:
	case TinyImageFormat_DXBC5_SNORM: func = Image_DecompressDXBC5Block;
		break;
	case TinyImageFormat_DXBC7_UNORM:
	case TinyImageFormat_DXBC7_SRGB: func = Image_DecompressDXBC7Block;
		break;

	}
	uint8_t *rawData = (uint8_t *) Image_RawDataPtr(dst);

	uint32_t const dstSize = TinyImageFormat_BitSizeOfBlock(dst->format) / 8;
	uint32_t const dstPitch = TinyImageFormat_WidthOfBlock(src->format) * dstSize;

	for (uint32_t w = 0; w < src->slices; ++w) {
		for (uint32_t y = 0; y < blocksY; ++y) {
			for (uint32_t x = 0; x < blocksX; ++x) {

				uint32_t const sx = x * TinyImageFormat_WidthOfBlock(src->format);
				uint32_t const sy = y * TinyImageFormat_HeightOfBlock(src->format);

				ReadNxNBlock(src, sx, sy, w,
										 sizeof(compressedBlock), compressedBlock);

				func(compressedBlock, uncompressedBlock);

				uint8_t const* ub = (uint8_t const*) uncompressedBlock;
				for (uint32_t dy = 0; dy < TinyImageFormat_HeightOfBlock(src->format); ++dy) {
					uint8_t *dstPtr = rawData + (Image_CalculateIndex(dst,
							sx,sy + dy, 0, w) * dstSize);

					memcpy(dstPtr, ub, dstPitch);
					ub += dstPitch;;
				}
			}
		}
	}
	return dst;
}
