#include "debugrender.h"

#include "game/drawable.h"

void rageam::graphics::DebugRender::RenderDrawable(gtaDrawable* drawable, const rage::Mat44V& mtx)
{
	rage::phBound* bound = drawable->GetBound();
	rage::crSkeletonData* skeleton = drawable->GetSkeletonData();
	rage::rmcLodGroup& lodGroup = drawable->GetLodGroup();

	// if (bRenderBoundMesh) Render(bound, mtx);
	/*if (bRenderSkeleton) Render(skeleton, mtx);
	if (bRenderLodGroupExtents) Render(lodGroup.GetBoundingBox(), mtx, 0xFF0000FF);
	if (bRenderGeometryExtents)
	{
		for (auto& model : lodGroup.GetLod(0)->GetModels())
		{
			Render(model->GetCombinedBoundingBox(), mtx, 0x00FF00FF);
		}
	}*/
}
