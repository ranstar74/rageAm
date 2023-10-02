#pragma once

#include "imgui.h"
#include "rage/math/vecv.h"
#include "rage/math/vec.h"

using ImVec3V = rage::Vec3V;
using ImVec3 = rage::Vector3;
using ImMat44V = rage::Mat44V;

namespace Im3D
{
	ImVec2 WorldToScreen(const ImVec3V& pos, bool& isCulled);

	void Text(const ImVec3V& pos, ConstString fmt, ...);
	void TextBg(const ImVec3V& pos, ConstString fmt, ...);
	void TextBgV(const ImVec3V& pos, ConstString fmt, va_list args);
	void TextBgColored(const ImVec3V& pos, u32 col, ConstString fmt, ...);

	bool GizmoTrans(ImMat44V& mtx);
	bool GizmoTrans(ImVec3V& translation);
	bool GizmoTrans(ImVec3& translation);

	// Creates invisible window at given world coordinate, use ImGui functions to draw the rest
	bool Begin(const ImVec3V& startPos, bool background = false);
	void End();
}
