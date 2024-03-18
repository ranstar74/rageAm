#include "entry.h"

#include "am/asset/factory.h"
#include "am/ui/extensions.h"
#include "am/ui/slwidgets.h"
#include "am/ui/image.h"
#include "rage/file/iterator.h"

namespace
{
	// We store drag data here and pass it from ExplorerEntryBeginDragDropSource to user to fill when drag drop begins

	rageam::ui::ExplorerEntryDragData s_DragData;
	rageam::ui::ExplorerEntryDragData* s_pDragData = &s_DragData; // To make ImGui copy just pointer and not whole struct
}

bool rageam::ui::ExplorerEntryBeginDragSource(const ExplorerEntryPtr& entry, ExplorerEntryDragData** outDragData)
{
	*outDragData = nullptr;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2)); // Icon popup inner padding
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6); // Icon popup rounding
	bool dragging = ImGui::BeginDragDropSource2(ImGuiDragDropFlags_SourceAllowNullID);
	ImGui::PopStyleVar(2); // WindowPadding, WindowRounding

	if (dragging)
	{
		entry->GetIcon().Render(96, 96);

		// Display number of items under icon for multi-drag
		if (s_pDragData->Entries.GetSize() > 1)
		{
			ImGui::PushFont(ImFont_Medium);
			ImGui::TextCentered(
				ImGui::FormatTemp("%u items", s_pDragData->Entries.GetSize()),
				ImGuiTextCenteredFlags_Horizontal);
			ImGui::PopFont();
		}

		// Let user set it up after first call
		if (GImGui->DragDropPayload.Data == nullptr)
			*outDragData = s_pDragData;
	}

	if (!ImGui::IsDragDropActive())
	{
		// Clean-up entries so they don't hang in air
		s_pDragData->Entries.Destroy();
	}

	return dragging;
}

void rageam::ui::ExplorerEntryEndDragSource()
{
	if (GImGui->DragDropPayload.Data == nullptr)
		ImGui::SetDragDropPayload(ExplorerEntryDragType, &s_pDragData, sizeof(ExplorerEntryDragData**), ImGuiCond_Once);

	ImGui::EndDragDropSource();
}

void rageam::ui::ExplorerEntryDragTargetBehaviour(const ExplorerEntryPtr& entry)
{
	// Handle drag-drop
	if (!ImGui::BeginDragDropTarget())
		return;

	ImGuiDragDropFlags dragFlags = ImGuiDragDropFlags_AcceptNoDrawDefaultRect;
	IExplorerEntry* targetEntryParent = entry->GetParent();

	SlGuiNodeDragPosition dragPos;
	SlGuiNodeDragFlags nodeDragFlags = 0;
	// For user type we can specify entry index,
	// Fi index is controlled by file system itself so we support only copy 
	if (targetEntryParent && targetEntryParent->GetEntryType() == ExplorerEntryType_User)
		nodeDragFlags |= SlGuiNodeDragFlags_AllowAboveAndBelow;

	SlGui::NodeDragBehaviour(dragPos, nodeDragFlags);

	// Handle copy/cut
	const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ExplorerEntryDragType, dragFlags);
	if (!payload)
		return;

	ExplorerEntryDragData* dragData = *static_cast<ExplorerEntryDragData**>(payload->Data);
	if (dragPos == SlGuiNodeDragPosition_Center) // Copy / Cut
	{
		switch (entry->GetEntryType())
		{
		case ExplorerEntryType_User:
		{
			ExplorerEntryUser* targetUserEntry = dynamic_cast<ExplorerEntryUser*>(entry.get());
			for (ExplorerEntryPtr& dragEntry : dragData->Entries)
				targetUserEntry->AddChildren(dragEntry);
		}
		break;
		default:
			AM_UNREACHABLE("File / Folder copy is not supported yet.");
		}
	}
	else // Changed entry position
	{
		ExplorerEntryUser* targetUserParent = dynamic_cast<ExplorerEntryUser*>(entry->GetParent());
		AM_ASSERT(targetUserParent, "Drag insert is only supported for EntryUser!");

		// Remove entry from old position
		if (dragData->Cut)
		{
			// We assume here that all entries have the same parent
			ExplorerEntryUser* sourceUserParent = dynamic_cast<ExplorerEntryUser*>(dragData->Entries[0]->GetParent());
			AM_ASSERT(sourceUserParent, "Drag insert is only supported for EntryUser!");
			for (ExplorerEntryPtr& dragEntry : dragData->Entries)
				sourceUserParent->RemoveChildren(dragEntry);
		}

		// First / Last
		u16 dragTargetIndex = targetEntryParent->GetIndexFromHash(entry->GetHashKey());
		u16 dragInsertIndex;
		if (dragPos == SlGuiNodeDragPosition_Above)
		{
			dragInsertIndex = dragTargetIndex;
		}
		else
		{
			dragInsertIndex = dragTargetIndex + 1;
		}

		AM_DEBUGF("Inserting (%u) item(s) at index %u", dragInsertIndex, dragData->Entries.GetSize());

		for (ExplorerEntryPtr& dragEntry : dragData->Entries)
			targetUserParent->InsertChildren(dragInsertIndex, dragEntry);
	}

	ImGui::EndDragDropTarget();
}

u16 rageam::ui::ExplorerEntryBase::GetIndexFromID(u16 id) const
{
	for (u16 i = 0; i < m_Children.GetSize(); i++)
	{
		if (m_Children[i]->GetID() == id)
			return i;
	}
	AM_UNREACHABLE("ExplorerEntryBase::GetIndexFromID(%u) -> ID is invalid.", id);
}

u16 rageam::ui::ExplorerEntryBase::GetIndexFromHash(u32 hash) const
{
	for (u16 i = 0; i < m_Children.GetSize(); i++)
	{
		if (m_Children[i]->GetHashKey() == hash)
			return i;
	}
	AM_UNREACHABLE("ExplorerEntryBase::GetIndexFromHash(%u) -> Hash is invalid.", hash);
}

u16 rageam::ui::ExplorerEntryBase::TransformToSorted(u16 index)
{
	for (u16 i = 0; i < m_Children.GetSize(); i++)
	{
		if (m_SortedIndexToEntry[i] == index)
			return i;
	}
	AM_UNREACHABLE("ExplorerEntryBase::GetSortedIndex(%u) -> Index is invalid.", index);
}

u16 rageam::ui::ExplorerEntryBase::TransformFromSorted(u16 index)
{
	return m_SortedIndexToEntry[index];
}

rageam::ui::ExplorerEntryPtr& rageam::ui::ExplorerEntryBase::GetChildFromIndex(u16 index)
{
	return m_Children[index];
}

rageam::ui::ExplorerEntryPtr& rageam::ui::ExplorerEntryBase::GetChildFromID(u16 id)
{
	for (u16 i = 0; i < m_Children.GetSize(); i++)
	{
		if (m_Children[i]->GetID() == id)
			return m_Children[i];
	}
	AM_UNREACHABLE("ExplorerEntryBase::GetIndexFromID(%u) -> ID is invalid.", id);
}

rageam::ui::ExplorerEntryPtr& rageam::ui::ExplorerEntryBase::GetSortedChildFromIndex(u16 index)
{
	return m_Children[TransformFromSorted(index)];
}

void rageam::ui::ExplorerEntryBase::Sort(ImGuiTableSortSpecs* specs)
{
	if (GetChildCount() == 0)
		return;

	if (!specs->SpecsDirty)
		return;

	static ImGuiTableSortSpecs* currentSortSpecs = nullptr;
	auto sortPredicate = [this](const u16& l, const u16& r) -> bool
		{
			const ExplorerEntryPtr lhs = GetChildFromIndex(l);
			const ExplorerEntryPtr rhs = GetChildFromIndex(r);

			// Group by directory / file first
			if (lhs->IsDirectory() != rhs->IsDirectory())
			{
				return lhs->IsDirectory() ? true : false;
			}

			for (int i = 0; i < currentSortSpecs->SpecsCount; i++)
			{
				const ImGuiTableColumnSortSpecs* sortSpec = &currentSortSpecs->Specs[i];
				int delta;
				switch (sortSpec->ColumnUserID)
				{
				case ExplorerEntryColumnID_Name:			delta = strcmp(lhs->GetName(), rhs->GetName()); break;
				case ExplorerEntryColumnID_DateModified:	delta = (lhs->GetTime() - rhs->GetTime()).GetTicks(); break;
				case ExplorerEntryColumnID_TypeName:		delta = strcmp(lhs->GetTypeName(), rhs->GetTypeName()); break;
				case ExplorerEntryColumnID_Size:			delta = (int)lhs->GetSize() - (int)rhs->GetSize(); break;
				default: AM_UNREACHABLE("ExplorerEntrySortFn() -> Column sorting (%u) is not implemented.", sortSpec->ColumnUserID);
				}

				if (delta > 0)
					return sortSpec->SortDirection == ImGuiSortDirection_Ascending ? true : false;
				if (delta < 0)
					return sortSpec->SortDirection == ImGuiSortDirection_Ascending ? false : true;
			}
			return lhs->GetID() < rhs->GetID(); // Default sort by ID
		};

	currentSortSpecs = specs;

	std::ranges::sort(m_SortedIndexToEntry, sortPredicate);

	specs->SpecsDirty = false;
}

void rageam::ui::ExplorerEntryBase::SetUserData(u32 key, u32 value)
{
	m_UserData.InsertAt(key, value);
}

u32 rageam::ui::ExplorerEntryBase::GetUserData(u32 key)
{
	u32* value = m_UserData.TryGetAt(key);
	if (value)
		return *value;
	return 0;
}

void rageam::ui::ExplorerEntryFi::ScanSubFolders()
{
	m_HasSubFolders = false;

	if (!m_IsDirectory)
		return;

	rage::fiIterator iterator(m_Path, m_Device);
	while (iterator.Next())
	{
		if (iterator.IsDirectory())
		{
			m_HasSubFolders = true;
			break;
		}
	}
}

void rageam::ui::ExplorerEntryFi::SetPath(const file::U8Path& path, rage::fiDevice* parentDevice)
{
	m_Path = path;
	m_HashKey = rage::atHashString(m_Path);

	m_Name = m_Path.GetFileNameWithoutExtension();

	m_FullName = file::GetFileName(m_Path.GetCStr());
	m_Type = file::GetExtension(m_Path.GetCStr());

	// This is special case for drive names 'C:/'
	if (String::IsNullOrEmpty(m_Name))
	{
		m_Name = m_Path;
		m_FullName = m_Path;
	}

	// NOTE:
	// GetDeviceImpl is very slow (especially when we're dealing with lot of files)
	// basically what we can do is to check if device of parent entry contains this path
	// and it's much faster thing to do. If not - then given new path is within different RPF/device
	if (parentDevice && parentDevice->GetAttributes(path) != FI_INVALID_ATTRIBUTES)
		m_Device = parentDevice;
	else
		m_Device = rage::fiDevice::GetDeviceImpl(m_Path);

	AM_ASSERT(m_Device, "ExplorerEntryFi::SetPath() -> Failed to get device.");

	Refresh();
}

void rageam::ui::ExplorerEntryFi::UpdateIcon()
{
	auto SetIcon = [this](ConstString name)
		{
			m_StaticIcon = GetUI()->GetIcon(name);
			return m_StaticIcon != nullptr;
		};

	ConstString type = GetType();

	if (m_IconOverride)
	{
		SetIcon(m_IconOverride);
		return;
	}

	// Retrieve 'dynamic' icon for image
	if (graphics::ImageFactory::IsSupportedImageFormat(String::ToWideTemp(type)))
	{
		// TODO: How do we pick max resolution correctly? ...
		m_DynamicIcon.Load(PATH_TO_WIDE(GetPath()), 16);
		return;
	}

	// TODO: Better interface to register custom icons for specific folders
	if (String::Equals("Grand Theft Auto V", m_Name))
	{
		// TODO: We need to force 256 icon size here because 16 looks ugly
		m_StaticIcon = GetUI()->GetIcon("GTAV");
		return;
	}

	// Try to retrieve extension icon
	// Type might be empty string if we're working with folder
	if (String::IsNullOrEmpty(type) || !SetIcon(type))
	{
		// There's no static icon for specified type, retrieve default one
		SetIcon(IsDirectory() ? "folder" : "file");
	}
}

rageam::ui::ExplorerEntryFi::ExplorerEntryFi(const file::U8Path& path, ExplorerEntryFlags flags, rage::fiDevice* parentDevice)
{
	ExplorerEntryFi::SetFlags(flags);
	SetPath(path, parentDevice);
}

void rageam::ui::ExplorerEntryFi::Refresh()
{
	file::WPath wPath = file::PathConverter::Utf8ToWide(m_Path);

	m_Attributes = m_Device->GetAttributes(m_Path);
	m_IsDirectory = m_Attributes & FI_ATTRIBUTE_DIRECTORY;

	// For some reason WinApi can return unrelated size for folders?
	m_Size = m_IsDirectory ? 0 : static_cast<u32>(m_Device->GetFileSize(m_Path));

	m_TimeModified = DateTime(m_Device->GetFileTime(m_Path)).ToLocalTime(); // Additionally convert to local cuz file time is UTC
	RefreshDisplayInfo();

	m_IsAsset = asset::AssetFactory::IsAsset(file::PathConverter::Utf8ToWide(m_Path));

	// Since projects are in fact just folders we have to override their name in explorer to prevent confusion
	if (m_IsAsset)
	{
		String::Copy(m_TypeName, TYPE_MAX_NAME, asset::AssetFactory::GetAssetKindName(wPath));
	}
	else
	{
		wchar_t typeNameBuffer[TYPE_MAX_NAME];
		GetDisplayTypeName(typeNameBuffer, TYPE_MAX_NAME, wPath, m_Attributes);
		String::WideToUtf8(m_TypeName, TYPE_MAX_NAME, typeNameBuffer);
	}

	m_HasSubFoldersDirty = true;
	m_IconDirty = true;
}

void rageam::ui::ExplorerEntryFi::RefreshDisplayInfo()
{
	m_DisplayInfoDirty = true;
}

void rageam::ui::ExplorerEntryFi::PrepareToBeDisplayed()
{
	if (m_IconDirty)
	{
		UpdateIcon();
		m_IconDirty = false;
	}

	if (m_HasSubFoldersDirty)
	{
		ScanSubFolders();
		m_HasSubFoldersDirty = false;
	}

	if (m_DisplayInfoDirty)
	{
		m_TimeModified.FormatTimeSince(m_TimeModifiedFormatted, MAX_TIME);
		m_DisplayInfoDirty = false;
	}
}

void rageam::ui::ExplorerEntryFi::LoadChildren()
{
	if (m_ChildrenLoaded) return;

	u16 index = 0;
	rage::fiIterator iterator(m_Path, m_Device);
	while (iterator.Next())
	{
		ConstString path = iterator.GetFilePath();

		ExplorerEntryPtr& child = m_Children.Construct(new ExplorerEntryFi(path, 0, m_Device));
		child->SetID(index);
		child->SetParent(this);

		m_SortedIndexToEntry.Add(index);
		index++;
	}

	m_ChildrenLoaded = true;
}

void rageam::ui::ExplorerEntryFi::UnloadChildren()
{
	if (!m_ChildrenLoaded) return;

	m_Children.Clear();
	m_SortedIndexToEntry.Clear();
	m_ChildrenLoaded = false;
}

bool rageam::ui::ExplorerEntryFi::Rename(ConstString newName)
{
	// Option must be blocked in UI
	AM_ASSERT(!(GetFlags() & ExplorerEntryFlags_NoRename), "Entry cannot be renamed.");

	file::U8Path newPath = m_Path.GetParentDirectory();

	// Construct path with new name
	newPath /= newName;
	newPath += ".";
	newPath += m_Type;

	// TODO: Invalid file name message in status bar
	if (!m_Device->Rename(m_Path, newPath))
	{
		AM_ERRF("ExplorerEntryFi::Rename(%s) -> Unable to rename file to %s", GetName(), newName);
		return false;
	}

	SetPath(newPath, m_Device);
	return true;
}

void rageam::ui::ExplorerEntryFi::SetIconOverride(ConstString name)
{
	m_IconOverride = name;
	m_IconDirty = true;
}

rageam::asset::AssetPtr rageam::ui::ExplorerEntryFi::GetAsset()
{
	AM_ASSERT(m_IsAsset, "ExplorerEntryFi::GetAsset() -> Entry is not asset!");

	// Load asset on request
	if (m_IsAsset && !m_Asset)
		m_Asset = asset::AssetFactory::LoadFromPath(file::PathConverter::Utf8ToWide(m_Path));

	return m_Asset;
}

void rageam::ui::ExplorerEntryUser::ScanSubDirs()
{
	m_HasSubDirs = false;
	for (const ExplorerEntryPtr& entry : m_Children)
	{
		if (entry->IsDirectory())
		{
			m_HasSubDirs = true;
			return;
		}
	}
}

bool rageam::ui::ExplorerEntryUser::Rename(ConstString newName)
{
	AM_ASSERT(!(GetFlags() & ExplorerEntryFlags_NoRename), "Entry cannot be renamed.");

	String::Copy(m_Name, MAX_USER_NAME, newName);
	m_HashKey = rage::atStringHash(m_Name);
	return true;
}

rageam::ui::ExplorerEntryPtr& rageam::ui::ExplorerEntryUser::InsertChildren(u16 index, const ExplorerEntryPtr& entry)
{
	entry->SetParent(this);
	entry->SetID(m_Children.GetSize());
	ExplorerEntryPtr& child = m_Children.Insert(index, entry);
	ScanSubDirs();

	// Rebuild indices
	m_SortedIndexToEntry.Clear();
	for (u16 i = 0; i < m_Children.GetSize(); i++)
		m_SortedIndexToEntry.Add(i);

	return child;
}

rageam::ui::ExplorerEntryPtr& rageam::ui::ExplorerEntryUser::AddChildren(const ExplorerEntryPtr& entry)
{
	return InsertChildren(GetChildCount(), entry);
}

void rageam::ui::ExplorerEntryUser::RemoveChildren(const ExplorerEntryPtr& entry)
{
	u16 index = GetIndexFromHash(entry->GetHashKey());
	RemoveChildrenAtIndex(index);
}

void rageam::ui::ExplorerEntryUser::RemoveChildrenAtIndex(u16 index)
{
	m_Children.RemoveAt(index);
	ScanSubDirs();
}

//
//void rageam::ui::ExplorerEntryFi::RenderProperties()
//{
//	m_Icon->Render(48);
//	SlGui::RenderGloss(GImGui->LastItemData.Rect, SlGuiCol_GlossBg);
//
//	ImGui::SameLine();
//	ImGui::Text("%s", m_Name);
//	ImGui::SnapToPrevious();
//	ImGui::TextDisabled("%s", m_TypeName);
//	ImGui::SnapToPrevious();
//	ImGui::TextDisabled("Date modified: %s", m_TimeModifiedFormatted);
//}
//
//void rageam::ui::ExplorerEntryFi::RenderPreview()
//{
//	/*if (IsProject())
//	{
//		GetProject()->RenderPreview();
//	}
//	else if (IsProjectFile())
//	{
//		amPtr<ProjectFile> file = GetParentProject()->GetFile(m_Name);
//		if (file)
//			file->RenderPreview();
//	}*/
//}
//
//void rageam::ui::ExplorerEntryFi::Save()
//{
//	//// TODO: Should we really do it here? Seems very weird...
//	//if (IsProject()) GetProject()->Save();
//	//if (IsProjectFile()) GetParentProject()->Save();
//}
