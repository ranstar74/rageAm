#pragma once

#include "am/graphics/checkertexture.h"
#include "debugrender.h"

namespace rageam::graphics
{
	struct RenderContext
	{
		CheckerTexture	CheckerTexture;
		DebugRender		DebugRender;

		void Init()
		{
			CheckerTexture.Init();
#ifndef AM_STANDALONE
			DebugRender.Init();
#endif
		}
	};
}

extern rageam::graphics::RenderContext* GRenderContext;

void CreateRenderContext();
void DestroyRenderContext();
