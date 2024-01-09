//
// File: assetwindowfactory.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/asset/gameasset.h"
#include "am/ui/window.h"

namespace rageam::ui
{
	class AssetWindowFactory
	{
	public:
		static constexpr int MAX_ASSET_WINDOW_NAME = 64;

		/**
		 * \brief Formats title for asset window (AssetApp::GetTitle).
		 * \param destination	Buffer where title will be written to, use MAX_ASSET_WINDOW_NAME for size.
		 * \param asset			Asset to generate window tittle for.
		 * \remarks				Generated title only depends on asset name, this was made to find existing asset window.
		 */
		static void MakeAssetWindowName(char* destination, const asset::AssetPtr& asset);
		static WindowPtr OpenNewOrFocusExisting(const asset::AssetPtr& asset);
		static void Shutdown();
	};
}
