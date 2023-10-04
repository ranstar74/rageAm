#pragma once

#include "lightattr.h"
#include "am/graphics/render/context.h"
#include "am/ui/extensions.h"
#include "rage/framework/pool.h"
#include "rage/paging/template/array.h"
#include "rage/physics/inst.h"
#include "rage/physics/bounds/boundbase.h"
#include "rage/rmc/drawable.h"

class gtaDrawable : public rage::rmcDrawable
{
public: // TEST
	rage::pgArray<CLightAttr>	m_Lights;
	u64							m_UnknownC0;
	rage::phBoundPtr			m_Bound;

public:
	gtaDrawable() = default;

	// ReSharper disable once CppPossiblyUninitializedMember
	gtaDrawable(const rage::datResource& rsc) : rmcDrawable(rsc)
	{

	}

	gtaDrawable(const gtaDrawable& other) : rage::rmcDrawable(other)
	{

	}

	~gtaDrawable() override
	{

	}

	u16 GetLightCount() const { return m_Lights.GetSize(); }
	CLightAttr* GetLight(u16 index) { return &m_Lights[index]; }

	void SetBound(rage::phBound* bound) { m_Bound = bound; }
	rage::phBound* GetBound() const { return m_Bound.Get(); }

	//void RenderBoneRecurse(rage::crSkeletonData* skel, const rage::crBoneData* rootBone, const rage::Mat44V& mtx, u32 depth = 0)
	//{
	//	u32 colors[] =
	//	{
	//		rageam::graphics::COLOR_BLUE,
	//		rageam::graphics::COLOR_NAVY,
	//		rageam::graphics::COLOR_RED,
	//		rageam::graphics::COLOR_ORANGE,
	//		rageam::graphics::COLOR_YELLOW,
	//	};
	//	static constexpr u32 colorCount = 5;

	//	rage::crBoneData* childBone = skel->GetFirstChildBone(rootBone->GetIndex());
	//	rage::Vec3V rootWorld = skel->GetBoneWorldTransform(rootBone).Pos;
	//	while (childBone)
	//	{
	//		rage::Vec3V childWorld = skel->GetBoneWorldTransform(childBone).Pos;

	//		GRenderContext->OverlayRender.DrawLine(
	//			rootWorld, childWorld, mtx, colors[depth % colorCount]);
	//		RenderBoneRecurse(skel, childBone, mtx, depth + 1);
	//		childBone = skel->GetBone(childBone->GetNextIndex());
	//	}
	//}

	//void DrawDebug(const rage::Mat44V& mtx)
	//{
	//	// TODO: Move bools and well... code
	//	if (GRenderContext->DebugRender.bRenderLodGroupExtents)
	//		GRenderContext->OverlayRender.DrawAABB(m_LodGroup.GetBoundingBox(), mtx, rageam::graphics::COLOR_GREEN);

	//	rage::crSkeletonData* skel = m_SkeletonData.Get();
	//	if (skel && GRenderContext->DebugRender.bRenderSkeleton)
	//	{
	//		for (u16 k = 0; k < skel->GetBoneCount(); k++)
	//		{
	//			RenderBoneRecurse(skel, skel->GetBone(k), mtx);
	//		}
	//	}

	//	if (GRenderContext->DebugRender.bRenderBoundExtents && GetBound())
	//		GRenderContext->OverlayRender.DrawAABB(GetBound()->GetBoundingBox(), mtx, rageam::graphics::COLOR_WHITE);


	//	GRenderContext->DebugRender.RenderDrawable(this, mtx);
	//}

	void Draw(const rage::Mat34V& mtx, rage::grcDrawBucketMask mask, rage::eDrawableLod lod) override
	{
		rmcDrawable::Draw(mtx, mask, lod);
		//DrawDebug(mtx.To44());
	}

	void DrawSkinned(const rage::Mat34V& mtx, u64 a3, rage::grcDrawBucketMask mask, rage::eDrawableLod lod) override
	{
		rmcDrawable::DrawSkinned(mtx, a3, mask, lod);
		//DrawDebug(mtx.To44());
	}
};
