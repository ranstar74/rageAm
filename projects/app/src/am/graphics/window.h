//
// File: window.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/singleton.h"
#include "common/types.h"

#include <Windows.h>
#include <imgui.h>

// TODO: We are breaking game mouse cursor visibility, this will break social club window

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace rageam::graphics
{
	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	class Window : public Singleton<Window>
	{
		static constexpr ConstWString CLASS_NAME = L"amWindow";
#ifdef AM_STANDALONE
		static constexpr ConstWString WINDOW_NAME = L"rageAm";
#else
		static constexpr ConstWString WINDOW_NAME = L"Grand Theft Auto V | rageAm - RAGE Research Project | Game Build: " APP_BUILD_STRING;
#endif
		static constexpr int DEFAULT_WINDOW_WIDTH = 1200;
		static constexpr int DEFAULT_WINDOW_HEIGHT = 768;

		HMODULE m_Module = NULL;
		HWND	m_Handle = NULL;

		void Create(int width, int height, int x, int y);
		void Destroy();

	public:
		Window();
		~Window() override;

		void SetMouseVisible(bool visibility) const;
		void GetSize(int& outWidth, int& outHeight) const;
		// Relative to the window rect
		void GetMousePosition(int& outX, int& outY) const;
		void GetPosition(int& outX, int& outY) const;

#ifdef AM_INTEGRATED
		// In integration mode we hook WndProc, we can't do this in constructor because UI is not initialized yet
		// Must be called once before update loop
		void SetHooks() const;
		void UnsetHooks() const;
		void Update() const;
#else
		bool Update() const;
#endif

		HWND GetHandle() const { return m_Handle; }
	};

	// Gets new size of window after resizing, if resize operation was done
	bool WindowGetNewSize(int& newWidth, int& newHeight);

	// Might be NULL if window was not created
	HWND WindowGetHWND();
}
