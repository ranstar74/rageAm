//
// File: gameupdate.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once
#ifdef AM_INTEGRATED

#include "memory/address.h"
#include "memory/hook.h"

namespace rageam::integration
{
	// Hook for CApp::GameUpdate function in main game thread,
	// we expose two - early and late update methods

	extern void EarlyGameUpdateFn();
	extern void LateGameUpdateFn();

	static inline bool(*CApp_GameUpdate_gImpl)();
	static bool CApp_GameUpdate_aImpl()
	{
		EarlyGameUpdateFn();
		bool result = CApp_GameUpdate_gImpl();
		LateGameUpdateFn();
		return result;
	}

	inline void HookGameUpdate()
	{
		gmAddress gameUpdate_Addr =
			gmAddress::Scan("48 83 EC 28 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? B9");
		Hook::Create(gameUpdate_Addr, CApp_GameUpdate_aImpl, &CApp_GameUpdate_gImpl, true);
	}
}
#endif
