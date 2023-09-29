#pragma once

#include "am/asset/gameasset.h"

namespace rageam::ui::assetview
{
	static constexpr int MAX_ASSET_WINDOW_NAME = 64;

	/**
	 * \brief Formats title for asset window (AssetApp::GetTitle).
	 * \param destination	Buffer where title will be written to, use MAX_ASSET_WINDOW_NAME for size.
	 * \param asset			Asset to generate window tittle for.
	 * \remarks				Generated title only depends on asset name, this was made to find existing asset window.
	 */
	inline void MakeAssetWindowName(char* destination, const asset::AssetPtr& asset)
	{
		file::WPath assetName = asset->GetDirectoryPath().GetFileName();

		sprintf_s(destination, MAX_ASSET_WINDOW_NAME, "%ls", assetName.GetCStr());
	}
}
