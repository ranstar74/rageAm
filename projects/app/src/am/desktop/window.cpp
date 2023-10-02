#include "window.h"

#include "input.h"
#include "am/ui/context.h"

#ifdef AM_STANDALONE
#include "window_standalone.h"
#else
#include "window_integrated.h"
#endif

amUniquePtr<rageam::Window> rageam::WindowFactory::sm_Window;

LRESULT rageam::Common_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

	if (Gui) Gui->Input.HandleProc(msg, wParam, lParam);

	return 0;
}

void rageam::Window::Show()
{
	ShowWindow(m_Handle, SW_SHOWDEFAULT);
	UpdateWindow(m_Handle);
}

void rageam::Window::GetSize(u32& width, u32& height) const
{
	RECT rect;
	GetWindowRect(m_Handle, &rect);
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;
}

void rageam::WindowFactory::CreateRenderWindow()
{
#ifdef AM_STANDALONE
	sm_Window = std::make_unique<WindowStandalone>();
#else
	sm_Window = std::make_unique<WindowIntegrated>();
#endif
	sm_Window->Show();
}

void rageam::WindowFactory::DestroyRenderWindow()
{
	sm_Window.reset();
}
