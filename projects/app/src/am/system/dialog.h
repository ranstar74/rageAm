//
// File: dialog.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <CommCtrl.h>
#include <Windows.h>

#include "am/desktop/window.h"
#include "common/types.h"

namespace rageam
{
	enum eDialogType
	{
		DIALOG_INFORMATION,
		DIALOG_ERROR,
		DIALOG_WARNING,
	};

	inline void ShowDialog(ConstString title, ConstString content, eDialogType type)
	{
		long uType = NULL;
		switch (type)
		{
		case DIALOG_INFORMATION:
			uType = MB_ICONINFORMATION; break;
		case DIALOG_ERROR:
			uType = MB_ICONERROR; break;
		case DIALOG_WARNING:
			uType = MB_ICONWARNING; break;
		}

		HWND windowHandle = WindowFactory::GetWindowHandle();
		MessageBoxA(windowHandle, content, title, uType);
	}

	inline s32 ShowTaskDialog(
		ConstWString title,
		ConstWString footer,
		ConstWString header,
		ConstWString content,
		ConstWString expandedContent,
		eDialogType type,
		const TASKDIALOG_BUTTON* pButtons = nullptr, u32 numButtons = 0)
	{
		PCWSTR icon = nullptr;
		switch (type)
		{
		case DIALOG_INFORMATION:
			icon = MAKEINTRESOURCEW(-5); break; // TD_SHIELD_GRADIENT_ICON
		case DIALOG_ERROR:
			icon = MAKEINTRESOURCEW(-7); break; // TD_SHIELD_ERROR_ICON
		case DIALOG_WARNING:
			icon = MAKEINTRESOURCEW(-6); break; // TD_SHIELD_WARNING_ICON
		}

		HWND windowHandle = WindowFactory::GetWindowHandle();

		TASKDIALOGCONFIG config{};
		config.cbSize = sizeof(TASKDIALOGCONFIG);
		config.hwndParent = windowHandle;
		config.pszWindowTitle = title;
		config.pszContent = content;
		config.pszCollapsedControlText = L"Show details";
		config.pszExpandedControlText = L"Hide details";
		config.pszExpandedInformation = expandedContent;
		config.pszMainInstruction = header;
		config.dwFlags = TDF_EXPANDED_BY_DEFAULT;
		config.pszMainIcon = icon;
		config.pszFooterIcon = TD_INFORMATION_ICON;
		config.pszFooter = footer;
		config.cxWidth = 360;

		if (numButtons == 0)
		{
			config.dwCommonButtons = TDCBF_OK_BUTTON;
		}
		else
		{
			config.pButtons = pButtons;
			config.cButtons = numButtons;
		}

		s32 clickedButton;
		if (TaskDialogIndirect(&config, &clickedButton, NULL, NULL) == S_OK)
			return clickedButton;
		return 0;
	}
}
