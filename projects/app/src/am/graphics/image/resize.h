//
// File: resize.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "image.h"

namespace rageam::graphics
{
	class ImageResizer
	{
	public:
		static void Resample(
			pVoid src, pVoid dst, ImagePixelFormat fmt, int xFrom, int yFrom, int xTo, int yTo);
	};
}
