#pragma once

#include "txd.h"
#include "am/graphics/color.h"
#include "am/graphics/render/context.h"
#include "rage/grcore/texture/texturedx11.h"

namespace rageam::asset
{
	inline rage::grcTexture* CreateSingleColorTexture(graphics::ColorU32 col)
	{
		rage::grcTextureDX11* texture = new rage::grcTextureDX11(
			1, 1, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &col, false);
		return texture;
	}

	// Missing texture name is composed in format:
	// TEX_NAME (Missing)###$MT_TEXNAME
	// ## is ImGui special char sequence, everything past ## is ignored visually but used as ID
	// Actual texture name is stored after '$MT_' prefix
	inline rage::grcTexture* CreateMissingTexture(ConstString textureName)
	{
		char nameBuffer[Texture::MAX_NAME];
		sprintf_s(nameBuffer, sizeof nameBuffer, "%s (Missing)##$MT_%s", textureName, textureName);

		auto texture = new rage::grcTextureDX11(*GRenderContext->CheckerTexture.GetTexture());
		texture->SetName(nameBuffer);
		return texture;
	}
	
	// Gets whether given texture was created using CreateMissingTexture function
	inline bool IsMissingTexture(const rage::grcTexture* texture)
	{
		ImmutableString texName = texture->GetName();
		return texName.Contains("(Missing)##$MT_");
	}

	// Gets actual name from missing texture name (which is in format 'TEX_NAME (Missing)###$MT_TEXNAME')
	// Returns NULL if texture is not missing or if texture was NULL
	inline ConstString GetMissingTextureName(const rage::grcTexture* texture)
	{
		if (!texture) 
			return nullptr;
		ImmutableString texName = texture->GetName();
		int tokenIndex = texName.IndexOf("##$MT_");
		if (tokenIndex == -1)
			return nullptr;
		return texName.Substring(tokenIndex + 6); // Length of ##$MT_
	}
}
