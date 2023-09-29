//
// File: thread.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <functional>
#include "am/system/ptr.h"

namespace rageam::render
{
	// Depending on build type:
	// - Integration: hooks up game present function that's called by game render thread
	// - Standalone: creates a new thread

	using TRenderFn = std::function<bool()>;

	class Thread
	{
		std::atomic_bool m_StopRequested = false;
		TRenderFn m_RenderFn;
	protected:
		bool UpdateAndCallRenderFn(); // Must be called by implementation.

	public:
		Thread(const TRenderFn& renderFn);
		virtual ~Thread() = default;

		virtual bool IsRunning() const = 0; // Whether thread still running or was stopped after calling RequestStop

		virtual void WaitRenderDone() = 0;

		// Requests thread to release resources and stop, for e.g. integration has to revert custom viewport changes
		void RequestStop() { m_StopRequested = true; }
	};
}
