#include "explorer.h"

#include <shlobj_core.h>

#include "entry.h"
#include "imgui_internal.h"
#include "am/file/driveinfo.h"
#include "am/ui/extensions.h"
#include "am/ui/styled/slwidgets.h"

rageam::ui::Explorer::Explorer()
{
	ExplorerEntryUserPtr quickAccess = std::make_shared<ExplorerEntryUser>("Quick Access");
	ExplorerEntryUserPtr thisPc = std::make_shared<ExplorerEntryUser>("This PC");
	ExplorerEntryUserPtr cycleBin = std::make_shared<ExplorerEntryUser>("Recycle Bin");

	quickAccess->SetIcon("quick_access");
	thisPc->SetIcon("pc");
	cycleBin->SetIcon("bin_empty");

	m_TreeView.Entries.Add(quickAccess);
	m_TreeView.Entries.Add(thisPc);
	m_TreeView.Entries.Add(cycleBin);

	// Default selection
	m_TreeView.SetSelectedEntry(quickAccess);
	m_FolderView.SetRootEntry(quickAccess);

	// To add user folders (Desktop, Documents, Downloads etc)
	auto AddSpecialFolder = [&thisPc](REFKNOWNFOLDERID rfid, ConstString iconName)
	{
		wchar_t* path;
		if (SUCCEEDED(SHGetKnownFolderPath(rfid, 0, 0, &path)))
		{
			auto& entry = thisPc->AddChildren(std::make_shared<ExplorerEntryFi>(String::ToUtf8Temp(path)));
			entry->SetIconOverride(iconName);

			CoTaskMemFree(path);
		}
	};
	AddSpecialFolder(FOLDERID_Desktop, "desktop");
	AddSpecialFolder(FOLDERID_Documents, "documents");
	AddSpecialFolder(FOLDERID_Downloads, "downloads");
	AddSpecialFolder(FOLDERID_Pictures, "pictures");
	AddSpecialFolder(FOLDERID_Videos, "videos");

	// Add every logical drive
	auto drives = file::DriveInfo::GetDrives();
	for (auto& info : drives)
	{
		ConstWString volumeLabel = info.VolumeLabel;
		if (String::IsNullOrEmpty(volumeLabel))
			volumeLabel = L"Local Disk";

		// Note that for drive name (%.2ls) we use only 2 chars to display 'C:/' as 'C:' without '/'
		ConstWString displayName = String::FormatTemp(L"%ls (%.2ls)", volumeLabel, info.Name);

		// Protect fool users from messing up with drive names,
		// I have no idea what even will happen if you attempt to rename it and not going to try
		ExplorerEntryFlags flags =
			ExplorerEntryFlags_CantBeRemoved |
			ExplorerEntryFlags_NoRename;

		ExplorerEntryPtr diskEntry = std::make_shared<ExplorerEntryFi>(
			String::ToUtf8Temp(info.Name), flags);
		diskEntry->SetCustomName(String::ToUtf8Temp(displayName));
		diskEntry->SetIconOverride(info.IsSystem ? "disk_system" : "disk");

		thisPc->AddChildren(diskEntry);
	}
}

void rageam::ui::Explorer::OnRender()
{
	// Toolbar
	bool navLeft, navRight;
	{
		bool canNavLeft = m_TreeView.CanGoLeft();
		bool canNavRight = m_TreeView.CanGoRight();

		ImGui::BeginToolBar("ASSETS_TOOLBAR");

		navLeft = ImGui::NavButton("ASSET_NAV_LEFT", ImGuiDir_Left, canNavLeft);
		ImGui::ToolTip("Back (Alt + Left Arrow)");
		navRight = ImGui::NavButton("ASSET_NAV_RIGHT", ImGuiDir_Right, canNavRight);
		ImGui::ToolTip("Forward (Alt + Right Arrow)");

		ImGui::EndToolBar();

		if (canNavLeft)
		{
			if (ImGui::Shortcut(ImGuiKey_LeftArrow | ImGuiMod_Alt, 0))
				navLeft = true;
			if (ImGui::IsKeyReleased(ImGuiKey_MouseX1))
				navLeft = true;
		}
		if (canNavRight)
		{
			if (ImGui::Shortcut(ImGuiKey_RightArrow | ImGuiMod_Alt))
				navRight = true;
			if (ImGui::IsKeyReleased(ImGuiKey_MouseX2))
				navRight = true;
		}
	}

	// Remove huge gap between the columns
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(1, 0));
	bool splitViewOpened = ImGui::BeginTable("ExplorerSplitView", 2, ImGuiTableFlags_Resizable);
	ImGui::PopStyleVar(); // CellPadding

	if (splitViewOpened)
	{
		ImGui::TableSetupColumn("TreeView", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() * 10);
		ImGui::TableSetupColumn("FolderView", ImGuiTableColumnFlags_WidthStretch, 0);

		// Column: Directory tree
		if (ImGui::TableNextColumn())
		{
			ImGui::BeginChild("ASSETS_TREE_VIEW");
			SlGui::ShadeItem(SlGuiCol_Bg);

			m_TreeView.Render();
			if (navLeft) m_TreeView.GoLeft();
			if (navRight) m_TreeView.GoRight();
			// This has to be called from tree view window or ImGuiId won't match
			if (ExplorerEntryPtr& openedEntry = m_FolderView.GetOpenedEntry())
			{
				m_TreeView.SetSelectedEntryWithUndo(openedEntry, false);
			}

			ImGui::EndChild();
		}

		// Column: Folder view
		if (ImGui::TableNextColumn())
		{
			ImGui::BeginChild("ASSETS_FOLDER_VIEW");
			SlGui::ShadeItem(SlGuiCol_Bg2);

			if (m_TreeView.GetSelectionChanged())
			{
				m_FolderView.SetRootEntry(m_TreeView.GetSelectedEntry());
			}
			m_FolderView.Render();

			ImGui::EndChild();
		}
		ImGui::EndTable();
	}
}
