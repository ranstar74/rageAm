#pragma once

#include <fstream>
#include <utility>

#include "common/types.h"

namespace rageam::texture
{
	// Retrieves width & height from .png file header.
	inline std::pair<u32, u32> GetPNGResolution(const wchar_t* path)
	{
		std::ifstream in(path);
		if (!AM_VERIFY(in.is_open(), L"GetPNGResolution() -> Unable to open %ls", path))
		{
			return { 0, 0 };
		}

		unsigned char buf[8];
		in.seekg(16);
		in.read(reinterpret_cast<char*>(&buf), 8);

		u32 width = (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + (buf[3] << 0);
		u32 height = (buf[4] << 24) + (buf[5] << 16) + (buf[6] << 8) + (buf[7] << 0);

		return { width, height };
	}
}
