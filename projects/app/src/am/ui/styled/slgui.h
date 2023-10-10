//
// File: slgui.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once
#include "imgui.h"
#include "imgui_internal.h"

struct SlGradient
{
	ImVec4 Start;
	ImVec4 End;
};
static SlGradient ImLerp(const SlGradient& a, const SlGradient& b, float t) { return { ImLerp(a.Start, b.Start, t), ImLerp(a.End, b.End, t) }; }

enum SlGuiCol_
{
	SlGuiCol_None,

	SlGuiCol_Bg,
	SlGuiCol_Bg2,

	SlGuiCol_DockspaceBg,

	SlGuiCol_ToolbarBg,

	SlGuiCol_Gloss,
	SlGuiCol_GlossBg,
	SlGuiCol_GlossBg2,

	SlGuiCol_Border,
	SlGuiCol_BorderHighlight,
	SlGuiCol_Shadow,

	SlGuiCol_Button,
	SlGuiCol_ButtonHovered,
	SlGuiCol_ButtonPressed,

	SlGuiCol_Node,
	SlGuiCol_NodeHovered,
	SlGuiCol_NodePressed,
	SlGuiCol_NodeBorderHighlight,

	SlGuiCol_GraphNode,
	SlGuiCol_GraphNodeHovered,
	SlGuiCol_GraphNodePressed,
	SlGuiCol_GraphNodeBorderHighlight,

	SlGuiCol_List,
	SlGuiCol_ListHovered,
	SlGuiCol_ListPressed,
	SlGuiCol_ListBorderHighlight,

	SlGuiCol_TableHeader,
	SlGuiCol_TableHeaderHovered,
	SlGuiCol_TableHeaderPressed,
	SlGuiCol_TableHeaderBorderHighlight,

	SlGuiCol_COUNT
};
typedef int SlGuiCol;

struct SlGuiStyle
{
	float		ButtonRounding;
	SlGradient	Colors[SlGuiCol_COUNT];

	SlGuiStyle();
};

struct SlGuiContext : ImGuiContext // TODO: Inheritance is not necessary
{
	SlGuiStyle SlStyle;

	SlGuiContext() : ImGuiContext(nullptr) {}
};

enum SlFont // Keep in sync with GuiCore::CreateFonts
{
	SlFont_Regular,
	SlFont_Medium,
	SlFont_Small,
	SlFont_Test,
};

// Styled widgets for ImGui
namespace SlGui
{
	SlGuiContext* CreateContext();
	SlGuiContext* GetContext();
	SlGuiStyle& GetStyle();

	void StyleColorsLight();
	void StyleColorsDark();
	void AddCustomIcons();
	void LoadFonts();

	inline SlGradient ColorConvertU32ToGradient(ImU32 start, ImU32 end) { return { ImGui::ColorConvertU32ToFloat4(start),ImGui::ColorConvertU32ToFloat4(end) }; }

	ImVec4 StorageGetVec4(ImGuiID id);
	void StorageSetVec4(ImGuiID id, const ImVec4& vec);
	SlGradient StorageGetGradient(ImGuiID id);
	void StorageSetGradient(ImGuiID id, const SlGradient& col);

	SlGradient GetColorGradient(SlGuiCol col);
	SlGradient GetColorAnimated(ConstString strId, SlGuiCol col, float time = 1.0f);

	void PushFont(SlFont font);
	void PopFont();

	void ShadeVerts(const ImRect& bb, const SlGradient& col, int vtx0, int vtx1, float bias = 0, float shift = 0, ImGuiAxis axis = ImGuiAxis_Y);

	void RenderFrame(const ImRect& bb, const SlGradient& col, float bias = 1, float shift = 0, ImGuiAxis axis = ImGuiAxis_Y);
	void RenderBorder(const ImRect& bb, const SlGradient& col, float bias = 1, float shift = 0, ImGuiAxis axis = ImGuiAxis_Y);

	void ShadeItem(SlGuiCol col);

	// Creates new child with specified padding
	bool BeginPadded(ConstString name, const ImVec2& padding);
	void EndPadded();
}
