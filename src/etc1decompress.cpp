// File: rg_etc1.cpp - Fast, high quality ETC1 block packer/unpacker - Rich Geldreich <richgel99@gmail.com>
// Please see ZLIB license at the end of rg_etc1.h.
//
// For more information Ericsson Texture Compression (ETC/ETC1), see:
// http://www.khronos.org/registry/gles/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
//
// v1.04 - 5/15/14 - Fix signed vs. unsigned subtraction problem (noticed when compiled with gcc) in pack_etc1_block_init().
//         This issue would cause an assert when this func. was called in debug. (Note this module was developed/testing with MSVC,
//         I still need to test it throughly when compiled with gcc.)
//
// v1.03 - 5/12/13 - Initial public release
// File: rg_etc1.h - Fast, high quality ETC1 block packer/unpacker - Rich Geldreich <richgel99@gmail.com>
// Please see ZLIB license at the end of this file.

#include "al2o3_platform/platform.h"

#if defined(_DEBUG) || defined(DEBUG)
#define RG_ETC1_BUILD_DEBUG
#endif

#define RG_ETC1_ASSERT ASSERT

namespace rg_etc1 {
// Unpacks an 8-byte ETC1 compressed block to a block of 4x4 32bpp RGBA pixels.
// Returns false if the block is invalid. Invalid blocks will still be unpacked with clamping.
// This function is thread safe, and does not dynamically allocate any memory.
// If preserve_alpha is true, the alpha channel of the destination pixels will not be overwritten. Otherwise, alpha will be set to 255.
bool unpack_etc1_block(const void *pETC1_block, unsigned int *pDst_pixels_rgba, bool preserve_alpha = false);

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;

const uint32 cUINT32_MAX = 0xFFFFFFFFU;
const uint64 cUINT64_MAX = 0xFFFFFFFFFFFFFFFFULL; //0xFFFFFFFFFFFFFFFFui64;

template<typename T> inline T minimum(T a, T b) { return (a < b) ? a : b; }
template<typename T> inline T minimum(T a, T b, T c) { return minimum(minimum(a, b), c); }
template<typename T> inline T maximum(T a, T b) { return (a > b) ? a : b; }
template<typename T> inline T maximum(T a, T b, T c) { return maximum(maximum(a, b), c); }
template<typename T> inline T clamp(T value, T low, T high) {
	return (value < low) ? low : ((value > high) ? high : value);
}
template<typename T> inline T square(T value) { return value * value; }
template<typename T> inline void zero_object(T &obj) { memset((void *) &obj, 0, sizeof(obj)); }
template<typename T> inline void zero_this(T *pObj) { memset((void *) pObj, 0, sizeof(*pObj)); }

template<class T, size_t N> T decay_array_to_subtype(T (&a)[N]);

#define RG_ETC1_ARRAY_SIZE(X) (sizeof(X) / sizeof(decay_array_to_subtype(X)))

enum eNoClamp { cNoClamp };

struct color_quad_u8 {
	static inline int clamp(int v) {
		if (v & 0xFFFFFF00U) {
			v = (~(static_cast<int>(v) >> 31)) & 0xFF;
		}
		return v;
	}

	struct component_traits { enum { cSigned = false, cFloat = false, cMin = 0U, cMax = 255U }; };

public:
	typedef unsigned char component_t;
	typedef int parameter_t;

	enum { cNumComps = 4 };

	union {
		struct {
			component_t b;
			component_t g;
			component_t r;
			component_t a;
		};

		component_t c[cNumComps];

		uint32 m_u32;
	};

	inline color_quad_u8() {
	}

	inline color_quad_u8(const color_quad_u8 &other) : m_u32(other.m_u32) {
	}

	explicit inline color_quad_u8(parameter_t y, parameter_t alpha = component_traits::cMax) {
		set(y, alpha);
	}

	inline color_quad_u8(parameter_t red,
											 parameter_t green,
											 parameter_t blue,
											 parameter_t alpha = component_traits::cMax) {
		set(red, green, blue, alpha);
	}

	explicit inline color_quad_u8(eNoClamp, parameter_t y, parameter_t alpha = component_traits::cMax) {
		set_noclamp_y_alpha(y, alpha);
	}

	inline color_quad_u8(eNoClamp,
											 parameter_t red,
											 parameter_t green,
											 parameter_t blue,
											 parameter_t alpha = component_traits::cMax) {
		set_noclamp_rgba(red, green, blue, alpha);
	}

	inline void clear() {
		m_u32 = 0;
	}

	inline color_quad_u8 &operator=(const color_quad_u8 &other) {
		m_u32 = other.m_u32;
		return *this;
	}

	inline color_quad_u8 &set_rgb(const color_quad_u8 &other) {
		r = other.r;
		g = other.g;
		b = other.b;
		return *this;
	}

	inline color_quad_u8 &operator=(parameter_t y) {
		set(y, component_traits::cMax);
		return *this;
	}

	inline color_quad_u8 &set(parameter_t y, parameter_t alpha = component_traits::cMax) {
		y = clamp(y);
		alpha = clamp(alpha);
		r = static_cast<component_t>(y);
		g = static_cast<component_t>(y);
		b = static_cast<component_t>(y);
		a = static_cast<component_t>(alpha);
		return *this;
	}

	inline color_quad_u8 &set_noclamp_y_alpha(parameter_t y, parameter_t alpha = component_traits::cMax) {
		RG_ETC1_ASSERT((y >= component_traits::cMin) && (y <= component_traits::cMax));
		RG_ETC1_ASSERT((alpha >= component_traits::cMin) && (alpha <= component_traits::cMax));

		r = static_cast<component_t>(y);
		g = static_cast<component_t>(y);
		b = static_cast<component_t>(y);
		a = static_cast<component_t>(alpha);
		return *this;
	}

	inline color_quad_u8 &set(parameter_t red,
														parameter_t green,
														parameter_t blue,
														parameter_t alpha = component_traits::cMax) {
		r = static_cast<component_t>(clamp(red));
		g = static_cast<component_t>(clamp(green));
		b = static_cast<component_t>(clamp(blue));
		a = static_cast<component_t>(clamp(alpha));
		return *this;
	}

	inline color_quad_u8 &set_noclamp_rgba(parameter_t red, parameter_t green, parameter_t blue, parameter_t alpha) {
		RG_ETC1_ASSERT((red >= component_traits::cMin) && (red <= component_traits::cMax));
		RG_ETC1_ASSERT((green >= component_traits::cMin) && (green <= component_traits::cMax));
		RG_ETC1_ASSERT((blue >= component_traits::cMin) && (blue <= component_traits::cMax));
		RG_ETC1_ASSERT((alpha >= component_traits::cMin) && (alpha <= component_traits::cMax));

		r = static_cast<component_t>(red);
		g = static_cast<component_t>(green);
		b = static_cast<component_t>(blue);
		a = static_cast<component_t>(alpha);
		return *this;
	}

	inline color_quad_u8 &set_noclamp_rgb(parameter_t red, parameter_t green, parameter_t blue) {
		RG_ETC1_ASSERT((red >= component_traits::cMin) && (red <= component_traits::cMax));
		RG_ETC1_ASSERT((green >= component_traits::cMin) && (green <= component_traits::cMax));
		RG_ETC1_ASSERT((blue >= component_traits::cMin) && (blue <= component_traits::cMax));

		r = static_cast<component_t>(red);
		g = static_cast<component_t>(green);
		b = static_cast<component_t>(blue);
		return *this;
	}

	static inline parameter_t get_min_comp() { return component_traits::cMin; }
	static inline parameter_t get_max_comp() { return component_traits::cMax; }
	static inline bool get_comps_are_signed() { return component_traits::cSigned; }

	inline component_t operator[](uint i) const {
		RG_ETC1_ASSERT(i < cNumComps);
		return c[i];
	}
	inline component_t &operator[](uint i) {
		RG_ETC1_ASSERT(i < cNumComps);
		return c[i];
	}

	inline color_quad_u8 &set_component(uint i, parameter_t f) {
		RG_ETC1_ASSERT(i < cNumComps);

		c[i] = static_cast<component_t>(clamp(f));

		return *this;
	}

	inline color_quad_u8 &set_grayscale(parameter_t l) {
		component_t x = static_cast<component_t>(clamp(l));
		c[0] = x;
		c[1] = x;
		c[2] = x;
		return *this;
	}

	inline color_quad_u8 &clamp(const color_quad_u8 &l, const color_quad_u8 &h) {
		for (uint i = 0; i < cNumComps; i++)
			c[i] = static_cast<component_t>(rg_etc1::clamp<parameter_t>(c[i], l[i], h[i]));
		return *this;
	}

	inline color_quad_u8 &clamp(parameter_t l, parameter_t h) {
		for (uint i = 0; i < cNumComps; i++)
			c[i] = static_cast<component_t>(rg_etc1::clamp<parameter_t>(c[i], l, h));
		return *this;
	}

	// Returns CCIR 601 luma (consistent with color_utils::RGB_To_Y).
	inline parameter_t get_luma() const {
		return static_cast<parameter_t>((19595U * r + 38470U * g + 7471U * b + 32768U) >> 16U);
	}

	// Returns REC 709 luma.
	inline parameter_t get_luma_rec709() const {
		return static_cast<parameter_t>((13938U * r + 46869U * g + 4729U * b + 32768U) >> 16U);
	}

	inline uint squared_distance_rgb(const color_quad_u8 &c) const {
		return rg_etc1::square(r - c.r) + rg_etc1::square(g - c.g) + rg_etc1::square(b - c.b);
	}

	inline uint squared_distance_rgba(const color_quad_u8 &c) const {
		return rg_etc1::square(r - c.r) + rg_etc1::square(g - c.g) + rg_etc1::square(b - c.b) + rg_etc1::square(a - c.a);
	}

	inline bool rgb_equals(const color_quad_u8 &rhs) const {
		return (r == rhs.r) && (g == rhs.g) && (b == rhs.b);
	}

	inline bool operator==(const color_quad_u8 &rhs) const {
		return m_u32 == rhs.m_u32;
	}

	color_quad_u8 &operator+=(const color_quad_u8 &other) {
		for (uint i = 0; i < 4; i++)
			c[i] = static_cast<component_t>(clamp(c[i] + other.c[i]));
		return *this;
	}

	color_quad_u8 &operator-=(const color_quad_u8 &other) {
		for (uint i = 0; i < 4; i++)
			c[i] = static_cast<component_t>(clamp(c[i] - other.c[i]));
		return *this;
	}

	friend color_quad_u8 operator+(const color_quad_u8 &lhs, const color_quad_u8 &rhs) {
		color_quad_u8 result(lhs);
		result += rhs;
		return result;
	}

	friend color_quad_u8 operator-(const color_quad_u8 &lhs, const color_quad_u8 &rhs) {
		color_quad_u8 result(lhs);
		result -= rhs;
		return result;
	}
}; // class color_quad_u8

enum etc_constants {
	cETC1BytesPerBlock = 8U,

	cETC1SelectorBits = 2U,
	cETC1SelectorValues = 1U << cETC1SelectorBits,
	cETC1SelectorMask = cETC1SelectorValues - 1U,

	cETC1BlockShift = 2U,
	cETC1BlockSize = 1U << cETC1BlockShift,

	cETC1LSBSelectorIndicesBitOffset = 0,
	cETC1MSBSelectorIndicesBitOffset = 16,

	cETC1FlipBitOffset = 32,
	cETC1DiffBitOffset = 33,

	cETC1IntenModifierNumBits = 3,
	cETC1IntenModifierValues = 1 << cETC1IntenModifierNumBits,
	cETC1RightIntenModifierTableBitOffset = 34,
	cETC1LeftIntenModifierTableBitOffset = 37,

	// Base+Delta encoding (5 bit bases, 3 bit delta)
			cETC1BaseColorCompNumBits = 5,
	cETC1BaseColorCompMax = 1 << cETC1BaseColorCompNumBits,

	cETC1DeltaColorCompNumBits = 3,
	cETC1DeltaColorComp = 1 << cETC1DeltaColorCompNumBits,
	cETC1DeltaColorCompMax = 1 << cETC1DeltaColorCompNumBits,

	cETC1BaseColor5RBitOffset = 59,
	cETC1BaseColor5GBitOffset = 51,
	cETC1BaseColor5BBitOffset = 43,

	cETC1DeltaColor3RBitOffset = 56,
	cETC1DeltaColor3GBitOffset = 48,
	cETC1DeltaColor3BBitOffset = 40,

	// Absolute (non-delta) encoding (two 4-bit per component bases)
			cETC1AbsColorCompNumBits = 4,
	cETC1AbsColorCompMax = 1 << cETC1AbsColorCompNumBits,

	cETC1AbsColor4R1BitOffset = 60,
	cETC1AbsColor4G1BitOffset = 52,
	cETC1AbsColor4B1BitOffset = 44,

	cETC1AbsColor4R2BitOffset = 56,
	cETC1AbsColor4G2BitOffset = 48,
	cETC1AbsColor4B2BitOffset = 40,

	cETC1ColorDeltaMin = -4,
	cETC1ColorDeltaMax = 3,

	// Delta3:
	// 0   1   2   3   4   5   6   7
	// 000 001 010 011 100 101 110 111
	// 0   1   2   3   -4  -3  -2  -1
};

static const int g_etc1_inten_tables[cETC1IntenModifierValues][cETC1SelectorValues] =
		{
				{-8, -2, 2, 8}, {-17, -5, 5, 17}, {-29, -9, 9, 29}, {-42, -13, 13, 42},
				{-60, -18, 18, 60}, {-80, -24, 24, 80}, {-106, -33, 33, 106}, {-183, -47, 47, 183}
		};

static const uint8 g_etc1_to_selector_index[cETC1SelectorValues] = {2, 3, 1, 0};
static const uint8 g_selector_index_to_etc1[cETC1SelectorValues] = {3, 2, 0, 1};


struct etc1_block {
	// big endian uint64:
	// bit ofs:  56  48  40  32  24  16   8   0
	// byte ofs: b0, b1, b2, b3, b4, b5, b6, b7
	union {
		uint64 m_uint64;
		uint8 m_bytes[8];
	};

	uint8 m_low_color[2];
	uint8 m_high_color[2];

	enum { cNumSelectorBytes = 4 };
	uint8 m_selectors[cNumSelectorBytes];

	inline void clear() {
		zero_this(this);
	}

	inline uint get_byte_bits(uint ofs, uint num) const {
		RG_ETC1_ASSERT((ofs + num) <= 64U);
		RG_ETC1_ASSERT(num && (num <= 8U));
		RG_ETC1_ASSERT((ofs >> 3) == ((ofs + num - 1) >> 3));
		const uint byte_ofs = 7 - (ofs >> 3);
		const uint byte_bit_ofs = ofs & 7;
		return (m_bytes[byte_ofs] >> byte_bit_ofs) & ((1 << num) - 1);
	}

	inline void set_byte_bits(uint ofs, uint num, uint bits) {
		RG_ETC1_ASSERT((ofs + num) <= 64U);
		RG_ETC1_ASSERT(num && (num < 32U));
		RG_ETC1_ASSERT((ofs >> 3) == ((ofs + num - 1) >> 3));
		RG_ETC1_ASSERT(bits < (1U << num));
		const uint byte_ofs = 7 - (ofs >> 3);
		const uint byte_bit_ofs = ofs & 7;
		const uint mask = (1 << num) - 1;
		m_bytes[byte_ofs] &= ~(mask << byte_bit_ofs);
		m_bytes[byte_ofs] |= (bits << byte_bit_ofs);
	}

	// false = left/right subblocks
	// true = upper/lower subblocks
	inline bool get_flip_bit() const {
		return (m_bytes[3] & 1) != 0;
	}

	inline void set_flip_bit(bool flip) {
		m_bytes[3] &= ~1;
		m_bytes[3] |= static_cast<uint8>(flip);
	}

	inline bool get_diff_bit() const {
		return (m_bytes[3] & 2) != 0;
	}

	inline void set_diff_bit(bool diff) {
		m_bytes[3] &= ~2;
		m_bytes[3] |= (static_cast<uint>(diff) << 1);
	}

	// Returns intensity modifier table (0-7) used by subblock subblock_id.
	// subblock_id=0 left/top (CW 1), 1=right/bottom (CW 2)
	inline uint get_inten_table(uint subblock_id) const {
		RG_ETC1_ASSERT(subblock_id < 2);
		const uint ofs = subblock_id ? 2 : 5;
		return (m_bytes[3] >> ofs) & 7;
	}

	// Sets intensity modifier table (0-7) used by subblock subblock_id (0 or 1)
	inline void set_inten_table(uint subblock_id, uint t) {
		RG_ETC1_ASSERT(subblock_id < 2);
		RG_ETC1_ASSERT(t < 8);
		const uint ofs = subblock_id ? 2 : 5;
		m_bytes[3] &= ~(7 << ofs);
		m_bytes[3] |= (t << ofs);
	}

	// Returned selector value ranges from 0-3 and is a direct index into g_etc1_inten_tables.
	inline uint get_selector(uint x, uint y) const {
		RG_ETC1_ASSERT((x | y) < 4);

		const uint bit_index = x * 4 + y;
		const uint byte_bit_ofs = bit_index & 7;
		const uint8 *p = &m_bytes[7 - (bit_index >> 3)];
		const uint lsb = (p[0] >> byte_bit_ofs) & 1;
		const uint msb = (p[-2] >> byte_bit_ofs) & 1;
		const uint val = lsb | (msb << 1);

		return g_etc1_to_selector_index[val];
	}

	// Selector "val" ranges from 0-3 and is a direct index into g_etc1_inten_tables.
	inline void set_selector(uint x, uint y, uint val) {
		RG_ETC1_ASSERT((x | y | val) < 4);
		const uint bit_index = x * 4 + y;

		uint8 *p = &m_bytes[7 - (bit_index >> 3)];

		const uint byte_bit_ofs = bit_index & 7;
		const uint mask = 1 << byte_bit_ofs;

		const uint etc1_val = g_selector_index_to_etc1[val];

		const uint lsb = etc1_val & 1;
		const uint msb = etc1_val >> 1;

		p[0] &= ~mask;
		p[0] |= (lsb << byte_bit_ofs);

		p[-2] &= ~mask;
		p[-2] |= (msb << byte_bit_ofs);
	}

	inline void set_base4_color(uint idx, uint16 c) {
		if (idx) {
			set_byte_bits(cETC1AbsColor4R2BitOffset, 4, (c >> 8) & 15);
			set_byte_bits(cETC1AbsColor4G2BitOffset, 4, (c >> 4) & 15);
			set_byte_bits(cETC1AbsColor4B2BitOffset, 4, c & 15);
		} else {
			set_byte_bits(cETC1AbsColor4R1BitOffset, 4, (c >> 8) & 15);
			set_byte_bits(cETC1AbsColor4G1BitOffset, 4, (c >> 4) & 15);
			set_byte_bits(cETC1AbsColor4B1BitOffset, 4, c & 15);
		}
	}

	inline uint16 get_base4_color(uint idx) const {
		uint r, g, b;
		if (idx) {
			r = get_byte_bits(cETC1AbsColor4R2BitOffset, 4);
			g = get_byte_bits(cETC1AbsColor4G2BitOffset, 4);
			b = get_byte_bits(cETC1AbsColor4B2BitOffset, 4);
		} else {
			r = get_byte_bits(cETC1AbsColor4R1BitOffset, 4);
			g = get_byte_bits(cETC1AbsColor4G1BitOffset, 4);
			b = get_byte_bits(cETC1AbsColor4B1BitOffset, 4);
		}
		return static_cast<uint16>(b | (g << 4U) | (r << 8U));
	}

	inline void set_base5_color(uint16 c) {
		set_byte_bits(cETC1BaseColor5RBitOffset, 5, (c >> 10) & 31);
		set_byte_bits(cETC1BaseColor5GBitOffset, 5, (c >> 5) & 31);
		set_byte_bits(cETC1BaseColor5BBitOffset, 5, c & 31);
	}

	inline uint16 get_base5_color() const {
		const uint r = get_byte_bits(cETC1BaseColor5RBitOffset, 5);
		const uint g = get_byte_bits(cETC1BaseColor5GBitOffset, 5);
		const uint b = get_byte_bits(cETC1BaseColor5BBitOffset, 5);
		return static_cast<uint16>(b | (g << 5U) | (r << 10U));
	}

	void set_delta3_color(uint16 c) {
		set_byte_bits(cETC1DeltaColor3RBitOffset, 3, (c >> 6) & 7);
		set_byte_bits(cETC1DeltaColor3GBitOffset, 3, (c >> 3) & 7);
		set_byte_bits(cETC1DeltaColor3BBitOffset, 3, c & 7);
	}

	inline uint16 get_delta3_color() const {
		const uint r = get_byte_bits(cETC1DeltaColor3RBitOffset, 3);
		const uint g = get_byte_bits(cETC1DeltaColor3GBitOffset, 3);
		const uint b = get_byte_bits(cETC1DeltaColor3BBitOffset, 3);
		return static_cast<uint16>(b | (g << 3U) | (r << 6U));
	}

	// Base color 5
	static uint16 pack_color5(const color_quad_u8 &color, bool scaled, uint bias = 127U);
	static uint16 pack_color5(uint r, uint g, uint b, bool scaled, uint bias = 127U);

	static color_quad_u8 unpack_color5(uint16 packed_color5, bool scaled, uint alpha = 255U);
	static void unpack_color5(uint &r, uint &g, uint &b, uint16 packed_color, bool scaled);

	static bool unpack_color5(color_quad_u8 &result,
														uint16 packed_color5,
														uint16 packed_delta3,
														bool scaled,
														uint alpha = 255U);
	static bool unpack_color5(uint &r,
														uint &g,
														uint &b,
														uint16 packed_color5,
														uint16 packed_delta3,
														bool scaled,
														uint alpha = 255U);

	// Delta color 3
	// Inputs range from -4 to 3 (cETC1ColorDeltaMin to cETC1ColorDeltaMax)
	static uint16 pack_delta3(int r, int g, int b);

	// Results range from -4 to 3 (cETC1ColorDeltaMin to cETC1ColorDeltaMax)
	static void unpack_delta3(int &r, int &g, int &b, uint16 packed_delta3);

	// Abs color 4
	static uint16 pack_color4(const color_quad_u8 &color, bool scaled, uint bias = 127U);
	static uint16 pack_color4(uint r, uint g, uint b, bool scaled, uint bias = 127U);

	static color_quad_u8 unpack_color4(uint16 packed_color4, bool scaled, uint alpha = 255U);
	static void unpack_color4(uint &r, uint &g, uint &b, uint16 packed_color4, bool scaled);

	// subblock colors
	static void get_diff_subblock_colors(color_quad_u8 *pDst, uint16 packed_color5, uint table_idx);
	static bool get_diff_subblock_colors(color_quad_u8 *pDst, uint16 packed_color5, uint16 packed_delta3, uint table_idx);
	static void get_abs_subblock_colors(color_quad_u8 *pDst, uint16 packed_color4, uint table_idx);

	static inline void unscaled_to_scaled_color(color_quad_u8 &dst, const color_quad_u8 &src, bool color4) {
		if (color4) {
			dst.r = src.r | (src.r << 4);
			dst.g = src.g | (src.g << 4);
			dst.b = src.b | (src.b << 4);
		} else {
			dst.r = (src.r >> 2) | (src.r << 3);
			dst.g = (src.g >> 2) | (src.g << 3);
			dst.b = (src.b >> 2) | (src.b << 3);
		}
		dst.a = src.a;
	}
};

// Returns pointer to sorted array.
template<typename T, typename Q>
T *indirect_radix_sort(uint num_indices,
											 T *pIndices0,
											 T *pIndices1,
											 const Q *pKeys,
											 uint key_ofs,
											 uint key_size,
											 bool init_indices) {
	RG_ETC1_ASSERT((key_ofs >= 0) && (key_ofs < sizeof(T)));
	RG_ETC1_ASSERT((key_size >= 1) && (key_size <= 4));

	if (init_indices) {
		T *p = pIndices0;
		T *q = pIndices0 + (num_indices >> 1) * 2;
		uint i;
		for (i = 0; p != q; p += 2, i += 2) {
			p[0] = static_cast<T>(i);
			p[1] = static_cast<T>(i + 1);
		}

		if (num_indices & 1) {
			*p = static_cast<T>(i);
		}
	}

	uint hist[256 * 4];

	memset(hist, 0, sizeof(hist[0]) * 256 * key_size);

#define RG_ETC1_GET_KEY(p) (*(const uint*)((const uint8*)(pKeys + *(p)) + key_ofs))
#define RG_ETC1_GET_KEY_FROM_INDEX(i) (*(const uint*)((const uint8*)(pKeys + (i)) + key_ofs))

	if (key_size == 4) {
		T *p = pIndices0;
		T *q = pIndices0 + num_indices;
		for (; p != q; p++) {
			const uint key = RG_ETC1_GET_KEY(p);

			hist[key & 0xFF]++;
			hist[256 + ((key >> 8) & 0xFF)]++;
			hist[512 + ((key >> 16) & 0xFF)]++;
			hist[768 + ((key >> 24) & 0xFF)]++;
		}
	} else if (key_size == 3) {
		T *p = pIndices0;
		T *q = pIndices0 + num_indices;
		for (; p != q; p++) {
			const uint key = RG_ETC1_GET_KEY(p);

			hist[key & 0xFF]++;
			hist[256 + ((key >> 8) & 0xFF)]++;
			hist[512 + ((key >> 16) & 0xFF)]++;
		}
	} else if (key_size == 2) {
		T *p = pIndices0;
		T *q = pIndices0 + (num_indices >> 1) * 2;

		for (; p != q; p += 2) {
			const uint key0 = RG_ETC1_GET_KEY(p);
			const uint key1 = RG_ETC1_GET_KEY(p + 1);

			hist[key0 & 0xFF]++;
			hist[256 + ((key0 >> 8) & 0xFF)]++;

			hist[key1 & 0xFF]++;
			hist[256 + ((key1 >> 8) & 0xFF)]++;
		}

		if (num_indices & 1) {
			const uint key = RG_ETC1_GET_KEY(p);

			hist[key & 0xFF]++;
			hist[256 + ((key >> 8) & 0xFF)]++;
		}
	} else {
		RG_ETC1_ASSERT(key_size == 1);
		if (key_size != 1) {
			return NULL;
		}

		T *p = pIndices0;
		T *q = pIndices0 + (num_indices >> 1) * 2;

		for (; p != q; p += 2) {
			const uint key0 = RG_ETC1_GET_KEY(p);
			const uint key1 = RG_ETC1_GET_KEY(p + 1);

			hist[key0 & 0xFF]++;
			hist[key1 & 0xFF]++;
		}

		if (num_indices & 1) {
			const uint key = RG_ETC1_GET_KEY(p);

			hist[key & 0xFF]++;
		}
	}

	T *pCur = pIndices0;
	T *pNew = pIndices1;

	for (uint pass = 0; pass < key_size; pass++) {
		const uint *pHist = &hist[pass << 8];

		uint offsets[256];

		uint cur_ofs = 0;
		for (uint i = 0; i < 256; i += 2) {
			offsets[i] = cur_ofs;
			cur_ofs += pHist[i];

			offsets[i + 1] = cur_ofs;
			cur_ofs += pHist[i + 1];
		}

		const uint pass_shift = pass << 3;

		T *p = pCur;
		T *q = pCur + (num_indices >> 1) * 2;

		for (; p != q; p += 2) {
			uint index0 = p[0];
			uint index1 = p[1];

			uint c0 = (RG_ETC1_GET_KEY_FROM_INDEX(index0) >> pass_shift) & 0xFF;
			uint c1 = (RG_ETC1_GET_KEY_FROM_INDEX(index1) >> pass_shift) & 0xFF;

			if (c0 == c1) {
				uint dst_offset0 = offsets[c0];

				offsets[c0] = dst_offset0 + 2;

				pNew[dst_offset0] = static_cast<T>(index0);
				pNew[dst_offset0 + 1] = static_cast<T>(index1);
			} else {
				uint dst_offset0 = offsets[c0]++;
				uint dst_offset1 = offsets[c1]++;

				pNew[dst_offset0] = static_cast<T>(index0);
				pNew[dst_offset1] = static_cast<T>(index1);
			}
		}

		if (num_indices & 1) {
			uint index = *p;
			uint c = (RG_ETC1_GET_KEY_FROM_INDEX(index) >> pass_shift) & 0xFF;

			uint dst_offset = offsets[c];
			offsets[c] = dst_offset + 1;

			pNew[dst_offset] = static_cast<T>(index);
		}

		T *t = pCur;
		pCur = pNew;
		pNew = t;
	}

	return pCur;
}

#undef RG_ETC1_GET_KEY
#undef RG_ETC1_GET_KEY_FROM_INDEX

uint16 etc1_block::pack_color5(const color_quad_u8 &color, bool scaled, uint bias) {
	return pack_color5(color.r, color.g, color.b, scaled, bias);
}

uint16 etc1_block::pack_color5(uint r, uint g, uint b, bool scaled, uint bias) {
	if (scaled) {
		r = (r * 31U + bias) / 255U;
		g = (g * 31U + bias) / 255U;
		b = (b * 31U + bias) / 255U;
	}

	r = rg_etc1::minimum(r, 31U);
	g = rg_etc1::minimum(g, 31U);
	b = rg_etc1::minimum(b, 31U);

	return static_cast<uint16>(b | (g << 5U) | (r << 10U));
}

color_quad_u8 etc1_block::unpack_color5(uint16 packed_color5, bool scaled, uint alpha) {
	uint b = packed_color5 & 31U;
	uint g = (packed_color5 >> 5U) & 31U;
	uint r = (packed_color5 >> 10U) & 31U;

	if (scaled) {
		b = (b << 3U) | (b >> 2U);
		g = (g << 3U) | (g >> 2U);
		r = (r << 3U) | (r >> 2U);
	}

	return color_quad_u8(cNoClamp, r, g, b, rg_etc1::minimum(alpha, 255U));
}

void etc1_block::unpack_color5(uint &r, uint &g, uint &b, uint16 packed_color5, bool scaled) {
	color_quad_u8 c(unpack_color5(packed_color5, scaled, 0));
	r = c.r;
	g = c.g;
	b = c.b;
}

bool etc1_block::unpack_color5(color_quad_u8 &result,
															 uint16 packed_color5,
															 uint16 packed_delta3,
															 bool scaled,
															 uint alpha) {
	int dc_r, dc_g, dc_b;
	unpack_delta3(dc_r, dc_g, dc_b, packed_delta3);

	int b = (packed_color5 & 31U) + dc_b;
	int g = ((packed_color5 >> 5U) & 31U) + dc_g;
	int r = ((packed_color5 >> 10U) & 31U) + dc_r;

	bool success = true;
	if (static_cast<uint>(r | g | b) > 31U) {
		success = false;
		r = rg_etc1::clamp<int>(r, 0, 31);
		g = rg_etc1::clamp<int>(g, 0, 31);
		b = rg_etc1::clamp<int>(b, 0, 31);
	}

	if (scaled) {
		b = (b << 3U) | (b >> 2U);
		g = (g << 3U) | (g >> 2U);
		r = (r << 3U) | (r >> 2U);
	}

	result.set_noclamp_rgba(r, g, b, rg_etc1::minimum(alpha, 255U));
	return success;
}

bool etc1_block::unpack_color5(uint &r,
															 uint &g,
															 uint &b,
															 uint16 packed_color5,
															 uint16 packed_delta3,
															 bool scaled,
															 uint alpha) {
	color_quad_u8 result;
	const bool success = unpack_color5(result, packed_color5, packed_delta3, scaled, alpha);
	r = result.r;
	g = result.g;
	b = result.b;
	return success;
}

uint16 etc1_block::pack_delta3(int r, int g, int b) {
	RG_ETC1_ASSERT((r >= cETC1ColorDeltaMin) && (r <= cETC1ColorDeltaMax));
	RG_ETC1_ASSERT((g >= cETC1ColorDeltaMin) && (g <= cETC1ColorDeltaMax));
	RG_ETC1_ASSERT((b >= cETC1ColorDeltaMin) && (b <= cETC1ColorDeltaMax));
	if (r < 0) {
		r += 8;
	}
	if (g < 0) {
		g += 8;
	}
	if (b < 0) {
		b += 8;
	}
	return static_cast<uint16>(b | (g << 3) | (r << 6));
}

void etc1_block::unpack_delta3(int &r, int &g, int &b, uint16 packed_delta3) {
	r = (packed_delta3 >> 6) & 7;
	g = (packed_delta3 >> 3) & 7;
	b = packed_delta3 & 7;
	if (r >= 4) {
		r -= 8;
	}
	if (g >= 4) {
		g -= 8;
	}
	if (b >= 4) {
		b -= 8;
	}
}

uint16 etc1_block::pack_color4(const color_quad_u8 &color, bool scaled, uint bias) {
	return pack_color4(color.r, color.g, color.b, scaled, bias);
}

uint16 etc1_block::pack_color4(uint r, uint g, uint b, bool scaled, uint bias) {
	if (scaled) {
		r = (r * 15U + bias) / 255U;
		g = (g * 15U + bias) / 255U;
		b = (b * 15U + bias) / 255U;
	}

	r = rg_etc1::minimum(r, 15U);
	g = rg_etc1::minimum(g, 15U);
	b = rg_etc1::minimum(b, 15U);

	return static_cast<uint16>(b | (g << 4U) | (r << 8U));
}

color_quad_u8 etc1_block::unpack_color4(uint16 packed_color4, bool scaled, uint alpha) {
	uint b = packed_color4 & 15U;
	uint g = (packed_color4 >> 4U) & 15U;
	uint r = (packed_color4 >> 8U) & 15U;

	if (scaled) {
		b = (b << 4U) | b;
		g = (g << 4U) | g;
		r = (r << 4U) | r;
	}

	return color_quad_u8(cNoClamp, r, g, b, rg_etc1::minimum(alpha, 255U));
}

void etc1_block::unpack_color4(uint &r, uint &g, uint &b, uint16 packed_color4, bool scaled) {
	color_quad_u8 c(unpack_color4(packed_color4, scaled, 0));
	r = c.r;
	g = c.g;
	b = c.b;
}

void etc1_block::get_diff_subblock_colors(color_quad_u8 *pDst, uint16 packed_color5, uint table_idx) {
	RG_ETC1_ASSERT(table_idx < cETC1IntenModifierValues);
	const int *pInten_modifer_table = &g_etc1_inten_tables[table_idx][0];

	uint r, g, b;
	unpack_color5(r, g, b, packed_color5, true);

	const int ir = static_cast<int>(r), ig = static_cast<int>(g), ib = static_cast<int>(b);

	const int y0 = pInten_modifer_table[0];
	pDst[0].set(ir + y0, ig + y0, ib + y0);

	const int y1 = pInten_modifer_table[1];
	pDst[1].set(ir + y1, ig + y1, ib + y1);

	const int y2 = pInten_modifer_table[2];
	pDst[2].set(ir + y2, ig + y2, ib + y2);

	const int y3 = pInten_modifer_table[3];
	pDst[3].set(ir + y3, ig + y3, ib + y3);
}

bool etc1_block::get_diff_subblock_colors(color_quad_u8 *pDst,
																					uint16 packed_color5,
																					uint16 packed_delta3,
																					uint table_idx) {
	RG_ETC1_ASSERT(table_idx < cETC1IntenModifierValues);
	const int *pInten_modifer_table = &g_etc1_inten_tables[table_idx][0];

	uint r, g, b;
	bool success = unpack_color5(r, g, b, packed_color5, packed_delta3, true);

	const int ir = static_cast<int>(r), ig = static_cast<int>(g), ib = static_cast<int>(b);

	const int y0 = pInten_modifer_table[0];
	pDst[0].set(ir + y0, ig + y0, ib + y0);

	const int y1 = pInten_modifer_table[1];
	pDst[1].set(ir + y1, ig + y1, ib + y1);

	const int y2 = pInten_modifer_table[2];
	pDst[2].set(ir + y2, ig + y2, ib + y2);

	const int y3 = pInten_modifer_table[3];
	pDst[3].set(ir + y3, ig + y3, ib + y3);

	return success;
}

void etc1_block::get_abs_subblock_colors(color_quad_u8 *pDst, uint16 packed_color4, uint table_idx) {
	RG_ETC1_ASSERT(table_idx < cETC1IntenModifierValues);
	const int *pInten_modifer_table = &g_etc1_inten_tables[table_idx][0];

	uint r, g, b;
	unpack_color4(r, g, b, packed_color4, true);

	const int ir = static_cast<int>(r), ig = static_cast<int>(g), ib = static_cast<int>(b);

	const int y0 = pInten_modifer_table[0];
	pDst[0].set(ir + y0, ig + y0, ib + y0);

	const int y1 = pInten_modifer_table[1];
	pDst[1].set(ir + y1, ig + y1, ib + y1);

	const int y2 = pInten_modifer_table[2];
	pDst[2].set(ir + y2, ig + y2, ib + y2);

	const int y3 = pInten_modifer_table[3];
	pDst[3].set(ir + y3, ig + y3, ib + y3);
}

bool unpack_etc1_block(const void *pETC1_block, unsigned int *pDst_pixels_rgba, bool preserve_alpha) {
	color_quad_u8 *pDst = reinterpret_cast<color_quad_u8 *>(pDst_pixels_rgba);
	const etc1_block &block = *static_cast<const etc1_block *>(pETC1_block);

	const bool diff_flag = block.get_diff_bit();
	const bool flip_flag = block.get_flip_bit();
	const uint table_index0 = block.get_inten_table(0);
	const uint table_index1 = block.get_inten_table(1);

	color_quad_u8 subblock_colors0[4];
	color_quad_u8 subblock_colors1[4];
	bool success = true;

	if (diff_flag) {
		const uint16 base_color5 = block.get_base5_color();
		const uint16 delta_color3 = block.get_delta3_color();
		etc1_block::get_diff_subblock_colors(subblock_colors0, base_color5, table_index0);

		if (!etc1_block::get_diff_subblock_colors(subblock_colors1, base_color5, delta_color3, table_index1)) {
			success = false;
		}
	} else {
		const uint16 base_color4_0 = block.get_base4_color(0);
		etc1_block::get_abs_subblock_colors(subblock_colors0, base_color4_0, table_index0);

		const uint16 base_color4_1 = block.get_base4_color(1);
		etc1_block::get_abs_subblock_colors(subblock_colors1, base_color4_1, table_index1);
	}

	if (preserve_alpha) {
		if (flip_flag) {
			for (uint y = 0; y < 2; y++) {
				pDst[0].set_rgb(subblock_colors0[block.get_selector(0, y)]);
				pDst[1].set_rgb(subblock_colors0[block.get_selector(1, y)]);
				pDst[2].set_rgb(subblock_colors0[block.get_selector(2, y)]);
				pDst[3].set_rgb(subblock_colors0[block.get_selector(3, y)]);
				pDst += 4;
			}

			for (uint y = 2; y < 4; y++) {
				pDst[0].set_rgb(subblock_colors1[block.get_selector(0, y)]);
				pDst[1].set_rgb(subblock_colors1[block.get_selector(1, y)]);
				pDst[2].set_rgb(subblock_colors1[block.get_selector(2, y)]);
				pDst[3].set_rgb(subblock_colors1[block.get_selector(3, y)]);
				pDst += 4;
			}
		} else {
			for (uint y = 0; y < 4; y++) {
				pDst[0].set_rgb(subblock_colors0[block.get_selector(0, y)]);
				pDst[1].set_rgb(subblock_colors0[block.get_selector(1, y)]);
				pDst[2].set_rgb(subblock_colors1[block.get_selector(2, y)]);
				pDst[3].set_rgb(subblock_colors1[block.get_selector(3, y)]);
				pDst += 4;
			}
		}
	} else {
		if (flip_flag) {
			// 0000
			// 0000
			// 1111
			// 1111
			for (uint y = 0; y < 2; y++) {
				pDst[0] = subblock_colors0[block.get_selector(0, y)];
				pDst[1] = subblock_colors0[block.get_selector(1, y)];
				pDst[2] = subblock_colors0[block.get_selector(2, y)];
				pDst[3] = subblock_colors0[block.get_selector(3, y)];
				pDst += 4;
			}

			for (uint y = 2; y < 4; y++) {
				pDst[0] = subblock_colors1[block.get_selector(0, y)];
				pDst[1] = subblock_colors1[block.get_selector(1, y)];
				pDst[2] = subblock_colors1[block.get_selector(2, y)];
				pDst[3] = subblock_colors1[block.get_selector(3, y)];
				pDst += 4;
			}
		} else {
			// 0011
			// 0011
			// 0011
			// 0011
			for (uint y = 0; y < 4; y++) {
				pDst[0] = subblock_colors0[block.get_selector(0, y)];
				pDst[1] = subblock_colors0[block.get_selector(1, y)];
				pDst[2] = subblock_colors1[block.get_selector(2, y)];
				pDst[3] = subblock_colors1[block.get_selector(3, y)];
				pDst += 4;
			}
		}
	}

	return success;
}

} // namespace rg_etc1

AL2O3_EXTERN_C void Image_DecompressETC1Block(void const *input, uint8_t output[4 * 4 * sizeof(uint32_t)]) {
	rg_etc1::unpack_etc1_block(input, (unsigned int *) output, false);
}
