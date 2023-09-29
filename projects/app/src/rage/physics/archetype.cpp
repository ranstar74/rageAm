#include "archetype.h"

void rage::phArchetype::Erase()
{
	SetBound(nullptr);

	m_PropertyFlags = 0;
	m_TypeFlags = 0;
	m_IncludeFlags = 0;
}

void rage::phArchetype::CopyData(phArchetype* from)
{
	m_Filename = from->m_Filename;
	m_IncludeFlags = from->m_IncludeFlags;
	m_PropertyFlags = from->m_PropertyFlags;
	m_TypeFlags = from->m_TypeFlags;
}

rage::phArchetype* rage::phArchetype::Place(const datResource& rsc, phArchetype* that)
{
	switch (that->GetType())
	{
	case ARCHETYPE_BASE:
		return new (that) phArchetype(rsc);
	case ARCHETYPE_PHYS:
		return new (that) phArchetypePhys(rsc);
	case ARCHETYPE_DAMP:
		return new (that) phArchetypeDamp(rsc);
	default:
		AM_UNREACHABLE("phArchetype::Place() -> Type %u is not supported.", that->m_Type);
	}
}
