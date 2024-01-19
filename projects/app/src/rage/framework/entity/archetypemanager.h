//
// File: archetypemanager.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

namespace rage
{
	class fwArchetypeManager
	{
	public:
		static u16 RegisterStreamedArchetype(fwArchetype* pArchetype, strLocalIndex mapTypeDefIndex)
		{
#ifdef AM_INTEGRATED
			static auto fn = gmAddress::Scan("8B 40 50 0F BA E8 1F", "rage::fwArchetypeManager::RegisterStreamedArchetype+0x1AD").GetAt(-0x1AD)
				.ToFunc<u16(fwArchetype*, strLocalIndex)>();
			return fn(pArchetype, mapTypeDefIndex);
#endif
			return INVALID_STREAM_SLOT;
		}

		static u16 RegisterPermanentArchetype(fwArchetype* pArchetype, strLocalIndex mapTypeDefIndex, bool bMemLock)
		{
#ifdef AM_INTEGRATED
			static auto fn = gmAddress::Scan("E9 E0 01 00 00 33 D2", "rage::fwArchetypeManager::RegisterPermanentArchetype+0x83").GetAt(-0x83)
				.ToFunc<u16(fwArchetype*, strLocalIndex)>();
			return fn(pArchetype, mapTypeDefIndex);
#endif
			return INVALID_STREAM_SLOT;
		}

		static void UnregisterStreamedArchetype(fwArchetype* pArchetype)
		{
#ifdef AM_INTEGRATED
			static auto fn = gmAddress::Scan("75 FA 8B 54 24 28", "rage::fwArchetypeManager::UnregisterStreamedArchetype+0x68").GetAt(-0x68)
				.ToFunc<void(fwArchetype*)>();
			return fn(pArchetype);
#endif
		}

		static constexpr u16 INVALID_STREAM_SLOT = u16(-1);
	};
}
