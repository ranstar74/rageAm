//
// File: gameentity.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "am/integration/updatecomponent.h"
#include "rage/framework/entity/archetype.h"
#include "rage/streaming/streamingdefs.h"
#include "game/modelinfo/basemodelinfo.h"
#include "game/drawable.h"
#include "script/types.h"

namespace rageam::integration
{
	/**
	 * \brief Wrapper for gta entity, allows to easy spawn given drawable in the game world.
	 */
	class GameEntity : public IUpdateComponent
	{
		gtaDrawablePtr              m_Drawable;
		amPtr<CBaseModelInfo>       m_Archetype;
		amPtr<rage::fwArchetypeDef> m_ArchetypeDef;
		Vec3V                       m_DefaultPos;
		Vec3V						m_CachedPosition;

		rage::strLocalIndex         m_DrawableSlot;
		pVoid                       m_Entity = nullptr; // rage::fwEntity
		scrObjectIndex              m_EntityHandle;
		Mat44V                      m_EntityWorld;

		bool						m_Spawned = false;
		bool					    m_IsEntityWasAllocatedByGame = false;

		void Spawn();
		void OnEarlyUpdate() override;
		void OnLateUpdate() override;
		bool OnAbort() override;

	public:
		GameEntity(const gtaDrawablePtr& drawable, const amPtr<rage::fwArchetypeDef>& archetypeDef, const Vec3V& pos);

		void           SetPosition(const Vec3V& pos);
		const Vec3V&   GetPosition()		const { return m_CachedPosition; }
		const Mat44V&  GetWorldTransform()	const { return m_EntityWorld; }
		scrObjectIndex GetEntityHandle()	const { return m_EntityHandle; }
		pVoid          GetEntityPointer()	const { return m_Entity; }
		gtaDrawable*   GetDrawable()		const { return m_Drawable.Get(); }

		// Used for ModelInspector, we use game memory there (because game builds the drawable),
		// we have to use game allocators to destroy m_Drawable
		void SetEntityWasAllocatedByGame(bool v) { m_IsEntityWasAllocatedByGame = v; }

		// Entity may take few frames to spawn if there's already the same entity spawned, and it currently unloads
		// At the moment we don't allow multiple entities with the same archetype
		bool IsSpawned() const { return m_Entity != nullptr; }
	};
}

#endif
