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

		// Check if asset type is set in flags
		eAssetType assetType = AssetFactory::GetAssetType(findData.Path);
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
	m_FailedCount = 0;
	ScanRecurse(m_Path);
	if (m_FailedCount != 0)
		AM_ERRF("AssetWorkspace::Refresh() -> Failed to load %u assets.", m_FailedCount);
}

const amPtr<rageam::asset::TxdAsset>& rageam::asset::Workspace::GetTexDict(u16 index) const
{
	return std::reinterpret_pointer_cast<TxdAsset>(m_Assets[m_TDs[index]]);
}

const amPtr<rageam::asset::DrawableAsset>& rageam::asset::Workspace::GetDrawable(u16 index) const
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

bool rageam::asset::Workspace::GetParentWorkspacePath(ConstWString assetPath, file::WPath& outPath)
{
	int extIndex = StringWrapper(assetPath).LastIndexOf(WORKSPACE_EXT);
	if (extIndex == -1)
		return false;

	extIndex += 5; // To include '.pack'

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
