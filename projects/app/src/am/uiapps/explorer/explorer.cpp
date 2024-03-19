#include "explorer.h"

#include <shlobj_core.h>

#include "entry.h"
#include "imgui_internal.h"
#include "am/file/driveinfo.h"
#include "am/system/datamgr.h"
#include "am/ui/extensions.h"
#include "am/ui/slwidgets.h"
#include "am/xml/doc.h"
#include "am/xml/iterator.h"

rageam::file::WPath rageam::ui::Explorer::GetExplorerSettingsPath() const
{
	return DataManager::GetAppData() / L"Explorer.xml";
}

void rageam::ui::Explorer::ReadSettings() const
{
	file::WPath settingsPath = GetExplorerSettingsPath();
	if (!IsFileExists(settingsPath))
		return;

	try
	{
		XmlDoc xDoc;
		xDoc.LoadFromFile(settingsPath);
		XmlHandle xSettings = xDoc.Root();

		XmlHandle xQuickAccessDirs = xSettings.GetChild("QuickAccessDirs", true);
		for (const XmlHandle& xItem : XmlIterator(xQuickAccessDirs, "Item"))
		{
			file::U8Path uPath = xItem.GetText();
			file::WPath wPath = file::PathConverter::Utf8ToWide(uPath);
			if (!IsFileExists(wPath)) // file/directory was moved or removed...
				continue;

			ExplorerEntryPtr entry = std::make_shared<ExplorerEntryFi>(uPath, ExplorerEntryFlags_NoRename);
			m_QuickAccess->AddChildren(entry);
		}
	}
	catch (const XmlException& ex)
	{
		ex.Print();
	}
}

void rageam::ui::Explorer::SaveSettings() const
{
	XmlDoc xDoc("Explorer");
	XmlHandle xSettings = xDoc.Root();

	XmlHandle xQuickAccessDirs = xSettings.AddChild("QuickAccessDirs");
	for (u16 i = 0; i < m_QuickAccess->GetChildCount(); i++)
	{
		const ExplorerEntryPtr& entry = m_QuickAccess->GetChildFromIndex(i);

		XmlHandle xItem = xQuickAccessDirs.AddChild("Item");
		xItem.SetText(entry->GetPath());
	}

	xDoc.SaveToFile(GetExplorerSettingsPath());
}

rageam::ui::Explorer::Explorer()
{
	m_QuickAccess = std::make_shared<ExplorerEntryUser>("Quick Access");
	m_ThisPC = std::make_shared<ExplorerEntryUser>("This PC");
	ExplorerEntryUserPtr cycleBin = std::make_shared<ExplorerEntryUser>("Recycle Bin");

	m_QuickAccess->SetIcon("quick_access");
	m_ThisPC->SetIcon("pc");
	cycleBin->SetIcon("bin_empty");

	m_TreeView.Entries.Add(m_QuickAccess);
	m_TreeView.Entries.Add(m_ThisPC);
	m_TreeView.Entries.Add(cycleBin);

	// To add user folders (Desktop, Documents, Downloads etc)
	auto AddSpecialFolder = [&](REFKNOWNFOLDERID rfid, ConstString iconName)
		{
			wchar_t* path;
			if (SUCCEEDED(SHGetKnownFolderPath(rfid, 0, 0, &path)))
			{
				auto& entry = m_ThisPC->AddChildren(std::make_shared<ExplorerEntryFi>(String::ToUtf8Temp(path)));
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

		m_ThisPC->AddChildren(diskEntry);
	}

	ReadSettings();
}

rageam::ui::Explorer::~Explorer()
{
	SaveSettings();
}

void rageam::ui::Explorer::OnStart()
{
	// Default selection
	m_TreeView.SetSelectedEntry(m_QuickAccess);
	m_FolderView.SetRootEntry(m_QuickAccess);
}

void rageam::ui::Explorer::OnRender()
{
	// Toolbar
	bool navLeft, navRight;
	{
		bool canNavLeft = m_TreeView.CanGoLeft();
		bool canNavRight = m_TreeView.CanGoRight();

		ImGui::BeginToolBar("NAV_TOOLBAR");
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
			// SlGui::ShadeItem(SlGuiCol_Bg);

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
			// A little bit brighter than the rest
			ImU32 folderBgCol = ImGui::ColorModifyHSV(ImGui::GetColorU32(ImGuiCol_WindowBg), 1, 1, 1.75f);

			ImGui::PushStyleColor(ImGuiCol_ChildBg, folderBgCol);
			ImGui::BeginChild("ASSETS_FOLDER_VIEW");
			if (m_TreeView.GetSelectionChanged())
			{
				m_FolderView.SetRootEntry(m_TreeView.GetSelectedEntry());
			}
			m_FolderView.Render();
			ImGui::EndChild();
			ImGui::PopStyleColor();
		}
		ImGui::EndTable();
	}
}
