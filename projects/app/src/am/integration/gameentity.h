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

		rage::strLocalIndex         m_DrawableSlot;
		pVoid                       m_Entity = nullptr; // rage::fwEntity
		scrObjectIndex              m_EntityHandle;
		Mat44V                      m_EntityWorld;

		void Spawn();
		void OnEarlyUpdate() override;
		void OnLateUpdate() override;
		bool OnAbort() override;

	public:
		GameEntity(const gtaDrawablePtr& drawable, const amPtr<rage::fwArchetypeDef>& archetypeDef, const Vec3V& pos);

		void           SetPosition(const Vec3V& pos) const;
		const Mat44V&  GetWorldTransform()	const { return m_EntityWorld; }
		scrObjectIndex GetEntityHandle()	const { return m_EntityHandle; }
		pVoid          GetEntityPointer()	const { return m_Entity; }
		gtaDrawable*   GetDrawable()		const { return m_Drawable.Get(); }
	};
}

#endif
