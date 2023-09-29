#pragma once

#include "assethelper.h"
#include "am/asset/gameasset.h"
#include "am/ui/context.h"
#include "am/ui/apps/windowmgr.h"
#include "txd/txdwindow.h"

namespace rageam::ui::assetview
{
	class AssetWindowFactory
	{
	public:
		static WindowPtr OpenNewOrFocusExisting(const asset::AssetPtr& asset)
		{
			asset->Refresh();

			WindowManager* windows = Gui->Windows;

			char title[MAX_ASSET_WINDOW_NAME];
			MakeAssetWindowName(title, asset);

			// Try to find existing opened window
			WindowPtr existing = windows->FindByTitle(title);
			if (existing)
			{
				windows->Focus(existing);
				return existing;
			}

			// Open new window
			Window* window;
			switch (asset->GetType())
			{
			case asset::AssetType_Txd:
				window = new TxdWindow(std::dynamic_pointer_cast<asset::TxdAsset>(asset));
				break;

			default:
				AM_UNREACHABLE("Asset type is not implemented.");
			}

			return windows->Add(window);
		}
	};
}
