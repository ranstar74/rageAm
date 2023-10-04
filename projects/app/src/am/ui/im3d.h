//
// File: im3d.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "imgui.h"
#include "ImGuizmo.h"
#include "rage/math/vecv.h"
#include "rage/math/vec.h"

using ImVec3V = rage::Vec3V;
using ImVec3 = rage::Vector3;
using ImMat44V = rage::Mat44V;

namespace Im3D
{
	// Returns whether point is not culled (visible on screen and not behind camera)
	bool WorldToScreen(const ImVec3V& worldPos, ImVec2& screenPos);

	// Next drawn text will be centered at given world pos
	void CenterNext();
	void Text(const ImVec3V& pos, ConstString fmt, ...);
	void TextBg(const ImVec3V& pos, ConstString fmt, ...);
	void TextBgV(const ImVec3V& pos, ConstString fmt, va_list args);
	void TextBgColored(const ImVec3V& pos, u32 col, ConstString fmt, ...);

	bool GizmoTrans(ImMat44V& mtx);
	bool Gizmo(const ImMat44V& mtx, ImMat44V& delta, ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE);
	bool GizmoTrans(ImVec3V& translation);
	bool GizmoTrans(ImVec3& translation);

	bool GizmoBehaviour(
		bool& isDragging, 
		const ImVec3V& planePos, const ImVec3V& planeNormal, 
		ImVec3V& moveDelta, ImVec3V& startPos, ImVec3V& dragPos);

	// Creates invisible window at given world coordinate, use ImGui functions to draw the rest
	bool Begin(const ImVec3V& startPos, bool background = false);
	void End();
}
