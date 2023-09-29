//
// File: thread_integrated.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once
#ifndef AM_STANDALONE

#include <condition_variable>

#include "thread.h"

namespace rageam::render
{
	class ThreadIntegrated : public Thread
	{
		std::atomic_bool m_Stopped = false;

		std::condition_variable m_RenderDoneCondition;
		std::mutex m_RenderDoneMutex;

		// This function is called instead of game present function
		// so we can draw UI on top and then call game present function
		static void PresentCallback();
		void Render();
	public:
		ThreadIntegrated(const TRenderFn& renderFn);

		bool IsRunning() const override { return !m_Stopped; }
		void WaitRenderDone() override;
	};
}
#endif
