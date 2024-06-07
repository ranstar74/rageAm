//
// File: gizmorotation.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "gizmobase.h"

namespace rageam::gizmo
{
	// TODO: Manipulation in screen space is not implemented!

	class GizmoRotation : public Gizmo
	{
		static constexpr float SENSETIVITY = 1.0f / 150.0f;

		static constexpr GizmoPlaneSelector GizmoHandlePlaneSelector[] =
		{
			GizmoPlaneSelector_Invalid,
			GizmoPlaneSelector_AxisX,	// AxisX
			GizmoPlaneSelector_AxisY,	// AxisY
			GizmoPlaneSelector_AxisZ,	// AxisZ
			GizmoPlaneSelector_View,	// View
		};

	public:
		bool DynamicScale() const override { return true; }
		bool WrapCursor() const override { return true; }
		bool TopMost() const override { return true; }
		void Draw(const GizmoContext& context) override;
		void Manipulate(const GizmoContext& context, Vec3V& outOffset, Vec3V& outScale, QuatV& outRotation) override;
		List<HitResult> HitTest(const GizmoContext& context) override;

		GIZMO_DECLARE_INFO;

		enum
		{
			GizmoRotation_Invalid,
			GizmoRotation_AxisX,
			GizmoRotation_AxisY,
			GizmoRotation_AxisZ,
			GizmoRotation_View,
		};
	};
}
