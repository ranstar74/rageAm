//
// File: thread.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <Windows.h>

#include "ptr.h"
#include "common/types.h"
#include "rage/atl/fixedarray.h"

namespace rageam
{
	class Thread;
	struct ThreadContext;

	using ThreadEntryPoint = u32(const ThreadContext* ctx);

	struct ThreadContext
	{
		ThreadEntryPoint* EntryPoint;
		Thread* Thread;
		pVoid Param;
	};

	class Thread
	{
		static constexpr u32 MAX_THREADS = 64;

		static rage::atFixedArray<ThreadContext, MAX_THREADS> sm_ThreadContexts;

		std::atomic_bool m_ExitRequested = false;
		ConstString m_DebugName;
		HANDLE m_Handle;

		std::atomic_bool m_Running = false;
		std::atomic_bool m_Suspended;

		static DWORD ThreadEntry(LPVOID lpParam);
	public:
		Thread(ConstString debugName, ThreadEntryPoint entryPoint, pVoid param = nullptr, bool paused = false);
		~Thread();

		void RequestExitAndWait();
		void RequestExit() { m_ExitRequested = true; }
		void WaitExit();
		bool ExitRequested() const { return m_ExitRequested; }
		bool IsRunning() const { return m_Running; }
		bool IsSuspended() const { return m_Suspended; }
		void Resume() { ResumeThread(m_Handle); m_Suspended = false; }
		void Pause() { SuspendThread(m_Handle); m_Suspended = true; }
		void Join() const { WaitForSingleObject(m_Handle, INFINITE); }
	};
}
