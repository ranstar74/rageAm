#pragma once

#include <mutex>
#include <Windows.h>

namespace rageam
{
	struct InputState
	{
		// Mouse or Stick movement delta in X/Y direction
		float Horizontal, Vertical;
	};

	class Input
	{
		std::recursive_mutex m_Mutex;
		InputState m_AccumulateState{};
	public:
		InputState State;

		static void InitClass();

		// Processes input events from WndProc
		void HandleProc(UINT msg, WPARAM wParam, LPARAM lParam);

		void BeginFrame();
		void EndFrame();
	};
}
