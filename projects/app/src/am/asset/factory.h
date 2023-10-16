//
// File: factory.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "gameasset.h"
#include "am/system/ptr.h"

namespace rageam::asset
{
	class AssetFactory
	{
		struct AssetDefinition
		{
			using CreateFn = AssetBase * (*)(const file::WPath& path);

			ConstString DisplayName;
			eAssetType	Type;
			CreateFn	Create;
		};

		// Asset path extension to asset definition
		static rage::atMap<ConstWString, AssetDefinition> sm_ExtToAssetDef;
		// static rage::atSet<AssetPtr> sm_AssetCache;

		static const AssetDefinition* TryGetDefinition(const file::WPath& path);

	public:
		static void Init();
		static void Shutdown();

		static AssetPtr LoadFromPath(const file::WPath& path);

		template<typename TAsset>
		static amPtr<TAsset> LoadFromPath(const file::WPath& path)
		{
			return std::dynamic_pointer_cast<TAsset>(LoadFromPath(path));
		}

		static bool IsAsset(const file::WPath& path);

		static eAssetType GetAssetType(const file::WPath& path);
		static ConstString GetAssetKindName(const file::WPath& path);
	};
}
