#include "extensions.h"

#include "imgui_internal.h"
#include "common/logger.h"
#include "font_icons/icons_awesome.h"
#include "misc/freetype/imgui_freetype.h"
#include "styled/slgui.h"
#include "styled/slwidgets.h"

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
		buttonFlags |= ImGuiButtonFlags_AllowItemOverlap;

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
		SetItemAllowOverlap();

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
	ImVec2 windowSize = GetWindowSize();
	ImVec2 textSize = CalcTextSize(text);
	ImVec2 cursor = GetCursorPos();

	if (flags & ImGuiTextCenteredFlags_Horizontal)
	{
		float availX = windowSize.x - cursor.x;
		SetCursorPosX(cursor.x + (availX - textSize.x) * 0.5f);
	}

	if (flags & ImGuiTextCenteredFlags_Vertical)
	{
		float availY = windowSize.y - cursor.y;
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

void ImGui::StatusBar()
{
	ImGuiWindow* window = GetCurrentWindow();
	ImGuiStyle& style = GImGui->Style;

	float frameHeight = GetFrameHeight();

	ImVec2 barMin(window->ClipRect.Min.x, window->ClipRect.Max.y - frameHeight);
	ImVec2 barMax(window->ClipRect.Max.x, window->ClipRect.Max.y);

	SetCursorScreenPos(ImVec2(barMin.x + style.FramePadding.x, barMin.y));

	RenderFrame(barMin, barMax, GetColorU32(ImGuiCol_MenuBarBg));
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

	SlGui::PushFont(SlFont_Medium);
	RenderText(textPos, text);
	PopFont();

	if (isOpen)
		TreePushOverrideID(id);
	return isOpen;
}

void ImGui::IconTreeNodeSetOpened(ConstString text, bool opened)
{
	ImGuiWindow* window = GetCurrentWindow();
	ImGuiID id = window->GetID(text);

	window->StateStorage.SetBool(id, opened);

	TreePushOverrideID(id);
}

bool ImGui::DragSelectionBehaviour(ImGuiID id, bool& stopped, ImRect& selection, ImGuiDragSelectionFlags flags)
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
	float& alpha = *storage.GetFloatRef(id + 5, 1.0f);

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
			bool canStart = !disabled && mouseClicked && GImGui->HoveredWindow == window;
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
	selection.Min.x = min(bb.Min.x, bb.Max.x);
	selection.Max.x = max(bb.Min.x, bb.Max.x);
	selection.Min.y = min(bb.Min.y, bb.Max.y);
	selection.Max.y = max(bb.Min.y, bb.Max.y);
	selection.ClipWith(window->ClipRect);

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

	if (IsItemHovered(ImGuiHoveredFlags_DelayShort) && BeginTooltip())
	{
		PushTextWrapPos(GetFontSize() * 35.0f);
		TextUnformatted(text);
		PopTextWrapPos();
		EndTooltip();
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

	SlGui::RenderFrame(ImRect(min, max), SlGui::GetColorGradient(SlGuiCol_ToolbarBg), 1, 0, ImGuiAxis_X);
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

	int col = ImGuiCol_Button;
	if (pressed)		col = ImGuiCol_ButtonActive;
	else if (hovered)	col = ImGuiCol_ButtonHovered;

	window->DrawList->AddText(textPos, GetColorU32(col), text);
	RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin);

	if (!enabled) EndDisabled();

	return pressed;
}

void ImGui::ShadeVertsLinearColorGradient(const ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1)
{
	ImVec2 gradient_extent = gradient_p1 - gradient_p0;
	float gradient_inv_length2 = 1.0f / ImLengthSqr(gradient_extent);
	ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
	ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
	const int col0_r = (int)(col0 >> IM_COL32_R_SHIFT) & 0xFF;
	const int col0_g = (int)(col0 >> IM_COL32_G_SHIFT) & 0xFF;
	const int col0_b = (int)(col0 >> IM_COL32_B_SHIFT) & 0xFF;
	const int col0_a = (int)(col0 >> IM_COL32_A_SHIFT) & 0xFF;
	const int col_delta_r = ((int)(col1 >> IM_COL32_R_SHIFT) & 0xFF) - col0_r;
	const int col_delta_g = ((int)(col1 >> IM_COL32_G_SHIFT) & 0xFF) - col0_g;
	const int col_delta_b = ((int)(col1 >> IM_COL32_B_SHIFT) & 0xFF) - col0_b;
	const int col_delta_a = ((int)(col1 >> IM_COL32_A_SHIFT) & 0xFF) - col0_a;
	for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
	{
		float d = ImDot(vert->pos - gradient_p0, gradient_extent);
		float t = ImClamp(d * gradient_inv_length2, 0.0f, 1.0f);
		int r = (int)((float)col0_r + (float)col_delta_r * t);
		int g = (int)((float)col0_g + (float)col_delta_g * t);
		int b = (int)((float)col0_b + (float)col_delta_b * t);
		int a = (int)((float)col0_a + (float)col_delta_a * t);

		// To preserve round borders
		int oldAlpha = (vert->col >> IM_COL32_A_SHIFT) & 0xFF;
		if (oldAlpha == 0)
			a = 0;

		vert->col = IM_COL32(r, g, b, a);
	}
}

void ImGui::RenderFrameGradient(ImRect& bb, ImU32 startColor, ImU32 endColor, ImGuiAxis axis, float bias, float offset)
{
	ImGuiWindow* window = GetCurrentWindow();
	ImGuiStyle& style = GImGui->Style;

	int vStart = window->DrawList->VtxBuffer.Size;
	window->DrawList->AddRectFilled(bb.Min, bb.Max, IM_COL32_WHITE, style.FrameRounding);
	int vEnd = window->DrawList->VtxBuffer.Size;

	ImVec2 p0 = bb.Min;
	ImVec2 p1 = axis == ImGuiAxis_X ? bb.GetTR() : bb.GetBL();
	p1 = ImLerp(p0, p1, bias);

	float offsetDistance = abs(p1.y - p0.y) * offset;

	p0.y += offsetDistance;
	p1.y += offsetDistance;

	ShadeVertsLinearColorGradient(
		window->DrawList, vStart, vEnd, p0, p1, startColor, endColor);
}

void ImGui::RenderRectGradient(ImRect& bb, ImU32 startColor, ImU32 endColor, ImGuiAxis axis, float bias)
{
	ImGuiWindow* window = GetCurrentWindow();
	ImGuiStyle& style = GImGui->Style;

	int vStart = window->DrawList->VtxBuffer.Size;
	window->DrawList->AddRect(bb.Min, bb.Max, IM_COL32_WHITE, style.FrameRounding);
	int vEnd = window->DrawList->VtxBuffer.Size;

	ImVec2& p0 = bb.Min;
	ImVec2 p1 = axis == ImGuiAxis_X ? bb.GetTR() : bb.GetBL();
	p1 = ImLerp(p0, p1, bias);

	ShadeVertsLinearColorGradient(
		window->DrawList, vStart, vEnd, p0, p1,
		startColor, endColor);
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
	SlGui::RenderFrame(window->ClipRect, SlGui::GetColorGradient(SlGuiCol_DockspaceBg));

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
