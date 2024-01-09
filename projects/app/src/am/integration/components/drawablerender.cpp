#include "drawablerender.h"

#ifdef AM_INTEGRATED

#include "am/integration/im3d.h"
#include "rage/physics/bounds/composite.h"

void rageam::integration::DrawableRender::RenderBoneRecurse(rage::crSkeletonData* skel, const rage::crBoneData* rootBone, u32 depth)
{
	DrawList& dlFg = GameIntegration::GetInstance()->DrawListForeground;

	u32 colors[] =
	{
		graphics::COLOR_BLUE,
		graphics::COLOR_NAVY,
		graphics::COLOR_RED,
		graphics::COLOR_ORANGE,
		graphics::COLOR_YELLOW,
	};
	static constexpr u32 colorCount = 5;

	rage::crBoneData* childBone = skel->GetFirstChildBone(rootBone->GetIndex());
	rage::Vec3V rootWorldPos = skel->GetBoneWorldTransform(rootBone).Pos;
	while (childBone)
	{
		rage::Vec3V childWorld = skel->GetBoneWorldTransform(childBone).Pos;

		dlFg.DrawLine(rootWorldPos, childWorld, colors[depth % colorCount]);
		RenderBoneRecurse(skel, childBone, depth + 1);
		childBone = skel->GetBone(childBone->GetNextIndex());
	}
}

void rageam::integration::DrawableRender::RenderBound_Geometry(const rage::phBoundGeometry* bound) const
{
	DrawList& drawList = GameIntegration::GetInstance()->DrawListCollision;

	// TODO: Can we optimize this to use to use indices properly?
	for (u16 i = 0; i < bound->GetPolygonCount(); i++)
	{
		Vec3V v1, v2, v3;

		rage::phPrimTriangle& tri = bound->GetPolygon(i);
		bound->DecompressPoly(tri, v1, v2, v3);

		drawList.DrawTriFill(v1, v2, v3, graphics::ColorU32(255, 200, 60));
	}
}

void rageam::integration::DrawableRender::RenderBound(rage::phBound* bound)
{
	if (!bound)
		return;

	DrawList& drawList = GameIntegration::GetInstance()->DrawListCollision;

	Mat44V oldTransform = drawList.GetTransform();
	switch (bound->GetShapeType())
	{
	case rage::PH_BOUND_GEOMETRY: RenderBound_Geometry((rage::phBoundGeometry*)bound); break;
	case rage::PH_BOUND_COMPOSITE:
	{
		rage::phBoundComposite* composite = (rage::phBoundComposite*)bound;
		u16 numBounds = composite->GetNumBounds();
		for (u16 i = 0; i < numBounds; i++)
		{
			drawList.SetTransform(composite->GetMatrix(i) * oldTransform);
			RenderBound(composite->GetBound(i).Get());
		}
		break;
	}

	default: break;
	}
	drawList.SetTransform(oldTransform);
}

void rageam::integration::DrawableRender::OnEarlyUpdate()
{
	DrawList& dlGame = GameIntegration::GetInstance()->DrawListGame;
	DrawList& dlFg = GameIntegration::GetInstance()->DrawListForeground;

	dlGame.SetTransform(m_Entity->GetWorldTransform());
	{
		gtaDrawable* drawable = m_Entity->GetDrawable();
		rage::crSkeletonData* skel = drawable->GetSkeletonData().Get();

		if (BoundingBox)
		{
			dlFg.DrawAABB(drawable->GetBoundingBox(), graphics::ColorU32(0, 255, 0, 200));
		}

		if (BoundingSphere)
		{
			const Sphere& bs = drawable->GetBoundingSphere();
			dlFg.DrawSphere(bs, graphics::ColorU32(0, 255, 0, 200));
		}

		if (Skeleton && skel)
		{
			for (u16 k = 0; k < skel->GetBoneCount(); k++)
			{
				RenderBoneRecurse(skel, skel->GetBone(k));
			}
		}

		if (Collision)
		{
			RenderBound(drawable->GetBound().Get());
		}
	}
	dlGame.ResetTransform();
}

void rageam::integration::DrawableRender::OnUiUpdate()
{
	if (!m_Entity)
		return;
	
	gtaDrawable* drawable = m_Entity->GetDrawable();
	rage::crSkeletonData* skel = drawable->GetSkeletonData().Get();

	// Bone names
	if (Skeleton && skel)
	{
		std::function<void(const rage::crBoneData*)> boneLabelRecurse;
		boneLabelRecurse = [&](const rage::crBoneData* bone)
			{
				if (!bone) return;

				rage::Mat44V rootWorld = skel->GetBoneWorldTransform(bone) * m_Entity->GetWorldTransform();

				Im3D::CenterNext();
				Im3D::TextBg(rootWorld.Pos, "Bone<%s, Tag: %u>", bone->GetName(), bone->GetBoneTag());

				// Child tree
				boneLabelRecurse(skel->GetFirstChildBone(bone->GetIndex()));
				// Siblings
				while (bone->GetNextIndex() != -1)
				{
					bone = skel->GetBone(bone->GetNextIndex());
					boneLabelRecurse(bone);
				}
			};

		boneLabelRecurse(skel->GetBone(0));
	}
}

#endif
