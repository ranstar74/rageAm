#pragma once
#include "compressor.h"

namespace rageam::texture
{
	struct CompressedInfo;

	class CompressorCache
	{
	public:
		static bool RetrieveFromCache(ConstWString path, const CompressedInfo& outInfo, char** ppPixelData)
		{
			return false;
		}
	};
}
