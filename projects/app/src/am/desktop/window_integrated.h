//
// File: window_integrated.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once
#ifndef AM_STANDALONE

#include "window.h"

namespace rageam
{
	class WindowIntegrated : public Window
	{
		std::atomic_bool m_CursorVisible = false;
		std::atomic_bool m_ClipCursor = true;
	public:
		WindowIntegrated();
		~WindowIntegrated() override;

		bool Update() override { return true; /* Nothing to update, done by game */ }

		bool IsCursorVisible() const override { return m_CursorVisible; }
		void SetCursorVisible(bool visible) override;

		void SetClipCursor(bool clip) override;
		bool GetClipCursor() override;
	};
}
#endif
