//
// File: lighteditor.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "am/graphics/shapetest.h"
#include "am/gizmo/gizmobase.h"
#include "game/drawable.h"

namespace rageam::integration
{
	struct LightDrawContext
	{
		rage::Vec3V   CamFront, CamRight, CamUp;
		graphics::Ray WorldMouseRay;
		rage::Mat44V  LightWorld;
		rage::Mat44V  LightBind;
		rage::Mat44V  LightLocal;
		rage::Mat44V  LightBoneWorld;
		CLightAttr*   Light;
	};

	class LightGizmo : public gizmo::Gizmo
	{
		CLightAttr m_StartLightState;

	protected:
		static constexpr float TARGET_BOX_EXTENT = 0.35f;
		static constexpr float TARGET_BOX_EXTENT_RADIUS = 0.55f;
		static constexpr u32 COLOR_DEFAULT = AM_COL32(220, 185, 5, 120);
		static constexpr u32 COLOR_HOVERED = AM_COL32(225, 220, 10, 200);
		static constexpr u32 COLOR_SELECTED = AM_COL32(225, 220, 10, 255);
	public:
		void OnStart(const gizmo::GizmoContext& context) override;
		void OnCancel(const gizmo::GizmoContext& context) override;
		bool DynamicScale() const override { return false; }

		LightDrawContext& GetLightContext(const gizmo::GizmoContext& context) const { return context.GetUserData<LightDrawContext>(); }

		enum
		{
			Light_None,

			PointLight_Sphere,
			PointLight_Edge,

			SpotLight_None,
			SpotLight_HemisphereCone,
			SpotLight_Target,

			CapsuleLight_Capsule,
			CapsuleLight_Extent1, // -Dir
			CapsuleLight_Extent2, // +Dir
		};
	};

	class LightGizmo_Point : public LightGizmo
	{
	public:
		bool CanSelect(gizmo::GizmoHandleID handle) const override { return handle == PointLight_Sphere; }
		void Draw(const gizmo::GizmoContext& context) override;
		void Manipulate(const gizmo::GizmoContext& context, Vec3V& outOffset, Vec3V& outScale, QuatV& outRotation) override;
		List<gizmo::HitResult> HitTest(const gizmo::GizmoContext& context) override;

		GIZMO_DECLARE_INFO;
	};

	class LightGizmo_Spot : public LightGizmo
	{
		static constexpr float TARGET_BOX_OFFSET = TARGET_BOX_EXTENT * 2.0f;

	public:
		bool CanSelect(gizmo::GizmoHandleID handle) const override
		{
			return handle == SpotLight_HemisphereCone || handle ==  SpotLight_Target;
		}
		void Draw(const gizmo::GizmoContext& context) override;
		void Manipulate(const gizmo::GizmoContext& context, Vec3V& outOffset, Vec3V& outScale, QuatV& outRotation) override;
		List<gizmo::HitResult> HitTest(const gizmo::GizmoContext& context) override;

		// Gets point at end of the light in direction where it points
		static Vec3V GetTargetPointLocal(const LightDrawContext& lightContext);
		static Vec3V GetTargetPointWorld(const LightDrawContext& lightContext);
		static void  SetTargetPointLocal(const LightDrawContext& lightContext, const Vec3V& targetPoint);

		GIZMO_DECLARE_INFO;
	};

	class LightGizmo_Capsule : public LightGizmo
	{

	public:
		bool CanSelect(gizmo::GizmoHandleID handle) const override
		{
			return
				handle == CapsuleLight_Capsule || 
				handle == CapsuleLight_Extent1 ||
				handle == CapsuleLight_Extent2;
		}
		void Draw(const gizmo::GizmoContext& context) override;
		void Manipulate(const gizmo::GizmoContext& context, Vec3V& outOffset, Vec3V& outScale, QuatV& outRotation) override;
		List<gizmo::HitResult> HitTest(const gizmo::GizmoContext& context) override;

		GIZMO_DECLARE_INFO;
	};

	class DrawList;
	struct ModelSceneContext;

	class LightEditor
	{
		static constexpr ConstString LIGHT_OUTLINE_GIZMO_FORMAT = "Light_Outline%i";
		static constexpr ConstString LIGHT_TRANSFORM_GIZMO_FORMAT = "Light_Transform%i";
		static constexpr ConstString LIGHT_TARGET_TRANSFORM_GIZMO_FORMAT = "Light_TargetTransform%i";

		ModelSceneContext*	m_SceneContext;
		int					m_SelectedLight = -1;
		int					m_HoveredLight = -1;
		bool				m_SelectionFreezed = false;

		// We freeze selecting during cull plane editing, so save state
		bool				m_SelectionWasFreezed = false;
		bool				m_EditingCullPlane = false;
		// We are maintaining additional matrix for culling plane because
		// in light cull plane is stored as normal + offset so we can
		// only "rotate" it around the light. Storing this transform during editing
		// gives user ability to freely rotate/move plane
		rage::Mat44V		m_CullPlane;

		void SetCullPlaneFromLight(const LightDrawContext& ctx);

		//bool DrawPointLightFalloffGizmo(const LightDrawContext& ctx);
		void DrawCullPlaneEditGizmo(const LightDrawContext& ctx);

		// Light bind transforms world light position into local
		void ComputeLightMatrices(
			u16 lightIndex,
			rage::Mat44V& lightWorld,
			rage::Mat44V& lightBind,
			rage::Mat44V& lightLocal,
			rage::Mat44V& lightBoneWorld) const;

		void DrawLightUI(const LightDrawContext& ctx);

	public:
		LightEditor(ModelSceneContext* sceneContext);

		void Render();
		// Pass -1 to select none
		// Nothing is done if selection was frozen by user
		void SelectLight(s32 index);

		// Reset state for new entity
		void Reset();

		enum OutlineModes
		{
			OutlineMode_All,
			OutlineMode_OnlySelected,
			OutlineMode_None,
		} OutlineMode = OutlineMode_All;
	};
}

#endif
