#include "imgui.h"

#include "ImGuiImplRage.h"

#include <windows.h>
#include <windowsx.h> 
#include <tchar.h>
#include <dwmapi.h>

#ifndef IMGUI_IMPL_Rage_DISABLE_GAMEPAD
#include <xinput.h>
typedef DWORD(WINAPI* PFN_XInputGetCapabilities)(DWORD, DWORD, XINPUT_CAPABILITIES*);
typedef DWORD(WINAPI* PFN_XInputGetState)(DWORD, XINPUT_STATE*);
#endif

struct ImGui_ImplRage_Data
{
	HWND                        hWnd;
	HWND                        MouseHwnd;
	bool                        MouseTracked;
	int                         MouseButtonsDown;
	INT64                       Time;
	INT64                       TicksPerSecond;
	ImGuiMouseCursor            LastMouseCursor;
	bool                        HasGamepad;
	bool                        WantUpdateHasGamepad;

#ifndef IMGUI_IMPL_Rage_DISABLE_GAMEPAD
	HMODULE                     XInputDLL;
	PFN_XInputGetCapabilities   XInputGetCapabilities;
	PFN_XInputGetState          XInputGetState;
#endif

	ImGui_ImplRage_Data() { memset((void*)this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
// FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using multi-context.
static ImGui_ImplRage_Data* ImGui_ImplRage_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImGui_ImplRage_Data*)ImGui::GetIO().BackendPlatformUserData : NULL;
}

// Functions
bool ImGui_ImplRage_Init(void* hwnd)
{
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(io.BackendPlatformUserData == NULL && "Already initialized a platform backend!");

	INT64 perf_frequency, perf_counter;
	if (!::QueryPerformanceFrequency((LARGE_INTEGER*)&perf_frequency))
		return false;
	if (!::QueryPerformanceCounter((LARGE_INTEGER*)&perf_counter))
		return false;

	// Setup backend capabilities flags
	ImGui_ImplRage_Data* bd = IM_NEW(ImGui_ImplRage_Data)();
	io.BackendPlatformUserData = (void*)bd;
	io.BackendPlatformName = "imgui_impl_Rage";
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)

	bd->hWnd = (HWND)hwnd;
	bd->WantUpdateHasGamepad = true;
	bd->TicksPerSecond = perf_frequency;
	bd->Time = perf_counter;
	bd->LastMouseCursor = ImGuiMouseCursor_COUNT;

	// Set platform dependent data in viewport
	ImGui::GetMainViewport()->PlatformHandleRaw = (void*)hwnd;

	// Dynamically load XInput library
#ifndef IMGUI_IMPL_Rage_DISABLE_GAMEPAD
	const char* xinput_dll_names[] =
	{
		"xinput1_4.dll",   // Windows 8+
		"xinput1_3.dll",   // DirectX SDK
		"xinput9_1_0.dll", // Windows Vista, Windows 7
		"xinput1_2.dll",   // DirectX SDK
		"xinput1_1.dll"    // DirectX SDK
	};
	for (int n = 0; n < IM_ARRAYSIZE(xinput_dll_names); n++)
		if (HMODULE dll = ::LoadLibraryA(xinput_dll_names[n]))
		{
			bd->XInputDLL = dll;
			bd->XInputGetCapabilities = (PFN_XInputGetCapabilities)::GetProcAddress(dll, "XInputGetCapabilities");
			bd->XInputGetState = (PFN_XInputGetState)::GetProcAddress(dll, "XInputGetState");
			break;
		}
#endif // IMGUI_IMPL_Rage_DISABLE_GAMEPAD

	return true;
}

void ImGui_ImplRage_Shutdown()
{
	ImGui_ImplRage_Data* bd = ImGui_ImplRage_GetBackendData();
	IM_ASSERT(bd != NULL && "No platform backend to shutdown, or already shutdown?");
	ImGuiIO& io = ImGui::GetIO();

	// Unload XInput library
#ifndef IMGUI_IMPL_Rage_DISABLE_GAMEPAD
	if (bd->XInputDLL)
		::FreeLibrary(bd->XInputDLL);
#endif // IMGUI_IMPL_Rage_DISABLE_GAMEPAD

	io.BackendPlatformName = NULL;
	io.BackendPlatformUserData = NULL;
	IM_DELETE(bd);
}

static bool ImGui_ImplRage_UpdateMouseCursor()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
		return false;

	ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
	if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
	{
		// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
		::SetCursor(NULL);
	}
	else
	{
		// Show OS mouse cursor
		LPTSTR Rage_cursor = IDC_ARROW;
		switch (imgui_cursor)
		{
		case ImGuiMouseCursor_Arrow:        Rage_cursor = IDC_ARROW; break;
		case ImGuiMouseCursor_TextInput:    Rage_cursor = IDC_IBEAM; break;
		case ImGuiMouseCursor_ResizeAll:    Rage_cursor = IDC_SIZEALL; break;
		case ImGuiMouseCursor_ResizeEW:     Rage_cursor = IDC_SIZEWE; break;
		case ImGuiMouseCursor_ResizeNS:     Rage_cursor = IDC_SIZENS; break;
		case ImGuiMouseCursor_ResizeNESW:   Rage_cursor = IDC_SIZENESW; break;
		case ImGuiMouseCursor_ResizeNWSE:   Rage_cursor = IDC_SIZENWSE; break;
		case ImGuiMouseCursor_Hand:         Rage_cursor = IDC_HAND; break;
		case ImGuiMouseCursor_NotAllowed:   Rage_cursor = IDC_NO; break;
		}
		::SetCursor(::LoadCursor(NULL, Rage_cursor));
	}
	return true;
}

static bool IsVkDown(int vk)
{
	return (::GetKeyState(vk) & 0x8000) != 0;
}

static void ImGui_ImplRage_AddKeyEvent(ImGuiKey key, bool down, int native_keycode, int native_scancode = -1)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddKeyEvent(key, down);
	io.SetKeyEventNativeData(key, native_keycode, native_scancode); // To support legacy indexing (<1.87 user code)
	IM_UNUSED(native_scancode);
}

static void ImGui_ImplRage_ProcessKeyEventsWorkarounds()
{
	// Left & right Shift keys: when both are pressed together, Windows tend to not generate the WM_KEYUP event for the first released one.
	if (ImGui::IsKeyDown(ImGuiKey_LeftShift) && !IsVkDown(VK_LSHIFT))
		ImGui_ImplRage_AddKeyEvent(ImGuiKey_LeftShift, false, VK_LSHIFT);
	if (ImGui::IsKeyDown(ImGuiKey_RightShift) && !IsVkDown(VK_RSHIFT))
		ImGui_ImplRage_AddKeyEvent(ImGuiKey_RightShift, false, VK_RSHIFT);

	// Sometimes WM_KEYUP for Win key is not passed down to the app (e.g. for Win+V on some setups, according to GLFW).
	if (ImGui::IsKeyDown(ImGuiKey_LeftSuper) && !IsVkDown(VK_LWIN))
		ImGui_ImplRage_AddKeyEvent(ImGuiKey_LeftSuper, false, VK_LWIN);
	if (ImGui::IsKeyDown(ImGuiKey_RightSuper) && !IsVkDown(VK_RWIN))
		ImGui_ImplRage_AddKeyEvent(ImGuiKey_RightSuper, false, VK_RWIN);
}

static void ImGui_ImplRage_UpdateKeyModifiers()
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddKeyEvent(ImGuiKey_ModCtrl, IsVkDown(VK_CONTROL));
	io.AddKeyEvent(ImGuiKey_ModShift, IsVkDown(VK_SHIFT));
	io.AddKeyEvent(ImGuiKey_ModAlt, IsVkDown(VK_MENU));
	io.AddKeyEvent(ImGuiKey_ModSuper, IsVkDown(VK_APPS));
}

static void ImGui_ImplRage_UpdateMouseData()
{
	ImGui_ImplRage_Data* bd = ImGui_ImplRage_GetBackendData();
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(bd->hWnd != 0);

	const bool is_app_focused = (::GetForegroundWindow() == bd->hWnd);
	if (is_app_focused)
	{
		// (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
		if (io.WantSetMousePos)
		{
			POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
			if (::ClientToScreen(bd->hWnd, &pos))
				::SetCursorPos(pos.x, pos.y);
		}

		// (Optional) Fallback to provide mouse position when focused (WM_MOUSEMOVE already provides this when hovered or captured)
		if (!io.WantSetMousePos && !bd->MouseTracked)
		{
			POINT pos;
			if (::GetCursorPos(&pos) && ::ScreenToClient(bd->hWnd, &pos))
				io.AddMousePosEvent((float)pos.x, (float)pos.y);
		}
	}
}

// Gamepad navigation mapping
static void ImGui_ImplRage_UpdateGamepads()
{
#ifndef IMGUI_IMPL_Rage_DISABLE_GAMEPAD
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplRage_Data* bd = ImGui_ImplRage_GetBackendData();
	if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
		return;

	// Calling XInputGetState() every frame on disconnected gamepads is unfortunately too slow.
	// Instead we refresh gamepad availability by calling XInputGetCapabilities() _only_ after receiving WM_DEVICECHANGE.
	if (bd->WantUpdateHasGamepad)
	{
		XINPUT_CAPABILITIES caps = {};
		bd->HasGamepad = bd->XInputGetCapabilities ? (bd->XInputGetCapabilities(0, XINPUT_FLAG_GAMEPAD, &caps) == ERROR_SUCCESS) : false;
		bd->WantUpdateHasGamepad = false;
	}

	io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
	XINPUT_STATE xinput_state;
	XINPUT_GAMEPAD& gamepad = xinput_state.Gamepad;
	if (!bd->HasGamepad || bd->XInputGetState == NULL || bd->XInputGetState(0, &xinput_state) != ERROR_SUCCESS)
		return;
	io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

#define IM_SATURATE(V)                      (V < 0.0f ? 0.0f : V > 1.0f ? 1.0f : V)
#define MAP_BUTTON(KEY_NO, BUTTON_ENUM)     { io.AddKeyEvent(KEY_NO, (gamepad.wButtons & BUTTON_ENUM) != 0); }
#define MAP_ANALOG(KEY_NO, VALUE, V0, V1)   { float vn = (float)(VALUE - V0) / (float)(V1 - V0); io.AddKeyAnalogEvent(KEY_NO, vn > 0.10f, IM_SATURATE(vn)); }
	MAP_BUTTON(ImGuiKey_GamepadStart, XINPUT_GAMEPAD_START);
	MAP_BUTTON(ImGuiKey_GamepadBack, XINPUT_GAMEPAD_BACK);
	MAP_BUTTON(ImGuiKey_GamepadFaceDown, XINPUT_GAMEPAD_A);
	MAP_BUTTON(ImGuiKey_GamepadFaceRight, XINPUT_GAMEPAD_B);
	MAP_BUTTON(ImGuiKey_GamepadFaceLeft, XINPUT_GAMEPAD_X);
	MAP_BUTTON(ImGuiKey_GamepadFaceUp, XINPUT_GAMEPAD_Y);
	MAP_BUTTON(ImGuiKey_GamepadDpadLeft, XINPUT_GAMEPAD_DPAD_LEFT);
	MAP_BUTTON(ImGuiKey_GamepadDpadRight, XINPUT_GAMEPAD_DPAD_RIGHT);
	MAP_BUTTON(ImGuiKey_GamepadDpadUp, XINPUT_GAMEPAD_DPAD_UP);
	MAP_BUTTON(ImGuiKey_GamepadDpadDown, XINPUT_GAMEPAD_DPAD_DOWN);
	MAP_BUTTON(ImGuiKey_GamepadL1, XINPUT_GAMEPAD_LEFT_SHOULDER);
	MAP_BUTTON(ImGuiKey_GamepadR1, XINPUT_GAMEPAD_RIGHT_SHOULDER);
	MAP_ANALOG(ImGuiKey_GamepadL2, gamepad.bLeftTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD, 255);
	MAP_ANALOG(ImGuiKey_GamepadR2, gamepad.bRightTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD, 255);
	MAP_BUTTON(ImGuiKey_GamepadL3, XINPUT_GAMEPAD_LEFT_THUMB);
	MAP_BUTTON(ImGuiKey_GamepadR3, XINPUT_GAMEPAD_RIGHT_THUMB);
	MAP_ANALOG(ImGuiKey_GamepadLStickLeft, gamepad.sThumbLX, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32768);
	MAP_ANALOG(ImGuiKey_GamepadLStickRight, gamepad.sThumbLX, +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
	MAP_ANALOG(ImGuiKey_GamepadLStickUp, gamepad.sThumbLY, +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
	MAP_ANALOG(ImGuiKey_GamepadLStickDown, gamepad.sThumbLY, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32768);
	MAP_ANALOG(ImGuiKey_GamepadRStickLeft, gamepad.sThumbRX, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32768);
	MAP_ANALOG(ImGuiKey_GamepadRStickRight, gamepad.sThumbRX, +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
	MAP_ANALOG(ImGuiKey_GamepadRStickUp, gamepad.sThumbRY, +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
	MAP_ANALOG(ImGuiKey_GamepadRStickDown, gamepad.sThumbRY, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32768);
#undef MAP_BUTTON
#undef MAP_ANALOG
#endif // #ifndef IMGUI_IMPL_Rage_DISABLE_GAMEPAD
}

void ImGui_ImplRage_NewFrame()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplRage_Data* bd = ImGui_ImplRage_GetBackendData();
	IM_ASSERT(bd != NULL && "Did you call ImGui_ImplRage_Init()?");

	// Setup display size (every frame to accommodate for window resizing)
	RECT rect = { 0, 0, 0, 0 };
	::GetClientRect(bd->hWnd, &rect);
	io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

	// Setup time step
	INT64 current_time = 0;
	::QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
	io.DeltaTime = (float)(current_time - bd->Time) / bd->TicksPerSecond;
	bd->Time = current_time;

	// Update OS mouse position
	ImGui_ImplRage_UpdateMouseData();

	// Process workarounds for known Windows key handling issues
	ImGui_ImplRage_ProcessKeyEventsWorkarounds();

	// Update OS mouse cursor with the cursor requested by imgui
	ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
	if (bd->LastMouseCursor != mouse_cursor)
	{
		bd->LastMouseCursor = mouse_cursor;
		ImGui_ImplRage_UpdateMouseCursor();
	}

	// Update game controllers (if enabled and available)
	ImGui_ImplRage_UpdateGamepads();
}

// There is no distinct VK_xxx for keypad enter, instead it is VK_RETURN + KF_EXTENDED, we assign it an arbitrary value to make code more readable (VK_ codes go up to 255)
#define IM_VK_KEYPAD_ENTER      (VK_RETURN + 256)

// Map VK_xxx to ImGuiKey_xxx.
static ImGuiKey ImGui_ImplRage_VirtualKeyToImGuiKey(WPARAM wParam)
{
	switch (wParam)
	{
	case VK_TAB: return ImGuiKey_Tab;
	case VK_LEFT: return ImGuiKey_LeftArrow;
	case VK_RIGHT: return ImGuiKey_RightArrow;
	case VK_UP: return ImGuiKey_UpArrow;
	case VK_DOWN: return ImGuiKey_DownArrow;
	case VK_PRIOR: return ImGuiKey_PageUp;
	case VK_NEXT: return ImGuiKey_PageDown;
	case VK_HOME: return ImGuiKey_Home;
	case VK_END: return ImGuiKey_End;
	case VK_INSERT: return ImGuiKey_Insert;
	case VK_DELETE: return ImGuiKey_Delete;
	case VK_BACK: return ImGuiKey_Backspace;
	case VK_SPACE: return ImGuiKey_Space;
	case VK_RETURN: return ImGuiKey_Enter;
	case VK_ESCAPE: return ImGuiKey_Escape;
	case VK_OEM_7: return ImGuiKey_Apostrophe;
	case VK_OEM_COMMA: return ImGuiKey_Comma;
	case VK_OEM_MINUS: return ImGuiKey_Minus;
	case VK_OEM_PERIOD: return ImGuiKey_Period;
	case VK_OEM_2: return ImGuiKey_Slash;
	case VK_OEM_1: return ImGuiKey_Semicolon;
	case VK_OEM_PLUS: return ImGuiKey_Equal;
	case VK_OEM_4: return ImGuiKey_LeftBracket;
	case VK_OEM_5: return ImGuiKey_Backslash;
	case VK_OEM_6: return ImGuiKey_RightBracket;
	case VK_OEM_3: return ImGuiKey_GraveAccent;
	case VK_CAPITAL: return ImGuiKey_CapsLock;
	case VK_SCROLL: return ImGuiKey_ScrollLock;
	case VK_NUMLOCK: return ImGuiKey_NumLock;
	case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
	case VK_PAUSE: return ImGuiKey_Pause;
	case VK_NUMPAD0: return ImGuiKey_Keypad0;
	case VK_NUMPAD1: return ImGuiKey_Keypad1;
	case VK_NUMPAD2: return ImGuiKey_Keypad2;
	case VK_NUMPAD3: return ImGuiKey_Keypad3;
	case VK_NUMPAD4: return ImGuiKey_Keypad4;
	case VK_NUMPAD5: return ImGuiKey_Keypad5;
	case VK_NUMPAD6: return ImGuiKey_Keypad6;
	case VK_NUMPAD7: return ImGuiKey_Keypad7;
	case VK_NUMPAD8: return ImGuiKey_Keypad8;
	case VK_NUMPAD9: return ImGuiKey_Keypad9;
	case VK_DECIMAL: return ImGuiKey_KeypadDecimal;
	case VK_DIVIDE: return ImGuiKey_KeypadDivide;
	case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
	case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
	case VK_ADD: return ImGuiKey_KeypadAdd;
	case IM_VK_KEYPAD_ENTER: return ImGuiKey_KeypadEnter;
	case VK_LSHIFT: return ImGuiKey_LeftShift;
	case VK_LCONTROL: return ImGuiKey_LeftCtrl;
	case VK_LMENU: return ImGuiKey_LeftAlt;
	case VK_LWIN: return ImGuiKey_LeftSuper;
	case VK_RSHIFT: return ImGuiKey_RightShift;
	case VK_RCONTROL: return ImGuiKey_RightCtrl;
	case VK_RMENU: return ImGuiKey_RightAlt;
	case VK_RWIN: return ImGuiKey_RightSuper;
	case VK_APPS: return ImGuiKey_Menu;
	case '0': return ImGuiKey_0;
	case '1': return ImGuiKey_1;
	case '2': return ImGuiKey_2;
	case '3': return ImGuiKey_3;
	case '4': return ImGuiKey_4;
	case '5': return ImGuiKey_5;
	case '6': return ImGuiKey_6;
	case '7': return ImGuiKey_7;
	case '8': return ImGuiKey_8;
	case '9': return ImGuiKey_9;
	case 'A': return ImGuiKey_A;
	case 'B': return ImGuiKey_B;
	case 'C': return ImGuiKey_C;
	case 'D': return ImGuiKey_D;
	case 'E': return ImGuiKey_E;
	case 'F': return ImGuiKey_F;
	case 'G': return ImGuiKey_G;
	case 'H': return ImGuiKey_H;
	case 'I': return ImGuiKey_I;
	case 'J': return ImGuiKey_J;
	case 'K': return ImGuiKey_K;
	case 'L': return ImGuiKey_L;
	case 'M': return ImGuiKey_M;
	case 'N': return ImGuiKey_N;
	case 'O': return ImGuiKey_O;
	case 'P': return ImGuiKey_P;
	case 'Q': return ImGuiKey_Q;
	case 'R': return ImGuiKey_R;
	case 'S': return ImGuiKey_S;
	case 'T': return ImGuiKey_T;
	case 'U': return ImGuiKey_U;
	case 'V': return ImGuiKey_V;
	case 'W': return ImGuiKey_W;
	case 'X': return ImGuiKey_X;
	case 'Y': return ImGuiKey_Y;
	case 'Z': return ImGuiKey_Z;
	case VK_F1: return ImGuiKey_F1;
	case VK_F2: return ImGuiKey_F2;
	case VK_F3: return ImGuiKey_F3;
	case VK_F4: return ImGuiKey_F4;
	case VK_F5: return ImGuiKey_F5;
	case VK_F6: return ImGuiKey_F6;
	case VK_F7: return ImGuiKey_F7;
	case VK_F8: return ImGuiKey_F8;
	case VK_F9: return ImGuiKey_F9;
	case VK_F10: return ImGuiKey_F10;
	case VK_F11: return ImGuiKey_F11;
	case VK_F12: return ImGuiKey_F12;
	default: return ImGuiKey_None;
	}
}

// Allow compilation with old Windows SDK. MinGW doesn't have default _Rage_WINNT/WINVER versions.
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef DBT_DEVNODES_CHANGED
#define DBT_DEVNODES_CHANGED 0x0007
#endif

// Rage message handler (process Rage mouse/keyboard inputs, etc.)
// Call from your application's message handler. Keep calling your message handler unless this function returns TRUE.
// When implementing your own backend, you can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if Dear ImGui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to Dear ImGui, and hide them from your application based on those two flags.
// PS: In this Rage handler, we use the capture API (GetCapture/SetCapture/ReleaseCapture) to be able to read mouse coordinates when dragging mouse outside of our window bounds.
// PS: We treat DBLCLK messages as regular mouse down messages, so this code will work on windows classes that have the CS_DBLCLKS flag set. Our own example app code doesn't set this flag.
#if 0
// Copy this line into your .cpp file to forward declare the function.
extern IMGUI_IMPL_API LRESULT ImGui_ImplRage_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
#include <format>
IMGUI_IMPL_API LRESULT ImGui_ImplRage_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui::GetCurrentContext() == NULL)
		return 0;
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplRage_Data* bd = ImGui_ImplRage_GetBackendData();

	switch (msg)
	{
	case WM_MOUSEMOVE:
		// We need to call TrackMouseEvent in order to receive WM_MOUSELEAVE events
		bd->MouseHwnd = hwnd;
		if (!bd->MouseTracked)
		{
			TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
			::TrackMouseEvent(&tme);
			bd->MouseTracked = true;
		}
		io.AddMousePosEvent((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam));
		break;
	case WM_MOUSELEAVE:
		if (bd->MouseHwnd == hwnd)
			bd->MouseHwnd = NULL;
		bd->MouseTracked = false;
		io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
		break;
	case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
	{
		int button = 0;
		if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = 0; }
		if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = 1; }
		if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = 2; }
		if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
		if (bd->MouseButtonsDown == 0 && ::GetCapture() == NULL)
			::SetCapture(hwnd);
		bd->MouseButtonsDown |= 1 << button;
		io.AddMouseButtonEvent(button, true);
		return 0;
	}
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
	{
		int button = 0;
		if (msg == WM_LBUTTONUP) { button = 0; }
		if (msg == WM_RBUTTONUP) { button = 1; }
		if (msg == WM_MBUTTONUP) { button = 2; }
		if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
		bd->MouseButtonsDown &= ~(1 << button);
		if (bd->MouseButtonsDown == 0 && ::GetCapture() == hwnd)
			::ReleaseCapture();
		io.AddMouseButtonEvent(button, false);
		return 0;
	}
	case WM_MOUSEWHEEL:
		io.AddMouseWheelEvent(0.0f, (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
		return 0;
	case WM_MOUSEHWHEEL:
		io.AddMouseWheelEvent((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, 0.0f);
		return 0;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	{
		const bool is_key_down = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
		if (wParam < 256)
		{
			// Submit modifiers
			ImGui_ImplRage_UpdateKeyModifiers();

			// Obtain virtual key code
			// (keypad enter doesn't have its own... VK_RETURN with KF_EXTENDED flag means keypad enter, see IM_VK_KEYPAD_ENTER definition for details, it is mapped to ImGuiKey_KeyPadEnter.)
			int vk = (int)wParam;
			if ((wParam == VK_RETURN) && (HIWORD(lParam) & KF_EXTENDED))
				vk = IM_VK_KEYPAD_ENTER;

			// Submit key event
			const ImGuiKey key = ImGui_ImplRage_VirtualKeyToImGuiKey(vk);
			const int scancode = (int)LOBYTE(HIWORD(lParam));
			if (key != ImGuiKey_None)
				ImGui_ImplRage_AddKeyEvent(key, is_key_down, vk, scancode);

			// Submit individual left/right modifier events
			if (vk == VK_SHIFT)
			{
				// Important: Shift keys tend to get stuck when pressed together, missing key-up events are corrected in ImGui_ImplRage_ProcessKeyEventsWorkarounds()
				if (IsVkDown(VK_LSHIFT) == is_key_down) { ImGui_ImplRage_AddKeyEvent(ImGuiKey_LeftShift, is_key_down, VK_LSHIFT, scancode); }
				if (IsVkDown(VK_RSHIFT) == is_key_down) { ImGui_ImplRage_AddKeyEvent(ImGuiKey_RightShift, is_key_down, VK_RSHIFT, scancode); }
			}
			else if (vk == VK_CONTROL)
			{
				if (IsVkDown(VK_LCONTROL) == is_key_down) { ImGui_ImplRage_AddKeyEvent(ImGuiKey_LeftCtrl, is_key_down, VK_LCONTROL, scancode); }
				if (IsVkDown(VK_RCONTROL) == is_key_down) { ImGui_ImplRage_AddKeyEvent(ImGuiKey_RightCtrl, is_key_down, VK_RCONTROL, scancode); }
			}
			else if (vk == VK_MENU)
			{
				if (IsVkDown(VK_LMENU) == is_key_down) { ImGui_ImplRage_AddKeyEvent(ImGuiKey_LeftAlt, is_key_down, VK_LMENU, scancode); }
				if (IsVkDown(VK_RMENU) == is_key_down) { ImGui_ImplRage_AddKeyEvent(ImGuiKey_RightAlt, is_key_down, VK_RMENU, scancode); }
			}
		}
		return 0;
	}
	case WM_SETFOCUS:
	case WM_KILLFOCUS:
		io.AddFocusEvent(msg == WM_SETFOCUS);
		return 0;
	case WM_CHAR:
		// You can also use ToAscii()+GetKeyboardState() to retrieve characters.
		if (wParam > 0 && wParam < 0x10000)
			io.AddInputCharacterUTF16((unsigned short)wParam);
		return 0;
	case WM_SETCURSOR:
		// This is required to restore cursor when transitioning from e.g resize borders to client area.
		if (LOWORD(lParam) == HTCLIENT && ImGui_ImplRage_UpdateMouseCursor())
			return 1;
		return 0;
	case WM_DEVICECHANGE:
		if ((UINT)wParam == DBT_DEVNODES_CHANGED)
			bd->WantUpdateHasGamepad = true;
		return 0;
	}
	return 0;
}


//--------------------------------------------------------------------------------------------------------
// DPI-related helpers (optional)
//--------------------------------------------------------------------------------------------------------
// - Use to enable DPI awareness without having to create an application manifest.
// - Your own app may already do this via a manifest or explicit calls. This is mostly useful for our examples/ apps.
// - In theory we could call simple functions from Windows SDK such as SetProcessDPIAware(), SetProcessDpiAwareness(), etc.
//   but most of the functions provided by Microsoft require Windows 8.1/10+ SDK at compile time and Windows 8/10+ at runtime,
//   neither we want to require the user to have. So we dynamically select and load those functions to avoid dependencies.
//---------------------------------------------------------------------------------------------------------
// This is the scheme successfully used by GLFW (from which we borrowed some of the code) and other apps aiming to be highly portable.
// ImGui_ImplRage_EnableDpiAwareness() is just a helper called by main.cpp, we don't call it automatically.
// If you are trying to implement your own backend for your own engine, you may ignore that noise.
//---------------------------------------------------------------------------------------------------------

// Perform our own check with RtlVerifyVersionInfo() instead of using functions from <VersionHelpers.h> as they
// require a manifest to be functional for checks above 8.1. See https://github.com/ocornut/imgui/issues/4200
static BOOL _IsWindowsVersionOrGreater(WORD major, WORD minor, WORD)
{
	typedef LONG(WINAPI* PFN_RtlVerifyVersionInfo)(OSVERSIONINFOEXW*, ULONG, ULONGLONG);
	static PFN_RtlVerifyVersionInfo RtlVerifyVersionInfoFn = NULL;
	if (RtlVerifyVersionInfoFn == NULL)
		if (HMODULE ntdllModule = ::GetModuleHandleA("ntdll.dll"))
			RtlVerifyVersionInfoFn = (PFN_RtlVerifyVersionInfo)GetProcAddress(ntdllModule, "RtlVerifyVersionInfo");
	if (RtlVerifyVersionInfoFn == NULL)
		return FALSE;

	RTL_OSVERSIONINFOEXW versionInfo = { };
	ULONGLONG conditionMask = 0;
	versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
	versionInfo.dwMajorVersion = major;
	versionInfo.dwMinorVersion = minor;
	VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
	return (RtlVerifyVersionInfoFn(&versionInfo, VER_MAJORVERSION | VER_MINORVERSION, conditionMask) == 0) ? TRUE : FALSE;
}

#define _IsWindowsVistaOrGreater()   _IsWindowsVersionOrGreater(HIBYTE(0x0600), LOBYTE(0x0600), 0) // _Rage_WINNT_VISTA
#define _IsWindows8OrGreater()       _IsWindowsVersionOrGreater(HIBYTE(0x0602), LOBYTE(0x0602), 0) // _Rage_WINNT_WIN8
#define _IsWindows8Point1OrGreater() _IsWindowsVersionOrGreater(HIBYTE(0x0603), LOBYTE(0x0603), 0) // _Rage_WINNT_WINBLUE
#define _IsWindows10OrGreater()      _IsWindowsVersionOrGreater(HIBYTE(0x0A00), LOBYTE(0x0A00), 0) // _Rage_WINNT_WINTHRESHOLD / _Rage_WINNT_WIN10

#ifndef DPI_ENUMS_DECLARED
typedef enum { PROCESS_DPI_UNAWARE = 0, PROCESS_SYSTEM_DPI_AWARE = 1, PROCESS_PER_MONITOR_DPI_AWARE = 2 } PROCESS_DPI_AWARENESS;
typedef enum { MDT_EFFECTIVE_DPI = 0, MDT_ANGULAR_DPI = 1, MDT_RAW_DPI = 2, MDT_DEFAULT = MDT_EFFECTIVE_DPI } MONITOR_DPI_TYPE;
#endif
#ifndef _DPI_AWARENESS_CONTEXTS_
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    (DPI_AWARENESS_CONTEXT)-3
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 (DPI_AWARENESS_CONTEXT)-4
#endif
typedef HRESULT(WINAPI* PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);                     // Shcore.lib + dll, Windows 8.1+
typedef HRESULT(WINAPI* PFN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);        // Shcore.lib + dll, Windows 8.1+
typedef DPI_AWARENESS_CONTEXT(WINAPI* PFN_SetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT); // User32.lib + dll, Windows 10 v1607+ (Creators Update)

// Helper function to enable DPI awareness without setting up a manifest
void ImGui_ImplRage_EnableDpiAwareness()
{
	if (_IsWindows10OrGreater())
	{
		static HINSTANCE user32_dll = ::LoadLibraryA("user32.dll"); // Reference counted per-process
		if (PFN_SetThreadDpiAwarenessContext SetThreadDpiAwarenessContextFn = (PFN_SetThreadDpiAwarenessContext)::GetProcAddress(user32_dll, "SetThreadDpiAwarenessContext"))
		{
			SetThreadDpiAwarenessContextFn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
			return;
		}
	}
	if (_IsWindows8Point1OrGreater())
	{
		static HINSTANCE shcore_dll = ::LoadLibraryA("shcore.dll"); // Reference counted per-process
		if (PFN_SetProcessDpiAwareness SetProcessDpiAwarenessFn = (PFN_SetProcessDpiAwareness)::GetProcAddress(shcore_dll, "SetProcessDpiAwareness"))
		{
			SetProcessDpiAwarenessFn(PROCESS_PER_MONITOR_DPI_AWARE);
			return;
		}
	}
#if _Rage_WINNT >= 0x0600
	::SetProcessDPIAware();
#endif
}

#if defined(_MSC_VER) && !defined(NOGDI)
#pragma comment(lib, "gdi32")   // Link with gdi32.lib for GetDeviceCaps(). MinGW will require linking with '-lgdi32'
#endif

float ImGui_ImplRage_GetDpiScaleForMonitor(void* monitor)
{
	UINT xdpi = 96, ydpi = 96;
	if (_IsWindows8Point1OrGreater())
	{
		static HINSTANCE shcore_dll = ::LoadLibraryA("shcore.dll"); // Reference counted per-process
		static PFN_GetDpiForMonitor GetDpiForMonitorFn = NULL;
		if (GetDpiForMonitorFn == NULL && shcore_dll != NULL)
			GetDpiForMonitorFn = (PFN_GetDpiForMonitor)::GetProcAddress(shcore_dll, "GetDpiForMonitor");
		if (GetDpiForMonitorFn != NULL)
		{
			GetDpiForMonitorFn((HMONITOR)monitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
			IM_ASSERT(xdpi == ydpi); // Please contact me if you hit this assert!
			return xdpi / 96.0f;
		}
	}
#ifndef NOGDI
	const HDC dc = ::GetDC(NULL);
	xdpi = ::GetDeviceCaps(dc, LOGPIXELSX);
	ydpi = ::GetDeviceCaps(dc, LOGPIXELSY);
	IM_ASSERT(xdpi == ydpi); // Please contact me if you hit this assert!
	::ReleaseDC(NULL, dc);
#endif
	return xdpi / 96.0f;
}

float ImGui_ImplRage_GetDpiScaleForHwnd(void* hwnd)
{
	HMONITOR monitor = ::MonitorFromWindow((HWND)hwnd, MONITOR_DEFAULTTONEAREST);
	return ImGui_ImplRage_GetDpiScaleForMonitor(monitor);
}