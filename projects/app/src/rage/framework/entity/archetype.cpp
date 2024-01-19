#include "archetype.h"

#include "archetypemanager.h"
#include "rage/framework/streaming/assetstores.h"
#include "rage/rmc/drawable.h"

rage::fwDynamicArchetypeComponent& rage::fwArchetype::CreateDynamicComponentIfMissing()
{
	if (!m_DynamicComponent)
		m_DynamicComponent = new fwDynamicArchetypeComponent();
	return *m_DynamicComponent;
}

void rage::fwArchetype::DestroyDynamicComponent()
{
	delete m_DynamicComponent;
	m_DynamicComponent = nullptr;
}

rage::fwArchetype::~fwArchetype()
{
	if (m_ModelIndex != fwArchetypeManager::INVALID_STREAM_SLOT)
	{
		DestroyDynamicComponent();
		fwArchetypeManager::UnregisterStreamedArchetype(this);
	}
}

void rage::fwArchetype::Init()
{
	m_DrawableType = DT_UNINITIALIZED;
	m_Flags = 0;
	m_DwdIndex = -1;
	m_DrawableIndex = -1;
	m_RefCount = 0;

	m_BoundingMin = {};
	m_BoundingMax = {};
	m_BsCentre = {};
	m_BsRadius = 0.0f;
	m_LodDist = 100.0f;
}

void rage::fwArchetype::InitArchetypeFromDefinition(strLocalIndex mapTypesSlot, fwArchetypeDef* def, bool loadModels)
{
	Init();

	m_ModelName = def->Name;
	m_LodDist = def->LodDist;
	m_BoundingMin = def->BoundingBox.Min;
	m_BoundingMax = def->BoundingBox.Max;
	m_BsCentre = def->BsCentre;
	m_BsRadius = def->BsRadius;
	m_HdTextureDist = def->HdTextureDist;

	// Game implementation requires map types, but it's not really required for archetype to work...
	fwMapTypesDef* mapTypeDef = GetMapTypesStore()->GetSlot(mapTypesSlot);
	// AM_ASSERTS(mapTypeDef);
	if (mapTypeDef && mapTypeDef->Flags.IsPermanent)
		fwArchetypeManager::RegisterPermanentArchetype(this, mapTypesSlot, false);
	else
		fwArchetypeManager::RegisterStreamedArchetype(this, mapTypesSlot);

	if (def->ClipDictionary)
	{
		SetClipDictionary(def->ClipDictionary);
	}

	if (def->PhysicsDictionary)
	{
		CreateDynamicComponentIfMissing();
		m_DynamicComponent->HasPhysicsInDrawable = true;
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
		m_DrawableType = DT_ASSETLESS;
	}
}

void rage::fwArchetype::Shutdown()
{
	SetPhysics(nullptr);
	m_ExtensionList.DestroyAll();
}

void rage::fwArchetype::SetPhysics(const pgPtr<phArchetype>& physics)
{
	if (!physics)
	{
		// Don't create dynamic component if we want to delete physics
		if (m_DynamicComponent) m_DynamicComponent->Physics = physics;
		return;
	}

	CreateDynamicComponentIfMissing();
	m_DynamicComponent->Physics = physics;
	if (physics && physics->GetBound())
	{
		UpdateBoundingVolumes(*physics->GetBound());
	}
}

bool rage::fwArchetype::SetDrawableOrFragmentFile(u32 nameHash, strLocalIndex txdSlot, bool isFragment)
{
	m_DrawableType = DT_UNINITIALIZED;
	m_DrawableIndex = INVALID_STR_INDEX;

	if (atIsNullHash(nameHash))
		return false;

	auto tryGetFromStore = [&](auto store, DrawableType assetType) -> bool
	{
		strLocalIndex slot = store->FindSlotFromHashKey(nameHash);
		if (slot == INVALID_STR_INDEX)
			return false;

		m_DrawableType = assetType;
		m_DrawableIndex = slot;

		if (txdSlot.IsValid())
			store->GetSlot(slot)->TxdIndex = txdSlot;

		return true;
	};

	// Try drawable...
	if (!isFragment && tryGetFromStore(GetDrawableStore(), DT_DRAWABLE))
		return true;

	// Try fragment...
	if (tryGetFromStore(GetFragmentStore(), DT_FRAGMENT))
		return true;

	// Not in drawable/fragment stores, model doesn't exists
	return false;
}

bool rage::fwArchetype::SetDrawableDictionaryFile(u32 nameHash, strLocalIndex txdSlot)
{
	m_DrawableType = DT_UNINITIALIZED;
	m_DrawableIndex = INVALID_STR_INDEX;

	if (atIsNullHash(nameHash))
		return false;

	fwDwdStore* store = GetDwdStore();

	strLocalIndex slot = store->FindSlotFromHashKey(nameHash);
	if (slot == INVALID_STR_INDEX)
		return false;

	m_DrawableType = DT_DRAWABLEDICTIONARY;
	m_DrawableIndex = slot;

	if (txdSlot != INVALID_STR_INDEX)
		store->GetSlot(slot)->TxdIndex = txdSlot;

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
		CreateDynamicComponentIfMissing();
		m_DynamicComponent->SetClipDictionary(clipDictNameHash);
	}
}

rage::strLocalIndex rage::fwArchetype::GetClipDictionarySlot() const
{
	if (!m_DynamicComponent) 
		return INVALID_STR_INDEX;

	return m_DynamicComponent->ClipDictionarySlot;
}

rage::rmcDrawable* rage::fwArchetype::GetDrawable() const
{
	if (!IsFlagSet(MODEL_HAS_LOADED))
		return nullptr;

	switch (m_DrawableType)
	{
		case DT_DRAWABLE:
			{
				return GetDrawableStore()->Get(m_DrawableIndex);
			}

		case DT_DRAWABLEDICTIONARY:
			{
				pgDictionary<gtaDrawable>* drawDict = GetDwdStore()->Get(m_DrawableIndex);
				return drawDict->GetValueAt(m_DwdIndex);
			}

		case DT_FRAGMENT:
			{
				AM_ERRF("fwArchetype::GetDrawable() -> Fragments are not implemented.");
				break;
			}

		default: break;
	}

	return nullptr;
}

void rage::fwArchetype::UpdateBoundingVolumes(const phBound& bound)
{
	spdAABB mergedBoundingBox = bound.GetBoundingBox();
	mergedBoundingBox.Merge(spdAABB(m_BoundingMin, m_BoundingMax));
	m_BoundingMin = mergedBoundingBox.Min;
	m_BoundingMax = mergedBoundingBox.Max;

	spdSphere mergedBoundingSphere = bound.GetBoundingSphere();
	mergedBoundingSphere.Merge(spdSphere(m_BsCentre, m_BsRadius));
	m_BsCentre = mergedBoundingSphere.GetCenter();
	m_BsRadius = mergedBoundingSphere.GetRadius().Get();
}

void rage::fwArchetype::SetAutoStartAnim(bool autoStartAnim)
{
	if (autoStartAnim)
		CreateDynamicComponentIfMissing();

	if (m_DynamicComponent)
		m_DynamicComponent->AutoStartAnim = autoStartAnim;
}
