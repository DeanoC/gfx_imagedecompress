#pragma once

#include "al2o3_platform/platform.h"

// highest level API. If uncompressed return src, null if cant, new image returned if decompressed
AL2O3_EXTERN_C Image_ImageHeader const *Image_Decompress(Image_ImageHeader const *src);

// lowest level interface block decompression API
// inputs are normalised float values (0-1 for LDR) in most cases
// exceptions being HDR and SNORM modes
// TODO float decoder for BC4 and 5 should have UNORM & SNORM versions
// TODO sRGB decode for float versions

// RGB Single mode takes input 8 byte RGB block, outputs 16 x RGBA (4)
// Alpha Single mode takes input 8 byte Alpha block, outputs 16 x A
// Explicit Alpha takes input 8 byte Explicit 4 bit Alpha block, outputs 16 x A
// BC1 input 8 byte BC1 RGB block, outputs 16 x RGBA (4)
// Multi Mode LDR input 16 byte Multi Mode BC7 block, outputs 16 x RGBA (4)
AL2O3_EXTERN_C void Image_DecompressDXBCRGBSingleModeBlock(void const * input,	uint32_t output[4 * 4]);
AL2O3_EXTERN_C void Image_DecompressDXBCExplicitAlphaSingleModeBlock(void const *input, uint8_t* output, uint32_t pixelPitch);
AL2O3_EXTERN_C void Image_DecompressDXBCAlphaSingleModeBlock(void const * input, uint8_t* output, uint32_t pixelPitch);
AL2O3_EXTERN_C void Image_DecompressDXBCMultiModeLDRBlock(void const * input, uint32_t output[4 * 4]);


// these use the above partial blocks to make up the actual compression formats
AL2O3_EXTERN_C void Image_DecompressDXBC1Block(void const * input,	uint32_t output[4 * 4]);
AL2O3_EXTERN_C void Image_DecompressDXBC2Block(void const * input,	uint32_t output[4 * 4]);
AL2O3_EXTERN_C void Image_DecompressDXBC3Block(void const * input,	uint32_t output[4 * 4]);
AL2O3_EXTERN_C void Image_DecompressDXBC4Block(void const * input,	uint8_t output[4 * 4]);
AL2O3_EXTERN_C void Image_DecompressDXBC5Block(void const * input,	uint8_t output[4 * 4 * 2]);
AL2O3_EXTERN_C void Image_DecompressDXBC7Block(void const * input,	uint32_t output[4 * 4]);

AL2O3_EXTERN_C void Image_DecompressDXBC1BlockF(void const * input,	float output[4 * 4 * 4]);
AL2O3_EXTERN_C void Image_DecompressDXBC2BlockF(void const * input,	float output[4 * 4 * 4]);
AL2O3_EXTERN_C void Image_DecompressDXBC3BlockF(void const * input,	float output[4 * 4 * 4]);
AL2O3_EXTERN_C void Image_DecompressDXBC4BlockF(void const * input,	float output[4 * 4]);
AL2O3_EXTERN_C void Image_DecompressDXBC5BlockF(void const * input,	float output[4 * 4 * 2]);
AL2O3_EXTERN_C void Image_DecompressDXBC7BlockF(void const * input,	float output[4 * 4 * 4]);
