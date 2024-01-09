#include "assetwindowfactory.h"

#include "txdwindow.h"
#include "am/ui/imglue.h"

void rageam::ui::AssetWindowFactory::MakeAssetWindowName(char* destination, const asset::AssetPtr& asset)
{
	file::WPath assetName = asset->GetDirectoryPath().GetFileName();
	sprintf_s(destination, MAX_ASSET_WINDOW_NAME, "%ls", assetName.GetCStr());
}

rageam::ui::WindowPtr rageam::ui::AssetWindowFactory::OpenNewOrFocusExisting(const asset::AssetPtr& asset)
{
	asset->Refresh();

	WindowManager* windows = GetUI()->Windows;

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
		AM_UNREACHABLE("AssetWindowFactory -> Asset type '%s' is not implemented.",
			Enum::GetName(asset->GetType()));
	}

	return windows->Add(window);
}

void rageam::ui::AssetWindowFactory::Shutdown()
{
	TextureVM::ShutdownClass();
}
