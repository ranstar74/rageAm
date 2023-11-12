#pragma once

#include "am/graphics/checkertexture.h"

namespace rageam::graphics
{
	struct RenderContext
	{
		CheckerTexture	CheckerTexture;

		void Init()
		{
			CheckerTexture.Init();
		}
	};
}

extern rageam::graphics::RenderContext* GRenderContext;

void CreateRenderContext();
void DestroyRenderContext();
