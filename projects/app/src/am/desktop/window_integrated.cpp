#ifndef AM_STANDALONE
#include "window_integrated.h"

#include "am/integration/memory/address.h"
#include "am/integration/memory/hook.h"
#include "am/system/asserts.h"

LRESULT(*gImpl_WndProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT rageam::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT common = Common_WndProc(hWnd, msg, wParam, lParam);
	if (common != 0)
		return common;

	return gImpl_WndProc(hWnd, msg, wParam, lParam);
}

int (*gImpl_ShowCursor)(BOOL);
int aImpl_ShowCursor(BOOL show)
{
	// Do nothing here, we don't want game to override cursor visibility state

	// See remarks https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showcursor
	return show ? 0 : -1;
}

void (*gImpl_ClipCursor)(LPRECT);
void aImpl_ClipCursor(LPRECT)
{
	// Don't allow game to clip cursor
}

bool(*gImpl_UpdateResize)();
bool aImpl_UpdateResize()
{
	rageam::Window* window = rageam::WindowFactory::GetWindow();
	if (window)
	{
		RECT rect;
		GetClientRect(window->GetHandle(), &rect);

		window->OnResizeBegin.Invoke();
		window->OnResizeEnd.Invoke(rageam::WindowSize(rect.right, rect.bottom));
	}
	return gImpl_UpdateResize();
}

static gmAddress s_gAddr_UpdateResize;
static gmAddress s_gAddr_WndProc;
rageam::WindowIntegrated::WindowIntegrated()
{
	m_Handle = FindWindowA("grcWindow", NULL);
	AM_ASSERT(m_Handle, "WindowIntegrated -> Unable to find grcWindow, did you inject DLL into GTA5.exe?");

	// Just make it a little bit more fancy
	SetWindowTextA(m_Handle, "Grand Theft Auto V | RageAm");

	s_gAddr_WndProc = gmAddress::Scan("48 8D 05 ?? ?? ?? ?? 33 C9 89 75 20", "WndProc").GetRef(3);
	Hook::Create(s_gAddr_WndProc, WndProc, &gImpl_WndProc);

	// TODO: Disable cursor clipping when in imgui mode so we can easily move cursor out

	//Hook::Create(ShowCursor, aImpl_ShowCursor, &gImpl_ShowCursor, true);
	//Hook::Create(ClipCursor, aImpl_ClipCursor, &gImpl_ClipCursor, true);

	s_gAddr_UpdateResize = gmAddress::Scan("48 8B C4 55 53 56 57 41 54 41 56 41 57 48 8D A8 F8");
	Hook::Create(s_gAddr_UpdateResize, aImpl_UpdateResize, &gImpl_UpdateResize);

	// TODO: We use ImGui cursor for now, following code will show it so I commented it out

	// Game clips cursor, we don't want that
	// gImpl_ClipCursor(NULL);
	// Game cursor is hidden by default
	// WindowIntegrated::SetCursorVisible(true);
}

rageam::WindowIntegrated::~WindowIntegrated()
{
	// Bring back original title
	SetWindowTextA(m_Handle, "Grand Theft Auto V");

	Hook::Remove(s_gAddr_WndProc);
	Hook::Remove(s_gAddr_UpdateResize);
	//Hook::Remove(ShowCursor);
	//Hook::Remove(ClipCursor);
}

void rageam::WindowIntegrated::SetCursorVisible(bool visible)
{
	SetSystemCursor(LoadCursor(0, IDC_ARROW), 32512);

	// We call ShowCursor multiple times because it holds counter internally
	// See remarks https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showcursor
	if (visible)
	{
		gImpl_ClipCursor(NULL);

		while (gImpl_ShowCursor(true) < 0) {}
	}
	else
	{
		// Fix cursor at 0, 0
		POINT point = { 0, 0 };
		ClientToScreen(m_Handle, &point);
		RECT clipRect = { point.x, point.y, point.x, point.y };
		gImpl_ClipCursor(&clipRect);

		while (gImpl_ShowCursor(false) >= 0) {}
	}

	m_CursorVisible = visible;
}

#endif
