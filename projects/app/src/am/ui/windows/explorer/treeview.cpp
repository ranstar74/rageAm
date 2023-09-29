#include "treeview.h"

#include "am/ui/extensions.h"
#include "am/ui/font_icons/icons_am.h"
#include "am/ui/styled/slwidgets.h"
#include "rage/crypto/joaat.h"

void rageam::ui::TreeView::RemoveEntryFromClosed(const ExplorerEntryPtr& entry)
{
	for (u16 i = 0; i < m_ClosedEntryHistory.GetSize(); i++)
	{
		if (m_ClosedEntryHistory[i] == entry)
		{
			AM_DEBUGF("Explorer -> Remove %s from closed history", entry->GetName());
			m_ClosedEntryHistory.RemoveAt(i);
			break;
		}
	}
}

void rageam::ui::TreeView::AddEntryToClosed(const ExplorerEntryPtr& entry)
{
	AM_DEBUGF("TreeView::AddEntryToClosed() -> Adding %s to closed history", entry->GetName());
	m_ClosedEntryHistory.Add(entry);
	if (m_ClosedEntryHistory.GetSize() <= MAX_CLOSED_ENTRY_HISTORY_SIZE)
		return;

	AM_DEBUGF("TreeView::AddEntryToClosed() -> Too many entries in history, popping front... (%s)",
		m_ClosedEntryHistory.First()->GetName());

	m_ClosedEntryHistory.First()->UnloadChildren();
	m_ClosedEntryHistory.RemoveFirst();
}

void rageam::ui::TreeView::RenderEntryRecurse(const ExplorerEntryPtr& entry, bool root)
{
	// We allow to display files on user directories
	if (!entry->IsUserParent() && !entry->IsDirectory())
		return;

	SlGuiTreeNodeFlags nodeFlags = 0;
	if (root)
		nodeFlags |= SlGuiTreeNodeFlags_DefaultOpen;

	if (!entry->HasChildDirectories())
		nodeFlags |= SlGuiTreeNodeFlags_NoChildren;

	if (entry == m_MenuEntry) // Highlight node that's observed in context menu
		nodeFlags |= SlGuiTreeNodeFlags_DisplayAsHovered;

	ImTextureID icon = entry->GetIcon().GetID();
	ConstString nodeName = FormatNodeName(*entry);

	bool selected = entry == m_SelectedEntry;
	bool wasSelected = selected;
	bool toggled;
	bool opened = SlGui::TreeNode(nodeName, selected, toggled, icon, nodeFlags);

	if (ImGui::IsItemHovered())
		m_HoveredEntry = entry;

	bool nowSelected = selected && !wasSelected;
	if (nowSelected)
	{
		SetSelectedEntryWithUndo(entry, true);
	}

	// Here we load some CPU/GPU expensive data (like icons)
	// There's no point to load it up for entries that aren't even on screen
	if (ImGui::IsItemVisible())
		entry->PrepareToBeDisplayed();

	// Setup drag-drop
	if (entry->IsUserParent()) // We can only re-position items in user entry
	{
		ExplorerEntryDragData* dragData;
		if (ExplorerEntryBeginDragSource(entry, &dragData))
		{
			if (dragData)
			{
				dragData->Entries.Add(entry);
				dragData->Cut = true; // We re-position entry so original instance has to be removed
			}

			ExplorerEntryEndDragSource();
		}
	}
	ExplorerEntryDragTargetBehaviour(entry);

	// Track opened / closed entries so we can unload the very old ones
	if (toggled)
	{
		if (opened)
		{
			// We unfolded entry, make sure that children are loaded
			entry->LoadChildren();

			RemoveEntryFromClosed(entry);
		}
		else
		{
			AddEntryToClosed(entry);
		}
	}

	// Render all child entries if unfolded
	if (opened)
	{
		for (s32 i = 0; i < entry->GetChildCount(); i++)
			RenderEntryRecurse(entry->GetChildFromIndex(i));

		ImGui::TreePop();
	}
}

ConstString rageam::ui::TreeView::FormatNodeName(const IExplorerEntry& entry) const
{
	ConstString name = entry.GetCustomName();
	if (String::IsNullOrEmpty(name))
		name = entry.GetFullName();

	// We have to make sure that ImGuiID will be unique for two folders with same name
	return ImGui::FormatTemp("%s##%u", name, rage::joaat(entry.GetPath()));
}

rageam::ui::TreeView::TreeView() : m_NavigationContext(UNDO_CONTEXT_HARD_REDO)
{

}

void rageam::ui::TreeView::Render()
{
	m_SelectionChanged = false;
	m_HoveredEntry = nullptr;

	// Render all entries
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	for (ExplorerEntryPtr& entry : Entries) // Root entries - Quick Access, This PC, Trash Bin
	{
		RenderEntryRecurse(entry, true);
	}
	ImGui::PopStyleVar(); // ItemSpacing

	// Open context menu if we released rmb in tree view area
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsWindowHovered())
	{
		if (m_HoveredEntry) // If we're opening context menu on hovered entry
			m_MenuEntry = m_HoveredEntry;

		// TODO: Disabled, need more work
		//ImGui::OpenPopup(CONTEXT_MENU_ID);
	}

	// Draw context menu
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
	ImGui::SetNextWindowSize(ImVec2(190, 0));
	if (ImGui::BeginPopup(CONTEXT_MENU_ID))
	{
		if (m_MenuEntry)
		{
			ExplorerEntryFlags entryFlags = m_MenuEntry->GetFlags();
			bool noRemove = entryFlags & ExplorerEntryFlags_CantBeRemoved;

			if (noRemove) ImGui::BeginDisabled();
			switch (m_MenuEntry->GetEntryType())
			{
			case ExplorerEntryType_Fi:
				if (ImGui::MenuItem(ICON_AM_DELETE_FOLDER"  Delete directory"))
				{

				}
				break;
			case ExplorerEntryType_User:
				if (ImGui::MenuItem(ICON_AM_REMOVE_LINK"  Delete virtual directory"))
				{

				}
				break;
			}
			if (noRemove) ImGui::EndDisabled();

			if (m_MenuEntry->IsAsset())
			{
				ImGui::Separator();

				if (ImGui::MenuItem(ICON_AM_BUILD_SOLUTION"  Compile"))
				{
					// TODO: Need async here
					asset::AssetPtr asset = m_MenuEntry->GetAsset();
					asset->CompileToFile();
				}

				if (ImGui::MenuItem(ICON_AM_BUILD_SOLUTION_AS"  Compile as"))
				{
					// TODO: Need file dialog
				}
			}
		}
		else
		{
			if (ImGui::MenuItem(ICON_AM_ADD_LINK"  Create an virtual directory"))
			{

			}
		}
		ImGui::EndPopup();
	}
	else
	{
		m_MenuEntry = nullptr;
	}

	ImGui::PopStyleVar(2); // WindowPadding, ItemSpacing
}

void rageam::ui::TreeView::SetSelectedEntry(const ExplorerEntryPtr& entry)
{
	m_SelectedEntry = entry;

	// After selecting node we have to make sure that folder hierarchy is opened (in ImGui),
	// the only way to do that is open every parent node in hierarchy order (parent -> children)

	IExplorerEntry* itEntry = entry.get();
	rage::atArray<IExplorerEntry*> hierarchy;
	while ((itEntry = itEntry->GetParent()))
		hierarchy.Insert(0, itEntry);

	for (IExplorerEntry* node : hierarchy)
		ImGui::IconTreeNodeSetOpened(FormatNodeName(*node), true);

	for (IExplorerEntry* node : hierarchy)
		ImGui::TreePop();
}

void rageam::ui::TreeView::SetSelectedEntryWithUndo(const ExplorerEntryPtr& newSelected, bool notifySelectionChanged)
{
	ExplorerEntryPtr oldSelected = m_SelectedEntry;

	m_NavigationContext.Add(new UndoableFn(
		[=, this]
		{
			SetSelectedEntry(newSelected);
			if (notifySelectionChanged)
				m_SelectionChanged = true;
		},
			[=, this]
		{
			SetSelectedEntry(oldSelected);
			if (notifySelectionChanged)
				m_SelectionChanged = true;
		}));
}
