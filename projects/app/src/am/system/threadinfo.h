#pragma once

#include "asserts.h"
#include "singleton.h"

#include <Windows.h>

namespace rageam
{
	/**
	 * \brief Utilities for identifying game threads.
	 */
	class ThreadInfo : public Singleton<ThreadInfo>
	{
		static constexpr u32 INVALID_THREAD_ID = u32(-1);

		u32 m_MainThreadId = INVALID_THREAD_ID;
		u32 m_RenderThreadId = INVALID_THREAD_ID;

	public:

		u32 GetCurrentThreadId() const { return ::GetCurrentThreadId(); }
		u32 GetMainThreadId() const { return m_MainThreadId; }
		u32 GetRenderThreadId() const { return m_RenderThreadId; }

		// There's racing condition when those functions are called before
		// thread ID is set, but it doesn't matter because render/main threads
		// set ID right after they're hooked up, return value will always be correct

		bool IsMainThread() const { return GetCurrentThreadId() == m_MainThreadId; }
		bool IsRenderThread() const { return GetCurrentThreadId() == m_RenderThreadId; }

		// Those functions must only be called once at initialization
		// In standalone mode we have only single thread, at least now

		void SetIsMainThread() { m_MainThreadId = GetCurrentThreadId(); }
		void SetIsRenderThread() { m_RenderThreadId = GetCurrentThreadId(); }
	};

#define AM_ASSERT_MAIN_THREAD \
	AM_ASSERT(ThreadManager::GetInstance()->IsMainThread(), "Function " __FUNCTION__ " can only be called from the main thread!")

#define AM_ASSERT_RENDER_THREAD \
	AM_ASSERT(ThreadManager::GetInstance()->IsRenderThread(), "Function " __FUNCTION__ " can only be called from the render thread!")
}
