//
// File: lighteditor.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/graphics/ray.h"
#include "am/graphics/shapetest.h"
#include "game/drawable.h"

namespace rageam::integration
{
	enum eGimzoMode
	{
		GIZMO_None,
		GIZMO_Translate,
		GIZMO_Rotate,
	};

	class LightEditor
	{
		int	m_SelectedLight = -1;
		int	m_HoveredLight = -1;
		int	m_GizmoMode = GIZMO_None;

		struct LightDrawContext
		{
			rage::Vec3V		CamFront, CamRight, CamUp;
			graphics::Ray	WorldMouseRay;
			rage::Mat44V	LightWorld;
			CLightAttr*		Light;
			bool			IsSelected;
			u32				PrimaryColor;
			u32				SecondaryColor;
			u32				CulledColor;
		};

		u32 GetOutlinerColor(bool isSelected, bool isHovered, bool isPrimary) const;

		// Tests if screen mouse cursor intersects with light bounding sphere
		graphics::ShapeHit ProbeLightSphere(const LightDrawContext& ctx) const;

		// TODO: We should add IsSphereVisible in viewport to quickly cull out outlines

		graphics::ShapeHit DrawOutliner_Point(const LightDrawContext& ctx) const;
		graphics::ShapeHit DrawOutliner_Spot(const LightDrawContext& ctx) const;
		graphics::ShapeHit DrawOutliner(const LightDrawContext& ctx) const;

		rage::Mat44V ComputeLightWorldMatrix(gtaDrawable* drawable, const rage::Mat44V& entityMtx, u16 lightIndex) const;

		void RenderLightUI(CLightAttr* light) const;
		void RenderLightTransformGizmo(CLightAttr* light, const rage::Mat44V& lightWorld) const;
		void SelectGizmoMode(gtaDrawable* drawable);

	public:
		void Render(gtaDrawable* drawable, const rage::Mat44V& entityMtx);
	};
}
