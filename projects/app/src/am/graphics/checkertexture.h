//
// File: checkertexture.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/ptr.h"
#include "rage/grcore/texture/texturedx11.h"

namespace rageam::graphics
{
	/**
	 * \brief Source engine - like checker texture for missing textures.
	 */
	class CheckerTexture
	{
		amUniquePtr<rage::grcTextureDX11> m_Texture;
	public:
		CheckerTexture() = default;

		void Init(u32 size = 512, u32 tileSize = 8)
		{
			static constexpr DXGI_FORMAT pixelFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			static constexpr u32 COLOR0 = 0x00000000;
			static constexpr u32 COLOR1 = 0xFFFF00FF;

			u32 byteWidth;
			D3D::ComputePitch(pixelFormat, size, size, 0, nullptr, &byteWidth);

			auto pixelData = amUniquePtr<u32[]>(new u32[byteWidth]);

			for (u32 x = 0; x < size; x++)
			{
				bool isEvenRow = x / tileSize % 2 == 0;
				for (u32 y = 0; y < size; y++)
				{
					bool isEvenCol = y / tileSize % 2 == 0;

					u32 color0 = isEvenCol ? COLOR0 : COLOR1;
					u32 color1 = isEvenCol ? COLOR1 : COLOR0;
					u32 color = isEvenRow ? color0 : color1;

					u32 pixel = x + size * y;
					pixelData[pixel] = color;
				}
			}

			m_Texture = std::make_unique<rage::grcTextureDX11>(size, size, 1, 1, pixelFormat, pixelData.get(), false);
		}

		rage::grcTextureDX11* GetTexture() const { return m_Texture.get(); }
	};
}
