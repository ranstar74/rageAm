#include "input.h"

#include <hidusage.h>

#include "window.h"

void rageam::Input::InitClass()
{
	RAWINPUTDEVICE Rid[1];
	Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	Rid[0].dwFlags = RIDEV_INPUTSINK;
	Rid[0].hwndTarget = WindowFactory::GetWindowHandle();
	RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));
}

void rageam::Input::HandleProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Input will be processed before beginning new frame
	std::unique_lock lock(m_Mutex);

	// We are accumulating input because in between one frame there might be multiple input data

	switch (msg)
	{
	case WM_INPUT:
	{
		// Raw mouse input
		{
			UINT dwSize = sizeof(RAWINPUT);
			static BYTE lpb[sizeof(RAWINPUT)];

			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

			RAWINPUT* raw = (RAWINPUT*)lpb;

			if (raw->header.dwType == RIM_TYPEMOUSE)
			{
				m_AccumulateState.Horizontal += static_cast<float>(raw->data.mouse.lLastX);
				m_AccumulateState.Vertical += static_cast<float>(raw->data.mouse.lLastY);
			}
		}

		break;
	}
	}
}

void rageam::Input::BeginFrame()
{

}

void rageam::Input::EndFrame()
{
	// Get accumulated state and reset it
	std::unique_lock lock(m_Mutex);

	State = m_AccumulateState;
	m_AccumulateState = {};
}
