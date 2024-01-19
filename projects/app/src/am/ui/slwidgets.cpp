#include "slwidgets.h"

#include "extensions.h"
#include "imglue.h"
#include "font_icons/icons_awesome.h"

void SlGui::BeginHorizontal()
{
	ImGui::GetCurrentWindow()->DC.LayoutType = ImGuiLayoutType_Horizontal;
}

void SlGui::EndHorizontal()
{
	ImGui::GetCurrentWindow()->DC.LayoutType = ImGuiLayoutType_Vertical;
}

bool SlGui::BeginToolWindow(ConstString text)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
	bool opened = ImGui::Begin(text, 0, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::PopStyleVar(1); // WindowPadding
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
	BeginHorizontal();
	return opened;
}

void SlGui::EndToolWindow()
{
	EndHorizontal();
	ImGui::PopStyleVar(1); // ItemSpacing
	ImGui::End();
}

bool SlGui::ToggleButton(ConstString text, bool& isActive)
{
	float alpha = GImGui->Style.Alpha * (isActive ? 1.0f : 0.8f);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
	// Make inactive toggle buttons gray, easier to understand
	ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_FrameBg));
	bool pressed = MenuButton(text);
	if (pressed) isActive = !isActive;
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	// Render underline for active button
	if (isActive)
	{
		ImRect& btnRect = GImGui->LastItemData.Rect;
		constexpr float lineHeight = 2.0f;
		ImGui::RenderFrame(btnRect.GetBL() + ImVec2(0, lineHeight), btnRect.GetBR(),
			ImGui::GetColorU32(ImGuiCol_ButtonActive));
	}

	return pressed;
}

bool SlGui::MenuButton(ConstString text)
{
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
	bool pressed = ImGui::Button(text);
	ImGui::PopStyleVar(1);
	return pressed;
}

bool SlGui::IconButton(ConstString text, ImU32 color)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGuiStyle& style = ImGui::GetStyle();

	ImVec2 textSize = ImGui::CalcTextSize(text);
	ImVec2 size = textSize + style.FramePadding * 2.0f;
	ImVec2 pos = window->DC.CursorPos;
	ImRect bb(pos, pos + size);

	ImGuiID id = ImGui::GetID(text);

	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	bool hovered;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, NULL);

	// ImGui handles overlapping extremely retarded way
	if (hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left))
		pressed = true;

	rageam::graphics::ColorU32 finalColor = color;
	if (!hovered)
		finalColor.A = ImMax<u8>(finalColor.A - 40, 0);

	ImVec2 textPos = pos + style.FramePadding;
	window->DrawList->AddText(textPos, finalColor, text);

	return pressed;
}

bool SlGui::Begin(ConstString name, bool* open, ImGuiWindowFlags flags)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1, 1));
	bool opened = ImGui::Begin(name, open, flags);
	ImGui::PopStyleVar();
	return opened;
}

void SlGui::End()
{
	ImGui::End();
}

bool SlGui::TreeNode(ConstString text, bool& selected, bool& toggled, ImTextureID icon, ImGuiTreeNodeFlags flags)
{
	toggled = false;

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	constexpr float arrowSizeX = 18.0f; // Eyeballed
	constexpr float iconSize = 16.0f;

	ImGuiStorage* storage = window->DC.StateStorage;
	ImGuiContext* context = GImGui;
	ImGuiStyle& style = context->Style;
	ImGuiID id = window->GetID(text);

	bool& isOpen = *storage->GetBoolRef(id, (flags & SlGuiTreeNodeFlags_DefaultOpen) ? 1 : 0);

	ImVec2 cursor = window->DC.CursorPos;

	// Stretch horizontally on whole window
	ImVec2 frameSize(window->WorkRect.GetWidth(), iconSize + style.FramePadding.y);
	ImVec2 frameMin(window->WorkRect.Min.x, cursor.y);
	ImVec2 frameMax(frameMin.x + frameSize.x, frameMin.y + frameSize.y);
	ImRect bb(frameMin, frameMax);

	ImGui::ItemSize(frameSize, 0.0f);
	if (!ImGui::ItemAdd(bb, id))
	{
		if (isOpen)
			ImGui::TreePushOverrideID(id);

		return isOpen;
	}

	ImVec2 textSize = ImGui::CalcTextSize(text);
	float centerTextY = IM_GET_CENTER(frameMin.y, frameSize.y, textSize.y);

	// Based on whether we hover arrow / frame we select corresponding bounding box for button

	ImVec2 arrowMin(cursor.x + style.FramePadding.x, frameMin.y);
	ImVec2 arrowMax(arrowMin.x + arrowSizeX, frameMax.y);
	ImRect arrowBb(arrowMin, arrowMax);

	ImVec2 iconMin(arrowMax.x, arrowMin.y);
	ImVec2 iconMax(arrowMax.x, arrowMax.y);
	if (icon != nullptr)
	{
		iconMin = ImVec2(arrowMax.x, IM_GET_CENTER(frameMin.y, frameSize.y, iconSize));
		iconMax = ImVec2(iconMin.x + iconSize, iconMin.y + iconSize);
	}

	ImVec2 textMin(iconMax.x + style.FramePadding.x, frameMin.y);
	ImVec2 textMax(textMin.x + textSize.x, frameMax.y);

	ImVec2 arrowPos(arrowMin.x, centerTextY);
	ImVec2 textPos(textMin.x, centerTextY);

	bool hoversArrow = ImGui::IsMouseHoveringRect(arrowMin, arrowMax);

	ImGuiButtonFlags buttonFlags = ImGuiButtonFlags_MouseButtonLeft;
	if (flags & SlGuiTreeNodeFlags_RightClickSelect)
		buttonFlags |= ImGuiButtonFlags_MouseButtonRight;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(hoversArrow ? arrowBb : bb, id, &hovered, &held, buttonFlags);

	if (flags & SlGuiTreeNodeFlags_DisplayAsHovered)
		hovered = true;

	if (!hoversArrow && pressed)
		selected = true;

	bool textHovered = ImGui::ItemHoverable(ImRect(textMin, textMax), id, 0);
	if (textHovered)
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

	bool canOpen = !(flags & SlGuiTreeNodeFlags_NoChildren);
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
			ImGui::NavMoveRequestCancel();
		}

		// Arrow left closes node
		if (!isOpen && context->NavId == id && context->NavMoveDir == ImGuiDir_Right)
		{
			toggled = true;
			ImGui::NavMoveRequestCancel();
		}

		if (toggled)
		{
			isOpen = !isOpen;
			context->LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
		}
	}

	// Render
	{
		// Shrink it a little to create spacing between items
		// We do that after collision pass so you can't click in-between
		bb.Expand(ImVec2(-1, -1));

		//const SlGuiCol backgroundCol = selected || held && hovered ? SlGuiCol_NodePressed : hovered ? SlGuiCol_NodeHovered : SlGuiCol_Node;
		//const SlGuiCol borderCol = selected ? SlGuiCol_NodeBorderHighlight : SlGuiCol_None;

		//ImGui::PushOverrideID(id);
		///*SlGradient background = GetColorAnimated("NodeBG", backgroundCol);
		//SlGradient border = GetColorAnimated("NodeBorder", borderCol);*/
		//SlGradient background = GetColorGradient(backgroundCol);
		//SlGradient border = GetColorGradient(borderCol);
		//ImGui::PopID();

		//ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

		//// Shadow, Background, Gloss, Border
		//RenderFrame(bb, background);
		//RenderBorder(bb, border);
		//if (selected || hovered) RenderGloss(bb, SlGuiCol_GlossBg);
		//ImGui::PopStyleVar();
		__debugbreak();

	}
	ImGui::RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin);

	// Arrow, we add slow fading in/out just like in windows explorer
	if (canOpen)
	{
		bool arrowVisible = context->HoveredWindow == window || ImGui::IsWindowFocused();

		float& alpha = *storage->GetFloatRef(id + 1);

		// Fade in fast, fade out slower...
		alpha += ImGui::GetIO().DeltaTime * (arrowVisible ? 4.0f : -2.0f);

		// Make max alpha level a little dimmer for sub-nodes
		float maxAlpha = window->DC.TreeDepth == 0 ? 0.8f : 0.6f;
		alpha = ImClamp(alpha, 0.0f, maxAlpha);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, hoversArrow ? maxAlpha : alpha);
		ImGui::RenderText(arrowPos, isOpen ? ICON_FA_ANGLE_DOWN : ICON_FA_ANGLE_RIGHT);
		ImGui::PopStyleVar(); // Alpha
	}

	window->DrawList->AddImage(icon, iconMin, iconMax);

	ImGui::PushFont(ImFont_Medium);
	ImGui::RenderText(textPos, text);
	ImGui::PopFont();

	if (isOpen)
		ImGui::TreePushOverrideID(id);
	return isOpen;
}

bool SlGui::BeginTable(ConstString text, int columnCount, ImGuiTableFlags flags)
{
	// Remove huge gap between the columns
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(1, 0));
	bool opened = ImGui::BeginTable(text, columnCount, flags);
	ImGui::PopStyleVar(); // CellPadding
	return opened;
}

void SlGui::EndTable()
{
	ImGui::EndTable();
}

bool SlGui::Selectable(ConstString text, bool selected, ImTextureID icon, ImVec2 iconSize, float iconScale)
{
	SlRenamingSelectableState state{};
	state.TextDisplay = text;
	state.Icon = icon;
	state.IconWidth = iconSize.x;
	state.IconHeight = iconSize.y;
	state.IconScale = iconScale;
	state.Selected = selected;
	return RenamingSelectable(state, 0);
}

bool SlGui::RenamingSelectable(SlRenamingSelectableState& state, SlGuiRenamingSelectableFlags flags)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiStorage* storage = window->DC.StateStorage;
	ImGuiContext* context = GImGui;
	ImGuiStyle& imStyle = ImGui::GetStyle();

	bool isFocused = ImGui::IsWindowFocused();

	bool allowRename = isFocused && !(flags & ImGuiRenamingSelectableFlags_NoRename);

	state.WasRenaming = state.Renaming;

	ImGuiID id = window->GetID(ImGui::FormatTemp("%s%s", state.TextDisplay, state.TextEditable));

	float iconSize = ImGui::GetFontSize() * state.IconScale;
	if (state.IconWidth < 0.0f) state.IconWidth = iconSize;
	if (state.IconHeight < 0.0f) state.IconHeight = iconSize;
	// Calculate icon dimensions to fit it
	float iconScale = iconSize / ImMax(state.IconWidth, state.IconHeight);
	float iconWidth = state.IconWidth * iconScale;
	float iconHeight = state.IconHeight * iconScale;

	float height = ImMax(ImGui::GetFrameHeight(), iconSize + imStyle.FramePadding.y);

	// Compute size & bounding box of control
	// Button will stretch to window size so we can click on it anywhere
	// but name input won't, so we can use it in table too

	ImVec2 buttonSize;
	buttonSize.x = state.SpanAllColumns ? window->ParentWorkRect.GetWidth() : window->WorkRect.GetWidth();
	buttonSize.y = height;

	ImVec2 inputSize;
	inputSize.x = window->WorkRect.GetWidth();
	inputSize.y = height;

	ImVec2 pos = ImGui::GetCursorScreenPos();

	float minX = pos.x;
	float minY = pos.y;
	float maxX = pos.x + buttonSize.x;
	float maxY = pos.y + buttonSize.y;

	ImVec2 min = ImVec2(minX, minY);
	ImVec2 max = ImVec2(maxX, maxY);
	ImRect bb(min, max);

	ImGui::BeginGroup();

	ImGui::ItemSize(buttonSize);
	if (!ImGui::ItemAdd(bb, id))
	{
		ImGui::EndGroup();
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

	ImVec2 iconPadding(state.IconPadding ? imStyle.FramePadding.x : 1, 0);

	ImVec2 iconMin(
		IM_GET_CENTER(min.x, iconSize, iconWidth),
		IM_GET_CENTER(min.y, buttonSize.y, iconHeight));
	ImVec2 iconMax(iconMin.x + iconWidth, iconMin.y + iconHeight);
	iconMin += iconPadding;
	iconMax += iconPadding;

	bool renamingActive = allowRename && state.Renaming;

	// Position emitting cursor right after icon, for inputBox we don't add padding because it's already included there 
	if (renamingActive)
		ImGui::SetCursorScreenPos(ImVec2(min.x + iconSize, min.y) + iconPadding);
	else
		ImGui::SetCursorScreenPos(ImVec2(min.x + iconSize + imStyle.FramePadding.x, min.y) + iconPadding);

	if(state.SpanAllColumns)
		ImGui::ColumnsBeginBackground();

	// Handle button events
	bool pressed = false;
	bool hovered = false;
	ImGuiButtonFlags buttonFlags = 0;
	buttonFlags |= ImGuiButtonFlags_PressedOnRelease;
	buttonFlags |= ImGuiButtonFlags_AllowOverlap;
	buttonFlags |= ImGuiButtonFlags_MouseButtonLeft;
	if (flags & SlGuiRenamingSelectableFlags_RightClickSelect)
		buttonFlags |= ImGuiButtonFlags_MouseButtonRight;

	bool disabled = flags & ImGuiRenamingSelectableFlags_Disabled;
	if (!disabled && !state.Renaming)
		pressed = ImGui::ButtonBehavior(bb, id, &hovered, 0, buttonFlags);

	// Render button frame and text
	{
		// Shrink it a little to create spacing between items
		// We do that after collision pass, so you can't click in-between, just visual
		bb.Max.y -= 1;

		const ImU32 backgroundCol = ImGui::GetColorU32(
			state.Selected ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
		const ImU32 borderCol = state.Selected || hovered ? ImGui::GetColorU32(ImGuiCol_Border) : 0;

		ImDrawList* dl = window->DrawList;
		dl->AddRectFilled(bb.Min, bb.Max, backgroundCol);
		dl->AddRect(bb.Min, bb.Max, borderCol);
	}

	if(state.SpanAllColumns)
		ImGui::ColumnsEndBackground();

	window->DrawList->AddImage(state.IconBg, iconMin, iconMax, { 0.0f, 0.0f }, state.IconBgUV2);
	window->DrawList->AddImage(state.Icon, iconMin, iconMax, { 0.0f, 0.0f }, state.IconUV2);
	if (state.IconBorder)
		window->DrawList->AddRect(iconMin, iconMax, ImGui::GetColorU32(ImGuiCol_Border));

	// In 'Renaming' state we display input field, in regular state we display just text
	if (renamingActive)
	{
		ConstString inputId = ImGui::FormatTemp("##BTI_ID_%u", id);

		ImGui::SetNextItemWidth(inputSize.x);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::InputText(inputId, state.Buffer, state.BufferSize);
		ImGui::PopStyleVar();

		// Input just appeared, we want to move focus on it
		if (!wasRenamming)
			ImGui::SetKeyboardFocusHere(-1);
	}
	else
	{
		// We can open entry only when we are not in rename state
		if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			state.DoubleClicked = true;

		if (pressed)
		{
			// Update NavId when clicking so navigation can be resumed with gamepad/keyboard
			if (!context->NavDisableMouseHover && context->NavWindow == window && context->NavLayer == window->DC.NavLayerCurrent)
			{
				ImGui::SetNavID(id, window->DC.NavLayerCurrent, context->CurrentFocusScopeId, ImGui::WindowRectAbsToRel(window, bb));
				context->NavDisableHighlight = true;
			}

			ImGui::MarkItemEdited(id);
		}
		ImGui::SetNextItemAllowOverlap();

		// So out stretched button doesn't get clipped
		if(state.SpanAllColumns)
			ImGui::ColumnsBeginBackground();

		// We don't need navigation highlight because we consider navigation as selection
		// RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);

		if (flags & ImGuiRenamingSelectableFlags_Outline)
		{
			window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_Border));
		}

		// Now we need to disable that to render text
		if(state.SpanAllColumns)
			ImGui::ColumnsEndBackground();

		if(!state.ScrollingText)
		{
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", state.TextDisplay);
		}
		else
		{
			// Dummy...
			ImGui::AlignTextToFramePadding();
			ImVec2 textPos = window->DC.CursorPos;
			ImGui::Text("");

			ImGui::ScrollingLabel(textPos, bb, state.TextDisplay);
		}
	}

	if (allowRename)
	{
		// Clicking outside or pressing enter will save changes
		bool enterPressed = ImGui::IsKeyPressed(ImGuiKey_Enter);
		bool mouseClickedOutside = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemHovered();
		if (enterPressed || mouseClickedOutside)
		{
			state.Renaming = false;
			state.AcceptRenaming = true;
		}

		// Escape will exit editing and discard changes
		bool escPressed = ImGui::IsKeyPressed(ImGuiKey_Escape);
		if (escPressed)
		{
			state.Renaming = false;
			state.AcceptRenaming = false;
		}

		// Update saved state of 'Renaming' if it was changed outside / inside scope of this function
		storage->SetBool(id, state.Renaming);

		// Enable editing if F2 was pressed
		bool canBeginEdit = !state.Renaming && state.Selected;
		if (canBeginEdit && ImGui::IsKeyPressed(ImGuiKey_F2, false))
		{
			state.Renaming = true;
		}
	}

	ImGui::EndGroup();

	// Select entry if navigation was used
	if (context->NavJustMovedToId == id)
		return true; // 'Pressed'

	return pressed;
}

SlPickerState SlGui::InputPicker(const char* name, const char* text, int hintCount, const SlHintGetFn& hintFn, const SlHintCompareFn& compareFn)
{
	struct slPickerInstanceState
	{
		// - Easier way to understand those indices:
		// 0: Apple
		// 1: Orange
		// 2: Papaya
		// 2: > Potato
		// 3: Cornbread
		// - Search result for "p":
		// 0: Papaya
		// 1: > Potato
		// - Potato is selected,	
		// hintIndex = 3
		// hintIndexSearch = 1
		int	HintIndex;
		int	HintIndexSearch;

		// Max hint index within search result array (see comment above, it's gonna be 1)
		int	SearchResultCount;

		int	LastSearchHash;

		// We open hints overlay when input fields gets focused then close it if user clicked
		// outside of hints and input field
		bool HintsOpened;

		bool SearchChangedSinceFocus;
	};

	SlPickerState outState{};

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGuiStorage* storage = window->DC.StateStorage;

	ImGuiID id = ImGui::GetID(name);
	ImGui::PushOverrideID(id);

	slPickerInstanceState state;
	state.HintIndex = storage->GetInt(id + 0);
	state.HintIndexSearch = storage->GetInt(id + 1);
	state.SearchResultCount = storage->GetInt(id + 2);
	state.LastSearchHash = storage->GetInt(id + 3);
	state.HintsOpened = storage->GetBool(id + 4);
	state.SearchChangedSinceFocus = storage->GetBool(id + 5);

	static char buffer[256]{};

	// This is pretty dangerous
	char* displayBuffer = state.HintsOpened ? buffer : (char*)text;
	bool textChanged = ImGui::InputText(name, displayBuffer, IM_ARRAYSIZE(buffer), ImGuiInputTextFlags_AutoSelectAll);

	outState.Search = buffer;
	outState.SearchChanged = textChanged;

	bool inputFocused = ImGui::IsItemActive();
	bool inputJustActivated = ImGui::IsItemActivated();
	// User just clicked on input field, show hints to him
	if (inputJustActivated)
	{
		state.HintsOpened = true;
		state.SearchChangedSinceFocus = false;

		String::Copy(buffer, IM_ARRAYSIZE(buffer), text);
	}

	// We have to reset highlighted index if search request was changed
	int lastSearchHash = state.LastSearchHash;
	int	searchHash = (int)rage::atStringHash(buffer);
	bool searchChanged = lastSearchHash != searchHash;
	state.LastSearchHash = searchHash;
	if (!inputJustActivated && searchChanged)
	{
		state.SearchChangedSinceFocus = true;
		state.HintIndex = 0;
		state.HintIndexSearch = 0;
	}

	bool navigatedHint = false;
	bool hintsHovered = false;
	if (state.HintsOpened)
	{
		// Navigate through search results manually because hovered window prevents that
		if (IsKeyDownDelayed(ImGuiKey_DownArrow))
		{
			navigatedHint = true;
			state.HintIndexSearch++;
		}
		if (IsKeyDownDelayed(ImGuiKey_UpArrow))
		{
			navigatedHint = true;
			state.HintIndexSearch--;
		}

		if (navigatedHint)
		{
			// Wrap highlighted index instead of clamping
			if (state.SearchResultCount == 0)
			{
				// Prevent division by zero
				state.HintIndexSearch = 0;
			}
			else
			{
				state.HintIndexSearch = state.HintIndexSearch % state.SearchResultCount;
				if (state.HintIndexSearch < 0)
					state.HintIndexSearch += state.SearchResultCount;
			}
		}

		// Prevent our hint navigation to mess up ImGui navigation
		ImGui::NavMoveRequestCancel();

		// Position hints right below input field and stretch it to get the same width
		constexpr float hintsHeight = 200.0f; // We have to force window height because otherwise scroll won't work
		ImRect& inputRect = GImGui->LastItemData.Rect;
		ImGui::SetNextWindowPos(inputRect.GetBL(), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(inputRect.GetWidth(), hintsHeight), ImGuiCond_Always);

		ImGuiWindowFlags hintWindowFlags =
			ImGuiWindowFlags_Tooltip |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoFocusOnAppearing;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		if (ImGui::Begin("##InputText_HintWindow", 0, hintWindowFlags))
		{
			ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

			// Hints window is always on top so we can simply do cheap rect check
			ImGuiWindow* hintsWindow = ImGui::GetCurrentWindow();
			hintsHovered = hintsWindow->OuterRectClipped.Contains(GImGui->IO.MousePos);

			// Index within search results array
			int indexSearch = 0;

			ConstString hint;
			ImTextureID icon = 0;
			ImVec2 iconSize = ImVec2(-1, -1);
			float iconScale = 1;
			for (int i = 0; i < hintCount; i++)
			{
				hintFn(i, &hint, &icon, &iconSize, &iconScale);

				// We allow user to see all hints when he focuses text box even if there's
				// search text filled in already, this helps to pick new hint easier without
				// removing existing search text
				if (state.SearchChangedSinceFocus)
				{
					// Check if hint name matches search request:
					// Empty search request will match any hint, then we check if hint starts with request phrase
					if (!String::IsNullOrEmpty(buffer))
					{
						// Default fallback to starts with if no comparison function was set
						if (compareFn)
						{
							if (!compareFn(buffer, hint))
								continue;
						}
						else
						{
							if (!ImmutableString(hint).StartsWith(buffer, true))
								continue;
						}
					}
				}
				bool hintSelected = state.HintIndexSearch == indexSearch;

				// We allow to accept selected hint using enter
				bool pressedEnter = ImGui::IsKeyPressed(ImGuiKey_Enter, false) && state.HintIndexSearch == indexSearch;
				bool clickedHint = Selectable(hint, hintSelected, icon, iconSize, iconScale);

				// Since we do item selecting manually we have to maintain scroll too
				if (navigatedHint && hintSelected)
					ImGui::ScrollToItem();

				// Accept hint
				if (pressedEnter || clickedHint)
				{
					state.HintIndexSearch = indexSearch;
					state.HintIndex = i;
					state.HintsOpened = false;
					outState.HintAccepted = true;
				}
				indexSearch++;
			}
			state.SearchResultCount = indexSearch;

			ImGui::PopStyleVar(); // ItemSpacing
		}
		ImGui::PopStyleVar(); // WindowPadding
		ImGui::End(); // InputText_HintWindow
	}

	// We clicked somewhere outside input and hints area, hide hints
	bool clickedOutside = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !hintsHovered;
	if (!inputFocused && clickedOutside)
	{
		state.HintsOpened = false;
	}

	outState.NeedHints = state.HintsOpened;
	outState.HintIndex = state.HintIndex;

	storage->SetInt(id + 0, state.HintIndex);
	storage->SetInt(id + 1, state.HintIndexSearch);
	storage->SetInt(id + 2, state.SearchResultCount);
	storage->SetInt(id + 3, state.LastSearchHash);
	storage->SetBool(id + 4, state.HintsOpened);
	storage->SetBool(id + 5, state.SearchChangedSinceFocus);

	ImGui::PopID();

	return outState;
}

bool SlGui::IsKeyDownDelayed(ImGuiKey key, float delay)
{
	ImGuiKeyData* data = ImGui::GetKeyData(key);
	return ImGui::IsKeyPressed(key, data->DownDuration > delay);
}

void SlGui::TableHeadersRow()
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGuiStyle& style = ImGui::GetStyle();
	ImGuiContext& g = *GImGui;
	ImGuiTable* table = g.CurrentTable;
	IM_ASSERT(table != NULL && "Need to call TableHeadersRow() after BeginTable()!");

	// Layout if not already done (this is automatically done by TableNextRow, we do it here solely to facilitate stepping in debugger as it is frequent to step in TableUpdateLayout)
	if (!table->IsLayoutLocked)
		ImGui::TableUpdateLayout(table);

	// Open row
	const float row_y1 = ImGui::GetCursorScreenPos().y;

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10, 2));
	const float row_height = ImGui::TableGetHeaderRowHeight();
	ImGui::PopStyleVar();

	ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, 0);
	ImGui::TableNextRow(ImGuiTableRowFlags_Headers, row_height);
	ImGui::PopStyleColor();
	if (table->HostSkipItems) // Merely an optimization, you may skip in your own code.
		return;

	const int columns_count = ImGui::TableGetColumnCount();
	for (int column_n = 0; column_n < columns_count; column_n++)
	{
		if (!ImGui::TableSetColumnIndex(column_n))
			continue;

		// FIXME: TableHeader ignores cell padding for first item
		if (column_n == 0)
		{
			window->DC.CursorPos.x += style.CellPadding.x;
		}

		// Push an id to allow unnamed labels (generally accidental, but let's behave nicely with them)
		// In your own code you may omit the PushID/PopID all-together, provided you know they won't collide.
		const char* name = (ImGui::TableGetColumnFlags(column_n) & ImGuiTableColumnFlags_NoHeaderLabel) ? "" : ImGui::TableGetColumnName(column_n);

		// Make sure text is centered for any height
		window->DC.CursorPos.y = IM_GET_CENTER(window->DC.CursorPos.y, row_height, ImGui::CalcTextSize(name).y);

		ImGui::PushID(column_n);
		TableHeader(name);
		ImGui::PopID();
	}

	// Allow opening popup from the right-most section after the last column.
	ImVec2 mouse_pos = ImGui::GetMousePos();
	if (ImGui::IsMouseReleased(1) && ImGui::TableGetHoveredColumn() == columns_count)
		if (mouse_pos.y >= row_y1 && mouse_pos.y < row_y1 + row_height)
			ImGui::TableOpenContextMenu(-1); // Will open a non-column-specific popup.
}

void SlGui::TableHeader(const char* label)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	if (window->SkipItems)
		return;

	ImGuiTable* table = g.CurrentTable;
	IM_ASSERT(table != NULL && "Need to call TableHeader() after BeginTable()!");
	IM_ASSERT(table->CurrentColumn != -1);
	const int column_n = table->CurrentColumn;
	ImGuiTableColumn* column = &table->Columns[column_n];

	// Label
	if (label == NULL)
		label = "";
	const char* label_end = ImGui::FindRenderedTextEnd(label);
	ImVec2 label_size = ImGui::CalcTextSize(label, label_end, true);
	ImVec2 label_pos = window->DC.CursorPos;

	// If we already got a row height, there's use that.
	// FIXME-TABLE: Padding problem if the correct outer-padding CellBgRect strays off our ClipRect?
	ImRect cell_r = ImGui::TableGetCellBgRect(table, column_n);
	float label_height = ImMax(label_size.y, table->RowMinHeight - table->RowCellPaddingY/*CellPaddingY*/ * 2.0f);

	// Calculate ideal size for sort order arrow
	float w_arrow = 0.0f;
	float w_sort_text = 0.0f;
	char sort_order_suf[4] = "";
	const float ARROW_SCALE = 0.65f;
	if ((table->Flags & ImGuiTableFlags_Sortable) && !(column->Flags & ImGuiTableColumnFlags_NoSort))
	{
		w_arrow = ImFloor(g.FontSize * ARROW_SCALE + g.Style.FramePadding.x);
		if (column->SortOrder > 0)
		{
			ImFormatString(sort_order_suf, IM_ARRAYSIZE(sort_order_suf), "%d", column->SortOrder + 1);
			w_sort_text = g.Style.ItemInnerSpacing.x + ImGui::CalcTextSize(sort_order_suf).x;
		}
	}

	// We feed our unclipped width to the column without writing on CursorMaxPos, so that column is still considering for merging.
	float max_pos_x = label_pos.x + label_size.x + w_sort_text + w_arrow;
	column->ContentMaxXHeadersUsed = ImMax(column->ContentMaxXHeadersUsed, column->WorkMaxX);
	column->ContentMaxXHeadersIdeal = ImMax(column->ContentMaxXHeadersIdeal, max_pos_x);

	// Keep header highlighted when context menu is open.
	const bool selected = (table->IsContextPopupOpen && table->ContextPopupColumn == column_n && table->InstanceInteracted == table->InstanceCurrent);
	ImGuiID id = window->GetID(label);
	ImRect bb(cell_r.Min.x, cell_r.Min.y, cell_r.Max.x, ImMax(cell_r.Max.y, cell_r.Min.y + label_height + g.Style.CellPadding.y * 2.0f));
	ImGui::ItemSize(ImVec2(0.0f, label_height)); // Don't declare unclipped width, it'll be fed ContentMaxPosHeadersIdeal
	if (!ImGui::ItemAdd(bb, id))
		return;

	//GetForegroundDrawList()->AddRect(cell_r.Min, cell_r.Max, IM_COL32(255, 0, 0, 255)); // [DEBUG]
	//GetForegroundDrawList()->AddRect(bb.Min, bb.Max, IM_COL32(255, 0, 0, 255)); // [DEBUG]

	// Using AllowItemOverlap mode because we cover the whole cell, and we want user to be able to submit subsequent items.
	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_AllowOverlap);
	if (g.ActiveId != id)
		ImGui::SetNextItemAllowOverlap();
	//if (held || hovered || selected)
	//{
	//	const ImU32 col = ImGui::GetColorU32(held ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
	//	//RenderFrame(bb.Min, bb.Max, col, false, 0.0f);
	//	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, col, table->CurrentColumn);
	//}
	//else
	//{
	//	// Submit single cell bg color in the case we didn't submit a full header row
	//	if ((table->RowFlags & ImGuiTableRowFlags_Headers) == 0)
	//		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImGuiCol_TableHeaderBg), table->CurrentColumn);
	//}

	// Render
	{
		//const ImGuiCol backgroundCol = selected || held && hovered ? ImGuiCol_TableHeaderPressed : hovered ? ImGuiCol_TableHeaderHovered : ImGuiCol_TableHeaderBg;
		//const ImGuiCol borderCol = selected ? ImGuiCol_Border : ImGuiCol_None;

		//ImGui::PushOverrideID(id);
		//SlGradient background = GetColorGradient(backgroundCol);
		//ImGui::PopID();

		//ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

		//// Shadow, Background, Gloss, Border
		//RenderFrame(bb, background);
		//RenderBorder(bb, border);
		//if (selected || hovered) RenderGloss(bb, SlGuiCol_GlossBg);

		//ImGui::PopStyleVar();

		__debugbreak();
	}

	ImGui::RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);
	if (held)
		table->HeldHeaderColumn = (ImGuiTableColumnIdx)column_n;
	window->DC.CursorPos.y -= g.Style.ItemSpacing.y * 0.5f;

	// Drag and drop to re-order columns.
	// FIXME-TABLE: Scroll request while reordering a column and it lands out of the scrolling zone.
	if (held && (table->Flags & ImGuiTableFlags_Reorderable) && ImGui::IsMouseDragging(0) && !g.DragDropActive)
	{
		// While moving a column it will jump on the other side of the mouse, so we also test for MouseDelta.x
		table->ReorderColumn = (ImGuiTableColumnIdx)column_n;
		table->InstanceInteracted = table->InstanceCurrent;

		// We don't reorder: through the frozen<>unfrozen line, or through a column that is marked with ImGuiTableColumnFlags_NoReorder.
		if (g.IO.MouseDelta.x < 0.0f && g.IO.MousePos.x < cell_r.Min.x)
			if (ImGuiTableColumn* prev_column = (column->PrevEnabledColumn != -1) ? &table->Columns[column->PrevEnabledColumn] : NULL)
				if (!((column->Flags | prev_column->Flags) & ImGuiTableColumnFlags_NoReorder))
					if ((column->IndexWithinEnabledSet < table->FreezeColumnsRequest) == (prev_column->IndexWithinEnabledSet < table->FreezeColumnsRequest))
						table->ReorderColumnDir = -1;
		if (g.IO.MouseDelta.x > 0.0f && g.IO.MousePos.x > cell_r.Max.x)
			if (ImGuiTableColumn* next_column = (column->NextEnabledColumn != -1) ? &table->Columns[column->NextEnabledColumn] : NULL)
				if (!((column->Flags | next_column->Flags) & ImGuiTableColumnFlags_NoReorder))
					if ((column->IndexWithinEnabledSet < table->FreezeColumnsRequest) == (next_column->IndexWithinEnabledSet < table->FreezeColumnsRequest))
						table->ReorderColumnDir = +1;
	}

	// Sort order arrow
	const float ellipsis_max = cell_r.Max.x - w_arrow - w_sort_text;
	if ((table->Flags & ImGuiTableFlags_Sortable) && !(column->Flags & ImGuiTableColumnFlags_NoSort))
	{
		if (column->SortOrder != -1)
		{
			//float x = ImMax(cell_r.Min.x, cell_r.Max.x - w_arrow - w_sort_text);
			float x = IM_GET_CENTER(cell_r.Min.x, cell_r.GetWidth(), w_arrow - w_sort_text);
			float y = cell_r.Min.y - 2; // Hardcoded offset to align arrow

			if (column->SortOrder > 0)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_Text, 0.70f));
				ImGui::RenderText(ImVec2(x + g.Style.ItemInnerSpacing.x, y), sort_order_suf);
				ImGui::PopStyleColor();
				x += w_sort_text;
			}
			// ImGui::RenderArrow(window->DrawList, ImVec2(x, y), ImGui::GetColorU32(ImGuiCol_Text), column->SortDirection == ImGuiSortDirection_Ascending ? ImGuiDir_Up : ImGuiDir_Down, ARROW_SCALE);

			ConstString arrow = column->SortDirection == ImGuiSortDirection_Ascending ? ICON_FA_ANGLE_DOWN : ICON_FA_ANGLE_UP;

			window->DrawList->AddText(
				g.Font, 12, ImVec2(x, y), ImGui::GetColorU32(ImGuiCol_Text), arrow, arrow + strlen(arrow));
		}

		// Handle clicking on column header to adjust Sort Order
		if (pressed && table->ReorderColumn != column_n)
		{
			ImGuiSortDirection sort_direction = ImGui::TableGetColumnNextSortDirection(column);
			ImGui::TableSetColumnSortDirection(column_n, sort_direction, g.IO.KeyShift);
		}
	}

	// Render clipped label. Clipping here ensure that in the majority of situations, all our header cells will
	// be merged into a single draw call.
	//window->DrawList->AddCircleFilled(ImVec2(ellipsis_max, label_pos.y), 40, IM_COL32_WHITE);
	ImGui::RenderTextEllipsis(window->DrawList, label_pos, ImVec2(ellipsis_max, label_pos.y + label_height + g.Style.FramePadding.y), ellipsis_max, ellipsis_max, label, label_end, &label_size);

	const bool text_clipped = label_size.x > (ellipsis_max - label_pos.x);
	if (text_clipped && hovered && g.ActiveId == 0 && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
		ImGui::SetTooltip("%.*s", (int)(label_end - label), label);

	// We don't use BeginPopupContextItem() because we want the popup to stay up even after the column is hidden
	if (ImGui::IsMouseReleased(1) && ImGui::IsItemHovered())
		ImGui::TableOpenContextMenu(column_n);
}

void SlGui::CategoryText(const char* text)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiID id = ImGui::GetID(text);

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 textSize = ImGui::CalcTextSize(text);

	ImVec2 min = pos;
	ImVec2 max = ImVec2(window->WorkRect.Max.x, pos.y + ImGui::GetFrameHeight());
	ImRect rect(min, max);

	ImGui::ItemSize(rect);
	if (!ImGui::ItemAdd(rect, id))
		return;

	ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);

	// Draw text
	ImVec2 textPos = ImVec2(min.x, IM_GET_CENTER(min.y, rect.GetHeight(), textSize.y));
	window->DrawList->AddText(textPos, col, text);

	// Draw line
	constexpr float lineWidth = 1.0f;
	ImVec2 lineMin = ImVec2(
		min.x + textSize.x + ImGui::GetFrameHeight() * 0.25f,	// Right after text ends with small padding
		min.y + (max.y - min.y) / 2.0f);						// Centered Y
	ImVec2 lineMax = ImVec2(max.x, lineMin.y + lineWidth);
	ImRect lineRect(lineMin, lineMax);
	ImGui::RenderFrame(lineRect, ImGradient(ImVec4(1, 1, 1, 0.55f), ImVec4(1, 1, 1, 0.1f)), 1, 0, ImGuiAxis_X);
}

void SlGui::ColorEditGradient(ConstString name, ImGradient& gradient, ImGuiColorEditFlags flags)
{
	float* start = reinterpret_cast<float*>(&gradient.Start);
	float* end = reinterpret_cast<float*>(&gradient.End);

	ImGui::ColorEdit4(ImGui::FormatTemp("%s-Start", name), start, flags);
	ImGui::ColorEdit4(ImGui::FormatTemp("%s-End", name), end, flags);
}

void SlGui::NodeDragBehaviour(SlGuiNodeDragPosition& outPosition, SlGuiNodeDragFlags flags)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImRect& nodeRect = GImGui->LastItemData.Rect;

	SlGuiNodeDragPosition dragType;

	if (flags & SlGuiNodeDragFlags_AllowAboveAndBelow)
	{
		// Calculate normalized offset within node rect (0.0 / 1.0)
		ImVec2 mousePos = ImGui::GetMousePos();
		float mouseOffset = (mousePos.y - nodeRect.Min.y) / nodeRect.GetHeight();

		if (mouseOffset < 0.25f)
			dragType = SlGuiNodeDragPosition_Above;
		else if (mouseOffset < 0.75f)
			dragType = SlGuiNodeDragPosition_Center;
		else
			dragType = SlGuiNodeDragPosition_Below;
	}
	else
	{
		dragType = SlGuiNodeDragPosition_Center;
	}
	outPosition = dragType;

	// By how much pixels grow up & down from rect edge for 'Above' / 'Below' lines
	constexpr float lineExtent = 1;

	// Draw corresponding shapes
	ImVec2 lineMin, lineMax;
	switch (dragType)
	{
	case SlGuiNodeDragPosition_Above:
		lineMin = ImVec2(nodeRect.Min.x, nodeRect.Min.y - lineExtent);
		lineMax = ImVec2(nodeRect.Max.x, nodeRect.Min.y + lineExtent);
		break;
	case SlGuiNodeDragPosition_Center:
		lineMin = nodeRect.Min;
		lineMax = nodeRect.Max;
		break;
	case SlGuiNodeDragPosition_Below:
		lineMin = ImVec2(nodeRect.Min.x, nodeRect.Max.y - lineExtent);
		lineMax = ImVec2(nodeRect.Max.x, nodeRect.Max.y + lineExtent);
		break;
	}
	window->DrawList->AddRectFilled(lineMin, lineMax, IM_COL32(26, 159, 255, 120));
}

bool SlGui::CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	return TreeNodeBehavior(window->GetID(label), flags | ImGuiTreeNodeFlags_CollapsingHeader, label, label + strlen(label));
}

bool SlGui::TreeNodeBehavior(ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const bool display_frame = (flags & ImGuiTreeNodeFlags_Framed) != 0;
	const ImVec2 padding = (display_frame || (flags & ImGuiTreeNodeFlags_FramePadding)) ? style.FramePadding : ImVec2(style.FramePadding.x, ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));

	if (!label_end)
		label_end = ImGui::FindRenderedTextEnd(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, label_end, false);

	// We vertically grow up to current line height up the typical widget height.
	const float frame_height = ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2), label_size.y + padding.y * 2);
	ImRect frame_bb;
	frame_bb.Min.x = (flags & ImGuiTreeNodeFlags_SpanFullWidth) ? window->WorkRect.Min.x : window->DC.CursorPos.x;
	frame_bb.Min.y = window->DC.CursorPos.y;
	frame_bb.Max.x = window->WorkRect.Max.x;
	frame_bb.Max.y = window->DC.CursorPos.y + frame_height;
	if (display_frame)
	{
		// Framed header expand a little outside the default padding, to the edge of InnerClipRect
		// (FIXME: May remove this at some point and make InnerClipRect align with WindowPadding.x instead of WindowPadding.x*0.5f)
		frame_bb.Min.x -= IM_TRUNC(window->WindowPadding.x * 0.5f - 1.0f);
		frame_bb.Max.x += IM_TRUNC(window->WindowPadding.x * 0.5f);
	}

	const float text_offset_x = g.FontSize + (display_frame ? padding.x * 2 : padding.x * 1);           // Collapser arrow width + Spacing
	const float text_offset_y = ImMax(padding.y, window->DC.CurrLineTextBaseOffset);                    // Latch before ItemSize changes it
	const float text_width = g.FontSize + (label_size.x > 0.0f ? label_size.x + padding.x * 2 : 0.0f);  // Include collapser
	ImVec2 text_pos(window->DC.CursorPos.x + text_offset_x, window->DC.CursorPos.y + text_offset_y);
	ImGui::ItemSize(ImVec2(text_width, frame_height), padding.y);

	// For regular tree nodes, we arbitrary allow to click past 2 worth of ItemSpacing
	ImRect interact_bb = frame_bb;
	if (!display_frame && (flags & (ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth)) == 0)
		interact_bb.Max.x = frame_bb.Min.x + text_width + style.ItemSpacing.x * 2.0f;

	// Store a flag for the current depth to tell if we will allow closing this node when navigating one of its child.
	// For this purpose we essentially compare if g.NavIdIsAlive went from 0 to 1 between TreeNode() and TreePop().
	// This is currently only support 32 level deep and we are fine with (1 << Depth) overflowing into a zero.
	const bool is_leaf = (flags & ImGuiTreeNodeFlags_Leaf) != 0;
	bool is_open = ImGui::TreeNodeUpdateNextOpen(id, flags);
	if (is_open && !g.NavIdIsAlive && (flags & ImGuiTreeNodeFlags_NavLeftJumpsBackHere) && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
		window->DC.TreeJumpToParentOnPopMask |= (1 << window->DC.TreeDepth);

	bool item_add = ImGui::ItemAdd(interact_bb, id);
	g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HasDisplayRect;
	g.LastItemData.DisplayRect = frame_bb;

	if (!item_add)
	{
		if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
			ImGui::TreePushOverrideID(id);
		IMGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label, g.LastItemData.StatusFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
		return is_open;
	}

	ImGuiButtonFlags button_flags = ImGuiTreeNodeFlags_None;
	if (flags & ImGuiTreeNodeFlags_AllowOverlap)
		button_flags |= ImGuiButtonFlags_AllowOverlap;
	if (!is_leaf)
		button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;

	// We allow clicking on the arrow section with keyboard modifiers held, in order to easily
	// allow browsing a tree while preserving selection with code implementing multi-selection patterns.
	// When clicking on the rest of the tree node we always disallow keyboard modifiers.
	const float arrow_hit_x1 = (text_pos.x - text_offset_x) - style.TouchExtraPadding.x;
	const float arrow_hit_x2 = (text_pos.x - text_offset_x) + (g.FontSize + padding.x * 2.0f) + style.TouchExtraPadding.x;
	const bool is_mouse_x_over_arrow = (g.IO.MousePos.x >= arrow_hit_x1 && g.IO.MousePos.x < arrow_hit_x2);
	if (window != g.HoveredWindow || !is_mouse_x_over_arrow)
		button_flags |= ImGuiButtonFlags_NoKeyModifiers;

	// Open behaviors can be altered with the _OpenOnArrow and _OnOnDoubleClick flags.
	// Some alteration have subtle effects (e.g. toggle on MouseUp vs MouseDown events) due to requirements for multi-selection and drag and drop support.
	// - Single-click on label = Toggle on MouseUp (default, when _OpenOnArrow=0)
	// - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=0)
	// - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=1)
	// - Double-click on label = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1)
	// - Double-click on arrow = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1 and _OpenOnArrow=0)
	// It is rather standard that arrow click react on Down rather than Up.
	// We set ImGuiButtonFlags_PressedOnClickRelease on OpenOnDoubleClick because we want the item to be active on the initial MouseDown in order for drag and drop to work.
	if (is_mouse_x_over_arrow)
		button_flags |= ImGuiButtonFlags_PressedOnClick;
	else if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
		button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
	else
		button_flags |= ImGuiButtonFlags_PressedOnClickRelease;

	bool selected = (flags & ImGuiTreeNodeFlags_Selected) != 0;
	const bool was_selected = selected;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);
	bool toggled = false;
	if (!is_leaf)
	{
		if (pressed && g.DragDropHoldJustPressedId != id)
		{
			if ((flags & (ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) == 0 || (g.NavActivateId == id))
				toggled = true;
			if (flags & ImGuiTreeNodeFlags_OpenOnArrow)
				toggled |= is_mouse_x_over_arrow && !g.NavDisableMouseHover; // Lightweight equivalent of IsMouseHoveringRect() since ButtonBehavior() already did the job
			if ((flags & ImGuiTreeNodeFlags_OpenOnDoubleClick) && g.IO.MouseClickedCount[0] == 2)
				toggled = true;
		}
		else if (pressed && g.DragDropHoldJustPressedId == id)
		{
			IM_ASSERT(button_flags & ImGuiButtonFlags_PressedOnDragDropHold);
			if (!is_open) // When using Drag and Drop "hold to open" we keep the node highlighted after opening, but never close it again.
				toggled = true;
		}

		if (g.NavId == id && g.NavMoveDir == ImGuiDir_Left && is_open)
		{
			toggled = true;
			ImGui::NavMoveRequestCancel();
		}
		if (g.NavId == id && g.NavMoveDir == ImGuiDir_Right && !is_open) // If there's something upcoming on the line we may want to give it the priority?
		{
			toggled = true;
			ImGui::NavMoveRequestCancel();
		}

		if (toggled)
		{
			is_open = !is_open;
			window->DC.StateStorage->SetInt(id, is_open);
			g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
		}
	}
	if (flags & ImGuiTreeNodeFlags_AllowOverlap)
		ImGui::SetNextItemAllowOverlap();

	// In this branch, TreeNodeBehavior() cannot toggle the selection so this will never trigger.
	if (selected != was_selected) //-V547
		g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

	// Render
	const ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
	ImGuiNavHighlightFlags nav_highlight_flags = ImGuiNavHighlightFlags_TypeThin;
	if (display_frame)
	{
		// Framed type
		const ImU32 bg_col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
		ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, true, style.FrameRounding);
		ImGui::RenderNavHighlight(frame_bb, id, nav_highlight_flags);
		if (flags & ImGuiTreeNodeFlags_Bullet)
			ImGui::RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.60f, text_pos.y + g.FontSize * 0.5f), text_col);
		else if (!is_leaf)
			ImGui::RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y), text_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 1.0f);
		else // Leaf without bullet, left-adjusted text
			text_pos.x -= text_offset_x;
		if (flags & ImGuiTreeNodeFlags_ClipLabelForTrailingButton)
			frame_bb.Max.x -= g.FontSize + style.FramePadding.x;

		if (g.LogEnabled)
			ImGui::LogSetNextTextDecoration("###", "###");
		ImGui::RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
	}
	else
	{
		// Unframed typed for tree nodes
		if (hovered || selected)
		{
			const ImU32 bg_col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
			ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, false);
		}
		ImGui::RenderNavHighlight(frame_bb, id, nav_highlight_flags);
		if (flags & ImGuiTreeNodeFlags_Bullet)
			ImGui::RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.5f, text_pos.y + g.FontSize * 0.5f), text_col);
		else if (!is_leaf)
			ImGui::RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y + g.FontSize * 0.15f), text_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 0.70f);
		if (g.LogEnabled)
			ImGui::LogSetNextTextDecoration(">", NULL);
		ImGui::RenderText(text_pos, label, label_end, false);
	}

	if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
		ImGui::TreePushOverrideID(id);
	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
	return is_open;
}

bool SlGui::BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.Flags;
	g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
	if (window->SkipItems)
		return false;

	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

	const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : ImGui::GetFrameHeight();
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
	const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : ImGui::CalcItemWidth();
	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
	const ImRect total_bb(bb.Min, bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
	ImGui::ItemSize(total_bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(total_bb, id, &bb))
		return false;

	// Open on click
	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
	const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
	bool popup_open = ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None);
	if (pressed && !popup_open)
	{
		ImGui::OpenPopupEx(popup_id, ImGuiPopupFlags_None);
		popup_open = true;
	}

	// Render shape
	ImGui::RenderShadow(bb);
	const ImU32 frame_col = ImGui::GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
	const float value_x2 = ImMax(bb.Min.x, bb.Max.x /*- arrow_size*/);
	ImGui::RenderNavHighlight(bb, id);
	if (!(flags & ImGuiComboFlags_NoPreview))
		window->DrawList->AddRectFilled(bb.Min, ImVec2(value_x2, bb.Max.y), frame_col, style.FrameRounding, (flags & ImGuiComboFlags_NoArrowButton) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersLeft);
	if (!(flags & ImGuiComboFlags_NoArrowButton))
	{
		ImU32 bg_col = ImGui::GetColorU32((popup_open || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
		ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
		//window->DrawList->AddRectFilled(ImVec2(value_x2, bb.Min.y), bb.Max, bg_col, style.FrameRounding, (w <= arrow_size) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight);
		//if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x)
		ImGui::RenderArrow(window->DrawList,
		                   ImVec2(
			                   bb.Max.x - style.FramePadding.y - ImGui::CalcTextSize("+").x * 1.5f,
			                   bb.Min.y + style.FramePadding.y),
		                   text_col, ImGuiDir_Down, 1.0f);
	}
	ImGui::RenderFrameBorder(bb.Min, bb.Max, style.FrameRounding);

	// Custom preview
	if (flags & ImGuiComboFlags_CustomPreview)
	{
		g.ComboPreviewData.PreviewRect = ImRect(bb.Min.x, bb.Min.y, value_x2, bb.Max.y);
		IM_ASSERT(preview_value == NULL || preview_value[0] == 0);
		preview_value = NULL;
	}

	// Render preview and label
	if (preview_value != NULL && !(flags & ImGuiComboFlags_NoPreview))
	{
		if (g.LogEnabled)
			ImGui::LogSetNextTextDecoration("{", "}");
		ImGui::RenderTextClipped(bb.Min + style.FramePadding, ImVec2(value_x2, bb.Max.y), preview_value, NULL, NULL);
	}
	if (label_size.x > 0)
		ImGui::RenderText(ImVec2(bb.Max.x + style.ItemInnerSpacing.x, bb.Min.y + style.FramePadding.y), label);

	if (!popup_open)
		return false;

	g.NextWindowData.Flags = backup_next_window_data_flags;
	return ImGui::BeginComboPopup(popup_id, bb, flags);
}

static float CalcMaxPopupHeightFromItemCount(int items_count)
{
	ImGuiContext& g = *GImGui;
	if (items_count <= 0)
		return FLT_MAX;
	return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

bool SlGui::Combo(const char* label, int* current_item, bool(*items_getter)(void*, int, const char**), void* data,
	int items_count, int popup_max_height_in_items)
{
	ImGuiContext& g = *GImGui;

	// Call the getter to obtain the preview string which is a parameter to BeginCombo()
	const char* preview_value = NULL;
	if (*current_item >= 0 && *current_item < items_count)
		items_getter(data, *current_item, &preview_value);

	// The old Combo() API exposed "popup_max_height_in_items". The new more general BeginCombo() API doesn't have/need it, but we emulate it here.
	if (popup_max_height_in_items != -1 && !(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
		ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

	if (!BeginCombo(label, preview_value, ImGuiComboFlags_None))
		return false;

	// Display items
	// FIXME-OPT: Use clipper (but we need to disable it on the appearing frame to make sure our call to SetItemDefaultFocus() is processed)
	bool value_changed = false;
	for (int i = 0; i < items_count; i++)
	{
		ImGui::PushID(i);
		const bool item_selected = (i == *current_item);
		const char* item_text;
		if (!items_getter(data, i, &item_text))
			item_text = "*Unknown item*";
		if (ImGui::Selectable(item_text, item_selected))
		{
			value_changed = true;
			*current_item = i;
		}
		if (item_selected)
			ImGui::SetItemDefaultFocus();
		ImGui::PopID();
	}

	ImGui::EndCombo();

	if (value_changed)
		ImGui::MarkItemEdited(g.LastItemData.ID);

	return value_changed;
}

// Getter for the old Combo() API: const char*[]
static bool Items_ArrayGetter(void* data, int idx, const char** out_text)
{
	const char* const* items = (const char* const*)data;
	if (out_text)
		*out_text = items[idx];
	return true;
}

// Getter for the old Combo() API: "item1\0item2\0item3\0"
static bool Items_SingleStringGetter(void* data, int idx, const char** out_text)
{
	// FIXME-OPT: we could pre-compute the indices to fasten this. But only 1 active combo means the waste is limited.
	const char* items_separated_by_zeros = (const char*)data;
	int items_count = 0;
	const char* p = items_separated_by_zeros;
	while (*p)
	{
		if (idx == items_count)
			break;
		p += strlen(p) + 1;
		items_count++;
	}
	if (!*p)
		return false;
	if (out_text)
		*out_text = p;
	return true;
}

bool SlGui::Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
{
	const bool value_changed = Combo(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_in_items);
	return value_changed;
}

bool SlGui::Checkbox(const char* label, bool* v)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	const float square_sz = ImGui::GetFrameHeight();
	const ImVec2 pos = window->DC.CursorPos;
	const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
	ImGui::ItemSize(total_bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(total_bb, id))
	{
		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
		return false;
	}

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
	{
		*v = !(*v);
		ImGui::MarkItemEdited(id);
	}

	const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
	ImGui::RenderShadow(check_bb);
	ImGui::RenderNavHighlight(total_bb, id);
	ImGui::RenderFrame(check_bb.Min, check_bb.Max, ImGui::GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), true, style.FrameRounding);
	ImU32 check_col = ImGui::GetColorU32(ImGuiCol_CheckMark);
	bool mixed_value = (g.LastItemData.InFlags & ImGuiItemFlags_MixedValue) != 0;
	if (mixed_value)
	{
		// Undocumented tristate/mixed/indeterminate checkbox (#2644)
		// This may seem awkwardly designed because the aim is to make ImGuiItemFlags_MixedValue supported by all widgets (not just checkbox)
		ImVec2 pad(ImMax(1.0f, IM_TRUNC(square_sz / 3.6f)), ImMax(1.0f, IM_TRUNC(square_sz / 3.6f)));
		window->DrawList->AddRectFilled(check_bb.Min + pad, check_bb.Max - pad, check_col, style.FrameRounding);
	}
	else if (*v)
	{
		const float pad = ImMax(1.0f, IM_TRUNC(square_sz / 6.0f));
		//ImGui::RenderCheckMark(window->DrawList, check_bb.Min + ImVec2(pad, pad), check_col, square_sz - pad * 2.0f);
		window->DrawList->AddText(check_bb.Min + ImVec2(pad, pad), check_col, ICON_FA_CHECK);
	}

	ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
	if (g.LogEnabled)
		ImGui::LogRenderedText(&label_pos, mixed_value ? "[~]" : *v ? "[x]" : "[ ]");
	if (label_size.x > 0.0f)
		ImGui::RenderText(label_pos, label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
	return pressed;
}

bool SlGui::GraphTreeNode(ConstString text, bool& selected, bool& toggled, SlGuiTreeNodeFlags flags)
{
	toggled = false;

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiStorage* storage = window->DC.StateStorage;
	ImGuiContext* context = GImGui;
	ImGuiStyle& style = context->Style;
	ImGuiID id = window->GetID(text);

	bool& isOpen = *storage->GetBoolRef(id, flags & SlGuiTreeNodeFlags_DefaultOpen);

	ImVec2 cursor = window->DC.CursorPos;

	ImGui::ColumnsBeginBackground();
	// Stretch horizontally on whole window
	ImVec2 frameSize(window->ParentWorkRect.GetWidth(), ImGui::GetFrameHeight());
	ImVec2 frameMin(window->ParentWorkRect.Min.x, cursor.y);
	ImVec2 frameMax(frameMin.x + frameSize.x, frameMin.y + frameSize.y);
	//ImRect bb(frameMin, frameMax);

	// Based on whether we hover arrow / frame we select corresponding bounding box for button
	constexpr float arrowSizeX = 18.0f; // Eyeballed
	ImVec2 arrowMin(cursor.x + style.FramePadding.x, frameMin.y);
	ImVec2 arrowMax(arrowMin.x + arrowSizeX, frameMax.y);
	ImRect arrowBb(arrowMin, arrowMax);

	// Item bounding it set to only arrow to allow us to add more items in node later using ImGui::SameLine
	ImRect itemBb(arrowMin, arrowMax);
	ImRect bb(frameMin, frameMax);

	ImGui::BeginGroup();

	ImGui::ItemSize(itemBb.GetSize(), 0.0f);
	bool added = ImGui::ItemAdd(itemBb, id);
	ImGui::ColumnsEndBackground();
	if (!added)
	{
		ImGui::EndGroup();

		if (isOpen)
			ImGui::TreePushOverrideID(id);

		return isOpen;
	}

	ImVec2 textSize = ImGui::CalcTextSize(text);
	float centerTextY = IM_GET_CENTER(frameMin.y, frameSize.y, textSize.y);

	ImVec2 arrowPos(arrowMin.x, centerTextY);

	bool hoversArrow = ImGui::IsMouseHoveringRect(arrowMin, arrowMax);

	ImGuiButtonFlags buttonFlags = ImGuiButtonFlags_MouseButtonLeft;
	if (flags & SlGuiTreeNodeFlags_RightClickSelect)
		buttonFlags |= ImGuiButtonFlags_MouseButtonRight;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(hoversArrow ? arrowBb : bb, id, &hovered, &held, buttonFlags);
	ImGui::SetNextItemAllowOverlap();

	if (flags & SlGuiTreeNodeFlags_DisplayAsHovered)
		hovered = true;

	if (!hoversArrow && pressed)
		selected = true;

	bool canOpen = !(flags & ImGuiIconTreeNodeFlags_NoChildren);
	if (canOpen)
	{
		// Toggle on simple arrow click
		if (pressed && hoversArrow)
			toggled = true;

		// Toggle on mouse double click
		if (flags & SlGuiTreeNodeFlags_RightClickSelect &&
			hovered &&
			context->IO.MouseClickedCount[ImGuiMouseButton_Left] == 2)
			toggled = true;

		// Arrow right opens node
		if (isOpen && context->NavId == id && context->NavMoveDir == ImGuiDir_Left)
		{
			toggled = true;
			ImGui::NavMoveRequestCancel();
		}

		// Arrow left closes node
		if (!isOpen && context->NavId == id && context->NavMoveDir == ImGuiDir_Right)
		{
			toggled = true;
			ImGui::NavMoveRequestCancel();
		}

		if (toggled)
		{
			isOpen = !isOpen;
			context->LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
		}
	}

	// Render
	{
		// Shrink it a little to create spacing between items
		// We do that after collision pass so you can't click in-between
		//bb.Expand(ImVec2(-1, -1));

		//const SlGuiCol backgroundCol =
		//	selected ? SlGuiCol_GraphNodePressed : hovered ? SlGuiCol_GraphNodeHovered : SlGuiCol_GraphNode;
		//const SlGuiCol borderCol = selected ? SlGuiCol_GraphNodeBorderHighlight : SlGuiCol_None;

		//ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

		//// Shadow, Background, Gloss, Border
		//SlGui::RenderFrame(bb, SlGui::GetColorGradient(backgroundCol));
		//SlGui::RenderBorder(bb, SlGui::GetColorGradient(borderCol));
		//if (selected || hovered) SlGui::RenderGloss(bb, SlGuiCol_GlossBg);
		//ImGui::PopStyleVar();

		__debugbreak();

	}
	ImGui::RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin);

	// Arrow, we add slow fading in/out just like in windows explorer
	if (canOpen)
	{
		bool arrowVisible = 
			(flags & SlGuiTreeNodeFlags_AlwaysShowArrows) || 
			context->HoveredWindow == window || 
			ImGui::IsWindowFocused();

		float& alpha = *storage->GetFloatRef(id + 1);

		// Fade in fast, fade out slower...
		alpha += ImGui::GetIO().DeltaTime * (arrowVisible ? 4.0f : -2.0f);

		// Make max alpha level a little dimmer for sub-nodes
		float maxAlpha = window->DC.TreeDepth == 0 ? 0.8f : 0.6f;
		alpha = ImClamp(alpha, 0.0f, maxAlpha);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, hoversArrow ? maxAlpha : alpha);
		ImGui::RenderText(arrowPos, isOpen ? ICON_FA_ANGLE_DOWN : ICON_FA_ANGLE_RIGHT);
		ImGui::PopStyleVar(); // Alpha
	}

	const char* textDisplayEnd = ImGui::FindRenderedTextEnd(text);
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextEx(text, textDisplayEnd);

	ImGui::EndGroup();

	// We don't pass whole BB to add size to allow user to add controls on the same line
	// but in order to allow correct drawing on top of node we have to update rect manually
	GImGui->LastItemData.Rect = bb;
	GImGui->LastItemData.NavRect = bb;

	if (isOpen)
		ImGui::TreePushOverrideID(id);
	return isOpen;
}
