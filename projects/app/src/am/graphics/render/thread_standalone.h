//
// File: thread_standalone.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once
#ifdef AM_STANDALONE
#include <condition_variable>
#include <dxgi.h>

#include "am/graphics/render/thread.h"
#include "am/system/thread.h"

namespace rageam::render
{
	class ThreadStandalone : public Thread
	{
		rageam::Thread m_Thread;
		std::condition_variable m_RenderDoneCondition;
		std::mutex m_RenderDoneMutex;
		IDXGISwapChain* m_SwapChain;

		static u32 RenderThreadEntry(const ThreadContext* ctx);
	public:
		ThreadStandalone(IDXGISwapChain* swapchain, const TRenderFn& renderFn);

		bool IsRunning() const override { return m_Thread.IsRunning(); }
		void WaitRenderDone() override;
	};
}
#endif
