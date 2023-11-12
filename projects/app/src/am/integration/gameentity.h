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
#include "game/drawable.h"
#include "game/modelinfo.h"

namespace rageam::integration
{
	/**
	 * \brief Wrapper for gta entity, allows to easy spawn given drawable in the game world.
	 */
	class GameEntity : public IUpdateComponent
	{
		amPtr<gtaDrawable> 			m_Drawable;
		amPtr<rage::fwArchetypeDef>	m_ArchetypeDef;
		rage::Vec3V					m_InitialPosition;
		bool						m_IsCreated;

		amPtr<CBaseModelInfo>		m_Archetype;
		rage::strLocalIndex			m_DrawableSlot;
		pVoid						m_Entity;			// rage::fwEntity
		int							m_EntityHandle;
		rage::Mat44V				m_EntityWorld;

		void CreateIfNotCreated();

		bool OnAbort() override;
		void OnEarlyUpdate() override;
		void OnLateUpdate() override;

	public:
		GameEntity();

		// NOTE: Game entity will be spawned only on next game early update!
		// If spawned from render thread (e.g. UI app), entity will be already spawned next frame
		void Spawn(
			const amPtr<gtaDrawable>& drawable,
			const amPtr<rage::fwArchetypeDef>& archetypeDef,
			const rage::Vec3V& position);
		
		void SetPosition(const rage::Vec3V& pos) const;
		auto GetWorldTransform() const -> const rage::Mat44V&;
		auto GetHandle() const { return m_EntityHandle; }
		auto GetEntityPointer() const { return m_Entity; }
		auto GetDrawable() const { return m_Drawable.get(); }
	};
	using GameEntityOwner = ComponentOwner<GameEntity>;
}

#endif
