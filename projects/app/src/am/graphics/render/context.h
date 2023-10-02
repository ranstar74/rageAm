#pragma once

#include "am/graphics/checkertexture.h"
#include "debugrender.h"
#include "am/graphics/overlayrender.h"

namespace rageam::graphics
{
	struct RenderContext
	{
		CheckerTexture	CheckerTexture;
		DebugRender		DebugRender;
		OverlayRender	OverlayRender;

		void Init()
		{
			CheckerTexture.Init();
#ifndef AM_STANDALONE
			DebugRender.Init();
#endif
			OverlayRender.Init();
		}
	};
}

extern rageam::graphics::RenderContext* GRenderContext;

void CreateRenderContext();
void DestroyRenderContext();
