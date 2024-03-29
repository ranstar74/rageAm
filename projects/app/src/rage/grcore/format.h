//
// File: format.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <dxgiformat.h>

#include "am/system/asserts.h"

namespace rage
{
	enum grcFormat
	{
		GRC_FORMAT_R16_FLOAT,
		GRC_FORMAT_R16G16_FLOAT,
		GRC_FORMAT_UNKNOWN,
		GRC_FORMAT_R16G16B16A16_FLOAT,
		GRC_FORMAT_R32_FLOAT,
		GRC_FORMAT_R32G32_FLOAT,
		GRC_FORMAT_R32G32B32_FLOAT,
		GRC_FORMAT_R32G32B32A32_FLOAT,
		GRC_FORMAT_R8G8B8A8_UINT,
		GRC_FORMAT_R8G8B8A8_UNORM,
		GRC_FORMAT_R8G8B8A8_SNORM,
		GRC_FORMAT_R16_UNORM,
		GRC_FORMAT_R16G16_UNORM,
		GRC_FORMAT_R8G8_UNORM,
		GRC_FORMAT_R16G16_SINT,
		GRC_FORMAT_R16G16B16A16_SINT,
	};

	constexpr DXGI_FORMAT grcFormatToDXGI[] =
	{
		DXGI_FORMAT_R16_FLOAT,
		DXGI_FORMAT_R16G16_FLOAT,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UINT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R8G8B8A8_SNORM,
		DXGI_FORMAT_R16_UNORM,
		DXGI_FORMAT_R16G16_UNORM,
		DXGI_FORMAT_R8G8_UNORM,
		DXGI_FORMAT_R16G16_SINT,
		DXGI_FORMAT_R16G16B16A16_SINT,
	};

	constexpr u32 grcFormatToDataSize[] =
	{
		2, 4, 6, 8, 4, 8, 12, 16, 4, 4, 4, 2, 4, 2, 4, 8
	};

	constexpr grcFormat DXGIFormatToGrcFormat(DXGI_FORMAT format)
	{
		switch (format) // NOLINT(clang-diagnostic-switch-enum)
		{
		case DXGI_FORMAT_R16_FLOAT:				return GRC_FORMAT_R16_FLOAT;
		case DXGI_FORMAT_R16G16_FLOAT:			return GRC_FORMAT_R16G16_FLOAT;
		case DXGI_FORMAT_UNKNOWN:				return GRC_FORMAT_UNKNOWN;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:	return GRC_FORMAT_R16G16B16A16_FLOAT;
		case DXGI_FORMAT_R32_FLOAT:				return GRC_FORMAT_R32_FLOAT;
		case DXGI_FORMAT_R32G32_FLOAT:			return GRC_FORMAT_R32G32_FLOAT;
		case DXGI_FORMAT_R32G32B32_FLOAT:		return GRC_FORMAT_R32G32B32_FLOAT;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:	return GRC_FORMAT_R32G32B32A32_FLOAT;
		case DXGI_FORMAT_R8G8B8A8_UINT:			return GRC_FORMAT_R8G8B8A8_UINT;
		case DXGI_FORMAT_R8G8B8A8_UNORM:		return GRC_FORMAT_R8G8B8A8_UNORM;
		case DXGI_FORMAT_R8G8B8A8_SNORM:		return GRC_FORMAT_R8G8B8A8_SNORM;
		case DXGI_FORMAT_R16_UNORM:				return GRC_FORMAT_R16_UNORM;
		case DXGI_FORMAT_R16G16_UNORM:			return GRC_FORMAT_R16G16_UNORM;
		case DXGI_FORMAT_R8G8_UNORM:			return GRC_FORMAT_R8G8_UNORM;
		case DXGI_FORMAT_R16G16_SINT:			return GRC_FORMAT_R16G16_SINT;
		case DXGI_FORMAT_R16G16B16A16_SINT:		return GRC_FORMAT_R16G16B16A16_SINT;
		default: break;
		}
		AM_UNREACHABLE("DXGIToGrcFormat() -> Format %u is invalid.", format);
	}

#define GRC_FORMAT_INVALID grcFormat(-1)
}
