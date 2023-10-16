#include "folderview.h"

#include "am/asset/types/txd.h"
#include "am/task/undo.h"
#include "am/ui/extensions.h"
#include "am/ui/apps/windowmgr.h"
#include "am/ui/apps/integration/modelscene.h"
#include "am/ui/font_icons/icons_am.h"
#include "am/ui/windows/asset/assethelper.h"
#include "am/ui/windows/asset/assetwindowfactory.h"
#include "am/ui/windows/asset/txd/txdwindow.h"
#include "helpers/format.h"

const rageam::ui::EntrySelection& rageam::ui::FolderView::GetSelectedEntries() const
{
	return m_DragSelecting ? m_DragSelection : m_Selection.Entries;
}

void rageam::ui::FolderView::RenameEntry(const ExplorerEntryPtr& entry, ConstString newName)
{
	// We have to re-add entry in selection set because new name will affect hash set
	bool wasSelected = m_Selection.Entries.IsSelected(entry);
	if (wasSelected)
		m_Selection.Entries.SetSelected(entry, false);
	entry->Rename(newName);
	m_Selection.Entries.SetSelected(entry, true);

	m_SortIsDirty = true;
}

void rageam::ui::FolderView::CalculateSelectedSize()
{
	m_SelectionSize = 0;

	for (const ExplorerEntryPtr& entry : GetSelectedEntries())
	{
		m_SelectionSize += entry->GetSize();
	}
}

void rageam::ui::FolderView::SetSelectionStateWithUndo(const Selection& newState)
{
	if (m_Selection == newState)
		return;

	UndoStack* undo = UndoStack::GetCurrent();
	undo->Add(new UndoableState(m_Selection, newState, [this]
		{
			CalculateSelectedSize();
		}));
}

void rageam::ui::FolderView::UpdateEntrySelection(const ExplorerEntryPtr& entry)
{
	bool shift = ImGui::IsKeyDown(ImGuiKey_LeftShift);
	bool ctrl = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);

	// Toggling only requires CTRL to be pressed
	// Range (shift) selection requires at least ony entry to be selected
	bool canToggle = ctrl;
	bool canRange = shift && m_Selection.LastClickedID != -1;

	s32 currentId = entry->GetID();

	// Single-Selection
	if (!(canToggle || canRange))
	{
		Selection newState
		{
			currentId, { entry }
		};
		SetSelectionStateWithUndo(newState);
		return;
	}

	// Ctrl toggles selection of single entry
	if (ctrl)
	{
		Selection newState = m_Selection;
		newState.Entries.ToggleSelection(entry);
		newState.LastClickedID = currentId;

		SetSelectionStateWithUndo(newState);
		return;
	}

	Selection newState = m_Selection;

	// Range selection, we select all entries from first selected entry to current one
	s32 startIndex = m_RootEntry->GetIndexFromID(newState.LastClickedID);
	s32 endIndex = m_RootEntry->GetIndexFromID(currentId);

	startIndex = m_RootEntry->TransformToSorted(startIndex);
	endIndex = m_RootEntry->TransformToSorted(endIndex);

	// Range selection overrides any existing selection
	newState.Entries = { m_RootEntry->GetChildFromID(newState.LastClickedID) };

	// Note here that we subtract / increment index because we already added it above
	if (endIndex < startIndex)
	{
		std::swap(startIndex, endIndex);
		endIndex--; // startIndex is now endIndex
	}
	else
	{
		startIndex++;
	}

	// Loop through sorted region and add all those entries to selected
	for (s32 i = startIndex; i <= endIndex; i++)
	{
		u16 actualIndex = m_RootEntry->TransformFromSorted(i);
		newState.Entries.SetSelected(m_RootEntry->GetChildFromIndex(actualIndex), true);
	}

	SetSelectionStateWithUndo(newState);
}

void rageam::ui::FolderView::UpdateEntryRenaming(const ExplorerEntryPtr& entry, const SlRenamingSelectableState& state)
{
	// We've began renaming
	if (state.Renaming && !state.WasRenaming)
	{
		m_RenamingEntry = entry;
		return;
	}

	if (!state.StoppedRenaming())
		return;

	// Set only if name actually changed
	if (state.AcceptRenaming && !String::Equals(state.TextEditable, state.Buffer))
	{
		ExplorerEntryPtr renameEntry = entry;
		rage::atString oldName = state.TextEditable;
		rage::atString newName = state.Buffer;
		UndoStack::GetCurrent()->Add(new UndoableFn(
			[=, this]
			{
				RenameEntry(entry, newName);
			},
			[=, this]
			{
				RenameEntry(entry, oldName);
			}));
	}
	m_RenamingEntry.reset();
}

void rageam::ui::FolderView::CreateEditableState(const ExplorerEntryPtr& entry, SlRenamingSelectableState& state)
{
	// There's no point to allow user to edit file extension so we display it with extension
	// and in rename mode there's no extension (state::TextDisplay & state::TextEditable)

	const EntrySelection& selection = GetSelectedEntries();

	state.Icon = entry->GetIcon().GetID();
	state.Buffer = m_RenameBuffer;
	state.BufferSize = IM_ARRAYSIZE(m_RenameBuffer);
	state.Selected = selection.IsSelected(entry);
	state.Renaming = m_RenamingEntry == entry;
	state.TextDisplay = entry->GetFullName();
	state.TextEditable = m_AllowRenaming ? entry->GetName() : entry->GetFullName();
}

void rageam::ui::FolderView::RenderEntryTableRow(const ExplorerEntryPtr& entry)
{
	SlRenamingSelectableState state;
	CreateEditableState(entry, state);

	SlGuiRenamingSelectableFlags selectableFlags = SlGuiRenamingSelectableFlags_RightClickSelect;
	// We want to disable other items if we rename one
	if (state.Renaming && !state.Selected)
		selectableFlags |= SlGuiRenamingSelectableFlags_Disabled;

	if (!m_AllowRenaming || entry->GetFlags() & ExplorerEntryFlags_NoRename)
		selectableFlags |= SlGuiRenamingSelectableFlags_NoRename;

	// Selection was reset and now we just draw outline around last selected item
	if (!state.Selected && m_Selection.LastClickedID == entry->GetID())
		selectableFlags |= SlGuiRenamingSelectableFlags_Outline;

	// Disable other entries while renaming
	bool disabled = !state.Renaming && m_RenamingEntry != nullptr;

	if (disabled) ImGui::BeginDisabled();
	if (SlGui::RenamingSelectable(state, selectableFlags))
	{
		UpdateEntrySelection(entry);
	}
	if (disabled) ImGui::EndDisabled();

	if (state.DoubleClicked && entry->IsDirectory())
	{
		m_EntryToOpen = entry;
	}
	UpdateEntryRenaming(entry, state);
}

void rageam::ui::FolderView::UpdateStatusBar() const
{
	ImGui::StatusBar();
	ImGui::AlignTextToFramePadding();

	u16 childCount = m_RootEntry->GetChildCount();
	u16 selectedCount = GetSelectedEntries().GetCount();
	if (selectedCount != 0)
	{
		if (m_SelectionSize == 0) // 5 of 10 selected
		{
			ImGui::Text("%u of %u selected", selectedCount, childCount);
		}
		else // 5 of 10 selected  |  2.15MB
		{
			ImGui::Text("%u of %u selected ", selectedCount, childCount);

			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			char sizeBuffer[16];
			FormatBytes(sizeBuffer, 16, m_SelectionSize);
			ImGui::Text(" %s", sizeBuffer);
		}
	}
	else // 5 items
	{
		ImGui::Text("%u %s ", childCount, childCount == 1 ? "item" : "items");
	}
}

void rageam::ui::FolderView::UpdateSelectAll()
{
	if (ImGui::Shortcut(ImGuiKey_A | ImGuiMod_Ctrl) && ImGui::IsWindowFocused())
	{
		Selection newState = { m_Selection.LastClickedID };

		for (const ExplorerEntryPtr& entry : *m_RootEntry)
			newState.Entries.SetSelected(entry, true);

		SetSelectionStateWithUndo(newState);
	}
}

void rageam::ui::FolderView::UpdateQuickView()
{
	bool canOpenQuickView =
		m_Selection.LastClickedID != -1 &&
		m_Selection.Entries.Any();
	if (ImGui::IsKeyPressed(ImGuiKey_Space, false) && canOpenQuickView)
	{
		if (!m_QuickLook.IsOpened())
			m_QuickLook.Open(m_RootEntry->GetChildFromID(m_Selection.LastClickedID));
		else
			m_QuickLook.Close();
	}
	m_QuickLook.Render();
}

void rageam::ui::FolderView::UpdateEntryOpening()
{
	if (!m_EntryToOpen)
		return;

	if (m_EntryToOpen->IsAsset())
	{
		asset::AssetPtr asset = m_EntryToOpen->GetAsset();
		if (asset) // If asset was loaded successfully... TODO: We need to handle loading exceptions from UI
		{
			if(asset->GetType() == asset::AssetType_Drawable)
			{
#ifdef AM_INTEGRATED
				Gui->Apps.FindAppByType<ModelSceneUI>()->LoadFromPatch(asset->GetDirectoryPath());
#endif
			}
			else
			{
				assetview::AssetWindowFactory::OpenNewOrFocusExisting(asset);
			}
		}
		m_EntryToOpen = nullptr; // To prevent it from actually opening
	}
}

void rageam::ui::FolderView::BeginDragSelection(ImRect& dragSelectRect)
{
	ImGuiDragSelectionFlags dragSelectionFlags = 0;
	if (ImGui::IsAnyItemHovered())
		dragSelectionFlags |= ImGuiDragSelectionFlags_DisableBegin;
	bool dragStopped;
	bool wasDragSelecting = m_DragSelecting;
	m_DragSelecting = ImGui::DragSelectionBehaviour(
		ImGui::GetID("FOLDER_VIEW_DRAG_SELECTION"), dragStopped, dragSelectRect, dragSelectionFlags);

	bool startedDragSelecting = !wasDragSelecting && m_DragSelecting;
	if (startedDragSelecting)
	{
		m_DragSelection.AllocateForDirectory(m_RootEntry);
	}

	// Finish drag selection, we do that only on drag finishing because otherwise
	// we'll spam whole undo stack in less than a second.
	// To preview currently drag-selected entries we override ::IsCurrentEntrySelected behaviour while dragging
	if (dragStopped)
	{
		Selection newState
		{
			m_Selection.LastClickedID, std::move(m_DragSelection)
		};
		SetSelectionStateWithUndo(newState);
	}
}

void rageam::ui::FolderView::EndDragSelection(bool selectionChangedDuringDragging)
{
	if (m_DragSelecting && selectionChangedDuringDragging)
	{
		CalculateSelectedSize();
	}
}

void rageam::ui::FolderView::RenderEntries()
{
	if (!m_RootEntry)
		return;

	ImGui::PreStatusBar();

	m_EntryToOpen = nullptr;

	// Setup drag selection
	ImRect dragSelectRect;
	bool selectionChangedDuringDragging = false;
	BeginDragSelection(dragSelectRect);

	ImGuiTableFlags tableFlags =
		ImGuiTableFlags_Sortable |
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_NoBordersInBody |
		ImGuiTableFlags_Reorderable;

	// We leave a bit of empty space on the right to allow user to use drag selection
	ImVec2 tableSize(
		ImGui::GetCurrentWindow()->WorkRect.GetWidth() - ImGui::GetFrameHeight() * 2,
		0);

	if (ImGui::BeginTable("FolderTable", 4, tableFlags, tableSize))
	{
		// Reset scroll, otherwise if if we're on bottom of some huge folder and switch
		// to smaller one, scroll gonna break
		if (m_RootEntryChangedThisFrame)
		{
			ImGui::SetScrollY(0);
		}

		// Force-sort entry
		if (m_RootEntryChangedThisFrame || m_SortIsDirty)
		{
			ImGuiTable* table = ImGui::GetCurrentTable();
			table->IsSortSpecsDirty = true;
			m_SortIsDirty = false;
		}

		ImGuiTableColumnFlags nameFlags =
			ImGuiTableColumnFlags_DefaultSort |
			ImGuiTableColumnFlags_PreferSortAscending;

		ImGui::TableSetupColumn("Name", nameFlags, 0, ExplorerEntryColumnID_Name);
		ImGui::TableSetupColumn("Date Modified", 0, 0, ExplorerEntryColumnID_DateModified);
		ImGui::TableSetupColumn("Type", 0, 0, ExplorerEntryColumnID_TypeName);
		ImGui::TableSetupColumn("Size", 0, 0, ExplorerEntryColumnID_Size);

		if (ImGuiTableSortSpecs* sort = ImGui::TableGetSortSpecs())
		{
			m_RootEntry->Sort(sort);
		}

		SlGui::TableHeadersRow();

		for (u16 i = 0; i < m_RootEntry->GetChildCount(); i++)
		{
			ImGui::TableNextRow();

			ExplorerEntryPtr& entry = m_RootEntry->GetSortedChildFromIndex(i);

			// Column: Name & Selector
			ImGui::TableSetColumnIndex(0);
			{
				RenderEntryTableRow(entry);

				// Here we load some CPU/GPU expensive data (like icons)
				// There's no point to load it up for entries that aren't even on screen
				if (ImGui::IsItemVisible())
					entry->PrepareToBeDisplayed();

				// Entry is selectable and it covers whole table width
				ImRect entryRect = GImGui->LastItemData.Rect;

				// Handle mouse-drag selection
				if (m_DragSelecting)
				{
					bool nowSelected = dragSelectRect.Overlaps(entryRect);
					bool wasSelected = m_DragSelection.IsSelected(entry);
					if (nowSelected != wasSelected)
					{
						m_DragSelection.SetSelected(entry, nowSelected);
						selectionChangedDuringDragging = true;
					}
				}

				// Allow entry(s) to be drag-dropped
				ExplorerEntryDragData* dragData;
				if (ExplorerEntryBeginDragSource(entry, &dragData))
				{
					if (dragData)
					{
						// NOTE: Multi-selection dragging only works when we're dragging one of selected entries!
						// otherwise selection is set to single dragged entry
						if (!m_Selection.Entries.IsSelected(entry))
						{
							m_Selection.LastClickedID = entry->GetID();
							m_Selection.Entries.ClearSelection();
							m_Selection.Entries.SetSelected(entry, true);
						} // Else we do multi-selection drag

						dragData->Entries.Reserve(m_Selection.Entries.GetCount());
						for (const ExplorerEntryPtr& dragEntry : m_Selection.Entries)
							dragData->Entries.Add(dragEntry);
						dragData->Cut = false; // TODO: This must be different for QuickAccess (User) / Fi
					}

					ExplorerEntryEndDragSource();
				}
			}

			// We display remaining columns semi-transparent
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.75f);

			// Column: Date Modified
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%s", entry->GetTimeDisplay());

			// Column: Type
			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%s", entry->GetTypeName());

			// Column: Size
			ImGui::TableSetColumnIndex(3);
			if (!entry->IsDirectory())
			{
				char sizeBuffer[16];
				FormatBytes(sizeBuffer, 16, entry->GetSize());
				ImGui::Text("%s", sizeBuffer);
			}

			// Right padding, just for the look
			ImGui::SameLine(); ImGui::Dummy(ImVec2(10, 0));

			ImGui::PopStyleVar(); // Alpha
		}
		ImGui::EndTable(); // FolderTable

		if (m_RootEntry->GetChildCount() == 0)
		{
			ImGui::Dummy(ImVec2(0, 15));
			ImGui::TextCentered("This folder is empty.", ImGuiTextCenteredFlags_Horizontal);
		}
	}

	EndDragSelection(selectionChangedDuringDragging);
	UpdateStatusBar();
	UpdateSelectAll();
	UpdateQuickView();
	UpdateEntryOpening();
}

void rageam::ui::FolderView::UpdateSearchOnType()
{
	// TODO: ...
}

void rageam::ui::FolderView::Render()
{
	UpdateSearchOnType();

	// Refresh directory F5 was pressed or some file was changed
	bool changeOccured = m_DirectoryWatcher.GetChangeOccuredAndReset();
	bool f5Pressed = ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_F5, false);
	if (changeOccured || f5Pressed)
	{
		Refresh();
	}

	// Refresh formatted time on entries if necessary
	double time = ImGui::GetTime();
	if (time > m_NextEntryTimeUpdate)
	{
		m_NextEntryTimeUpdate = time + ENTRY_TIME_REFRESH_INTERVAL;

		for (const ExplorerEntryPtr& entry : *m_RootEntry)
		{
			entry->RefreshDisplayInfo();
		}
	}

	// Draw context menu
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
	ImGui::SetNextWindowSize(ImVec2(190, 0));
	if (ImGui::BeginPopup(CONTEXT_MENU_ID))
	{
		ExplorerEntryPtr selectedEntry = nullptr;
		if (m_Selection.LastClickedID != -1 && m_Selection.Entries.Any())
			selectedEntry = m_RootEntry->GetChildFromID(m_Selection.LastClickedID);

		if (selectedEntry)
		{
			ImGui::MenuItem(ICON_AM_OPEN"  Open");

			ImGui::Separator();

			ImGui::MenuItem(ICON_AM_CANCEL"  Remove");
			if (ImGui::MenuItem(ICON_AM_RENAME"  Rename", "F2"))
				m_RenamingEntry = selectedEntry;
		}
		else
		{
			if (ImGui::MenuItem(ICON_AM_REFRESH"  Refresh"))
				Refresh();
		}

		// TODO: Compile option
		/*if (selectedEntry && selectedEntry->IsAsset())
		{
			ImGui::Separator();

			asset::AssetPtr& project = selectedEntry->GetAsset();

			bool isCompiling = project->IsCompiling();

			if (isCompiling) ImGui::BeginDisabled();
			bool compile = ImGui::MenuItem("Compile");
			if (isCompiling)ImGui::EndDisabled();

			if (compile)
			{
				rage::atString compilePath = rage::Path::Combine(
					{ m_ContextMenuEntry->GetParent()->GetPath(), m_ContextMenuEntry->GetName() });
				compilePath += project->GetCompiledExtension();
				project->CompileToAsync(compilePath, [this]
					{
						m_NeedRefresh = true;
					});

				ImGui::CloseCurrentPopup();
			}
		}*/

		ImGui::EndPopup();
	}
	ImGui::PopStyleVar(2); // WindowPadding, ItemSpacing

	// We schedule entry opening to beginning of
	// next (this) frame so 'entry changed' event can be handled properly
	if (m_EntryToOpen)
	{
		SetRootEntry(m_EntryToOpen);
		m_EntryToOpen = nullptr;
	}

	// Render folder view
	RenderEntries();

	// Open context menu
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsWindowHovered())
	{
		if (m_RenamingEntry)
		{
			// Commit renaming
			SlRenamingSelectableState state;
			CreateEditableState(m_RenamingEntry, state);
			state.Renaming = false;
			state.WasRenaming = true;
			state.AcceptRenaming = true;
			UpdateEntryRenaming(m_RenamingEntry, state);
		}

		ImGui::OpenPopup(CONTEXT_MENU_ID);
	}

	m_RootEntryChangedThisFrame = false;
}

void rageam::ui::FolderView::SetRootEntry(const ExplorerEntryPtr& root)
{
	// TODO: Unload loaded assets for previous root entry

	m_RootEntryChangedThisFrame = true;

	m_RootEntry = root;
	m_RootEntry->LoadChildren();

	// Directory watcher only works with file system,
	// there's no point for watcher in user managed directory in either case
	if(m_RootEntry->GetEntryType() == ExplorerEntryType_Fi)
		m_DirectoryWatcher.SetEntry(m_RootEntry->GetPath());

	m_Selection.LastClickedID = -1;
	m_Selection.Entries.ClearSelection();
	m_Selection.Entries.AllocateForDirectory(m_RootEntry);

	// TODO: We either clean up selection undo or add navigation into undo too
	UndoStack* undo = UndoStack::GetCurrent();
	if (undo)
		undo->Clear();
}

void rageam::ui::FolderView::Refresh()
{
	m_RootEntry->UnloadChildren();
	m_RootEntry->LoadChildren();
	// This will trigger sorting update because RefreshChildren currently does hard-reset
	m_RootEntryChangedThisFrame = true;
}
