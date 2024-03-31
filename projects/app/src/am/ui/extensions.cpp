#include "extensions.h"

#include <imgui_gradient.h>

#include "imglue.h"
#include "imgui_internal.h"
#include "implot.h"
#include "implot_internal.h"
#include "am/graphics/color.h"
#include "font_icons/icons_awesome.h"
#include "helpers/ranges.h"
#include "rage/math/vecv.h"

void ImGui::HandleUndoHotkeys()
{
	auto undo = rageam::UndoStack::GetCurrent();
	if (IsWindowFocused(ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_NoPopupHierarchy))
	{
		if (undo->CanUndo() && Shortcut(ImGuiKey_Z | ImGuiMod_Ctrl, 0, ImGuiInputFlags_RouteGlobal))
		{
			undo->Undo();
		}

		if (undo->CanRedo() && Shortcut(ImGuiKey_Y | ImGuiMod_Ctrl, 0, ImGuiInputFlags_RouteGlobal))
		{
			undo->Redo();
		}
	}
}

ImRect ImGui::TableGetRowRect()
{
	ImGuiTable* table = GImGui->CurrentTable;
	IM_ASSERT(table);
	return ImRect(table->WorkRect.Min.x, table->RowPosY1, table->WorkRect.Max.x, table->RowPosY2);
}

bool ImGui::TableIsHoveringRow()
{
	ImRect rowRect = TableGetRowRect();
	return
		IsMouseHoveringRect(rowRect.Min, rowRect.Max, false) &&
		IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
}

bool ImGui::RenamingSelectable(RenamingSelectableState& state, ImGuiRenamingSelectableFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiStorage* storage = window->DC.StateStorage;
	ImGuiContext* context = GImGui;
	ImGuiStyle* style = &GetStyle();

	bool isFocused = IsWindowFocused();

	bool allowRename = isFocused && !(flags & ImGuiRenamingSelectableFlags_NoRename);

	state.WasRenaming = state.Renaming;

	ImGuiID id = window->GetID(FormatTemp("%s%s", state.TextDisplay, state.TextEditable));

	// Compute size & bounding box of control
	// Button will stretch to window size so we can click on it anywhere
	// but name input won't, so we can use it in table too

	ImVec2 buttonSize;
	buttonSize.x = window->ParentWorkRect.GetWidth();
	buttonSize.y = GetFrameHeight();

	ImVec2 inputSize;
	inputSize.x = window->WorkRect.GetWidth();
	inputSize.y = GetFrameHeight();

	ImVec2 pos = GetCursorScreenPos();

	float minX = pos.x;
	float minY = pos.y;
	float maxX = pos.x + buttonSize.x;
	float maxY = pos.y + buttonSize.y;

	ImVec2 min = ImVec2(minX, minY);
	ImVec2 max = ImVec2(maxX, maxY);
	ImRect bb(min, max);

	BeginGroup();

	ItemSize(buttonSize);
	if (!ItemAdd(bb, id))
	{
		EndGroup();
		return false;
	}

	// If we've just began editing we have to copy original name to editable buffer
	bool wasRenamming = storage->GetBool(id, false);
	bool beganRenaming = state.Renaming && !wasRenamming;
	if (allowRename && beganRenaming)
	{
		String::Copy(state.Buffer, state.BufferSize, state.TextEditable);
		storage->SetBool(id, true);
	}

	constexpr float iconSize = 16;
	ImVec2 iconMin(min.x + style->FramePadding.x, IM_GET_CENTER(min.y, buttonSize.y, iconSize));
	ImVec2 iconMax(iconMin.x + iconSize, iconMin.y + iconSize);
	window->DrawList->AddImage(state.Icon, iconMin, iconMax);

	bool renamingActive = allowRename && state.Renaming;

	// Position emitting cursor right after icon, for inputBox we don't add padding because it's already included there 
	if (renamingActive)
		SetCursorScreenPos(ImVec2(iconMax.x, min.y));
	else
		SetCursorScreenPos(ImVec2(iconMax.x + style->FramePadding.x, min.y));

	bool pressed = false;
	bool hovered = false;

	// In 'Renaming' state we display input field, in regular state we display just text
	if (renamingActive)
	{
		ConstString inputId = FormatTemp("##BTI_ID_%u", id);

		SetNextItemWidth(inputSize.x);
		PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		InputText(inputId, state.Buffer, state.BufferSize);
		PopStyleVar();

		// Input just appeared, we want to move focus on it
		if (!wasRenamming)
			SetKeyboardFocusHere(-1);
	}
	else
	{
		ImGuiButtonFlags buttonFlags = 0;
		buttonFlags |= ImGuiButtonFlags_PressedOnClick;
		buttonFlags |= ImGuiButtonFlags_AllowOverlap;

		// So out stretched button doesn't get clipped
		ColumnsBeginBackground();

		bool disabled = (flags & ImGuiRenamingSelectableFlags_Disabled) != 0;
		if (!disabled)
			pressed = ButtonBehavior(bb, id, &hovered, 0, buttonFlags);

		// We can open entry only when we are not in rename state
		if (hovered && IsMouseDoubleClicked(ImGuiMouseButton_Left))
			state.DoubleClicked = true;

		if (pressed)
		{
			// Update NavId when clicking so navigation can be resumed with gamepad/keyboard
			if (!context->NavDisableMouseHover && context->NavWindow == window && context->NavLayer == window->DC.NavLayerCurrent)
			{
				SetNavID(id, window->DC.NavLayerCurrent, context->CurrentFocusScopeId, WindowRectAbsToRel(window, bb));
				context->NavDisableHighlight = true;
			}

			MarkItemEdited(id);
		}
		SetNextItemAllowOverlap();

		// We don't need navigation highlight because we consider navigation as selection
		// RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);

		if (flags & ImGuiRenamingSelectableFlags_Outline)
		{
			window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_Border));
		}

		// Now we need to disable that to render text
		ColumnsEndBackground();

		AlignTextToFramePadding();
		Text("%s", state.TextDisplay);
	}

	// Render button frame and text
	ColumnsBeginBackground();
	if (hovered || state.Selected || state.Renaming)
	{
		int col = ImGuiCol_HeaderHovered;

		// This is really bad way to do this (god forgive me) but we simply don't have enough colors
		float alpha = 1.0f;
		if (!hovered)
			alpha -= 0.2f;
		if (state.Selected && !state.LastSelected)
			alpha -= 0.2f;
		if (!isFocused)
			alpha -= 0.4f;

		PushStyleVar(ImGuiStyleVar_Alpha, alpha);
		RenderFrame(bb.Min, bb.Max, GetColorU32(col), false, 0.0f);
		PopStyleVar();
	}
	ColumnsEndBackground();

	if (allowRename)
	{
		// Clicking outside or pressing enter will save changes
		bool enterPressed = IsKeyPressed(ImGuiKey_Enter);
		bool mouseClickedOutside = IsMouseClicked(ImGuiMouseButton_Left) && !IsItemHovered();
		if (enterPressed || mouseClickedOutside)
		{
			state.Renaming = false;
			state.AcceptRenaming = true;
		}

		// Escape will exit editing and discard changes
		bool escPressed = IsKeyPressed(ImGuiKey_Escape);
		if (escPressed)
		{
			state.Renaming = false;
			state.AcceptRenaming = false;
		}

		// Update saved state of 'Renaming' if it was changed outside / inside scope of this function
		storage->SetBool(id, state.Renaming);

		// Enable editing if F2 was pressed
		bool canBeginEdit = !state.Renaming && state.Selected;
		if (canBeginEdit && IsKeyPressed(ImGuiKey_F2, false))
		{
			state.Renaming = true;
		}
	}

	//// TODO: This can be fixed with ImGui::BeginGroup?
	//// This has been overwritten by text / input so we want to set actual rect
	//context->LastItemData.Rect = bb;

	EndGroup();

	// Select entry if navigation was used
	if (context->NavJustMovedToId == id)
		return true; // 'Pressed'

	return pressed;
}

void ImGui::ColumnsBeginBackground()
{
	ImGuiWindow* window = GetCurrentWindow();
	ImGuiContext* context = GImGui;

	if (window->DC.CurrentColumns)
		PushColumnsBackground();
	else if (context->CurrentTable)
		TablePushBackgroundChannel();
}

void ImGui::ColumnsEndBackground()
{
	ImGuiWindow* window = GetCurrentWindow();
	ImGuiContext* context = GImGui;

	if (window->DC.CurrentColumns)
		PopColumnsBackground();
	else if (context->CurrentTable)
		TablePopBackgroundChannel();
}

void ImGui::TextCentered(ConstString text, ImGuiTextCenteredFlags flags)
{
	ImVec2 workRectSize = GetCurrentWindow()->WorkRect.GetSize();
	ImVec2 textSize = CalcTextSize(text);
	ImVec2 cursor = GetCursorPos();

	if (flags & ImGuiTextCenteredFlags_Horizontal)
	{
		float availX = workRectSize.x - cursor.x;
		SetCursorPosX(cursor.x + (availX - textSize.x) * 0.5f);
	}

	if (flags & ImGuiTextCenteredFlags_Vertical)
	{
		float availY = workRectSize.y - cursor.y;
		SetCursorPosY(cursor.y + (availY - textSize.y) * 0.5f);
	}

	Text("%s", text);
}

ImU32 ImGui::AddHSV(ImU32 col, float h, float s, float v)
{
	ImVec4 c = ColorConvertU32ToFloat4(col);
	ColorConvertRGBtoHSV(c.x, c.y, c.z, c.x, c.y, c.z);
	c.x = ImClamp(c.x + h, 0.0f, 1.0f); // Hue
	c.y = ImClamp(c.y + s, 0.0f, 1.0f); // Saturation
	c.z = ImClamp(c.z + v, 0.0f, 1.0f); // Value
	ColorConvertHSVtoRGB(c.x, c.y, c.z, c.x, c.y, c.z);
	return ColorConvertFloat4ToU32(c);
}

ImU32 ImGui::GetMissingColor()
{
	float phase = fabs(sinf((float)GetTime() * 2.5f));

	static constexpr ImVec4 COL1 = { 0.78f, 0.26f, 0.30f, 1.0f };
	static constexpr ImVec4 COL2 = { 0.36f, 0.10f, 0.13f, 1.0f };

	return ColorConvertFloat4ToU32(ImLerp(COL1, COL2, phase));
}

void ImGui::ScrollingLabel(ConstString text)
{
	ImGuiWindow* window = GImGui->CurrentWindow;
	ImVec2 cursor = window->DC.CursorPos;
	// Height of text + span full width from cursor to work rect end
	ImRect rect(cursor, ImVec2(window->WorkRect.Max.x, cursor.y + GImGui->FontSize));
	ItemSize(rect);
	ItemAdd(rect, 0);
	ScrollingLabel(cursor, rect, text);
}

void ImGui::ScrollingLabel(const ImVec2& pos, const ImRect& bb, ConstString text)
{
	ScrollingLabel(pos, bb, text, GetColorU32(ImGuiCol_Text), 0.0f);
}

void ImGui::ScrollingLabel(const ImVec2& pos, const ImRect& bb, ConstString text, ImU32 col, double startTime)
{
	ConstString textEnd = FindRenderedTextEnd(text);
	ScrollingLabel(pos, bb, text, textEnd, col, startTime);
}

void ImGui::ScrollingLabel(const ImVec2& pos, const ImRect& bb, ConstString text, ConstString textEnd, ImU32 col, double startTime)
{
	const ImGuiStyle& style = GImGui->Style;
	ImGuiWindow* window = GetCurrentWindow();

	ImVec2 textSize = CalcTextSize(text, textEnd);
	ImVec2 animPos = pos;
	ImRect adjustedBB = bb;
	adjustedBB.Min.x = pos.x;

	float sidePadding = style.FramePadding.x * 2.0f;
	float textDisplayWidth = adjustedBB.GetWidth() - sidePadding;
	// Scrolling text animation
	bool animate = textSize.x > textDisplayWidth;
	if (animate)
	{
		// Total animation takes 'animTotalDuration' seconds,
		// scrolling starts after 'animWaitTime' seconds,
		// scrolls for 'animDuration' seconds and stays for 'animWaitTime' seconds
		constexpr float animWaitTime = 2.0f;
		constexpr float animDuration = 2.5f;
		constexpr float animTotalDuration = animDuration + animWaitTime * 2.0f;

		double currentTime = GetTime() - startTime;
		// Loop every X seconds
		float animTime = static_cast<float>(fmod(currentTime, static_cast<double>(animTotalDuration)));
		float currentPhase = ImRemap(animTime, animWaitTime, animWaitTime + animDuration, 0.0f, 1.0f);
		currentPhase = ImClamp(currentPhase, 0.0f, 1.0f);

		// Scroll to left
		float clippedWidth = textSize.x - textDisplayWidth;
		animPos.x -= clippedWidth * currentPhase;
	}

	PushClipRect(adjustedBB.Min, adjustedBB.Max, true);
	window->DrawList->AddText(animPos, col, text, textEnd);
	PopClipRect();
}

void ImGui::StatusBar()
{
	ImGuiWindow* window = GetCurrentWindow();
	ImGuiStyle& style = GImGui->Style;

	float frameHeight = GetFrameHeight();

	ImVec2 barMin(window->ClipRect.Min.x, window->ClipRect.Max.y - frameHeight);
	ImVec2 barMax(window->ClipRect.Max.x, window->ClipRect.Max.y);

	SetCursorScreenPos(ImVec2(barMin.x + style.FramePadding.x, barMin.y));

	RenderFrame(barMin, barMax, GetColorU32(ImGuiCol_WindowBg), false);
}

void ImGui::PreStatusBar()
{
	ImGuiWindow* window = GetCurrentWindow();
	window->ClipRect.Max.y -= GetFrameHeight();
}

bool ImGui::IconTreeNode(ConstString text, bool& selected, bool& toggled, ImTextureID icon, ImGuiIconTreeNodeFlags flags)
{
	toggled = false;

	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	constexpr float arrowSizeX = 18.0f; // Eyeballed
	constexpr float iconSize = 16.0f;

	ImGuiStorage* storage = window->DC.StateStorage;
	ImGuiContext* context = GImGui;
	ImGuiStyle& style = context->Style;
	ImGuiID id = window->GetID(text);

	bool& isOpen = *storage->GetBoolRef(id, flags & ImGuiIconTreeNodeFlags_DefaultOpen);

	ImVec2 cursor = window->DC.CursorPos;

	// Stretch horizontally on whole window
	ImVec2 frameSize(window->WorkRect.GetWidth(), iconSize + style.FramePadding.y);
	ImVec2 frameMin(window->WorkRect.Min.x, cursor.y);
	ImVec2 frameMax(frameMin.x + frameSize.x, frameMin.y + frameSize.y);
	ImRect frameBb(frameMin, frameMax);

	ItemSize(frameSize, 0.0f);
	if (!ItemAdd(frameBb, id))
	{
		if (isOpen)
			TreePushOverrideID(id);

		return isOpen;
	}

	ImVec2 textSize = CalcTextSize(text);
	float centerTextY = IM_GET_CENTER(frameMin.y, frameSize.y, textSize.y);

	// Based on whether we hover arrow / frame we select corresponding bounding box for button

	ImVec2 arrowMin(cursor.x + style.FramePadding.x, frameMin.y);
	ImVec2 arrowMax(arrowMin.x + arrowSizeX, frameMax.y);
	ImRect arrowBb(arrowMin, arrowMax);

	ImVec2 iconMin(arrowMax.x, IM_GET_CENTER(frameMin.y, frameSize.y, iconSize));
	ImVec2 iconMax(iconMin.x + iconSize, iconMin.y + iconSize);

	ImVec2 textMin(iconMax.x, frameMin.y);
	ImVec2 textMax(textMin.x + textSize.x, frameMax.y);

	ImVec2 arrowPos(arrowMin.x, centerTextY);
	ImVec2 textPos(textMin.x, centerTextY);

	bool hoversArrow = IsMouseHoveringRect(arrowMin, arrowMax);

	ImGuiButtonFlags buttonFlags = 0;
	bool hovered, held;
	bool pressed = ButtonBehavior(hoversArrow ? arrowBb : frameBb, id, &hovered, &held, buttonFlags);

	if (!hoversArrow && pressed)
		selected = true;

	if (IsMouseHoveringRect(textMin, textMax))
		SetMouseCursor(ImGuiMouseCursor_Hand);

	bool canOpen = !(flags & ImGuiIconTreeNodeFlags_NoChildren);
	if (canOpen)
	{
		// Toggle on simple arrow click
		if (pressed && hoversArrow)
			toggled = true;

		// Toggle on mouse double click
		if (hovered && context->IO.MouseClickedCount[ImGuiMouseButton_Left] == 2)
			toggled = true;

		// Arrow right opens node
		if (isOpen && context->NavId == id && context->NavMoveDir == ImGuiDir_Left)
		{
			toggled = true;
			NavMoveRequestCancel();
		}

		// Arrow left closes node
		if (!isOpen && context->NavId == id && context->NavMoveDir == ImGuiDir_Right)
		{
			toggled = true;
			NavMoveRequestCancel();
		}

		if (toggled)
		{
			isOpen = !isOpen;
			context->LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
		}
	}

	// Render

	if (hovered || selected)
	{
		ImU32 col1 = IM_COL32(100, 160, 220, 255);
		ImU32 col2 = IM_COL32(35, 110, 190, 255);

		float sIncrease = 0;
		float vIncrease = 0;
		if (selected) { vIncrease += 0.1f; sIncrease += 0.05f; }
		if (held) { vIncrease += 0.2f; sIncrease += 0.1f; }

		col1 = AddHSV(col1, 0, sIncrease, vIncrease);
		col2 = AddHSV(col2, 0, sIncrease, vIncrease);

		PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
		PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
		PushStyleColor(ImGuiCol_Border, 0);
		RenderFrameGradient(frameBb, col1, col2);
		PopStyleColor();
		PopStyleVar(2);
	}
	RenderNavHighlight(frameBb, id, ImGuiNavHighlightFlags_TypeThin);

	// Arrow, we add slow fading in/out just like in windows explorer
	if (canOpen)
	{
		bool arrowVisible = context->HoveredWindow == window || IsWindowFocused();

		float& alpha = *storage->GetFloatRef(id + 1);

		// Fade in fast, fade out slower...
		alpha += GetIO().DeltaTime * (arrowVisible ? 4.0f : -2.0f);

		// Make max alpha level a little dimmer for sub-nodes
		float maxAlpha = window->DC.TreeDepth == 0 ? 0.8f : 0.55f;
		alpha = ImClamp(alpha, 0.0f, maxAlpha);

		PushStyleVar(ImGuiStyleVar_Alpha, hoversArrow ? maxAlpha : alpha);
		RenderText(arrowPos, isOpen ? ICON_FA_ANGLE_DOWN : ICON_FA_ANGLE_RIGHT);
		PopStyleVar();
	}

	window->DrawList->AddImage(icon, iconMin, iconMax);

	ImGui::PushFont(ImFont_Medium);
	RenderText(textPos, text);
	PopFont();

	if (isOpen)
		TreePushOverrideID(id);
	return isOpen;
}

void ImGui::TreeNodeSetOpened(ConstString text, bool opened)
{
	ImGuiWindow* window = GetCurrentWindow();
	ImGuiID id = window->GetID(text);

	window->StateStorage.SetBool(id, opened);

	TreePushOverrideID(id);
}

bool ImGui::DragSelectionBehaviour(ImGuiID id, bool& stopped, ImRect& selection, const ImRect& clipRect, ImGuiDragSelectionFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	ImGuiStorage& storage = window->StateStorage;

	stopped = false;

	bool disabled = flags & ImGuiDragSelectionFlags_DisableBegin;

	// Although right mouse selection is a thing but do we need it?
	bool mouseDown = IsMouseDown(ImGuiMouseButton_Left);
	bool mouseClicked = IsMouseClicked(ImGuiMouseButton_Left, false);

	constexpr float maxAlpha = 1.0f;
	constexpr float fadeoutTime = 6.0f; // Higher = faster

	bool& dragging = *storage.GetBoolRef(id + 4);
	float& alpha = *storage.GetFloatRef(id + 5, 0.0f);

	if (!mouseDown)
	{
		if (dragging)
		{
			stopped = true;
			dragging = false;
		}

		// This alpha fading out is 'inspired' by Mac OS X look
		// And it's simply beautiful

		alpha -= GImGui->IO.DeltaTime * fadeoutTime;
		alpha = ImClamp(alpha, 0.0f, maxAlpha);
		if (alpha == 0.0f)
			return false;
	}

	ImRect bb = GetStorageRect(storage, id + 0);
	if (mouseDown)
	{
		ImVec2 cursor = GetMousePos();

		if (!dragging) // Begin selection
		{
			bool canStart = !disabled && mouseClicked;
			if (canStart)
			{
				alpha = maxAlpha;
				bb.Min = cursor;
				dragging = true;
			}
			else
			{
				return false;
			}
		}

		// Update on-going selection
		bb.Max = cursor;
		SetStorageRect(storage, id, bb);
	}

	// We have to make sure that rect is not inverted (min > max)
	// TODO: Maybe store it as two mouse positions?
	selection.Min.x = MIN(bb.Min.x, bb.Max.x);
	selection.Max.x = MAX(bb.Min.x, bb.Max.x);
	selection.Min.y = MIN(bb.Min.y, bb.Max.y);
	selection.Max.y = MAX(bb.Min.y, bb.Max.y);
	selection.ClipWith(clipRect);

	// Render Rect + Border

	// TODO: Hardcoded colors are bad, how we can even extend default color set?
	ImVec4 fill = ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 35));
	ImVec4 outline = ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 125));

	fill.w *= alpha;
	outline.w *= alpha;

	GetForegroundDrawList()->AddRectFilled(selection.Min, selection.Max, ColorConvertFloat4ToU32(fill));
	GetForegroundDrawList()->AddRect(selection.Min, selection.Max, ColorConvertFloat4ToU32(outline));

	return mouseDown;
}

ImRect ImGui::GetStorageRect(const ImGuiStorage& storage, ImGuiID id)
{
	float minX, minY, maxX, maxY;
	minX = storage.GetFloat(id + 0);
	minY = storage.GetFloat(id + 1);
	maxX = storage.GetFloat(id + 2);
	maxY = storage.GetFloat(id + 3);

	return { minX, minY, maxX, maxY };
}

void ImGui::SetStorageRect(ImGuiStorage& storage, ImGuiID id, const ImRect& rect)
{
	storage.SetFloat(id + 0, rect.Min.x);
	storage.SetFloat(id + 1, rect.Min.y);
	storage.SetFloat(id + 2, rect.Max.x);
	storage.SetFloat(id + 3, rect.Max.y);
}

void ImGui::TextEllipsis(ConstString text)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiStyle& style = GImGui->Style;

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size = CalcTextSize(text);

	ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

	ItemSize(size);
	if (!ItemAdd(bb, 0))
		return;

	ConstString textEnd = text + strlen(text);

	ImRect& workRect = window->WorkRect;
	ImRect& clipRect = window->ClipRect;

	float ellipsisMax = workRect.Max.x - style.FramePadding.x;

	RenderTextEllipsis(
		window->DrawList, bb.Min, workRect.Max, clipRect.Max.x, ellipsisMax, text, textEnd, &size);
}

void ImGui::ToolTip(ConstString text)
{
	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));

	if (IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_ForTooltip))
	{
		SetTooltip(text);
	}

	PopStyleVar();
}

void ImGui::HelpMarker(ConstString tooltip, ConstString iconText)
{
	// If there's no icon text we have to remove space between icon text and icon itself
	if (!iconText || iconText[0] == '\0')
		TextDisabled("%s", ICON_FA_CIRCLE_QUESTION);
	else
		TextDisabled("%s %s", iconText, ICON_FA_CIRCLE_QUESTION);
	ToolTip(tooltip);
}

void ImGui::BeginToolBar(ConstString name)
{
	ImGuiWindow* window = GetCurrentWindow();
	ImGuiStyle& style = GImGui->Style;

	float frameHeight = GetFrameHeight();
	float barHeight = frameHeight + style.FramePadding.y * 2;

	ImVec2 min = window->DC.CursorPos;
	ImVec2 max = ImVec2(
		window->WorkRect.Max.x - style.FramePadding.x,
		min.y + barHeight);

	window->DC.LayoutType = ImGuiLayoutType_Horizontal;
	window->DC.CursorPos.x += style.FramePadding.x;
	window->DC.CursorPos.y += (barHeight - frameHeight) / 2.0f;

	//ImGui::RenderFrame(ImRect(min, max), ImGui::GetColorGradient(ImGuiCol_ToolbarBg), 1, 0, ImGuiAxis_X);
	RenderFrame(min, max, GetColorU32(ImGuiCol_ChildBg), false);
}

void ImGui::EndToolBar()
{
	ImGuiWindow* window = GetCurrentWindow();
	ImGuiStyle& style = GImGui->Style;

	float frameHeight = GetFrameHeight();
	float barHeight = frameHeight + style.FramePadding.y * 2;

	window->DC.LayoutType = ImGuiLayoutType_Vertical;
	/*window->DC.CursorPos.x -= style.FramePadding.x;
	window->DC.CursorPos.y += (barHeight + frameHeight) / 2.0f;*/
	ImGui::NewLine();
}

bool ImGui::NavButton(ConstString idStr, ImGuiDir dir, bool enabled)
{
	ImGuiWindow* window = GetCurrentWindow();
	ImGuiStyle& style = GImGui->Style;

	ImGuiID id = window->GetID(idStr);

	ConstString text = "";
	switch (dir)
	{
	case ImGuiDir_Up:		text = ICON_FA_ARROW_UP;		break;
	case ImGuiDir_Left:		text = ICON_FA_ARROW_LEFT;		break;
	case ImGuiDir_Down:		text = ICON_FA_ARROW_DOWN;		break;
	case ImGuiDir_Right:	text = ICON_FA_ARROW_RIGHT;		break;
	}

	ImVec2 textSize = CalcTextSize(text);
	ImVec2 size = textSize;
	size.x += style.FramePadding.x * 2.0f;
	size.y += style.FramePadding.y * 2.0f;

	ImVec2 textPos = window->DC.CursorPos;
	textPos.x = IM_GET_CENTER(textPos.x, size.x, textSize.x);
	textPos.y = IM_GET_CENTER(textPos.y, size.y, textSize.y);

	ImVec2 min = window->DC.CursorPos;
	ImVec2 max = ImVec2(min.x + size.x, min.y + size.y);
	ImRect bb(min, max);

	ItemSize(size);
	if (!ItemAdd(bb, id))
		return false;

	if (!enabled) BeginDisabled();

	ImGuiButtonFlags buttonFlags = 0;
	buttonFlags |= ImGuiButtonFlags_MouseButtonLeft;

	bool hovered = false;
	bool pressed = false;
	if (enabled)
	{
		pressed = ButtonBehavior(bb, id, &hovered, 0, buttonFlags);
	}

	int col = ImGuiCol_Text;
	if (pressed)		col = ImGuiCol_NavHighlight;
	else if (hovered)	col = ImGuiCol_NavHighlight;

	window->DrawList->AddText(textPos, GetColorU32(col), text);
	RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin);

	if (!enabled) EndDisabled();

	return pressed;
}

void ImGui::SnapToPrevious()
{
	ImGuiWindow* window = GetCurrentWindow();
	window->DC.CursorPos.x = GImGui->LastItemData.Rect.Min.x;
	window->DC.CursorPos.y = GImGui->LastItemData.Rect.Max.y + GImGui->Style.ItemSpacing.y;
}

void ImGui::BeginDockSpace()
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	const ImGuiViewport* viewport = GetMainViewport();
	SetNextWindowPos(viewport->WorkPos);
	SetNextWindowSize(viewport->WorkSize);
	SetNextWindowViewport(viewport->ID);
	PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	window_flags |=
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	Begin("DockSpaceWindow", nullptr, window_flags);
	DockSpace(GetID("MyDockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	ImGuiWindow* window = GetCurrentWindow();
	//SlGui::RenderFrame(window->ClipRect, SlGui::GetColorGradient(SlGuiCol_DockspaceBg));
	// ImGui::RenderFrame(window->ClipRect.Min, window->ClipRect.Max, ImGui::GetColorU32(ImGuiCol_))

	PopStyleVar(3); // Dock window styles
}

bool ImGui::BeginDragDropSource2(ImGuiDragDropFlags flags)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;

	// FIXME-DRAGDROP: While in the common-most "drag from non-zero active id" case we can tell the mouse button,
	// in both SourceExtern and id==0 cases we may requires something else (explicit flags or some heuristic).
	ImGuiMouseButton mouse_button = ImGuiMouseButton_Left;

	bool source_drag_active = false;
	ImGuiID source_id = 0;
	ImGuiID source_parent_id = 0;
	if (!(flags & ImGuiDragDropFlags_SourceExtern))
	{
		source_id = g.LastItemData.ID;
		if (source_id != 0)
		{
			// Common path: items with ID
			if (g.ActiveId != source_id)
				return false;
			if (g.ActiveIdMouseButton != -1)
				mouse_button = g.ActiveIdMouseButton;
			if (g.IO.MouseDown[mouse_button] == false || window->SkipItems)
				return false;
			g.ActiveIdAllowOverlap = false;
		}
		else
		{
			// Uncommon path: items without ID
			if (g.IO.MouseDown[mouse_button] == false || window->SkipItems)
				return false;

			// This breaks selection in tables with background rendering (ImGui::Selectable)
			//if ((g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HoveredRect) == 0 && (g.ActiveId == 0 || g.ActiveIdWindow != window))
			//	return false;

			// If you want to use BeginDragDropSource() on an item with no unique identifier for interaction, such as Text() or Image(), you need to:
			// A) Read the explanation below, B) Use the ImGuiDragDropFlags_SourceAllowNullID flag.
			if (!(flags & ImGuiDragDropFlags_SourceAllowNullID))
			{
				IM_ASSERT(0);
				return false;
			}

			// Magic fallback to handle items with no assigned ID, e.g. Text(), Image()
			// We build a throwaway ID based on current ID stack + relative AABB of items in window.
			// THE IDENTIFIER WON'T SURVIVE ANY REPOSITIONING/RESIZINGG OF THE WIDGET, so if your widget moves your dragging operation will be canceled.
			// We don't need to maintain/call ClearActiveID() as releasing the button will early out this function and trigger !ActiveIdIsAlive.
			// Rely on keeping other window->LastItemXXX fields intact.
			source_id = g.LastItemData.ID = window->GetIDFromRectangle(g.LastItemData.Rect);
			KeepAliveID(source_id);

			// To manual hover check because ItemHoverable will create new item and break click for button behaviour
			bool is_hovered = IsMouseHoveringRect(
				g.LastItemData.Rect.Min,
				g.LastItemData.Rect.Max, false) && IsWindowHovered();
			// ItemHoverable(g.LastItemData.Rect, source_id);

			if (is_hovered && g.IO.MouseClicked[mouse_button])
			{
				SetActiveID(source_id, window);
				FocusWindow(window);
			}
			if (g.ActiveId == source_id) // Allow the underlying widget to display/return hovered during the mouse release frame, else we would get a flicker.
				g.ActiveIdAllowOverlap = is_hovered;
		}
		if (g.ActiveId != source_id)
			return false;
		source_parent_id = window->IDStack.back();
		source_drag_active = IsMouseDragging(mouse_button);

		// Disable navigation and key inputs while dragging + cancel existing request if any
		SetActiveIdUsingAllKeyboardKeys();
	}
	else
	{
		window = NULL;
		source_id = ImHashStr("#SourceExtern");
		source_drag_active = true;
	}

	if (source_drag_active)
	{
		if (!g.DragDropActive)
		{
			IM_ASSERT(source_id != 0);
			ClearDragDrop();
			ImGuiPayload& payload = g.DragDropPayload;
			payload.SourceId = source_id;
			payload.SourceParentId = source_parent_id;
			g.DragDropActive = true;
			g.DragDropSourceFlags = flags;
			g.DragDropMouseButton = mouse_button;
			if (payload.SourceId == g.ActiveId)
				g.ActiveIdNoClearOnFocusLoss = true;
		}
		g.DragDropSourceFrameCount = g.FrameCount;
		g.DragDropWithinSource = true;

		if (!(flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
		{
			// Target can request the Source to not display its tooltip (we use a dedicated flag to make this request explicit)
			// We unfortunately can't just modify the source flags and skip the call to BeginTooltip, as caller may be emitting contents.
			bool ret = BeginTooltip();
			IM_ASSERT(ret); // FIXME-NEWBEGIN: If this ever becomes false, we need to Begin("##Hidden", NULL, ImGuiWindowFlags_NoSavedSettings) + SetWindowHiddendAndSkipItemsForCurrentFrame().
			IM_UNUSED(ret);

			if (g.DragDropAcceptIdPrev && (g.DragDropAcceptFlags & ImGuiDragDropFlags_AcceptNoPreviewTooltip))
				SetWindowHiddendAndSkipItemsForCurrentFrame(g.CurrentWindow);
		}

		if (!(flags & ImGuiDragDropFlags_SourceNoDisableHover) && !(flags & ImGuiDragDropFlags_SourceExtern))
			g.LastItemData.StatusFlags &= ~ImGuiItemStatusFlags_HoveredRect;

		return true;
	}
	return false;
}

float ImGui::Distance(const ImVec2& a, const ImVec2& b)
{
	return rage::Vec3V(a.x, a.y).DistanceTo(rage::Vec3V(b.x, b.y)).Get();
}

bool ImGui::DragU8(const char* label, u8* value, u8 speed, u8 min, u8 max, ConstString format, ImGuiSliderFlags flags)
{
	float speedF = speed;
	return DragScalar(label, ImGuiDataType_U8, value, speedF, &min, &max, format, flags);
}

bool ImGui::SliderU8(const char* label, u8* value, u8 min, u8 max, ConstString format, ImGuiSliderFlags flags)
{
	return SliderScalar(label, ImGuiDataType_U8, value, &min, &max, format, flags);
}

bool ImGui::InputU16(const char* label, u16* value)
{
	return InputScalar(label, ImGuiDataType_U16, value, 0, 0);
}

void ImGui::InputTextPlaceholder(ConstString inputText, ConstString placeholder, bool showIfActive)
{
	// Show only if text box is not selected
	if (!showIfActive && IsItemActive())
		return;

	// There is some text... placeholder must be shown only in empty text box
	if (inputText[0] != '\0')
		return;

	rageam::graphics::ColorU32 textColor = GetColorU32(ImGuiCol_Text);
	textColor.A -= 80; // Make a little bit dimmer

	const ImRect& rect = GImGui->LastItemData.Rect;
	ImVec2 textPos = rect.Min + GImGui->Style.FramePadding;
	GetCurrentWindow()->DrawList->AddText(textPos, textColor, placeholder);
}

namespace ImGui
{
	template<typename TYPE, typename FLOATTYPE>
	float SliderCalcRatioFromValueT(
		ImGuiDataType data_type, TYPE v, TYPE v_min, TYPE v_max, float power, float linear_zero_pos)
	{
		if (v_min == v_max)
			return 0.0f;

		const bool is_power
			= (power != 1.0f) && (data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double);
		const TYPE v_clamped = (v_min < v_max) ? ImClamp(v, v_min, v_max) : ImClamp(v, v_max, v_min);
		if (is_power)
		{
			if (v_clamped < 0.0f)
			{
				const float f
					= 1.0f
					- static_cast<float>((v_clamped - v_min) / (ImMin(static_cast<TYPE>(0), v_max) - v_min));
				return (1.0f - ImPow(f, 1.0f / power)) * linear_zero_pos;
			}
			else
			{
				const float f = static_cast<float>(
					(v_clamped - ImMax(static_cast<TYPE>(0), v_min))
					/ (v_max - ImMax(static_cast<TYPE>(0), v_min)));
				return linear_zero_pos + ImPow(f, 1.0f / power) * (1.0f - linear_zero_pos);
			}
		}

		// Linear slider
		return static_cast<float>(static_cast<FLOATTYPE>(v_clamped - v_min) / static_cast<FLOATTYPE>(v_max - v_min));
	}


	float RoundScalarWithFormatFloat(const char* format, ImGuiDataType data_type, float v)
	{
		return ImGui::RoundScalarWithFormatT(format, data_type, v);
	}


	float SliderCalcRatioFromValueFloat(
		ImGuiDataType data_type, float v, float v_min, float v_max, float power, float linear_zero_pos)
	{
		return SliderCalcRatioFromValueT<float, float>(data_type, v, v_min, v_max, power, linear_zero_pos);
	}

	// ~80% common code with ImGui::SliderBehavior
	bool RangeSliderBehavior(const ImRect& frame_bb, ImGuiID id, float* v1, float* v2, float v_min, float v_max, float power, int decimal_precision, ImGuiSliderFlags flags)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = GetCurrentWindow();
		const ImGuiStyle& style = g.Style;

		// Draw frame
		RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

		const bool is_non_linear = (power < 1.0f - 0.00001f) || (power > 1.0f + 0.00001f);
		const bool is_horizontal = (flags & ImGuiSliderFlags_Vertical) == 0;

		const float grab_padding = 2.0f;
		const float slider_sz = is_horizontal ? (frame_bb.GetWidth() - grab_padding * 2.0f) : (frame_bb.GetHeight() - grab_padding * 2.0f);
		float grab_sz;
		if (decimal_precision > 0)
			grab_sz = ImMin(style.GrabMinSize, slider_sz);
		else
			grab_sz = ImMin(ImMax(1.0f * (slider_sz / ((v_min < v_max ? v_max - v_min : v_min - v_max) + 1.0f)), style.GrabMinSize), slider_sz);  // Integer sliders, if possible have the grab size represent 1 unit
		const float slider_usable_sz = slider_sz - grab_sz;
		const float slider_usable_pos_min = (is_horizontal ? frame_bb.Min.x : frame_bb.Min.y) + grab_padding + grab_sz * 0.5f;
		const float slider_usable_pos_max = (is_horizontal ? frame_bb.Max.x : frame_bb.Max.y) - grab_padding - grab_sz * 0.5f;

		// For logarithmic sliders that cross over sign boundary we want the exponential increase to be symmetric around 0.0f
		float linear_zero_pos = 0.0f;   // 0.0->1.0f
		if (v_min * v_max < 0.0f)
		{
			// Different sign
			const float linear_dist_min_to_0 = powf(fabsf(0.0f - v_min), 1.0f / power);
			const float linear_dist_max_to_0 = powf(fabsf(v_max - 0.0f), 1.0f / power);
			linear_zero_pos = linear_dist_min_to_0 / (linear_dist_min_to_0 + linear_dist_max_to_0);
		}
		else
		{
			// Same sign
			linear_zero_pos = v_min < 0.0f ? 1.0f : 0.0f;
		}

		// Process clicking on the slider
		bool value_changed = false;
		if (g.ActiveId == id)
		{
			if (g.IO.MouseDown[0])
			{
				const float mouse_abs_pos = is_horizontal ? g.IO.MousePos.x : g.IO.MousePos.y;
				float clicked_t = (slider_usable_sz > 0.0f) ? ImClamp((mouse_abs_pos - slider_usable_pos_min) / slider_usable_sz, 0.0f, 1.0f) : 0.0f;
				if (!is_horizontal)
					clicked_t = 1.0f - clicked_t;

				float new_value;
				if (is_non_linear)
				{
					// Account for logarithmic scale on both sides of the zero
					if (clicked_t < linear_zero_pos)
					{
						// Negative: rescale to the negative range before powering
						float a = 1.0f - (clicked_t / linear_zero_pos);
						a = powf(a, power);
						new_value = ImLerp(ImMin(v_max, 0.0f), v_min, a);
					}
					else
					{
						// Positive: rescale to the positive range before powering
						float a;
						if (fabsf(linear_zero_pos - 1.0f) > 1.e-6f)
							a = (clicked_t - linear_zero_pos) / (1.0f - linear_zero_pos);
						else
							a = clicked_t;
						a = powf(a, power);
						new_value = ImLerp(ImMax(v_min, 0.0f), v_max, a);
					}
				}
				else
				{
					// Linear slider
					new_value = ImLerp(v_min, v_max, clicked_t);
				}

				char fmt[64];
				snprintf(fmt, 64, "%%.%df", decimal_precision);

				// Round past decimal precision
				new_value = RoundScalarWithFormatFloat(fmt, ImGuiDataType_Float, new_value);
				if (*v1 != new_value || *v2 != new_value)
				{
					if (fabsf(*v1 - new_value) < fabsf(*v2 - new_value))
					{
						*v1 = new_value;
					}
					else
					{
						*v2 = new_value;
					}
					value_changed = true;
				}
			}
			else
			{
				ClearActiveID();
			}
		}

		// Calculate slider grab positioning
		float grab_t = SliderCalcRatioFromValueFloat(ImGuiDataType_Float, *v1, v_min, v_max, power, linear_zero_pos);

		// Draw
		if (!is_horizontal)
			grab_t = 1.0f - grab_t;
		float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
		ImRect grab_bb1;
		if (is_horizontal)
			grab_bb1 = ImRect(ImVec2(grab_pos - grab_sz * 0.5f, frame_bb.Min.y + grab_padding), ImVec2(grab_pos + grab_sz * 0.5f, frame_bb.Max.y - grab_padding));
		else
			grab_bb1 = ImRect(ImVec2(frame_bb.Min.x + grab_padding, grab_pos - grab_sz * 0.5f), ImVec2(frame_bb.Max.x - grab_padding, grab_pos + grab_sz * 0.5f));
		window->DrawList->AddRectFilled(grab_bb1.Min, grab_bb1.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

		// Calculate slider grab positioning
		grab_t = SliderCalcRatioFromValueFloat(ImGuiDataType_Float, *v2, v_min, v_max, power, linear_zero_pos);

		// Draw
		if (!is_horizontal)
			grab_t = 1.0f - grab_t;
		grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
		ImRect grab_bb2;
		if (is_horizontal)
			grab_bb2 = ImRect(ImVec2(grab_pos - grab_sz * 0.5f, frame_bb.Min.y + grab_padding), ImVec2(grab_pos + grab_sz * 0.5f, frame_bb.Max.y - grab_padding));
		else
			grab_bb2 = ImRect(ImVec2(frame_bb.Min.x + grab_padding, grab_pos - grab_sz * 0.5f), ImVec2(frame_bb.Max.x - grab_padding, grab_pos + grab_sz * 0.5f));
		window->DrawList->AddRectFilled(grab_bb2.Min, grab_bb2.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

		ImRect connector(grab_bb1.Min, grab_bb2.Max);
		connector.Min.x += grab_sz;
		connector.Min.y += grab_sz * 0.3f;
		connector.Max.x -= grab_sz;
		connector.Max.y -= grab_sz * 0.3f;

		window->DrawList->AddRectFilled(connector.Min, connector.Max, GetColorU32(ImGuiCol_SliderGrab), style.GrabRounding);

		return value_changed;
	}

	// ~95% common code with ImGui::SliderFloat
	bool RangeSliderFloat(const char* label, float* v1, float* v2, float v_min, float v_max, const char* display_format, float power)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const float w = CalcItemWidth();

		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
		const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

		// NB- we don't call ItemSize() yet because we may turn into a text edit box below
		if (!ItemAdd(total_bb, id))
		{
			ItemSize(total_bb, style.FramePadding.y);
			return false;
		}

		const bool hovered = ItemHoverable(frame_bb, id, 0);
		if (hovered)
			SetHoveredID(id);

		if (!display_format)
			display_format = "(%.3f, %.3f)";
		int decimal_precision = ImParseFormatPrecision(display_format, 3);

		// Tabbing or CTRL-clicking on Slider turns it into an input box
		bool start_text_input = false;
		const bool tab_focus_requested = false; // (GetItemStatusFlags() & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
		if (tab_focus_requested || (hovered && g.IO.MouseClicked[0]))
		{
			SetActiveID(id, window);
			FocusWindow(window);

			if (tab_focus_requested || g.IO.KeyCtrl)
			{
				start_text_input = true;
				g.TempInputId = 0;
			}
		}

		if (start_text_input || (g.ActiveId == id && g.TempInputId == id))
		{
			char fmt[64];
			snprintf(fmt, 64, "%%.%df", decimal_precision);
			return TempInputScalar(frame_bb, id, label, ImGuiDataType_Float, v1, fmt);
		}

		ItemSize(total_bb, style.FramePadding.y);

		// Actual slider behavior + render grab
		const bool value_changed = RangeSliderBehavior(frame_bb, id, v1, v2, v_min, v_max, power, decimal_precision, 0);

		// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
		char value_buf[64];
		const char* value_buf_end = value_buf + ImFormatString(value_buf, IM_ARRAYSIZE(value_buf), display_format, *v1, *v2);
		RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

		if (label_size.x > 0.0f)
			RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

		return value_changed;
	}
}

bool ImGui::BeginPadded(ConstString name, const ImVec2& padding)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
	bool opened = ImGui::BeginChild(name, ImVec2(0, 0), false, ImGuiChildFlags_AlwaysUseWindowPadding);
	ImGui::PopStyleVar();
	return opened;
}

void ImGui::EndPadded()
{
	ImGui::EndChild();
}
