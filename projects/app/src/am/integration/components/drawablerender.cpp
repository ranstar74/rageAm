#include "drawablerender.h"

#ifdef AM_INTEGRATED

#include "am/integration/im3d.h"
#include "am/integration/gamedrawlists.h"
#include "rage/physics/bounds/composite.h"

void rageam::integration::DrawableRender::RenderBoneRecurse(rage::crSkeletonData* skel, const rage::crBoneData* rootBone, u32 depth)
{
	DrawList& dl = GameDrawLists::GetInstance()->Overlay;

	u32 colors[] =
	{
		graphics::ColorU32(0, 229, 168),	// Aquamarine
		graphics::ColorU32(255, 104, 89),	// Tomato
		graphics::ColorU32(21, 209, 1698),  // Cyan
		graphics::ColorU32(255, 188, 43),	// Orange
		graphics::ColorU32(92, 206, 255),	// Light Blue
	};

	rage::Mat44V rootWorld = skel->GetBoneWorldTransform(rootBone);

	// TODO: We need push transform for Im3D
	// Bone label
	if (BoneTags)
	{
		rage::Vec3V textWorld = Vec3V(rootWorld.Pos).Transform(m_Entity->GetWorldTransform());
		Im3D::CenterNext();
		Im3D::TextBg(textWorld, "Bone<%s, Tag: %u>", rootBone->GetName(), rootBone->GetBoneTag());
	}

	// Child bones
	rage::crBoneData* childBone = skel->GetFirstChildBone(rootBone->GetIndex());
	while (childBone)
	{
		rage::Mat44V childWorld = skel->GetBoneWorldTransform(childBone);

		dl.DrawLine(rootWorld.Pos, childWorld.Pos, colors[depth % std::size(colors)]);
		RenderBoneRecurse(skel, childBone, depth + 1);
		childBone = skel->GetBone(childBone->GetNextIndex());
	}
}

void rageam::integration::DrawableRender::RenderBound_Geometry(const rage::phBoundGeometry* bound) const
{
	DrawList& dl = GameDrawLists::GetInstance()->CollisionMesh;

	// TODO: Can we optimize this to use to use indices properly?
	for (u16 i = 0; i < bound->GetPolygonCount(); i++)
	{
		Vec3V v1, v2, v3;

		rage::phPolygon& tri = bound->GetPolygon(i);
		bound->DecompressPoly(tri, v1, v2, v3);

		dl.DrawTriFill(v1, v2, v3, CollisionBoundColor);
	}
}

void rageam::integration::DrawableRender::RenderCollisionBound(rage::phBound* bound)
{
	if (!bound)
		return;

	Mat44V parentTransform = DrawList::GetTransform();
	switch (bound->GetShapeType())
	{
	case rage::PH_BOUND_GEOMETRY: RenderBound_Geometry((rage::phBoundGeometry*)bound); break;
	case rage::PH_BOUND_COMPOSITE:
	{
		rage::phBoundComposite* composite = (rage::phBoundComposite*)bound;
		u16 numBounds = composite->GetNumBounds();
		for (u16 i = 0; i < numBounds; i++)
		{
			DrawList::SetTransform(composite->GetMatrix(i) * parentTransform);
			RenderCollisionBound(composite->GetBound(i).Get());
		}
		break;
	}

	default: break;
	}
	DrawList::SetTransform(parentTransform);
}

void rageam::integration::DrawableRender::OnUpdate()
{
	if (!m_Entity || !IsVisible)
		return;

	DrawList::SetTransform(m_Entity->GetWorldTransform());
	{
		GameDrawLists* drawLists = GameDrawLists::GetInstance();
		gtaDrawable*   drawable = m_Entity->GetDrawable();

		DrawList& dlBounds = DrawBoundsOnTop ? drawLists->Overlay : drawLists->GameUnlit;
		if (BoundingBox)    dlBounds.DrawAABB(drawable->GetBoundingBox(), BoundingOutlineColor);
		if (BoundingSphere) dlBounds.DrawSphere(drawable->GetBoundingSphere(), BoundingOutlineColor);

		rage::crSkeletonData* skel = drawable->GetSkeletonData().Get();
		if (Skeleton && skel)
		{
			RenderBoneRecurse(skel, skel->GetBone(0)); // Start with root bone
		}

		if (Collision)
		{
			RenderCollisionBound(drawable->GetBound().Get());
		}
	}
	DrawList::ResetTransform();
}

#endif
