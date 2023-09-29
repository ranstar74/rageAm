//
// File: treeview.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "entry.h"
#include "am/task/undo.h"
#include "common/types.h"
#include "rage/atl/array.h"
#include "rage/atl/hashstring.h"

namespace rageam::ui
{
	class TreeView
	{
		static constexpr ConstString CONTEXT_MENU_ID = "TREE_VIEW_CONTEXT_MENU";
		static constexpr u32 MAX_CLOSED_ENTRY_HISTORY_SIZE = 32;

		// To keep memory use on optimal level, we keep history of every closed folder and unload old ones.
		// This won't cause 'I/O spamming' if user opens and closes the same directory many times.
		// Also note that we're controlling that from tree view and not from explorer itself, that's because
		// entries can be opened / closed only from tree view. Folder view always displays opened entry.
		// We use smart pointers so unloaded children won't cause any issues other than being 'de-synced'.
		rage::atArray<ExplorerEntryPtr> m_ClosedEntryHistory;

		// We can't store index here because all entries are dynamically allocated on heap (see ExplorerEntry::m_Children);
		ExplorerEntryPtr m_SelectedEntry;
		bool m_SelectionChanged = false;

		// Updated after rendering all entries and reset at beginning of render, needed to open context menu on hovered entry
		ExplorerEntryPtr m_HoveredEntry;

		// Selected entry in currently opened context menu, if there's any
		ExplorerEntryPtr m_MenuEntry;

		UndoStack m_NavigationContext;

		void RemoveEntryFromClosed(const ExplorerEntryPtr& entry);
		void AddEntryToClosed(const ExplorerEntryPtr& entry);

		void RenderEntryRecurse(const ExplorerEntryPtr& entry, bool root = false);

		// Creates unique display name for entry for ImGuiID
		ConstString FormatNodeName(const IExplorerEntry& entry) const;
	public:
		TreeView();

		void Render();

		bool GetSelectionChanged() const { return m_SelectionChanged; }
		ExplorerEntryPtr& GetSelectedEntry() { return m_SelectedEntry; }

		// # Navigation < - >

		void SetSelectedEntry(const ExplorerEntryPtr& entry);

		// Notify selection changed is used when we set entry internally (in RenderEntryRecurse) so FolderView can see
		// that it has been changed. But when selection was changed in FolderView, we don't want to
		// notify FolderView again because it would cause feedback loop
		void SetSelectedEntryWithUndo(const ExplorerEntryPtr& newSelected, bool notifySelectionChanged);

		bool CanGoLeft() const { return m_NavigationContext.CanUndo(); }
		bool CanGoRight() const { return m_NavigationContext.CanRedo(); }
		void GoLeft() { m_NavigationContext.Undo(); m_SelectionChanged = true; }
		void GoRight() { m_NavigationContext.Redo(); m_SelectionChanged = true; }

		// # Outline

		rage::atArray<ExplorerEntryPtr> Entries;
	};
}
