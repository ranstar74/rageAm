#pragma once

namespace rageam::texture
{
	struct CompressedInfo;

	// Loads DDS pixel data from file and parses metadata
	// NOTE: Pixel data is raw pointer!
	bool LoadDDSFromFile(const wchar_t* path, CompressedInfo& info, char** ppPixelData);
}
