//
// File: nvtthelper.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <d3d11.h>
#include "nvtt.h"
#include "rage/grcore/texture/utils.h"

namespace rageam
{
	inline nvtt::Format DxgiFormatToNvtt(DXGI_FORMAT fmt)
	{
		switch (fmt)
		{
		case DXGI_FORMAT_B8G8R8A8_UNORM: return nvtt::Format_RGBA;
		case DXGI_FORMAT_BC1_UNORM: return nvtt::Format_BC1;
		case DXGI_FORMAT_BC2_UNORM: return nvtt::Format_BC2;
		case DXGI_FORMAT_BC3_UNORM: return nvtt::Format_BC3;
		case DXGI_FORMAT_BC4_UNORM: return nvtt::Format_BC4;
		case DXGI_FORMAT_BC5_UNORM: return nvtt::Format_BC5;
		case DXGI_FORMAT_BC7_UNORM: return nvtt::Format_BC7;
		default:;
		}

		AM_ASSERT(0, "DxgiFormatToNvtt() -> Format: %s is not supported!", D3D::DxgiFormatToString(fmt));
		return nvtt::Format_Unset;
	}

	struct NvttRawOutputHandler : nvtt::OutputHandler
	{
		char* Buffer = nullptr;
		int	Size = 0;
		int	Offset = 0;

		~NvttRawOutputHandler() override
		{
			delete[] Buffer;
		}

		void beginImage(int size, int width, int height, int depth, int face, int miplevel) override
		{
			// If previous buffer can fit image, we can keep it
			if (Buffer && Size < size)
			{
				delete[] Buffer;
				Buffer = nullptr;
			}

			if (!Buffer) Buffer = new char[size];
			Offset = 0;
			Size = size;
		}

		bool writeData(const void* data, int size) override
		{
			memcpy(Buffer + Offset, data, size);
			Offset += size;

			return true;
		}

		void endImage() override {}
	};
}
