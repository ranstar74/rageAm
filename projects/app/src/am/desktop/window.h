//
// File: window.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <Windows.h>

#include "imgui.h"
#include "am/system/ptr.h"
#include "am/system/event.h"
#include "common/types.h"

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace rageam
{
	// Implemented in window_integrated.cpp / window_standalone.cpp
	extern LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT WINAPI Common_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// Must be called from window event update loop
	void UpdateImGuiPlatform();

	struct WindowSize { u32 Width, Height; };

	class Window
	{
	protected:
		HWND m_Handle = nullptr;

	public:
		virtual ~Window() = default;

		virtual bool Update() = 0;

		virtual bool IsFocused() const { return GetForegroundWindow() == m_Handle; }

		virtual bool IsCursorVisible() const = 0;
		virtual void SetCursorVisible(bool visible) = 0;

		virtual void Show();

		virtual void SetClipCursor(bool clip) = 0;
		virtual bool GetClipCursor() = 0;

		// Gets mouse cursor position relative to this window
		void GetMousePos(u32& x, u32& y) const;

		// We need two events because in integration mode we have to destroy render target before
		// game resizes back buffer, then game done his job we re-create render target
		// In standalone mode end is called right after begin

		// TODO: Add bool so we can cancel it in viewport? / Redefine resolution? OnEarlyResize?
		
		SimpleEvent OnResizeBegin;
		ActionEvent<WindowSize> OnResizeEnd;

		HWND GetHandle() const { return m_Handle; }

		void GetSize(u32& width, u32& height) const;
	};

	/**
	 * \brief Creates and holds window depending on build type (standalone / integrated)
	 */
	class WindowFactory
	{
		static amUniquePtr<Window> sm_Window;
	public:
		static void CreateRenderWindow();
		static void DestroyRenderWindow();

		static bool HasWindow() { return sm_Window != nullptr; }

		// Note that in CLI mode there's no window! Null will be returned.
		static Window* GetWindow() { return sm_Window.get(); }

		static HWND GetWindowHandle() { return HasWindow() ? GetWindow()->GetHandle() : nullptr; }
	};
}
