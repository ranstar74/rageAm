//
// File: window_standalone.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once
#ifdef AM_STANDALONE

#include "window.h"

namespace rageam
{
	class WindowStandalone : public Window
	{
		static constexpr ConstWString CLASS_NAME = L"amWindow";
		static constexpr ConstWString WINDOW_NAME = L"rageAm";

		static constexpr u32 WINDOW_WIDTH = 1200;
		static constexpr u32 WINDOW_HEIGHT = 768;

		HMODULE m_Module;
		bool m_CursorVisible = true;
	public:
		WindowStandalone();
		~WindowStandalone() override;

		bool IsCursorVisible() const override { return m_CursorVisible; }
		void SetCursorVisible(bool visible) override;
		bool Update() override;
	};
}
#endif
