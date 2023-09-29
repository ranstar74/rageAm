#pragma once

#include "am/system/asserts.h"
#include "common/types.h"

#undef min
#undef max

namespace rageam::texture
{
	enum eScalingMode
	{
		ScalingMode_Fill,
		ScalingMode_Fit,
		ScalingMode_Stretch,
	};

	inline std::pair<u32, u32> Resize(u32 wIn, u32 hIn, u32 wTo, u32 hTo, eScalingMode mode)
	{
		u32 maxIn = std::max(wIn, hIn);
		u32 minIn = std::min(wIn, hIn);
		u32 minTo = std::min(wTo, hTo);
		u32 maxTo = std::max(wTo, hTo);
		float ratio;

		switch (mode)
		{
		case ScalingMode_Stretch:
			return { wTo, hTo };
		case ScalingMode_Fill:
			ratio = static_cast<float>(maxTo) / static_cast<float>(minIn);
			return
			{
				static_cast<u32>(static_cast<float>(wIn) * ratio),
				static_cast<u32>(static_cast<float>(hIn) * ratio)
			};
		case ScalingMode_Fit:
			ratio = static_cast<float>(minTo) / static_cast<float>(maxIn);
			return
			{
				static_cast<u32>(static_cast<float>(wIn) * ratio),
				static_cast<u32>(static_cast<float>(hIn) * ratio)
			};
		}
		AM_UNREACHABLE("TextureResize() -> Mode %i is not supported.", mode);
	}
}
