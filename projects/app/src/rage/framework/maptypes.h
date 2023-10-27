//
// File: maptypes.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "archetype.h"
#include "extension.h"

namespace rage
{
	class fwMapTypes
	{
		fwExtensionDefs			m_Extensions;
		fwArchetypeDefs			m_Archetypes;
		atHashValue				m_Name;
		pgArray<atHashValue>	m_Dependences;

	public:
		fwMapTypes() = default;
		virtual ~fwMapTypes() = default;
	};
}
