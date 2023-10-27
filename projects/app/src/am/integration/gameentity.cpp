#ifdef AM_INTEGRATED

#include "gameentity.h"

#include "am/integration/shvthread.h"
#include "am/integration/memory/hook.h"
#include "rage/streaming/assetstores.h"

// dlDrawListMgr holds references on game assets (including fwArchetype)
// and this cause issues if we want to unload archetype instantly
// Solution for this is to hook function that adds ref to archetypes and
// ignore it for GTAEntity archetypes
static rage::atSet<u16> s_IgnoredArchetypeIdx;
static void(*gImpl_CDrawListMgr_AddArchetypeRef)(pVoid drawListMgr, u32 modelIndex);
static void aImpl_CDrawListMgr_AddArchetypeRef(pVoid drawListMgr, u32 modelIndex)
{
	if (!s_IgnoredArchetypeIdx.Contains(modelIndex))
		gImpl_CDrawListMgr_AddArchetypeRef(drawListMgr, modelIndex);
}
static void InitAddArchetypeHook()
{
	static bool init = false;
	if (init) return;
	init = true;

	gmAddress addr = gmAddress::Scan("45 33 D2 3B 15 ?? ?? ?? ?? 7D 31");
	Hook::Create(addr, aImpl_CDrawListMgr_AddArchetypeRef, &gImpl_CDrawListMgr_AddArchetypeRef, true);
}

void rageam::integration::GameEntity::CreateIfNotCreated()
{
	if (m_IsCreated)
		return;

	u32 assetName = m_ArchetypeDef->AssetName;

	// Register drawable
	auto drawableStore = rage::GetDrawableStore();
	drawableStore->AddSlot(m_DrawableSlot, assetName);
	drawableStore->Set(m_DrawableSlot, m_Drawable.get());

	// Create and register archetype
	m_Archetype = std::make_unique<CBaseModelInfo>();
	m_Archetype->InitArchetypeFromDefinition(rage::INVALID_STR_INDEX, m_ArchetypeDef.get(), true);
	m_Archetype->RefreshDrawable(0);

	InitAddArchetypeHook();
	s_IgnoredArchetypeIdx.Insert(m_Archetype->GetModelIndex());

	// Spawn entity
	scrBegin();
	m_EntityHandle = SHV::OBJECT::CREATE_OBJECT(
		assetName,
		m_InitialPosition.X(), m_InitialPosition.Y(), m_InitialPosition.Z(),
		FALSE, TRUE, FALSE);
	scrEnd();

	// Obtain rage::fwEntity pointer
	static auto getEntityFromGUID = gmAddress::Scan("E8 ?? ?? ?? ?? 90 EB 4B").GetCall().ToFunc<pVoid(u32 scriptIndex)>();
	m_Entity = getEntityFromGUID(m_EntityHandle);

	AM_DEBUGF("GameEntity created; Handle:%i Ptr: %p", m_EntityHandle, m_Entity);

	m_IsCreated = true;
}

bool rageam::integration::GameEntity::OnAbort()
{
	if (!m_IsCreated)
		return true;

	// README:
	// All rendering code is executed in parallel thread, rendering is started mid game update (after updating game itself)
	// and can last up until next 'mid game update'. This means that if we're going to remove archetype&drawable RIGHT now
	// render thread still MAY use it (although sometimes rendering can finish even before game early update)
	// In order to make sure that drawable will not be rendered after destroyed we have to manually sync with render thread
	render::Engine::GetInstance()->WaitRenderDone();

	AM_DEBUGF("GameEntity destroyed; Handle:%i Ptr: %p", m_EntityHandle, m_Entity);

	// Abort is called on early update before updating other components,
	// we are cleaning up things really fast here so next entity (if there is)
	// can easily take our place without issues
	// Destruct everything in the reverse order:

	// Entity
	scrBegin();
	SHV::ENTITY::SET_ENTITY_COORDS_NO_OFFSET(m_EntityHandle, -4000, 6000, -100, FALSE, FALSE, FALSE);
	SHV::ENTITY::SET_ENTITY_AS_MISSION_ENTITY(m_EntityHandle, FALSE, TRUE);
	SHV::OBJECT::DELETE_OBJECT(&m_EntityHandle);
	scrEnd();
	m_Entity = nullptr;

	// Archetype
	s_IgnoredArchetypeIdx.Remove(m_Archetype->GetModelIndex());
	if (s_IgnoredArchetypeIdx.GetNumUsedSlots() == 0)
	{
		// To prevent memory leaking destruct if when there's no more entities
		s_IgnoredArchetypeIdx.Destruct();
	}
	m_Archetype = nullptr;
	m_ArchetypeDef = nullptr;

	// Drawable
	auto drawableStore = rage::GetDrawableStore();
	drawableStore->Set(m_DrawableSlot, nullptr);
	drawableStore->RemoveSlot(m_DrawableSlot);
	m_DrawableSlot = rage::INVALID_STR_INDEX;
	m_Drawable = nullptr;

	m_IsCreated = false;
	return true;
}

void rageam::integration::GameEntity::OnEarlyUpdate()
{
	CreateIfNotCreated();
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

rageam::integration::GameEntity::GameEntity()
{
	m_DrawableSlot = rage::INVALID_STR_INDEX;
	m_EntityHandle = 0;
	m_Entity = nullptr;
	m_IsCreated = false;
}

void rageam::integration::GameEntity::Spawn(
	const amPtr<gtaDrawable>& drawable,
	const amPtr<rage::fwArchetypeDef>& archetypeDef,
	const rage::Vec3V& position)
{
	m_Drawable = drawable;
	m_ArchetypeDef = archetypeDef;
	m_InitialPosition = position;

	m_DrawableSlot = rage::INVALID_STR_INDEX;
	m_EntityHandle = 0;
	m_Entity = nullptr;

	m_IsCreated = false;
}

void rageam::integration::GameEntity::SetPosition(const rage::Vec3V& pos) const
{
	if (m_EntityHandle == 0)
		return;

	scrBegin();
	SHV::ENTITY::SET_ENTITY_COORDS_NO_OFFSET(m_EntityHandle, pos.X(), pos.Y(), pos.Z(), FALSE, FALSE, FALSE);
	// Entity lights collapse when moved outside of camera frustum, fix them up
	SHV::GRAPHICS::UPDATE_LIGHTS_ON_ENTITY(m_EntityHandle);
	scrEnd();
}

auto rageam::integration::GameEntity::GetWorldTransform() const -> const rage::Mat44V&
{
	return m_EntityWorld;
}

#endif
