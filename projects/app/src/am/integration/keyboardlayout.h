//
// File: keyboardlayout.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

namespace rageam::integration
{
	const static enum eKeyboardLayout
	{
		KeyboardLayout_QWERTY,
		KeyboardLayout_QWERTZ,
		KeyboardLayout_AZERTY
	};

	rageam::integration::eKeyboardLayout GetCurrentKeyboardLayout();
}
#endif
