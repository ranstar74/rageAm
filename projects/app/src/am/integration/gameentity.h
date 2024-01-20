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
		amPtr<gtaDrawable>          m_Drawable;
		amPtr<rage::fwArchetypeDef> m_ArchetypeDef;
		amPtr<CBaseModelInfo>       m_Archetype;
		rage::strLocalIndex			m_MapTypesSlot;
		rage::strLocalIndex         m_DrawableSlot;
		pVoid                       m_Entity; // rage::fwEntity
		scrObjectIndex              m_EntityHandle;
		Mat44V                      m_EntityWorld;

		void Create(const Vec3V& pos);
		void OnLateUpdate() override;

	public:
		GameEntity(
			const amPtr<gtaDrawable>& drawable,
			const amPtr<rage::fwArchetypeDef>& archetypeDef,
			const Vec3V& pos);
		~GameEntity() override;

		void           SetPosition(const Vec3V& pos) const;
		const Mat44V&  GetWorldTransform()	const { return m_EntityWorld; }
		scrObjectIndex GetEntityHandle()	const { return m_EntityHandle; }
		pVoid          GetEntityPointer()	const { return m_Entity; }
		gtaDrawable*   GetDrawable()		const { return m_Drawable.get(); }
	};
}

#endif
