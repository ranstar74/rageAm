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
	// Already spawned
	if (m_Entity)
		return;

	// We can't register archetype&drawable spawn entity if there's other instance that has the same name
	u32 nameHashKey = m_ArchetypeDef->Name;
	if (rage::GetDrawableStore()->FindSlotFromHashKey(nameHashKey).IsValid() || rage::fwArchetypeManager::IsArchetypeExists(nameHashKey))
	{
		AM_DEBUGF("GameEntity -> entity '%x' still exists, can't spawn", nameHashKey);
		return;
	}

	Spawn();
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
	if (rage::GetDrawableStore()->GetNumRefs(m_DrawableSlot) > 0 || m_Archetype->GetRefCount() > 0)
		return false;

	AM_DEBUGF("GameEntity -> no more refs, unregistering archetype and drawable");

	// Archetype
	m_Archetype->SetIsStreamedArchetype(true); // Set streamed archetype flag back
	m_Archetype = nullptr;

	// Drawable
	rage::fwDrawableStore* dwStore = rage::GetDrawableStore();
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

rageam::integration::GameEntity::GameEntity(const gtaDrawablePtr& drawable, const amPtr<rage::fwArchetypeDef>& archetypeDef, const Vec3V& pos)
{
	m_Drawable = drawable;
	m_ArchetypeDef = archetypeDef;
	m_DefaultPos = pos;
}

void rageam::integration::GameEntity::SetPosition(const rage::Vec3V& pos) const
{
	if (!m_EntityHandle.IsValid())
		return;

	scrSetEntityCoordsNoOffset(m_EntityHandle, pos);
	// Entity lights collapse when moved outside of camera frustum, fix them up
	scrUpdateLightsOnEntity(m_EntityHandle);
}

#endif
