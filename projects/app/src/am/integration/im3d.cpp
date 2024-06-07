#include "im3d.h"

#include "imgui_internal.h"
#include "am/graphics/shapetest.h"
#include "am/ui/imglue.h"
#include "game/viewport.h"
#include "rage/grcore/device.h"
#include "am/integration/script/core.h"

constexpr ImU32 bgColor = IM_COL32(0, 0, 0, 170);

static inline bool s_CenterNext = false;

auto GetDrawList()
{
	return ImGui::GetBackgroundDrawList();
}

bool Im3D::WorldToScreen(const ImVec3V& worldPos, ImVec2& screenPos)
{
	u32 width, height;
	rage::grcDevice::GetScreenSize(width, height);

	rage::Vec4V vec = worldPos.Transform4(CViewport::GetViewProjectionMatrix());

	// Check if position is behind of the screen
	bool isCulled = vec.Z() < 0.0f;

	// TODO: Replace on remap, add helper in CViewport?
	vec *= rage::S_HALF / vec.W();
	vec += rage::Vec4V(0.5f, 0.5f, 1.0f, 1.0f);
	vec.SetY(1.0f - vec.Y());
	vec *= rage::Vec4V(static_cast<float>(width), static_cast<float>(height), 1.0f, 1.0f);
	
	screenPos = ImVec2(vec.X(), vec.Y());

	// With viewports, we have to shift coordinates to position of main viewport (game window)
	// https://github.com/ocornut/imgui/wiki/Multi-Viewports
	// A: When enabling ImGuiConfigFlags_ViewportsEnable,
	// the coordinate system used by Dear ImGui changes to match the coordinate system of your OS/desktop
	// (e.g. (0,0) generally becomes the top-left corner of your primary monitor).
	// This shouldn't affect most code, but if you have code using absolute coordinates you may want to change them
	// to be relative to a window, relative to a monitor or relative to your main viewport (GetMainViewport()->Pos).
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		screenPos += ImGui::GetMainViewport()->Pos;
	}

	return !isCulled;
}

bool Im3D::ShouldDrawText()
{
	return true; // TODO: Check if pause menu is open
}

void Im3D::CenterNext()
{
	s_CenterNext = true;
}

void Im3D::Text(const ImVec3V& pos, ConstString fmt, ...)
{
	bool centerNext = s_CenterNext;
	s_CenterNext = false;

	va_list args;
	va_start(args, fmt);
	const char* text, * text_end;
	ImFormatStringToTempBufferV(&text, &text_end, fmt, args);
	va_end(args);

	ImVec2 sPos;
	if (!WorldToScreen(pos, sPos))
		return;

	ImGui::PushFont(ImFont_Medium);

	if (centerNext)
	{
		ImVec2 textSize = ImGui::CalcTextSize(text, text_end);
		sPos -= { textSize.x * 0.5f, textSize.y * 0.5f };
	}

	GetDrawList()->AddText(sPos, ImGui::GetColorU32(ImGuiCol_Text), text, text_end);

	ImGui::PopFont();
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
	bool centerNext = s_CenterNext;
	s_CenterNext = false;

	const char* text, * text_end;
	ImFormatStringToTempBufferV(&text, &text_end, fmt, args);

	ImVec2 sPos;
	if (!WorldToScreen(pos, sPos))
		return;

	ImGui::PushFont(ImFont_Medium);

	ImVec2 textSize = ImGui::CalcTextSize(text, text_end);
	if (centerNext)
	{
		sPos -= { textSize.x * 0.5f, textSize.y * 0.5f };
	}

	// Black rectangle background for text
	ImVec2 bgFrom = sPos;
	ImVec2 bgTo = sPos + textSize;
	GetDrawList()->AddRectFilled(bgFrom, bgTo, bgColor);
	GetDrawList()->AddText(sPos, ImGui::GetColorU32(ImGuiCol_Text), text, text_end);

	ImGui::PopFont();
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

	ImVec2 sPos;
	bool isCulled = !WorldToScreen(startPos, sPos);

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

bool Im3D::IsViewportHovered()
{
	return GImGui->HoveredWindow == nullptr; // TODO: Add gizmo here
}

bool Im3D::IsViewportFocused()
{
	return GImGui->ActiveIdWindow == nullptr; // TODO: Add gizmo here
}
