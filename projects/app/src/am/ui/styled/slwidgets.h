//
// File: slwidgets.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <functional>

#include "slgui.h"
#include "common/types.h"

enum SlGuiTreeNodeFlags_
{
	SlGuiTreeNodeFlags_None = 0,
	SlGuiTreeNodeFlags_DefaultOpen = 1 << 0,
	SlGuiTreeNodeFlags_NoChildren = 1 << 1,
	SlGuiTreeNodeFlags_RightClickSelect = 1 << 2,
	SlGuiTreeNodeFlags_DisplayAsHovered = 1 << 3,
};
typedef int SlGuiTreeNodeFlags;

enum SlGuiRenamingSelectableFlags_
{
	SlGuiRenamingSelectableFlags_None = 0,
	SlGuiRenamingSelectableFlags_Disabled = 1 << 0,
	SlGuiRenamingSelectableFlags_NoRename = 1 << 1,
	SlGuiRenamingSelectableFlags_Outline = 1 << 2,
	SlGuiRenamingSelectableFlags_RightClickSelect = 1 << 3,
};
typedef int SlGuiRenamingSelectableFlags;

enum SlGuiNodeDragFlags_
{
	SlGuiNodeDragFlags_None = 0,
	SlGuiNodeDragFlags_AllowAboveAndBelow = 1 << 0,
};
typedef int SlGuiNodeDragFlags;

enum SlGuiNodeDragPosition_
{
	SlGuiNodeDragPosition_Above,
	SlGuiNodeDragPosition_Center,
	SlGuiNodeDragPosition_Below
}; // Same as in windows explorer
typedef int SlGuiNodeDragPosition;

// This was made because we need way too many in & out parameters
// Can we really just merge it into state storage somehow?
struct SlRenamingSelectableState
{
	// In

	float IconScale = 1.0f;
	float IconWidth = -1, IconHeight = -1;
	ImTextureID Icon;
	ConstString TextDisplay; // Text that will be shown usually
	ConstString TextEditable; // Text that will be shown in edit mode

	// In & Out

	char* Buffer = nullptr; // New name (after editing) will be written here
	int	BufferSize = 0;

	bool Selected = false;
	bool Renaming = false;

	// Out

	bool WasRenaming = false;
	bool DoubleClicked = false;
	bool AcceptRenaming = true; // Set to true if editing changes were accepted by user.

	bool StoppedRenaming() const { return WasRenaming && !Renaming; }
};

using SlHintCompareFn = std::function<bool(const char* search, const char* hint)>;
using SlHintGetFn = std::function<void(int index, const char** hint, ImTextureID* icon, ImVec2* iconSize, float* iconScale)>;

struct SlPickerState
{
	int			HintIndex;
	bool		HintAccepted;
	bool		NeedHints;
	bool		SearchChanged;
	ConstString Search;
};

namespace SlGui
{
	void RenderGloss(const ImRect& bb, SlGuiCol col = SlGuiCol_Gloss);
	void RenderShadow(const ImRect& bb);
	void RenderArrow(ImDrawList* draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, float scale);

	void BeginHorizontal();
	void EndHorizontal();
	bool BeginToolWindow(ConstString text);
	void EndToolWindow();
	bool ToggleButton(ConstString text, bool& isActive);
	bool MenuButton(ConstString text);
	bool IconButton(ConstString text, ImU32 color = IM_COL32_WHITE);
	bool Button(ConstString text, const ImVec2& sizeOverride = ImVec2(0, 0));
	bool Begin(ConstString name, bool* open = 0, ImGuiWindowFlags flags = 0);
	void End();
	bool TreeNode(ConstString text, bool& selected, bool& toggled, ImTextureID icon, ImGuiTreeNodeFlags flags = 0);

	bool BeginTable(ConstString text, int columnCount, ImGuiTableFlags flags = 0);
	void EndTable();

	// Wrapper for renaming selectable without renaming option
	bool Selectable(ConstString text, bool selected, ImTextureID icon = nullptr, ImVec2 iconSize = ImVec2(-1, -1), float iconScale = 1.0f);

	// Button with editable name, works similar to Selectable.
	// Renaming can be started by pressing F2 or setting state::Renaming value to True.
	bool RenamingSelectable(SlRenamingSelectableState& state, SlGuiRenamingSelectableFlags flags = 0);

	SlPickerState InputPicker(const char* name, const char* text, int hintCount, const SlHintGetFn& hintFn, const SlHintCompareFn& compareFn = 0);

	// Will return once when button was pressed down and repeatedly after button was held for time that is greater than specified delay
	bool IsKeyDownDelayed(ImGuiKey key, float delay = 0.4f);

	void TableHeadersRow();
	void TableHeader(const char* label);

	void CategoryText(const char* text);

	void ColorEditGradient(ConstString name, SlGradient& gradient, ImGuiColorEditFlags flags = 0);

	void NodeDragBehaviour(SlGuiNodeDragPosition& outPosition, SlGuiNodeDragFlags flags = 0);

	// Those are slightly edited standard ones

	bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags = 0);
	bool TreeNodeBehavior(ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end);
	bool BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags);
	bool Combo(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int popup_max_height_in_items);
	bool Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);
	bool Checkbox(const char* label, bool* v);

	bool GraphTreeNode(ConstString text, bool& selected, bool& toggled, SlGuiTreeNodeFlags flags);
}
