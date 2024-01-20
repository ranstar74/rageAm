#include "window.h"

#include "am/file/fileutils.h"
#include "am/system/system.h"

#ifdef AM_INTEGRATED
#include "am/integration/memory/hook.h"
#include "am/integration/memory/address.h"
#endif

#ifdef AM_STANDALONE
// Window Icon
#include "../resources/resource.h"
#endif

#include <mutex>

namespace
{
	std::mutex		s_Mutex;
	int				s_NewWidth = 0;
	int				s_NewHeight = 0;
	bool			s_UpdatedPosition;
	RECT			s_LastWindowRect;
	WINDOWPLACEMENT s_LastWindowPlacement;

#ifdef AM_INTEGRATED
	gmAddress		s_Addr_WndProc;
	bool			s_HooksInitialized = false;
	bool			s_CursorVisible = false;
#endif
}

#ifdef AM_INTEGRATED
int (*gImpl_ShowCursor)(bool);
int aImpl_ShowCursor(bool visible)
{
	// Don't let game to do anything
	return visible ? 0 : -1;
}
void (*gImpl_ClipCursor)(LPRECT);
void aImpl_ClipCursor(LPRECT) {}
LRESULT(*gImpl_WndProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

void ClipCursorToWindowRect(HWND handle, bool clip)
{
	RECT rect;
	GetWindowRect(handle, &rect);
#ifdef AM_STANDALONE
	ClipCursor(clip ? &rect : NULL);
#else
	gImpl_ClipCursor(clip ? &rect : NULL);
#endif
}

LRESULT rageam::graphics::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

#ifdef AM_INTEGRATED
	gImpl_WndProc(hWnd, msg, wParam, lParam);
#endif

	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;

		s_NewWidth = LOWORD(lParam);
		s_NewHeight = HIWORD(lParam);
		return 0;

	case WM_MOVING:
		s_UpdatedPosition = true;
		break;

	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;

	case WM_DPICHANGED:
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
		{
			const RECT* suggested_rect = (RECT*)lParam;
			SetWindowPos(hWnd, nullptr,
				suggested_rect->left, suggested_rect->top,
				suggested_rect->right - suggested_rect->left,
				suggested_rect->bottom - suggested_rect->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		}
		break;

	case WM_DESTROY:
		GetWindowRect(hWnd, &s_LastWindowRect);
		s_LastWindowPlacement.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(hWnd, &s_LastWindowPlacement);
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void rageam::graphics::Window::Create(int width, int height, int x, int y)
{
#ifdef AM_STANDALONE
	m_Module = GetModuleHandle(NULL);

	WNDCLASSEX wc = {};
	wc.cbSize = sizeof WNDCLASSEX;
	wc.style = CS_CLASSDC;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = m_Module;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(m_Module, MAKEINTRESOURCE(IDI_ICON1));

	RegisterClassExW(&wc);

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// Center window
	if (x < 0) x = screenWidth / 2 - width / 2;
	if (y < 0) y = screenHeight / 2 - height / 2;

	m_Handle = CreateWindowW(CLASS_NAME, WINDOW_NAME, WS_OVERLAPPEDWINDOW, x, y, width, height, NULL, NULL, m_Module, NULL);
	AM_ASSERT(m_Handle != NULL, "PlatformWindow::Create() -> Failed to create main window. Code: %u", GetLastError());

	ShowWindow(m_Handle, SW_SHOWDEFAULT);
	UpdateWindow(m_Handle);
#else
	m_Handle = FindWindowW(L"grcWindow", NULL);
	AM_ASSERT(m_Handle, "latformWindow::Create() -> Unable to find grcWindow, did you inject DLL into correct process?");

	SetWindowTextW(m_Handle, WINDOW_NAME);
#endif
}

void rageam::graphics::Window::Destroy()
{
#ifdef AM_STANDALONE
	DestroyWindow(m_Handle);
	UnregisterClassW(CLASS_NAME, m_Module);
#else
	SetWindowTextW(m_Handle, L"Grand Theft Auto V");
	if (s_HooksInitialized)
	{
		Hook::Remove(s_Addr_WndProc);
		Hook::Remove(ClipCursor);
	}
#endif

	m_Handle = NULL;
}

rageam::graphics::Window::Window()
{
	// Window rect
	int width = DEFAULT_WINDOW_WIDTH;
	int height = DEFAULT_WINDOW_HEIGHT;
	int x = -1;
	int y = -1;

	// Try to get previous session window rect
	System* sys = System::GetInstance();
	bool maximized = false;
	if (sys->HasData)
	{
		auto& wndData = sys->Data.Window;
		width = wndData.Width;
		height = wndData.Height;
		x = wndData.X;
		y = wndData.Y;
		maximized = wndData.Maximized;
	}

	Create(width, height, x, y);

	if (maximized)
		PostMessage(m_Handle, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
}

rageam::graphics::Window::~Window()
{
	RECT test;
	GetWindowRect(m_Handle, &test);

	// Save window rect to config to restore on next load
	System* sys = System::GetInstance();
	auto& wndData = sys->Data.Window;
	wndData.Width = s_LastWindowRect.right - s_LastWindowRect.left;
	wndData.Height = s_LastWindowRect.bottom - s_LastWindowRect.top;
	wndData.X = s_LastWindowRect.left;
	wndData.Y = s_LastWindowRect.top;
	wndData.Maximized = s_LastWindowPlacement.showCmd == SW_MAXIMIZE;

	Destroy();
}

void rageam::graphics::Window::SetMouseVisible(bool visibility) const
{
	// We call ShowCursor multiple times because it holds counter internally
	// See remarks https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showcursor
#ifdef AM_INTEGRATED
	// Must be called from window thread, we call it in Update
	s_CursorVisible = visibility;
#else
	if (visibility)
		while (ShowCursor(true) < 0) {}
	else
		while (ShowCursor(false) >= 0) {}
#endif
}

bool rageam::graphics::Window::GetMouseClipped() const
{
	std::unique_lock lock(s_Mutex);
	return m_MouseClipped;
}

void rageam::graphics::Window::SetMouseClipped(bool clipped)
{
	std::unique_lock lock(s_Mutex);
	m_MouseClipped = clipped;
	ClipCursorToWindowRect(m_Handle, m_MouseClipped);
}

void rageam::graphics::Window::GetSize(int& outWidth, int& outHeight) const
{
	RECT rect;
	GetWindowRect(m_Handle, &rect);
	outWidth = rect.right - rect.left;
	outHeight = rect.bottom - rect.top;
}

void rageam::graphics::Window::GetMousePosition(int& outX, int& outY) const
{
	POINT point;
	GetCursorPos(&point);
	ScreenToClient(m_Handle, &point);
	outX = point.x;
	outY = point.y;
}

void rageam::graphics::Window::GetPosition(int& outX, int& outY) const
{
	POINT point(0, 0);
	ClientToScreen(m_Handle, &point);
	outX = point.x;
	outY = point.y;
}

#ifdef AM_INTEGRATED
void rageam::graphics::Window::UpdateInit() const
{
	s_Addr_WndProc = 
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		gmAddress::Scan("E9 2F 15 00 00", "rage::grcWindowProc+0x176").GetAt(-0x176);
#elif APP_BUILD_2699_16_RELEASE
		gmAddress::Scan("48 8B C4 48 89 58 08 4C 89 48 20 55 56 57 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC F0", "rage::grcWindowProc");
#else
		gmAddress::Scan("48 8D 05 ?? ?? ?? ?? 33 C9 89 75 20", "rage::grcWindowProc").GetRef(3);
#endif
	Hook::Create(s_Addr_WndProc, WndProc, &gImpl_WndProc);
	Hook::Create(ClipCursor, aImpl_ClipCursor, &gImpl_ClipCursor);
	Hook::Create(ShowCursor, aImpl_ShowCursor, &gImpl_ShowCursor);
	s_HooksInitialized = true;
}
#endif

#ifdef AM_INTEGRATED
void rageam::graphics::Window::Update() const
#else
bool rageam::graphics::Window::Update() const
#endif
{
#ifdef AM_STANDALONE
	s_NewWidth = -1;
	s_NewHeight = -1;
	s_UpdatedPosition = false;

	MSG msg;
	while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_QUIT)
			return false;
	}

	if (s_UpdatedPosition)
	{
		ClipCursorToWindowRect(m_Handle, m_MouseClipped);
	}
	return true;
#else
	// Update cursor visibility
	if (s_CursorVisible)
		while (gImpl_ShowCursor(true) < 0) {}
	else
		while (gImpl_ShowCursor(false) >= 0) {}
#endif
}

bool rageam::graphics::WindowGetNewSize(int& newWidth, int& newHeight)
{
	newWidth = s_NewWidth;
	newHeight = s_NewHeight;
	return s_NewWidth != -1;
}

HWND rageam::graphics::WindowGetHWND()
{
	Window* window = Window::GetInstance();
	if (window) return window->GetHandle();
	return NULL;
}
