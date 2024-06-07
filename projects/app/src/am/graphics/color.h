#pragma once

#include "common/types.h"
#include "rage/math/vec.h"

#define AM_COL32(R,G,B,A) (((u32)(A)<<24) | ((u32)(B)<<16) | ((u32)(G)<<8) | ((u32)(R)<<0))

namespace rageam::graphics
{
	union ColorU32;

	// https://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion
	void ColorConvertRGBtoHSL(u8 r, u8 g, u8 b, float& h, float& s, float& l);
	void ColorConvertHSLtoRGB(float h, float s, float l, u8& r, u8& g, u8& b);

	// Technique used in visual studio to adjust icons for white/dark theme
	ColorU32 ColorTransformLuminosity(ColorU32 color, float themeBackgroundLuminosity);

	float ColorGetLuminosity(ColorU32 color);

	constexpr float ColorRGBToLuminosity(u8 r, u8 g, u8 b)
	{
		float rf = static_cast<float>(r) / 255.0f;
		float gf = static_cast<float>(g) / 255.0f;
		float bf = static_cast<float>(b) / 255.0f;
		float max = rf > gf && rf > bf ? rf : gf > bf ? gf : bf;
		float min = rf < gf && rf < bf ? rf : gf < bf ? gf : bf;
		return (max + min) / 2.0f;
	}

	/**
	 * \brief Represents RGBA 32-bit color with each component packed in 8 bits.
	 */
	union ColorU32
	{
		u32 Value;
		struct
		{
			u8 R;
			u8 G;
			u8 B;
			u8 A;
		};

		ColorU32() = default; // Black
		ColorU32(u32 value) { Value = value; }
		ColorU32(u8 r, u8 g, u8 b, u8 a) { R = r; G = g; B = b; A = a; }
		ColorU32(u8 r, u8 g, u8 b) : ColorU32(r, g, b, 255) {}

		rage::Vector4 ToVec4() const
		{
			return
			{
				static_cast<float>(R) / 255.0f,
				static_cast<float>(G) / 255.0f,
				static_cast<float>(B) / 255.0f,
				static_cast<float>(A) / 255.0f,
			};
		}

		static ColorU32 FromVec4(const rage::Vector4& vec)
		{
			ColorU32 col;
			col.R = static_cast<u8>(vec.X * 255.0f);
			col.G = static_cast<u8>(vec.Y * 255.0f);
			col.B = static_cast<u8>(vec.Z * 255.0f);
			col.A = static_cast<u8>(vec.W * 255.0f);
			return col;
		}

		// Alpha is set to 255
		static ColorU32 FromVec3(const rage::Vector3& vec)
		{
			ColorU32 col;
			col.R = static_cast<u8>(vec.X * 255.0f);
			col.G = static_cast<u8>(vec.Y * 255.0f);
			col.B = static_cast<u8>(vec.Z * 255.0f);
			col.A = 255;
			return col;
		}

		static ColorU32 FromFloat4(const float vec[4])
		{
			return FromVec4(*(const rage::Vector4*)vec);
		}

		static ColorU32 FromFloat3(const float vec[3])
		{
			return FromVec3(*(const rage::Vector3*)vec);
		}

		u8 operator[](int index) const
		{
			AM_DEBUG_ASSERTS(index < 3);
			return reinterpret_cast<const u8*>(this)[index];
		}

		operator u32() const { return Value; }
	};

	static const ColorU32 COLOR_WHITE = { 255, 255, 255 };
	static const ColorU32 COLOR_GRAY = { 193, 193, 193 };
	static const ColorU32 COLOR_BLACK = { 0, 0, 0 };
	static const ColorU32 COLOR_RED = { 255, 0, 0 };
	static const ColorU32 COLOR_GREEN = { 0, 255, 0 };
	static const ColorU32 COLOR_BLUE = { 0, 0, 255 };
	static const ColorU32 COLOR_YELLOW = { 255, 255, 0 };
	static const ColorU32 COLOR_MAGENTA = { 255, 0, 255 };
	static const ColorU32 COLOR_PURPLE = { 144, 0, 144 };
	static const ColorU32 COLOR_CYAN = { 0, 255, 255 };
	static const ColorU32 COLOR_ORANGE = { 255, 170, 0 };
	static const ColorU32 COLOR_NAVY = { 0, 0, 128 };
	static const ColorU32 COLOR_PINK = { 255, 0, 255 };
}
