#include "gameentity.h"

#ifdef AM_INTEGRATED

#include "am/integration/memory/hook.h"
#include "am/types.h"
#include "am/graphics/render.h"
#include "rage/framework/streaming/assetstores.h"
#include "am/integration/script/core.h"
//
//// dlDrawListMgr holds references on game assets (including fwArchetype)
//// and this cause issues if we want to unload archetype instantly
//// Solution for this is to hook function that adds ref to archetypes and
//// ignore it for GTAEntity archetypes
//rageam::HashSet<u16> s_IgnoredArchetypeIdx;
//void(*gImpl_CDrawListMgr_AddArchetypeRef)(pVoid drawListMgr, u32 modelIndex);
//void aImpl_CDrawListMgr_AddArchetypeRef(pVoid drawListMgr, u32 modelIndex)
//{
//	if (!s_IgnoredArchetypeIdx.Contains(modelIndex))
//		gImpl_CDrawListMgr_AddArchetypeRef(drawListMgr, modelIndex);
//}
//void InitAddArchetypeHook()
//{
//	static bool init = false;
//	if (init) return;
//	init = true;
//
//#if APP_BUILD_2699_16_RELEASE_NO_OPT
//	gmAddress addr = gmAddress::Scan("75 FA 8B 94 24 B8 00 00 00", "rage::dlDrawListMgr::AddArchetypeReference+0x14").GetAt(-0x14);
//#else
//	gmAddress addr = gmAddress::Scan("45 33 D2 3B 15 ?? ?? ?? ?? 7D 31", "rage::dlDrawListMgr::AddArchetypeReference");
//#endif
//	Hook::Create(addr, aImpl_CDrawListMgr_AddArchetypeRef, &gImpl_CDrawListMgr_AddArchetypeRef);
//}

void rageam::integration::GameEntity::Create(const Vec3V& pos)
{
	u32 assetName = m_ArchetypeDef->AssetName;

	//// Map Types
	//rage::fwMapTypesStore* mapTypesStore = rage::GetMapTypesStore();
	//m_MapTypesSlot = mapTypesStore->AddSlot(assetName);

	// Register drawable
	rage::fwDrawableStore* dwStore = rage::GetDrawableStore();
	m_DrawableSlot = dwStore->AddSlot(assetName);
	dwStore->Set(m_DrawableSlot, m_Drawable.get());

	// Create and register archetype
	m_Archetype = std::make_unique<CBaseModelInfo>();
	m_Archetype->InitArchetypeFromDefinition(rage::INVALID_STR_INDEX/*m_MapTypesSlot*/, m_ArchetypeDef.get(), true);
	// In game code this is done by CModelInfoStreamingModule::Load
	m_Archetype->InitMasterDrawableData(0);

	// TODO: This is currently disabled because our implementation of ComputeBucketMask sets 0xFFFF
	// NOTE: Ugly hack! We set all buckets in drawable render mask to allow easier runtime editing
	// because it is easier than updating render mask on entities
	// RefreshDrawable recomputes render mask properly, so we have to reset it again
	// m_Drawable->ComputeBucketMask(); // Internally sets render mask to 0xFFFF, at least at the moment

	//InitAddArchetypeHook();
	//s_IgnoredArchetypeIdx.Insert(m_Archetype->GetModelIndex());

	// Spawn entity
	m_EntityHandle = scrCreateObject(assetName, pos);

	// Obtain rage::fwEntity pointer
	static auto getEntityFromGUID =
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		gmAddress::Scan("E8 ?? ?? ?? ?? 48 89 84 24 50 01 00 00 8B 44 24 58", "rage::fwScriptGuid::FromGuid<CEntity> ref").GetCall()
#else
		gmAddress::Scan("E8 ?? ?? ?? ?? 90 EB 4B").GetCall()
#endif
		.ToFunc<pVoid(u32 scriptIndex)>();
	m_Entity = getEntityFromGUID(m_EntityHandle.Get());

	AM_DEBUGF("GameEntity created; Handle:%i Ptr: %p", m_EntityHandle.Get(), m_Entity);
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

rageam::integration::GameEntity::GameEntity(
	const amPtr<gtaDrawable>& drawable,
	const amPtr<rage::fwArchetypeDef>& archetypeDef,
	const Vec3V& pos)
{
	m_Drawable = drawable;
	m_ArchetypeDef = archetypeDef;

	Create(pos);
}

rageam::integration::GameEntity::~GameEntity()
{
	// README:
	// All rendering code is executed in parallel thread, rendering is started mid-game update (after updating game itself)
	// and can last up until next 'mid-game update'. This means that if we're going to remove archetype&drawable RIGHT now
	// render thread still MAY use it (although sometimes rendering can finish even before game early update)
	// In order to make sure that drawable will not be rendered after destroyed we have to manually sync with render thread
	graphics::Render::GetInstance()->Lock();

	static auto CDrawListMgr_ReleaseAllRefs = gmAddress::Scan("E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 54 24 38", "CDrawListMgr::ReleaseAllRefs+0x12")
		.GetAt(-0x12)
		.ToFunc<void(pVoid mgr, u32 flags)>();
	static auto dlDrawListMgr = *gmAddress::Scan("41 B9 CE 01 00 00", "rage::fwRenderThreadInterface::GPU_IdleSection+0x2F")
		.GetAt(-0x2F) // Offset to the function
		.GetAt(0xC2)  // mov rax, cs:rage::dlDrawListMgr * rage::gDrawListMg
		.GetRef(3)
		.To<char**>();
	static auto flipUpdateFenceIdx = gmAddress::Scan("8B 40 38 FF C0 33 D2", "rage::dlDrawListMgr::FlipUpdateFenceIdx+0xA")
		.GetAt(-0xA)
		.ToFunc<void(pVoid)>();
	// Clear refs in all fences...
	for (int i = 0; i < 4; i++)
	{
		flipUpdateFenceIdx(dlDrawListMgr);
		CDrawListMgr_ReleaseAllRefs(dlDrawListMgr, 0); // gDrawListMgr->RemoveAllRefs();
	}

	AM_DEBUGF("GameEntity destroyed; Handle:%i Ptr: %p", m_EntityHandle.Get(), m_Entity);

	// Abort is called on early update before updating other components,
	// we are cleaning up things really fast here so next entity (if there is)
	// can easily take our place without issues
	// Destruct everything in the reverse order:

	// Entity
	scrSetEntityCoordsNoOffset(m_EntityHandle, scrVector(-4000, 6000, -100));
	scrSetEntityAsMissionEntity(m_EntityHandle, false, true);
	scrDeleteObject(m_EntityHandle);
	m_Entity = nullptr;

	// Archetype
	//s_IgnoredArchetypeIdx.Remove(m_Archetype->GetModelIndex());
	//if (s_IgnoredArchetypeIdx.GetNumUsedSlots() == 0)
	//{
	//	// To prevent memory leaking destruct if when there's no more entities
	//	s_IgnoredArchetypeIdx.Destroy();
	//}
	m_Archetype = nullptr;
	m_ArchetypeDef = nullptr;

	// Drawable
	rage::fwDrawableStore* dwStore = rage::GetDrawableStore();
	dwStore->Set(m_DrawableSlot, nullptr);
	dwStore->RemoveSlot(m_DrawableSlot);
	m_DrawableSlot = rage::INVALID_STR_INDEX; 
	m_Drawable = nullptr;

	//// Map Types
	//rage::fwMapTypesStore* mapTypesStore = rage::GetMapTypesStore();
	//mapTypesStore->RemoveSlot(m_MapTypesSlot);
	//m_MapTypesSlot = rage::INVALID_STR_INDEX;

	// Now we can safely resume rendering
	graphics::Render::GetInstance()->Unlock();
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
