#pragma once

#include "am/integration/memory/address.h"
#include "am/integration/memory/hook.h"

#include "am/system/errordisplay.h"

namespace hooks
{
	/**
	 * \brief Redirects ugly default message box with game error to our dialog with stack trace.
	 */
	class ErrorDialog
	{
		AM_NOINLINE static void aImpl_ShowErrorDialog(HWND hWnd, ConstWString title, ConstWString content, u32 type)
		{
			rageam::ErrorDisplay::GameError(content, 1 /* This */);
			Hook::Remove(MessageBoxW);
		}

		static void aImpl_ShowErrorAndExit(u32 errorHashKey)
		{
			Hook::Create(MessageBoxW, aImpl_ShowErrorDialog);
			gImpl_ShowErrorAndExit(errorHashKey);
		}
		static inline void(*gImpl_ShowErrorAndExit)(u32);
	public:
		static void Init()
		{
			// Logic here is a bit weird - we hook function that calls MessageBoxW with parsed error message,
			// them once its called we redirect MessageBoxW to our dialog.
			//
			// We could directly get error message but they're using atSet? and it's not researched yet.

			gmAddress gAddr_ShowErrorAndExit = gmAddress::Scan("E8 ?? ?? ?? ?? 49 39 7C 24 30", "ShowErrorAndExit");
			Hook::Create(gAddr_ShowErrorAndExit, aImpl_ShowErrorAndExit, &gImpl_ShowErrorAndExit);
		}
	};
}
