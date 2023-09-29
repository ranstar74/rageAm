#include "icons.h"

#include "am/file/fileutils.h"
#include "am/file/iterator.h"
#include "am/graphics/texture/ico.h"
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

		switch (rage::joaat(entry.Path.GetExtension()))
		{
		case rage::joaat("png"):
			// For png we don't really need different mip-maps so just reuse first image
			icon.Images[0].LoadAsync(entry.Path, 0, false);
			break;
		case rage::joaat("ico"):
		{
			// Ico contains user-generated mip maps, load all available ones
			icon.IsIco = true;
			file::WPath icoPath = entry.Path;
			BackgroundWorker::Run([icoPath, &icon]
				{
					texture::Ico ico;
					if (!ico.Load(icoPath))
						return false;

					for (int i = 0; i < IconSize_COUNT; i++)
					{
						int resolution = sm_IconSizes[i];
						icon.Images[i].SetView(ico.CreateView(resolution));
					}
					return true;
				});
		}
		break;
		default:
			AM_WARNINGF(L"Not supported icon file (%ls)", entry.Path.GetCStr());
			continue;
		}
	}
}

rageam::ui::Image* rageam::ui::Icons::GetIcon(ConstString name, eIconSize size) const
{
	Icon* icon = m_Icons.TryGetAt(rage::joaat(name));
	if (!icon)
		return nullptr;

	if (!icon->IsIco)
		return &icon->Images[0]; // PNG only has single image
	return &icon->Images[size]; // Retrieve corresponding mip-map
}
