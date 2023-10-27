#include "imgui_internal.h"
#ifdef AM_STANDALONE

#include "window_standalone.h"
#include "am/ui/context.h"
#include "../resources/resource.h"

LRESULT rageam::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT common = Common_WndProc(hWnd, msg, wParam, lParam);
	if (common != 0)
		return common;

	static bool resizing = false;

	switch (msg)
	{
	case WM_ENTERSIZEMOVE:
		resizing = true;
		return 0;
	case WM_EXITSIZEMOVE: resizing = false;
	case WM_SIZE:
		if (!resizing) // We don't want to spam resize million times
		{
			Window* window = WindowFactory::GetWindow();

			u16 width = LOWORD(lParam);
			u16 height = HIWORD(lParam);

			window->OnResizeBegin.Invoke();
			window->OnResizeEnd.Invoke(WindowSize(width, height));
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

rageam::WindowStandalone::WindowStandalone()
{
	m_Module = GetModuleHandle(NULL);

	WNDCLASSEX wc{};
	wc.cbSize = sizeof WNDCLASSEX;
	wc.style = CS_CLASSDC;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = m_Module;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(m_Module, MAKEINTRESOURCE(IDI_ICON1));

	RegisterClassExW(&wc);

	u32 nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	u32 nScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	// Center window
	u32 x = nScreenWidth / 2 - WINDOW_WIDTH / 2;
	u32 y = nScreenHeight / 2 - WINDOW_HEIGHT / 2;

	m_Handle = CreateWindowW(CLASS_NAME, WINDOW_NAME, WS_OVERLAPPEDWINDOW,
		x, y, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, m_Module, NULL);
}

rageam::WindowStandalone::~WindowStandalone()
{
	DestroyWindow(m_Handle);
	UnregisterClassW(CLASS_NAME, m_Module);
}

void rageam::WindowStandalone::SetCursorVisible(bool visible)
{
	// We call ShowCursor multiple times because it holds counter internally
	// See remarks https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showcursor
	if (visible)
	{
		ClipCursor(NULL);

		while (ShowCursor(true) < 0) {}
	}
	else
	{
		// Fix cursor at 0, 0
		POINT point = { 0, 0 };
		ClientToScreen(m_Handle, &point);
		RECT clipRect = { point.x, point.y, point.x, point.y };
		ClipCursor(&clipRect);

		while (ShowCursor(false) >= 0) {}
	}

	m_CursorVisible = visible;
}

bool rageam::WindowStandalone::Update()
{
	MSG msg;
	while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
	{
		UpdateImGuiPlatform();

		TranslateMessage(&msg);
		DispatchMessage(&msg);
		
		if (msg.message == WM_QUIT)
			return false;
	}
	return true;
}
#endif
