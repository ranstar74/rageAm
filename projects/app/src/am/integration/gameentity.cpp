#include "gameentity.h"

#ifdef AM_INTEGRATED

#include "rage/framework/entity/archetypemanager.h"
#include "rage/framework/streaming/assetstores.h"
#include "am/integration/memory/hook.h"
#include "am/integration/script/core.h"
#include "am/types.h"

void rageam::integration::GameEntity::Spawn()
{
	u32 name = m_ArchetypeDef->Name;

	// Register drawable
	rage::fwDrawableStore* dwStore = rage::GetDrawableStore();
	m_DrawableSlot = dwStore->AddSlot(name);
	dwStore->Set(m_DrawableSlot, m_Drawable.Get());
	// Add extra ref to prevent entity deleting pointer on destruction
	dwStore->AddRef(m_DrawableSlot);

	// Create and register archetype
	m_Archetype = std::make_unique<CBaseModelInfo>();
	m_Archetype->InitArchetypeFromDefinition(rage::INVALID_STR_INDEX/*m_MapTypesSlot*/, m_ArchetypeDef.get(), true);
	// In game code this is done by CModelInfoStreamingModule::Load
	m_Archetype->InitMasterDrawableData(0);
	// Prevent fwEntity from ref counting, we don't need that because we control lifetime manually
	m_Archetype->SetIsStreamedArchetype(false); // Permanent

	// TODO: This is currently disabled because our implementation of ComputeBucketMask sets 0xFFFF
	// NOTE: Ugly hack! We set all buckets in drawable render mask to allow easier runtime editing
	// because it is easier than updating render mask on entities
	// RefreshDrawable recomputes render mask properly, so we have to reset it again
	// m_Drawable->ComputeBucketMask(); // Internally sets render mask to 0xFFFF, at least at the moment

	// Spawn entity
	m_EntityHandle = scrCreateObject(name, m_DefaultPos);

	// Obtain rage::fwEntity pointer
	static auto getEntityFromGUID =
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		gmAddress::Scan("E8 ?? ?? ?? ?? 48 89 84 24 50 01 00 00 8B 44 24 58", "rage::fwScriptGuid::FromGuid<CEntity> ref").GetCall()
#else
		gmAddress::Scan("E8 ?? ?? ?? ?? 90 EB 4B").GetCall()
#endif
		.ToFunc<pVoid(u32 scriptIndex)>();
	m_Entity = getEntityFromGUID(m_EntityHandle.Get());

	AM_DEBUGF("GameEntity -> created; Handle:%i Ptr: %p", m_EntityHandle.Get(), m_Entity);
}

void rageam::integration::GameEntity::OnEarlyUpdate()
{
	if (m_Spawned)
	{
		// Entity will be NULL if we're aborting
		if (m_Entity)
			m_CachedPosition = GetWorldTransform().Pos;
		return;
	}

	// We can't register archetype&drawable spawn entity if there's other instance that has the same name
	u32 nameHashKey = m_ArchetypeDef->Name;
	if (rage::GetDrawableStore()->FindSlotFromHashKey(nameHashKey).IsValid() || rage::fwArchetypeManager::IsArchetypeExists(nameHashKey))
	{
		AM_DEBUGF("GameEntity -> entity '%x' still exists, can't spawn", nameHashKey);
		return;
	}

	Spawn();
	m_Spawned = true;
}

void rageam::integration::GameEntity::OnLateUpdate()
{
	if (m_Entity)
	{
		// Entity was updated, take fresh matrix from it
		const rage::Mat34V& entityWorld = *(rage::Mat34V*)((char*)m_Entity + 0x60);
		m_EntityWorld = entityWorld.To44();
	}
}

bool rageam::integration::GameEntity::OnAbort()
{
	if (!m_DrawableSlot.IsValid())
		return true;

	// We shouldn't delete entities mid-game update, this is not safe
	if (ComponentManager::GetUpdateStage() != UpdateStage_Early)
		return false;

	// Destroy spawned entity
	if (m_EntityHandle.IsValid())
	{
		AM_DEBUGF("GameEntity destroyed; Handle:%i Ptr: %p", m_EntityHandle.Get(), m_Entity);
		scrSetEntityCoordsNoOffset(m_EntityHandle, scrVector(-4000, 6000, -100));
		scrSetEntityAsMissionEntity(m_EntityHandle, false, true);
		scrDeleteObject(m_EntityHandle);
		m_Entity = nullptr;
	}

	// Draw list (or possible something else, including entity itself) still holds reference on this drawable
	// NOTE: There must be 1 ref for drawable because we added it in Spawn()
	if (rage::GetDrawableStore()->GetNumRefs(m_DrawableSlot) > 1 || m_Archetype->GetRefCount() > 0)
		return false;

	AM_DEBUGF("GameEntity -> no more refs, unregistering archetype and drawable");

	// Archetype
	m_Archetype->SetIsStreamedArchetype(true); // Set streamed archetype flag back
	m_Archetype = nullptr;

	// Drawable
	rage::fwDrawableStore* dwStore = rage::GetDrawableStore();
	// We added ref in Spawn() to not let the game remove drawable object,
	// use RemoveRefWithoutDelete to just decrement ref count, although it doesn't matter at this point
	{
		int dwRefs = dwStore->GetNumRefs(m_DrawableSlot);
		AM_ASSERT(dwRefs == 1, "GameEntity::OnAbort() -> Ref mismatch (%i), can't delete...", dwRefs);
		dwStore->RemoveRefWithoutDelete(m_DrawableSlot);
	}
	dwStore->Set(m_DrawableSlot, nullptr);
	dwStore->RemoveSlot(m_DrawableSlot);
	m_DrawableSlot = rage::INVALID_STR_INDEX;
	if (m_IsEntityWasAllocatedByGame)
	{
		rage::sysMemUseGameAllocators(true);
		AM_ASSERT(m_Drawable.GetRefCount() == 1, 
			"GameEntity::OnAbort() -> Can't destroy drawable with %u refs.", m_Drawable.GetRefCount());
		m_Drawable = nullptr;
		rage::sysMemUseGameAllocators(false);
	}
	else
	{
		m_Drawable = nullptr;
	}

	return true;
}

rageam::integration::GameEntity::GameEntity(ConstString name, const gtaDrawablePtr& drawable, const amPtr<rage::fwArchetypeDef>& archetypeDef, const Vec3V& pos)
{
	m_Drawable = drawable;
	m_ArchetypeDef = archetypeDef;
	m_DefaultPos = pos;

#if APP_BUILD_2699_16_RELEASE_NO_OPT
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;

		// On debug builds of the game we have to add archetype name to hash string dictionary
		// Otherwise game will crash on accesing it in rage::decalManager::KickProcessInstAsync
		// strncpy(td->debugName, GetEntityName(pEntity), NELEM(td->debugName));

		// rage::atHashStringNamespaceSupport::AddString(u32 nameSpace, u32 hash, const char *str)
		gmAddress addr = gmAddress::Scan("4C 89 44 24 18 89 54 24 10 89 4C 24 08 48 83 EC 78 83");
		auto fn = addr.ToFunc<void(u32, u32, const char*)>();
		fn(0 /* HSNS_ATHASHSTRING */, archetypeDef->Name, name);
	}
#endif
}

void rageam::integration::GameEntity::SetPosition(const rage::Vec3V& pos)
{
	m_CachedPosition = pos;
	if (!m_EntityHandle.IsValid())
		return;

	scrSetEntityCoordsNoOffset(m_EntityHandle, pos);
	// Entity lights collapse when moved outside of camera frustum, fix them up
	scrUpdateLightsOnEntity(m_EntityHandle);
}

#endif
