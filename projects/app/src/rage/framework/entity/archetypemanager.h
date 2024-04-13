//
// File: archetypemanager.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/integration/memory/address.h"

namespace rage
{
	class fwArchetypeManager
	{
	public:
		static u16 RegisterStreamedArchetype(fwArchetype* pArchetype, strLocalIndex mapTypeDefIndex)
		{
#ifdef AM_INTEGRATED
			static auto fn = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
				"8B 40 50 0F BA E8 1F", "rage::fwArchetypeManager::RegisterStreamedArchetype+0x1AD").GetAt(-0x1AD)
#else
				"89 54 24 10 53 56 57 48 83 EC 20 48 8B 01", "rage::fwArchetypeManager::RegisterStreamedArchetype")
				.GetAt(0x70).GetCall()
#endif
				.ToFunc<u16(fwArchetype*, strLocalIndex)>();
			return fn(pArchetype, mapTypeDefIndex);
#endif
			return INVALID_STREAM_SLOT;
		}

		static u16 RegisterPermanentArchetype(fwArchetype* pArchetype, strLocalIndex mapTypeDefIndex, bool bMemLock)
		{
#ifdef AM_INTEGRATED
			static auto fn = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
				"E9 E0 01 00 00 33 D2", "rage::fwArchetypeManager::RegisterPermanentArchetype+0x83").GetAt(-0x83)
#else
				"89 54 24 10 53 56 57 48 83 EC 20 48 8B 01", "rage::fwArchetypeManager::RegisterPermanentArchetype")
				.GetAt(0x7D).GetCall()
#endif
				.ToFunc<u16(fwArchetype*, strLocalIndex)>();
			return fn(pArchetype, mapTypeDefIndex);
#endif
			return INVALID_STREAM_SLOT;
		}

		static void UnregisterStreamedArchetype(fwArchetype* pArchetype)
		{
#ifdef AM_INTEGRATED
			static auto fn = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
				"75 FA 8B 54 24 28", "rage::fwArchetypeManager::UnregisterStreamedArchetype+0x68").GetAt(-0x68)
#else
				"E8 ?? ?? ?? ?? 80 7B 60 01", "rage::fwArchetypeManager::UnregisterStreamedArchetype").GetCall()
#endif
				.ToFunc<void(fwArchetype*)>();
			return fn(pArchetype);
#endif
		}

		static bool IsArchetypeExists(u32 hashKey)
		{
#ifdef AM_INTEGRATED
			static auto fn = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
				"C7 44 24 20 FF FF 00 00 8B 44 24 20 48 83 C4 38", "rage::fwArchetypeManager::GetArchetypeIndexFromHashKey+0x3D").GetAt(-0x3D)
#else
				"74 0A 0F B7 02", "rage::fwArchetypeManager::GetArchetypeIndexFromHashKey+0x3A").GetAt(-0x3A)
#endif
				.ToFunc<u16(u32)>();
			return fn(hashKey) != u16(-1);
#endif
			return false;
		}

		static constexpr u16 INVALID_STREAM_SLOT = u16(-1);
	};
}
