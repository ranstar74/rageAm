//
// File: textureinfo.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"

namespace rage
{
	struct grcTextureInfo
	{
		u32 MipLevels;
		u64 qword8;
		u32 RowPitch;
		u32 BitsPerPixel;
		u32 Width;
		u32 Height;
		u32 Format;
		u32 dword24;
	};
}
