//
// File: hook.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <functional>

#include "MinHook.h"
#include "am/system/asserts.h"

/**
 * \brief Utility helper for MinHook library.
 */
class Hook
{
public:
	/**
	 * \brief Initializes MinHok.
	 */
	static void Init()
	{
		MH_STATUS status = MH_Initialize();
		AM_ASSERT(status == MH_OK, "Hook::Init() -> Failed to initialize minhook... (%s)",
			MH_StatusToString(status));
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
	 * \brief Enables all existing hooks up to this moment.
	 */
	static void Seal()
	{
		MH_EnableHook(MH_ALL_HOOKS);
	}

	/**
	 * \brief Hooks and redirects given function.
	 * \param from		Pointer to function to replace.
	 * \param to		Pointer to function that will be called instead.
	 * \param backup	Pointer that will be set to address of replaced function.
	 * \param bEnable	Whether hook will be enabled instantly or with Hook::Seal;
	 */
	template<typename TFrom, typename TTo, typename TBackup>
	static void Create(TFrom from, TTo to, TBackup* backup, bool bEnable = false)
	{
		MH_STATUS create = MH_CreateHook((LPVOID)from, (LPVOID)to, (LPVOID*)backup);
		AM_ASSERT(create == MH_OK,
			"Hook::Create() -> Failed to create hook on (%p, %p, %p): %s", (pVoid)from, (pVoid)to, (pVoid)*backup, MH_StatusToString(create));

		if (!bEnable)
			return;

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
		MH_STATUS disable = MH_DisableHook((pVoid)from);
		AM_ASSERT(disable == MH_OK,
			"Hook::Remove() -> Failed to disable hook on (%p): %s", (pVoid)from, MH_StatusToString(disable));

		MH_STATUS remove = MH_RemoveHook((pVoid)from);
		AM_ASSERT(remove == MH_OK,
			"Hook::Remove() -> Failed to remove hook on (%p): %s", (pVoid)from, MH_StatusToString(remove));
	}
};
