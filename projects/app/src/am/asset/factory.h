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
#include "types/drawable.h"
#include "types/txd.h"

namespace rageam::asset
{
	class AssetFactory
	{
		struct AssetDefinition
		{
			using CreateFn = AssetBase * (*)(const file::WPath& path);

			ConstString DisplayName;
			CreateFn	Create;
		};

		// Asset path extension to asset definition
		static rage::atMap<ConstWString, AssetDefinition> sm_ExtToAssetDef;

		static const AssetDefinition* TryGetDefinition(const file::WPath& path)
		{
			file::WPath extension = path.GetExtension();
			return sm_ExtToAssetDef.TryGet(extension);
		}
	public:
		static void Init()
		{
			sm_ExtToAssetDef.InitAndAllocate(2); // Extend this as more added

			sm_ExtToAssetDef.Insert(L"itd", AssetDefinition("Texture Dictionary", TxdAsset::Allocate));
			sm_ExtToAssetDef.Insert(L"idr", AssetDefinition("Drawable", DrawableAsset::Allocate));
		}

		static void Shutdown()
		{
			sm_ExtToAssetDef.Destroy();
		}

		static AssetPtr LoadFromPath(const file::WPath& path)
		{
			const AssetDefinition* def = TryGetDefinition(path);

			if (!AM_VERIFY(def != nullptr, L"AssetFactory::Load() -> Unknown asset type %ls", path.GetCStr()))
				return nullptr;

			AssetBase* asset = def->Create(path);

			if (!AM_VERIFY(asset->LoadConfig(), L"AssetFactory::Load() -> Failed to load config for %ls", path.GetCStr()))
				return nullptr;

			return AssetPtr(asset);
		}

		template<typename TAsset>
		static amPtr<TAsset> LoadFromPath(const file::WPath& path)
		{
			return std::dynamic_pointer_cast<TAsset>(LoadFromPath(path));
		}

		static bool IsAsset(const file::WPath& path)
		{
			return TryGetDefinition(path) != nullptr;
		}

		static ConstString GetAssetKindName(const file::WPath& path)
		{
			const AssetDefinition* def = TryGetDefinition(path);
			if (!AM_VERIFY(def != nullptr, L"AssetFactory::GetAssetKindName() -> Not asset at path %ls", path.GetCStr()))
				return "Unknown";
			return def->DisplayName;
		}
	};
}
