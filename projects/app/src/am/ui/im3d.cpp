#include "im3d.h"

#include "ImGuizmo.h"
#include "imgui_internal.h"
#include "game/viewport.h"
#include "rage/grcore/device.h"

#define GUIZMO_MTX(mtx) ((float*)&mtx)

constexpr ImU32 bgColor = IM_COL32(0, 0, 0, 140);

static auto GetDrawList()
{
	return ImGui::GetForegroundDrawList();
}

ImVec2 Im3D::WorldToScreen(const ImVec3V& pos, bool& isCulled)
{
	u32 width, height;
	rage::grcDevice::GetScreenSize(width, height);

	rage::Vec4V vec = pos.Transform4(CViewport::GetViewProjectionMatrix());

	// Check if position is behind of the screen
	isCulled = vec.Z() < 0.0f;

	vec *= rage::S_HALF / vec.W();
	vec += rage::Vec4V(0.5f, 0.5f, 1.0f, 1.0f);
	vec.SetY(1.0f - vec.Y());
	vec *= rage::Vec4V(static_cast<float>(width), static_cast<float>(height), 1.0f, 1.0f);

	return ImVec2(vec.X(), vec.Y());
}

void Im3D::Text(const ImVec3V& pos, ConstString fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	const char* text, * text_end;
	ImFormatStringToTempBufferV(&text, &text_end, fmt, args);
	va_end(args);

	bool isCulled;
	ImVec2 sPos = WorldToScreen(pos, isCulled);
	if (isCulled)
		return;
	GetDrawList()->AddText(sPos, ImGui::GetColorU32(ImGuiCol_Text), text, text_end);
}

void Im3D::TextBg(const ImVec3V& pos, ConstString fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	TextBgV(pos, fmt, args);
	va_end(args);
}

void Im3D::TextBgV(const ImVec3V& pos, ConstString fmt, va_list args)
{
	const char* text, * text_end;
	ImFormatStringToTempBufferV(&text, &text_end, fmt, args);

	bool isCulled;
	ImVec2 sPos = WorldToScreen(pos, isCulled);
	if (isCulled)
		return;

	// Black rectangle background for text
	ImVec2 textSize = ImGui::CalcTextSize(text, text_end);
	ImVec2 bgFrom = sPos;
	ImVec2 bgTo = sPos + textSize;
	GetDrawList()->AddRectFilled(bgFrom, bgTo, bgColor);
	GetDrawList()->AddText(sPos, ImGui::GetColorU32(ImGuiCol_Text), text, text_end);
}

void Im3D::TextBgColored(const ImVec3V& pos, u32 col, ConstString fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	ImGui::PushStyleColor(ImGuiCol_Text, col);
	TextBgV(pos, fmt, args);
	ImGui::PopStyleColor();
	va_end(args);
}

bool Im3D::GizmoTrans(ImMat44V& mtx)
{
	return Manipulate(
		GUIZMO_MTX(CViewport::GetViewMatrix()),
		GUIZMO_MTX(CViewport::GetProjectionMatrix()),
		ImGuizmo::TRANSLATE,
		ImGuizmo::WORLD,
		GUIZMO_MTX(mtx));
}

bool Im3D::GizmoTrans(ImVec3V& translation)
{
	rage::Mat44V mtx(DirectX::XMMatrixTranslationFromVector(translation));
	bool moved = GizmoTrans(mtx);
	mtx.Decompose(&translation, nullptr, nullptr);
	return moved;
}

bool Im3D::GizmoTrans(ImVec3& translation)
{
	ImVec3V translationV = translation;
	bool moved = GizmoTrans(translationV);
	translation = translationV;
	return moved;
}

bool Im3D::Begin(const ImVec3V& startPos, bool background)
{
	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_AlwaysAutoResize;

	bool isCulled;
	ImVec2 sPos = WorldToScreen(startPos, isCulled);

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowSize(io.DisplaySize);
	ImGui::SetNextWindowPos(sPos);

	ImGui::PushStyleColor(ImGuiCol_WindowBg, background ? bgColor : 0);
	ImGui::PushStyleColor(ImGuiCol_Border, 0);
	ImGui::PushStyleColor(ImGuiCol_WindowShadow, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	ImGui::Begin("Im3D", 0, flags);
	return !isCulled;
}

void Im3D::End()
{
	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(3); // WindowBg, Border, WindowShadow
}
