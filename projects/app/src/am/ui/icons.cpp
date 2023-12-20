#include "icons.h"

#include "am/file/fileutils.h"
#include "am/file/iterator.h"
#include "am/system/datamgr.h"
#include "rage/crypto/joaat.h"

rageam::ui::Icons::Icons()
{
	file::Iterator it(DataManager::GetIconsFolder() / L"*.*");
	file::FindData entry;
	while (it.Next())
	{
		it.GetCurrent(entry);

		u32 nameHash = rage::joaat(entry.Path.GetFileNameWithoutExtension());
		Icon& icon = m_Icons.ConstructAt(nameHash);

		icon.Images[0].LoadAsync(entry.Path);
	}
}

rageam::ui::AsyncImage* rageam::ui::Icons::GetIcon(ConstString name, eIconSize size) const
{
	Icon* icon = m_Icons.TryGetAt(rage::joaat(name));
	if (!icon)
		return nullptr;

	//if (!icon->IsIco)
	//	return &icon->Images[0]; // PNG only has single image
	return &icon->Images[size]; // Retrieve corresponding mip-map
}
