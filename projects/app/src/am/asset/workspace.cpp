#include "workspace.h"

#include "factory.h"
#include "am/file/iterator.h"
#include "am/string/stringwrapper.h"
#include "types/drawable.h"

void rageam::asset::Workspace::ScanRecurse(ConstWString path)
{
	file::WPath searchPath = path;
	searchPath /= L"*";

	file::Iterator iterator(searchPath);
	file::FindData findData;
	while (iterator.Next())
	{
		iterator.GetCurrent(findData);

		if (!file::IsDirectory(path))
			continue;

		if (!AssetFactory::IsAsset(findData.Path))
		{
			ScanRecurse(findData.Path);
			continue;
		}

		eAssetType assetType = AssetFactory::GetAssetType(findData.Path);

		// Count totals
		if (assetType == AssetType_Txd)			m_TotalTDs++;
		if (assetType == AssetType_Drawable)	m_TotalDRs++;

		// Check if asset type is set in flags
		if ((m_Flags & WF_LoadTx) == 0 && assetType == AssetType_Txd) continue;
		if ((m_Flags & WF_LoadDr) == 0 && assetType == AssetType_Drawable) continue;

		AssetPtr asset = AssetFactory::LoadFromPath(findData.Path);
		if (!asset)
		{
			m_FailedCount++;
			continue;
		}

		u16 assetIndex = m_Assets.GetSize();
		switch (asset->GetType())
		{
		case AssetType_Txd:			m_TDs.Add(assetIndex); break;
		case AssetType_Drawable:	m_DRs.Add(assetIndex); break;
		default: break;
		}

		m_Assets.Emplace(std::move(asset));
	}
}

rageam::asset::Workspace::Workspace(ConstWString path, eWorkspaceFlags flags)
{
	m_Path = path;
	m_Flags = flags;
	Refresh();
}

void rageam::asset::Workspace::Refresh()
{
	m_Assets.Clear();
	m_TDs.Clear();
	m_DRs.Clear();
	m_TotalTDs = 0;
	m_TotalDRs = 0;
	m_FailedCount = 0;
	ScanRecurse(m_Path);
	if (m_FailedCount != 0)
		AM_ERRF("AssetWorkspace::Refresh() -> Failed to load %u assets.", m_FailedCount);
}

amPtr<rageam::asset::TxdAsset> rageam::asset::Workspace::GetTexDict(u16 index) const
{
	return std::reinterpret_pointer_cast<TxdAsset>(m_Assets[m_TDs[index]]);
}

amPtr<rageam::asset::DrawableAsset> rageam::asset::Workspace::GetDrawable(u16 index) const
{
	return std::reinterpret_pointer_cast<DrawableAsset>(m_Assets[m_DRs[index]]);
}

amPtr<rageam::asset::Workspace> rageam::asset::Workspace::FromAssetPath(ConstWString assetPath, eWorkspaceFlags flags)
{
	file::WPath workspacePath;
	if (GetParentWorkspacePath(assetPath, workspacePath))
		return std::make_unique<Workspace>(workspacePath, flags);
	return nullptr;
}

amPtr<rageam::asset::Workspace> rageam::asset::Workspace::FromPath(ConstWString workspacePath, eWorkspaceFlags flags)
{
	return std::make_unique<Workspace>(workspacePath, flags);
}

bool rageam::asset::Workspace::IsInWorkspace(ConstWString filePath)
{
	file::WPath path = filePath;
	while (!String::IsNullOrEmpty(path))
	{
		// Note that we first get parent directory because we don't want to check against file/dir specified in path
		path = path.GetParentDirectory();

		// Directory/file has no extension, keep searching...
		if (String::IsNullOrEmpty(path.GetExtension()))
			continue;

		return ImmutableWString(path).EndsWith(WORKSPACE_EXT, true);
	}
	return false;
}

bool rageam::asset::Workspace::GetParentWorkspacePath(ConstWString assetPath, file::WPath& outPath)
{
	int extIndex = StringWrapper(assetPath).IndexOf(WORKSPACE_EXT);
	if (extIndex == -1)
	{
		outPath = L"";
		return false;
	}

	extIndex += 3; // To include '.ws'

	// Copy and trim path
	wchar_t buffer[MAX_PATH];
	String::Copy(buffer, MAX_PATH, assetPath);
	buffer[extIndex] = '\0';
	outPath = buffer;

	return true;
}

bool rageam::asset::Workspace::IsWorkspace(ConstWString path)
{
	return StringWrapper(path).EndsWith(WORKSPACE_EXT);
}
