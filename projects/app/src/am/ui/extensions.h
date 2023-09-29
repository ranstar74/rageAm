//
// File: extensions.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <functional>

#include "imgui.h"
#include "imgui_internal.h"
#include "am/string/stringwrapper.h"
#include "common/types.h"
#include "helpers/resharper.h"

// Gets distance shift to center element of given size with total available space.
#define IM_GET_CENTER(start, available, size) ((start) + ((available) - (size)) / 2.0f)

enum ImGuiRenamingSelectableFlags_
{
	ImGuiRenamingSelectableFlags_None = 0,
	ImGuiRenamingSelectableFlags_Disabled = 1 << 0,
	ImGuiRenamingSelectableFlags_NoRename = 1 << 1,
	ImGuiRenamingSelectableFlags_Outline = 1 << 2,
};
typedef int ImGuiRenamingSelectableFlags;

enum ImGuiTextCenteredFlags_
{
	ImGuiTextCenteredFlags_None = 0,
	ImGuiTextCenteredFlags_Horizontal = 1 << 0,
	ImGuiTextCenteredFlags_Vertical = 1 << 1,
};
typedef int ImGuiTextCenteredFlags;

enum ImGuiIconTreeNodeFlags_
{
	ImGuiIconTreeNodeFlags_None = 0,
	ImGuiIconTreeNodeFlags_DefaultOpen = 1 << 0,
	ImGuiIconTreeNodeFlags_NoChildren = 1 << 1,
};
typedef int ImGuiIconTreeNodeFlags;

enum ImGuiDragSelectionFlags_
{
	ImGuiDragSelectionFlags_None = 0,
	ImGuiDragSelectionFlags_DisableBegin = 1 << 0,
};
typedef int ImGuiDragSelectionFlags;

namespace ImGui
{
	PRINTF_ATTR(1, 2) inline ConstString FormatTemp(ConstString fmt, ...)
	{
		static char buffer[256];

		va_list args;
		va_start(args, fmt);
		vsprintf_s(buffer, 256, fmt, args);
		va_end(args);

		return buffer;
	}

	// This was made because we need way too many in & out parameters
	// Can we really just merge it into state storage somehow?
	struct RenamingSelectableState
	{
		// In

		ImTextureID Icon;
		ConstString TextDisplay; // Text that will be shown usually
		ConstString TextEditable; // Text that will be shown in edit mode

		bool LastSelected;

		// In & Out

		char* Buffer; // New name (after editing) will be written here
		u32	BufferSize;

		bool Selected;
		bool Renaming;

		// Out

		bool WasRenaming;
		bool DoubleClicked;
		bool AcceptRenaming; // Set to true if editing changes were accepted by user.

		bool StoppedRenaming() const { return WasRenaming && !Renaming; }
	};

	// Button with editable name, works similar to Selectable.
	// Renaming can be started by pressing F2 or setting state::Renaming value to True.
	bool RenamingSelectable(RenamingSelectableState& state, ImGuiRenamingSelectableFlags flags = 0);

	void ColumnsBeginBackground();
	void ColumnsEndBackground();

	void TextCentered(ConstString text, ImGuiTextCenteredFlags flags);

	ImU32 AddHSV(ImU32 col, float h, float s, float v);

	// Draws status bar in the bottom of current window
	void StatusBar();
	// Has to be called right after window is shown to modify clip rect for status bar
	void PreStatusBar();

	// Windows-like tree node
	bool IconTreeNode(ConstString text, bool& selected, bool& toggled, ImTextureID icon, ImGuiIconTreeNodeFlags flags = 0);

	// You have to open whole hierarchy to open sub-node
	void IconTreeNodeSetOpened(ConstString text, bool opened);

	// Performs mouse-drag selection (selection rectangle like on windows desktop)
	// Returns true if selection is performed (left mouse button is held)
	bool DragSelectionBehaviour(ImGuiID id, bool& stopped, ImRect& selection, ImGuiDragSelectionFlags flags = 0);

	// NOTE: These functions reserve 4 next ID's (0, 1, 2, 3) for storing rect parts, use ID + 4 to get next ID
	ImRect GetStorageRect(const ImGuiStorage& storage, ImGuiID id);
	void SetStorageRect(ImGuiStorage& storage, ImGuiID id, const ImRect& rect);

	// Just like regular text but instead of clipping it will end with '...'
	void TextEllipsis(ConstString text);

	void ToolTip(ConstString text);

	void HelpMarker(ConstString tooltip, ConstString iconText = nullptr);

	void BeginToolBar(ConstString name);
	void EndToolBar();

	bool NavButton(ConstString idStr, ImGuiDir dir, bool enabled);

	void ShadeVertsLinearColorGradient(const ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1);

	void RenderFrameGradient(ImRect& bb, ImU32 startColor, ImU32 endColor, ImGuiAxis axis = ImGuiAxis_Y, float bias = 1.0f, float offset = 0.0f);
	void RenderRectGradient(ImRect& bb, ImU32 startColor, ImU32 endColor, ImGuiAxis axis = ImGuiAxis_Y, float bias = 1.0f);

	// Useful when doing vertical layouts, positions next widget right after previous one.
	void SnapToPrevious();

	void BeginDockSpace();

	// Allows to use with buttons (original function breaks such functionality)
	bool BeginDragDropSource2(ImGuiDragDropFlags flags);
}

namespace ImPlot
{
	struct ScrollingBuffer
	{
		int MaxSize;
		int Offset;
		ImVector<ImVec2> Data;
		ScrollingBuffer(int max_size = 2000) {

			MaxSize = max_size;
			Offset = 0;
			Data.reserve(MaxSize);
		}
		void AddPoint(double x, double y)
		{
			if (Data.size() < MaxSize)
				Data.push_back(ImVec2(x, y));
			else 
			{
				Data[Offset] = ImVec2(x, y);
				Offset = (Offset + 1) % MaxSize;
			}
		}
		void Erase()
		{
			if (Data.size() > 0) 
			{
				Data.shrink(0);
				Offset = 0;
			}
		}
	};
}
