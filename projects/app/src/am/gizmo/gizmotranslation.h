//
// File: gizmotranslation.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "gizmobase.h"

namespace rageam::gizmo
{
	class GizmoTranslation : public Gizmo
	{
		static constexpr GizmoPlaneSelector GizmoHandlePlaneSelector[] =
		{
			GizmoPlaneSelector_Invalid,
			GizmoPlaneSelector_BestMatchX,	// AxisX
			GizmoPlaneSelector_BestMatchY,	// AxisY
			GizmoPlaneSelector_BestMatchZ,	// AxisZ
			GizmoPlaneSelector_AxisZ,		// PlaneXY
			GizmoPlaneSelector_AxisY,		// PlaneXZ
			GizmoPlaneSelector_AxisX,		// PlaneYZ
			GizmoPlaneSelector_View,		// View
		};

	public:
		bool DynamicScale() const override { return true; }
		bool TopMost() const override { return true; }
		void Draw(const GizmoContext& context) override;
		void Manipulate(const GizmoContext& context, Vec3V& outOffset, Vec3V& outScale, QuatV& outRotation) override;
		List<HitResult> HitTest(const GizmoContext& context) override;

		GIZMO_DECLARE_INFO;

		enum
		{
			GzimoTranslation_None,
			GizmoTranslation_AxisX,
			GizmoTranslation_AxisY,
			GizmoTranslation_AxisZ,
			GizmoTranslation_PlaneXY,
			GizmoTranslation_PlaneXZ,
			GizmoTranslation_PlaneYZ,
			GizmoTranslation_View,
		};
	};
}
