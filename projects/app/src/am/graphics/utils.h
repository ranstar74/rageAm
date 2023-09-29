#pragma once

#include <dxgiformat.h>
#include <DirectXTex.h>

#include "am/system/asserts.h"
#include "am/system/enum.h"

namespace rageam::graphics
{
	// Gets size of format data type, in bytes.
	// For example return value is '4' for DXGI_FORMAT_R8G8B8A8_UINT (u32)
	// Throws assert if data type data size is fraction of byte
	// (Num of bits not multiple of 8)
	inline size_t FormatStride(DXGI_FORMAT fmt)
	{
		size_t bpp = DirectX::BitsPerPixel(fmt);
		AM_ASSERT(bpp % 8 == 0, "FormatStride() -> Format %s is not allowed.",
			Enum::GetName(fmt));
		return bpp / 8;
	}
}
