//
// File: ipc.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/asserts.h"

#include <Windows.h>

namespace rage
{
	using sysIpcMutex = HANDLE;
	using sysIpcSema = HANDLE;

	inline sysIpcMutex sysIpcCreateMutex() { return CreateMutex(NULL, FALSE, NULL); }
	inline sysIpcMutex sysIpcCreateSema(int initCount) { return CreateSemaphore(NULL, initCount, 32767, NULL); }
	inline void		   sysIpcDeleteMutex(sysIpcSema sema) { CloseHandle(sema); }
	inline void		   sysIpcDeleteSema(sysIpcMutex mutex) { CloseHandle(mutex); }

	inline void sysIpcLockMutex(sysIpcMutex mutex) { AM_ASSERTS(WaitForSingleObject(mutex, INFINITE) == WAIT_OBJECT_0); }
	inline void sysIpcWaitSema(sysIpcSema sema, int count = 1)
	{
		do
		{
			AM_ASSERTS(WaitForSingleObject(sema, INFINITE) == WAIT_OBJECT_0);
		} while (--count);
	}
	inline void sysIpcUnlockMutex(sysIpcMutex mutex) { AM_ASSERTS(mutex); ReleaseMutex(mutex); }

	class sysCriticalSectionToken
	{
		mutable RTL_CRITICAL_SECTION m_Token;
	public:
		sysCriticalSectionToken(u32 spinCount = 1000) { AM_ASSERTS(InitializeCriticalSectionAndSpinCount(&m_Token, spinCount)); }
		~sysCriticalSectionToken() { DeleteCriticalSection(&m_Token); }
		void Enter() const { EnterCriticalSection(&m_Token); }
		void Leave() const { LeaveCriticalSection(&m_Token); }
		bool IsLocked() const
		{
			if (TryEnterCriticalSection(&m_Token))
			{
				Leave();
				return false;
			}
			return true;
		}
	};

	class sysCriticalSectionLock
	{
		sysCriticalSectionToken* m_Token;
	public:
		sysCriticalSectionLock(sysCriticalSectionToken& token)
		{
			m_Token = &token;
			m_Token->Enter();
		}
		~sysCriticalSectionLock() { m_Token->Leave(); }
	};
}
