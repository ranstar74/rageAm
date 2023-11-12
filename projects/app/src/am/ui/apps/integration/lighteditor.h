//
// File: lighteditor.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "am/graphics/ray.h"
#include "am/graphics/shapetest.h"
#include "game/drawable.h"

namespace rageam::integration
{
	class DrawList;
	// TODO: Add color filling for light outlines so it's easier to see where light ends
	// TODO: Bitmap hit detection (how to allow user to select small light that's inside of larger one? larger one is always in front)
	// TODO: We should add IsSphereVisible in viewport to quickly cull out outlines
	// TODO: Color filling for cull plane (different color on both sides)

	struct ModelSceneContext;

	enum eGimzoMode
	{
		GIZMO_None,
		GIZMO_Translate,
		GIZMO_Rotate,
	};

	class LightEditor
	{
		ModelSceneContext*	m_SceneContext;
		int					m_SelectedLight = -1;
		int					m_HoveredLight = -1;
		int					m_GizmoMode = GIZMO_None;
		bool				m_SelectionFreezed = false;

		// We freeze selecting during cull plane editing, so save state
		bool				m_SelectionWasFreezed = false;
		bool				m_EditingCullPlane = false;
		// We are maintaining additional matrix for culling plane because
		// in light cull plane is stored as normal + offset so we can
		// only "rotate" it around the light. Storing this transform during editing
		// gives user ability to freely rotate/move plane
		rage::Mat44V		m_CullPlane;

		// -- Custom gizmos --

		bool				m_UsingPointFalloffGizmo = false;

		struct LightDrawContext
		{
			rage::Vec3V		CamFront, CamRight, CamUp;
			graphics::Ray	WorldMouseRay;
			rage::Mat44V	LightWorld;
			rage::Mat44V	LightBind;
			rage::Mat44V	LightLocal;
			rage::Mat44V	LightBoneWorld;
			CLightAttr*		Light;
			u16				LightIndex;
			bool			IsSelected;
			u32				PrimaryColor;
			u32				SecondaryColor;
			u32				CulledColor;
		};

		void SetCullPlaneFromLight(const LightDrawContext& ctx);

		u32 GetOutlinerColor(bool isSelected, bool isHovered, bool isPrimary) const;

		// Light outline draw list
		DrawList& GetDrawList() const;

		// Tests if screen mouse cursor intersects with light bounding sphere
		graphics::ShapeHit ProbeLightSphere(const LightDrawContext& ctx) const;

		graphics::ShapeHit DrawOutliner_Point(const LightDrawContext& ctx) const;
		graphics::ShapeHit DrawOutliner_Spot(const LightDrawContext& ctx) const;
		graphics::ShapeHit DrawOutliner_Capsule(const LightDrawContext& ctx) const;
		graphics::ShapeHit DrawOutliner(const LightDrawContext& ctx) const;

		bool DrawPointLightFalloffGizmo(const LightDrawContext& ctx);
		void DrawCullPlaneEditGizmo(const LightDrawContext& ctx);

		// Light bind transforms world light position into local
		void ComputeLightMatrices(
			u16 lightIndex,
			rage::Mat44V& lightWorld,
			rage::Mat44V& lightBind,
			rage::Mat44V& lightLocal,
			rage::Mat44V& lightBoneWorld) const;

		int GetImGuizmoOperation() const;
		void DrawLightUI(const LightDrawContext& ctx);
		void DrawLightTransformGizmo(const LightDrawContext& ctx) const;
		void DrawCustomGizmos(const LightDrawContext& ctx);
		void SelectGizmoMode();

	public:
		LightEditor(ModelSceneContext* sceneContext);

		void Render();
		// Pass -1 to select none
		// Nothing is done if selection was frozen by user
		void SelectLight(s32 index);

		// Reset state for new entity
		void Reset()
		{
			SelectLight(-1);
		}

		bool ShowLightOutlines = true;
		bool ShowOnlySelectedLightOutline = true;
	};
}

#endif
