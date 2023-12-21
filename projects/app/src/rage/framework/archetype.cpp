#include "archetype.h"

#include "animmanager.h"
#include "game/modelinfo.h"
#include "rage/streaming/assetstores.h"
#include "rage/rmc/drawable.h"

rage::fwDynamicArchetypeComponent* rage::fwArchetype::CreateDynamicComponentIfMissing()
{
	// TODO: Need atPool
	static auto fn = gmAddress::Scan("48 89 5C 24 08 57 48 83 EC 20 33 FF 48 8B D9 48 39 79 58").ToFunc<
		fwDynamicArchetypeComponent * (fwArchetype*)>();
	return fn(this);
}

void rage::fwArchetype::DestroyDynamicComponent()
{
	// TODO: Need atPool
	static auto fn = gmAddress::Scan("40 53 48 83 EC 20 48 8B 51 58 48").ToFunc<void(fwArchetype*)>();
	fn(this);
}

void rage::fwArchetype::UpdateBoundingVolumes(phBound* bound)
{
	static auto fn = gmAddress::Scan("48 83 EC 38 66 0F 70 51").ToFunc<void(fwArchetype*, phBound*)>();
	fn(this, bound);
}

void rage::fwArchetype::TryCreateCustomShaderEffect()
{
	static auto fn = gmAddress::Scan("40 53 48 83 EC 20 48 8B D9 E8 ?? ?? ?? ?? 48 85 C0 74 0E").ToFunc<void(fwArchetype*)>();
	fn(this);
}

rage::fwArchetype::~fwArchetype()
{
	static auto fn = gmAddress::Scan("40 53 48 83 EC 20 48 8D 05 ?? ?? ?? ?? 48 8B D9 48 89 01 B8 FF FF 00 00")
		.ToFunc<void(fwArchetype*)>();
	fn(this);

	//if(m_ModelIndex != u16(-1))
	//{
	//	DestroyDynamicComponent();
	//}
}

void rage::fwArchetype::Init()
{
	m_AssetType = ASSET_TYPE_UNINITIALIZED;
	m_Flags &= 0x80000000;
	m_DwdIndex = -1;
	m_AssetIndex = -1;
	m_RefCount = 0;

	m_BoundingMin = {};
	m_BoundingMax = {};
	m_BsCentre = {};
	m_BsRadius = 0.0f;
	m_LodDist = 100.0f;
}

void rage::fwArchetype::InitArchetypeFromDefinition(strLocalIndex slot, fwArchetypeDef* def, bool loadModels)
{
	Init();

	m_NameHash = def->Name;
	m_LodDist = def->LodDist;
	m_BoundingMin = def->BoundingBox.Min;
	m_BoundingMax = def->BoundingBox.Max;
	m_BsCentre = def->BsCentre;
	m_BsRadius = def->BsRadius;
	m_HdTextureDist = def->HdTextureDist;

	SetClipDictionary(def->ClipDictionary);
	if (def->PhysicsDictionary)
	{
		CreateDynamicComponentIfMissing();
		m_DynamicComponent->SetPhysicsEnabled(true);
	}

	if (loadModels)
	{
		// Lookup TXD
		strLocalIndex txdSlot = INVALID_STR_INDEX;
		if (!atIsNullHash(def->TextureDictionary))
			txdSlot = GetTxdStore()->FindSlotFromHashKey(def->TextureDictionary);

		if (atIsNullHash(def->DrawableDictionary))
		{
			SetDrawableOrFragmentFile(def->Name, txdSlot, false);
		}
		else
		{
			SetDrawableDictionaryFile(def->DrawableDictionary, txdSlot);
		}
	}
	else
	{
		m_AssetType = ASSET_TYPE_ASSETLESS;
	}

	// TODO: RegisterPermanentArchetype for MapTypesStore
	CModelInfo::RegisterModelInfo(this, slot);
}

void rage::fwArchetype::Shutdown()
{
	static auto fn = gmAddress::Scan("E8 ?? ?? ?? ?? 48 8B 03 48 8B CB FF 90 ?? ?? ?? ?? 48 8B 03 48 8B CB FF 90")
		.GetCall()
		.ToFunc<void(fwArchetype*)>();
	fn(this);
}

void rage::fwArchetype::SetPhysics(const pgPtr<phArchetype>& physics)
{
	if (!physics)
	{
		if (m_DynamicComponent) m_DynamicComponent->Physics = physics;
		return;
	}

	CreateDynamicComponentIfMissing();
	m_DynamicComponent->Physics = physics;

	const phBoundPtr& bound = physics->GetBound();
	if (bound) UpdateBoundingVolumes(bound.Get());
}

bool rage::fwArchetype::SetDrawableOrFragmentFile(u32 nameHash, strLocalIndex txdSlot, bool skipDrawable)
{
	m_AssetType = ASSET_TYPE_UNINITIALIZED;
	m_AssetIndex = INVALID_STR_INDEX;

	if (atIsNullHash(nameHash))
		return false;

	auto tryGetFromStore = [&](auto store, eAssetType assetType) -> bool
		{
			strLocalIndex slot = store->FindSlotFromHashKey(nameHash);
			if (slot == INVALID_STR_INDEX)
				return false;

			m_AssetType = assetType;
			m_AssetIndex = slot;

			if (txdSlot != INVALID_STR_INDEX)
				store->GetDef(slot)->DependencyTXD = txdSlot;

			return true;
		};

	// Try drawable...
	if (!skipDrawable && tryGetFromStore(GetDrawableStore(), ASSET_TYPE_DRAWABLE))
		return true;

	// Try fragment...
	if (tryGetFromStore(GetFragmentStore(), ASSET_TYPE_FRAGMENT))
		return true;

	// Not in drawable/fragment stores, model doesn't exists
	return false;
}

bool rage::fwArchetype::SetDrawableDictionaryFile(u32 nameHash, strLocalIndex txdSlot)
{
	m_AssetType = ASSET_TYPE_UNINITIALIZED;
	m_AssetIndex = INVALID_STR_INDEX;

	if (atIsNullHash(nameHash))
		return false;

	auto store = GetDrawableDictionaryStore();

	strLocalIndex slot = store->FindSlotFromHashKey(nameHash);
	if (slot == INVALID_STR_INDEX)
		return false;

	m_AssetType = ASSET_TYPE_DRAWABLEDICTIONARY;
	m_AssetIndex = slot;

	if (txdSlot != INVALID_STR_INDEX)
		store->GetDef(slot)->DependencyTXD = txdSlot;

	return true;
}

void rage::fwArchetype::SetClipDictionary(u32 clipDictNameHash)
{
	if (atIsNullHash(clipDictNameHash))
	{
		if (m_DynamicComponent)
			m_DynamicComponent->ClipDictionarySlot = INVALID_STR_INDEX;
	}
	else
	{
		// Note: slot truncated to 16 bits, gives us total 65 535 clip dictionaries
		CreateDynamicComponentIfMissing();
		m_DynamicComponent->ClipDictionarySlot =
			static_cast<u16>(fwAnimManager::FindSlotFromHashKey(clipDictNameHash));
	}
}

rage::rmcDrawable* rage::fwArchetype::GetDrawable() const
{
	if ((m_Flags & 0x1) == false)
		return nullptr;

	switch (m_AssetType)
	{
	case ASSET_TYPE_DRAWABLE:
	{
		return (rmcDrawable*)GetDrawableStore()->GetPtr(m_AssetIndex);
	}

	case ASSET_TYPE_DRAWABLEDICTIONARY:
	{
		auto drawDict = (pgDictionary<rmcDrawable>*)GetDrawableDictionaryStore()->GetPtr(m_AssetIndex);
		return drawDict->GetValueAt(m_DwdIndex);
	}

	case ASSET_TYPE_FRAGMENT:
	{
		AM_ERRF("fwArchetype::GetDrawable() -> Fragments are not implemented.");
		break;
	}

	default: break;
	}

	return nullptr;
}
