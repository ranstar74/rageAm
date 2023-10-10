#include "context.h"

#include "implot.h"
#include "am/graphics/render/engine.h"
#include "am/ui/apps/statusbar.h"
#include "am/ui/styled/slgui.h"
#include "ImGuizmo.h"

void rageam::ui::UIContext::SetupImGui() const
{
	IMGUI_CHECKVERSION();
	SlGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	SlGui::LoadFonts();

	ImGuiStyle& style = ImGui::GetStyle();
	style.CellPadding = ImVec2(8, 0);
	style.Colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 16));
	style.Colors[ImGuiCol_BorderShadow] = ImGui::ColorConvertU32ToFloat4(IM_COL32(31, 31, 31, 82));

	StyleBlack();
	// StyleLight();
}

rageam::ui::UIContext::UIContext()
{
	SetupImGui();

	ImPlot::CreateContext();
	ImPlot::GetStyle().Use24HourClock = true;
	ImPlot::GetStyle().UseLocalTime = true;

	ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
	ImGui::GetIO().ConfigDockingWithShift = true;

	Renderer.InitImGuiBackEnds();
	Apps.RegisterSystemApps();

	Windows = Apps.FindAppByType<WindowManager>();
	Status = Apps.FindAppByType<StatusBar>();
}

rageam::ui::UIContext::~UIContext()
{
	Renderer.DestroyContext();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
}

bool rageam::ui::UIContext::Update()
{
	Input.BeginFrame();
	Renderer.BeginFrame();
	{
		ImGuizmo::SetImGuiContext(GImGui);
		ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
		ImGuizmo::SetRect(0, 0, GImGui->IO.DisplaySize.x, GImGui->IO.DisplaySize.y);
		ImGuizmo::BeginFrame();
	}
	bool needContinue = Apps.UpdateAll();
	Renderer.EndFrame();
	Input.EndFrame();

	if (!needContinue)
		return false;

	return true;
}

void rageam::ui::UIContext::StyleBlack() const
{
	ImGui::StyleColorsDark();
	SlGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(IM_COL32(252, 252, 252, 245));

	style.Colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(36, 38, 43, 255));
	style.Colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(62, 64, 71, 255));
	style.Colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(62, 64, 71, 255));

	style.Colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(IM_COL32(61, 62, 63, 255));
#ifdef AM_STANDLONE
	style.Colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(41, 46, 54, 255));
#else
	style.Colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(41, 46, 54, 245));
#endif

	style.Colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(31, 37, 48, 255));
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImGui::ColorConvertU32ToFloat4(IM_COL32(31, 37, 48, 255));
	style.Colors[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(30, 32, 42, 255));

	style.Colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 0));
	style.Colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(62, 78, 105, 255));
	style.Colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(62, 78, 105, 255));

	style.Colors[ImGuiCol_SliderGrab] = ImGui::ColorConvertU32ToFloat4(IM_COL32(25, 159, 255, 255));
	style.Colors[ImGuiCol_SliderGrabActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(25, 159, 255, 255));

	style.Colors[ImGuiCol_MenuBarBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(34, 37, 43, 255));

	style.Colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 0));
	style.Colors[ImGuiCol_TabUnfocused] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 0));
	style.Colors[ImGuiCol_TabHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(14, 200, 97, 255));
	style.Colors[ImGuiCol_TabActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(12, 171, 74, 255));
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(10, 140, 68, 255));

	style.Colors[ImGuiCol_ResizeGrip] = ImGui::ColorConvertU32ToFloat4(IM_COL32(62, 78, 105, 255));
	style.Colors[ImGuiCol_ResizeGripHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(14, 200, 97, 255));
	style.Colors[ImGuiCol_ResizeGripActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(12, 171, 74, 255));

	style.Colors[ImGuiCol_DockingPreview] = ImGui::ColorConvertU32ToFloat4(IM_COL32(40, 108, 214, 255));

	style.Colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(46, 49, 55, 255));
}

void rageam::ui::UIContext::StyleLight() const
{
	ImGui::StyleColorsLight();
	SlGui::StyleColorsLight();

	ImGuiStyle& style = ImGui::GetStyle();

	style.Colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(237, 237, 237, 255));
	style.Colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(IM_COL32(6, 6, 6, 245));
}

rageam::ui::UIContext* Gui = nullptr;

void CreateUIContext()
{
	Gui = new rageam::ui::UIContext();
}

void DestroyUIContext()
{
	delete Gui;
	Gui = nullptr;
}
