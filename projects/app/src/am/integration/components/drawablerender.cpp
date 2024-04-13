#include "drawablerender.h"

#ifdef AM_INTEGRATED

#include "am/integration/im3d.h"
#include "am/integration/gamedrawlists.h"
#include "rage/physics/bounds/boundcomposite.h"
#include "rage/physics/bounds/boundbvh.h"
#include "rage/physics/bounds/boundprimitives.h"
#include "rage/math/mathv.h"

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

void rageam::integration::DrawableRender::RenderBound_BVH(const rage::phBoundBVH* bound) const
{
	DrawList& dl = GameDrawLists::GetInstance()->CollisionMesh;

	// Taken from model inspector for BVH debugging
	/*
	auto octree = bound->GetBVH();
	static int m_SelectedBoundBVHDepthDraw = 0;
	int m_SelectedBoundBVHDepth = octree ? octree->ComputeDepth() : 0;
	ImGui::Begin("BVH");
	ImGui::SetNextItemWidth(GImGui->FontSize * 5); // Slider is way too wide
	ImGui::SliderInt("Visualize Depth", &m_SelectedBoundBVHDepthDraw, -1, m_SelectedBoundBVHDepth);
	ImGui::ToolTip("Draws bounding box of BVH nodes at selected depth");
	ImGui::End();
	bound->GetBVH()->IterateTree([&](rage::phOptimizedBvhNode& node, int depth)
		{
			if (m_SelectedBoundBVHDepthDraw != depth)
				return;

			dl.DrawAABB(octree->GetNodeAABB(node), graphics::COLOR_RED);
		});
	*/

	int primCount = bound->GetPrimitiveCount();
	for (int i = 0; i < primCount; i++)
	{
		rage::phPrimitive& prim = bound->GetPrimitive(i);
		switch (prim.GetType())
		{
		case rage::PRIM_TYPE_POLYGON:
		{
			rage::phPolygon& poly = prim.GetPolygon();
			Vec3V v1 = bound->GetVertex(poly.GetVertexIndex(0));
			Vec3V v2 = bound->GetVertex(poly.GetVertexIndex(1));
			Vec3V v3 = bound->GetVertex(poly.GetVertexIndex(2));
			dl.DrawLine(v1, v2, CollisionBoundColor);
			dl.DrawLine(v2, v3, CollisionBoundColor);
			dl.DrawLine(v3, v1, CollisionBoundColor);
			break;
		}
		case rage::PRIM_TYPE_SPHERE:
		{
			rage::phPrimSphere& sphere = prim.GetSphere();
			Vec3V center = bound->GetVertex(sphere.GetCenterIndex());
			dl.DrawSphere(Sphere(center, sphere.GetRadius()), CollisionBoundColor);
			break;
		}
		case rage::PRIM_TYPE_CAPSULE:
		{
			rage::phPrimCapsule& capsule = prim.GetCapsule();
			float radius = capsule.GetRadius();
			Vec3V end0 = bound->GetVertex(capsule.GetEndIndex0());
			Vec3V end1 = bound->GetVertex(capsule.GetEndIndex1());
			Vec3V normal = (end0 - end1).Normalized();
			dl.DrawCapsule(radius, normal, end0, end1, CollisionBoundColor);
			break;
		}
		case rage::PRIM_TYPE_BOX:
		{
			// https://i.imgur.com/FmYrPUb.png
			rage::phPrimBox& box = prim.GetBox();
			// Indices are in format: FaceIndex|VertexIndex
			Vec3V v00 = bound->GetVertex(box.GetVertexIndex(0));
			Vec3V v01 = bound->GetVertex(box.GetVertexIndex(1));
			Vec3V v10 = bound->GetVertex(box.GetVertexIndex(2));
			Vec3V v11 = bound->GetVertex(box.GetVertexIndex(3));
			Vec3V center0 = (v00 + v01) * rage::S_HALF;
			Vec3V center1 = (v10 + v11) * rage::S_HALF;
			Vec3V normal0 = (center0 - center1).Normalized();
			Vec3V normal1 = -normal0;
			// Project vertices from opposite planes
			Vec3V v02 = ProjectOnPlane(v10, center0, normal0);
			Vec3V v03 = ProjectOnPlane(v11, center0, normal0);
			Vec3V v12 = ProjectOnPlane(v00, center1, normal1);
			Vec3V v13 = ProjectOnPlane(v01, center1, normal1);
			// Face 0
			dl.DrawLine(v00, v02, CollisionBoundColor);
			dl.DrawLine(v02, v01, CollisionBoundColor);
			dl.DrawLine(v01, v03, CollisionBoundColor);
			dl.DrawLine(v03, v00, CollisionBoundColor);
			// Face 1
			dl.DrawLine(v10, v12, CollisionBoundColor);
			dl.DrawLine(v12, v11, CollisionBoundColor);
			dl.DrawLine(v11, v13, CollisionBoundColor);
			dl.DrawLine(v13, v10, CollisionBoundColor);
			// Connecting lines
			dl.DrawLine(v03, v11, CollisionBoundColor);
			dl.DrawLine(v01, v13, CollisionBoundColor);
			dl.DrawLine(v02, v10, CollisionBoundColor);
			dl.DrawLine(v00, v12, CollisionBoundColor);
			break;
		}
		case rage::PRIM_TYPE_CYLINDER:
		{
			rage::phPrimCylinder& cylinder = prim.GetCylinder();
			float radius = cylinder.GetRadius();
			Vec3V end0 = bound->GetVertex(cylinder.GetEndIndex0());
			Vec3V end1 = bound->GetVertex(cylinder.GetEndIndex1());
			Vec3V normal = (end0 - end1).Normalized();
			dl.DrawCylinder(radius, normal, end0, end1, CollisionBoundColor);
			break;
		}

		default:
			break;
		}
	}
}

void rageam::integration::DrawableRender::RenderCollisionBound(rage::phBound* bound)
{
	DrawList& dl = GameDrawLists::GetInstance()->CollisionMesh;

	if (!bound)
		return;

	// Matrix for drawing cylinder & capsule because they're Y axis up (god knows why)
	Mat44V yUp;
	yUp.Right = { 1, 0, 0, 0 };
	yUp.Front = { 0, 0, 1, 0 };
	yUp.Up = { 0, 1, 0, 0 };
	yUp.Pos = { 0, 0, 0, 1 };

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
	case rage::PH_BOUND_BOX:		dl.DrawAABB(bound->GetBoundingBox(), CollisionBoundColor);		break;
	case rage::PH_BOUND_SPHERE:		dl.DrawSphere(bound->GetBoundingSphere(), CollisionBoundColor); break;
	case rage::PH_BOUND_CYLINDER:
		{
			rage::phBoundCylinder* cylinder = (rage::phBoundCylinder*)bound;
			DrawList::SetTransform(yUp * parentTransform);
			dl.DrawCylinder(cylinder->GetRadius(), cylinder->GetHalfHeight(), CollisionBoundColor);
			DrawList::SetTransform(parentTransform);
			break;
		}
	case rage::PH_BOUND_CAPSULE:
		{
			rage::phBoundCapsule* capsule = (rage::phBoundCapsule*)bound;
			DrawList::SetTransform(yUp * parentTransform);
			dl.DrawCapsule(capsule->GetRadius(), capsule->GetHalfHeight(), CollisionBoundColor);
			DrawList::SetTransform(parentTransform);
			break;
		}
	case rage::PH_BOUND_BVH: RenderBound_BVH((rage::phBoundBVH*)bound);

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
