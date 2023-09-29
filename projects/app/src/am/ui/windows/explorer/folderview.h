//
// File: folderview.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "entry.h"
#include "entryselection.h"
#include "am/ui/styled/slwidgets.h"
#include "rage/file/watcher.h"
#include "quicklook.h"

namespace rageam::ui
{
	class FolderView
	{
		static constexpr ConstString CONTEXT_MENU_ID = "FOLDER_VIEW_CONTEXT_MENU";
		static constexpr double ENTRY_TIME_REFRESH_INTERVAL = 1.0; // See m_NextEntryTimeUpdate and ExplorerEntry::m_TimeFormatted

		struct Selection
		{
			// In range selection case, will be set to first clicked ID, same way as in windows explorer
			// Not reset when selection is cleared
			s32 LastClickedID = -1;

			EntrySelection Entries;

			bool operator==(const Selection&) const = default;
		};

		bool						m_RootEntryChangedThisFrame = false;
		bool						m_SortIsDirty = false;		// Will force sort entries, used if entry was renamed or root entry changed

		ExplorerEntryPtr			m_RootEntry;				// Directory that we're observing
		rage::fiDirectoryWatcher	m_DirectoryWatcher;			// To track changes in root entry and refresh view

		Selection					m_Selection;
		u32							m_SelectionSize = 0;		// Total size of selected files in bytes

		EntrySelection				m_DragSelection;			// Temporary set of entries that were drag selected
		bool						m_DragSelecting = false;

		ExplorerEntryPtr			m_RenamingEntry;
		char						m_RenameBuffer[FI_MAX_PATH]{};
		bool						m_AllowRenaming = true;

		ExplorerEntryPtr			m_EntryToOpen;				// Set to entry that was double clicked this frame

		QuickLook					m_QuickLook;

		double						m_NextEntryTimeUpdate = 0;	// We have to refresh formatted time on entries (i.e. 5 minutes ago) at least once per minute

		// We override selected entries while drag selecting (aka selection rectangle) to preview them without spamming undo stack,
		// when drag selection is over we push undo action with newly selected entries (in ::Render)
		const EntrySelection& GetSelectedEntries() const;

		// Must be used instead of IExplorerEntry::Rename to automatically fixup entry in selection set
		void RenameEntry(const ExplorerEntryPtr& entry, ConstString newName);

		void CalculateSelectedSize();

		void SetSelectionStateWithUndo(const Selection& newState);

		void UpdateEntrySelection(const ExplorerEntryPtr& entry);
		void UpdateEntryRenaming(const ExplorerEntryPtr& entry, const SlRenamingSelectableState& state);

		void CreateEditableState(const ExplorerEntryPtr& entry, SlRenamingSelectableState& state);
		// Renders selectable that spans across all table columns
		void RenderEntryTableRow(const ExplorerEntryPtr& entry);

		// Status bar with current folder and selection info
		void UpdateStatusBar() const;
		// Select all using Ctrl + A
		void UpdateSelectAll();
		// Open/Close QuickView if space was pressed
		void UpdateQuickView();
		// Handles custom behaviour for entry double click
		// We open asset-specific window here, in future we'll add handling files with default windows shell editors
		void UpdateEntryOpening();
		// Handles selection rectangle
		void BeginDragSelection(ImRect& dragSelectRect);
		// Accumulates size of drag-selected entries
		void EndDragSelection(bool selectionChangedDuringDragging);

		void RenderEntries();

		// Performs windows-like search by typing first letters of file name
		void UpdateSearchOnType();
	public:
		FolderView() = default;

		void Render();
		void SetRootEntry(const ExplorerEntryPtr& root);

		void Refresh();

		// Gets directory entry that was opened this frame.
		ExplorerEntryPtr& GetOpenedEntry() { return m_EntryToOpen; }

		bool ShowDetails = true;
	};
}
