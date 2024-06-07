//
// File: hook.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/asserts.h"
#include "helpers/win32.h"

#include <MinHook.h>

/**
 * \brief Utility helper for MinHook library.
 */
class Hook
{
	static inline u64 sm_ModuleStart;
	static inline u64 sm_ModuleEnd;

	template<typename T>
	static void AssertAddressInModule(T fn)
	{
		if (!tl_EnableAddressInModuleCheck)
			return;

		u64 addr = (u64) fn;
		// At the moment we don't install hooks outside GTA5.exe module, so for
		// safety reasons add this check. Later we can add whitelist for required libraries.
		AM_ASSERT(addr >= sm_ModuleStart && addr < sm_ModuleEnd,
			"Hook -> Attempt to install hook outside of game module.");
	}

	template<typename T>
	static void LogFunction(T fn)
	{
		// AM_DEBUGF("Hook -> %p", (void*) fn);
	}

public:
	// Set FALSE to install hooks outside of game module (GTA5.exe)
	static inline thread_local bool tl_EnableAddressInModuleCheck = true;

	/**
	 * \brief Initializes MinHook.
	 */
	static void Init()
	{
		MH_STATUS status = MH_Initialize();
		AM_ASSERT(status == MH_OK, "Hook::Init() -> Failed to initialize minhook... (%s)",
			MH_StatusToString(status));

		u64 moduleBase, moduleSize;
		GetModuleBaseAndSize(&moduleBase, &moduleSize);
		sm_ModuleStart = moduleBase;
		sm_ModuleEnd = moduleBase + moduleSize;
	}

	/**
	 * \brief Removes all existing hooks and un-initializes MinHook.
	 */
	static void Shutdown()
	{
		MH_RemoveHook(MH_ALL_HOOKS);
		MH_Uninitialize();
	}

	/**
	 * \brief Shortcut to create a detour to nullsub function.
	 * \param that Pointer to function to replace.
	 */
	template<typename T>
	static void Nullsub(T that)
	{
		struct Ns { static void Fn() {} };
		Create(that, Ns::Fn);
	}

	/**
	 * \brief Hooks and redirects given function.
	 * \param from		Pointer to function to replace.
	 * \param to		Pointer to function that will be called instead.
	 * \param backup	Pointer that will be set to address of replaced function.
	 */
	template<typename TFrom, typename TTo, typename TBackup>
	static void Create(TFrom from, TTo to, TBackup* backup)
	{
		LogFunction(from);
		AssertAddressInModule(from);

		MH_STATUS create = MH_CreateHook((LPVOID)from, (LPVOID)to, (LPVOID*)backup);
		AM_ASSERT(create == MH_OK,
			"Hook::Create() -> Failed to create hook on (%p, %p, %p): %s", (pVoid)from, (pVoid)to, (pVoid)*backup, MH_StatusToString(create));

		MH_STATUS enable = MH_EnableHook((LPVOID)from);
		AM_ASSERT(create == MH_OK,
			"Hook::Create() -> Failed to enable hook on (%p, %p, %p): %s", (pVoid)from, (pVoid)to, (pVoid)*backup, MH_StatusToString(enable));
	}

	/**
	 * \brief Hooks and redirects given function.
	 * \param from		Pointer to function to replace.
	 * \param to		Pointer to function that will be called instead.
	 */
	template<typename TFrom, typename TTo>
	static void Create(TFrom from, TTo to)
	{
		LogFunction(from);
		AssertAddressInModule(from);
		
		MH_STATUS create = MH_CreateHook((LPVOID)from, (LPVOID)to, NULL);
		AM_ASSERT(create == MH_OK,
			"Hook::Create() -> Failed to create hook on (%p, %p): %s", from, to, MH_StatusToString(create));

		MH_STATUS enable = MH_EnableHook((LPVOID)from);
		AM_ASSERT(create == MH_OK,
			"Hook::Create() -> Failed to enable hook on (%p, %p): %s", from, to, MH_StatusToString(enable));
	}

	/**
	 * \brief Removes already installed hook.
	 * \param from Address of function that was passed in corresponding argument of Create function.
	 */
	template<typename TFrom>
	static void Remove(TFrom from)
	{
		if (from == 0)
			return;

		MH_STATUS disable = MH_DisableHook((pVoid)from);
		AM_ASSERT(disable == MH_OK,
			"Hook::Remove() -> Failed to disable hook on (%p): %s", (pVoid)from, MH_StatusToString(disable));

		MH_STATUS remove = MH_RemoveHook((pVoid)from);
		AM_ASSERT(remove == MH_OK,
			"Hook::Remove() -> Failed to remove hook on (%p): %s", (pVoid)from, MH_StatusToString(remove));
	}
};
