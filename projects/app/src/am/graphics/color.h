#pragma once

#include "common/types.h"
#include "rage/math/vec.h"

namespace rageam::graphics
{
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

		ColorU32(u32 value) { Value = value; }
		ColorU32(u8 r, u8 g, u8 b, u8 a) { R = r; G = g; B = b; A = a; }

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

		operator u32() const { return Value; }
	};

	static const ColorU32 COLOR_WHITE = { 255, 255, 255, 255 };
	static const ColorU32 COLOR_BLACK = { 0, 0, 0, 255 };
	static const ColorU32 COLOR_RED = { 255, 0, 0, 255 };
	static const ColorU32 COLOR_GREEN = { 0, 255, 0, 255 };
	static const ColorU32 COLOR_BLUE = { 0, 0, 255, 255 };
	static const ColorU32 COLOR_YELLOW = { 255, 255, 0, 255 };
	static const ColorU32 COLOR_MAGENTA = { 255, 0, 255, 255 };
	static const ColorU32 COLOR_PURPLE = { 144, 0, 144, 255 };
	static const ColorU32 COLOR_CYAN = { 0, 255, 255, 255 };
	static const ColorU32 COLOR_ORANGE = { 255, 170, 0, 255 };
	static const ColorU32 COLOR_NAVY = { 0, 0, 128, 255 };
}
