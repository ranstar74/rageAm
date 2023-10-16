#include "factory.h"
#include "types/drawable.h"
#include "types/txd.h"

rage::atMap<ConstWString, rageam::asset::AssetFactory::AssetDefinition> rageam::asset::AssetFactory::sm_ExtToAssetDef;

const rageam::asset::AssetFactory::AssetDefinition* rageam::asset::AssetFactory::TryGetDefinition(const file::WPath& path)
{
	file::WPath extension = path.GetExtension();
	return sm_ExtToAssetDef.TryGet(extension);
}

void rageam::asset::AssetFactory::Init()
{
	sm_ExtToAssetDef.InitAndAllocate(2); // Extend this as more added

	sm_ExtToAssetDef.Insert(L"itd", AssetDefinition("Texture Dictionary", AssetType_Txd, TxdAsset::Allocate));
	sm_ExtToAssetDef.Insert(L"idr", AssetDefinition("Drawable", AssetType_Drawable, DrawableAsset::Allocate));
}

void rageam::asset::AssetFactory::Shutdown()
{
	sm_ExtToAssetDef.Destroy();
}

rageam::asset::AssetPtr rageam::asset::AssetFactory::LoadFromPath(const file::WPath& path)
{
	const AssetDefinition* def = TryGetDefinition(path);

	if (!AM_VERIFY(def != nullptr, L"AssetFactory::Load() -> Unknown asset type %ls", path.GetCStr()))
		return nullptr;

	AssetBase* asset = def->Create(path);

	if (!AM_VERIFY(asset->LoadConfig(), L"AssetFactory::Load() -> Failed to load config for %ls", path.GetCStr()))
		return nullptr;

	return AssetPtr(asset);
}

bool rageam::asset::AssetFactory::IsAsset(const file::WPath& path)
{
	return TryGetDefinition(path) != nullptr;
}

rageam::asset::eAssetType rageam::asset::AssetFactory::GetAssetType(const file::WPath& path)
{
	const AssetDefinition* def = TryGetDefinition(path);
	return def->Type;
}

ConstString rageam::asset::AssetFactory::GetAssetKindName(const file::WPath& path)
{
	const AssetDefinition* def = TryGetDefinition(path);
	if (!AM_VERIFY(def != nullptr, L"AssetFactory::GetAssetKindName() -> Not asset at path %ls", path.GetCStr()))
		return "None";
	return def->DisplayName;
}
