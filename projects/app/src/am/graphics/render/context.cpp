#include "context.h"

rageam::graphics::RenderContext* GRenderContext = nullptr;

void CreateRenderContext()
{
	GRenderContext = new rageam::graphics::RenderContext();
	//GRenderContext->Init();
}

void DestroyRenderContext()
{
	delete GRenderContext;
	GRenderContext = nullptr;
}
