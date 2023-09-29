//
// File: inst.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "archetype.h"
#include "rage/paging/base.h"

namespace rage
{
	class phInst : public pgBase
	{
	public:
		pgPtr<phArchetype> m_Archetype;
		u16 m_LevelIndex;
		u16 m_Flags;
		// Mat44V m_Matrix;
	public:
		phInst() = default;

		phArchetype* GetArchetype() { return m_Archetype.Get(); }
		void SetArchetype(phArchetype* archetype) { m_Archetype = archetype; }
	};
}
