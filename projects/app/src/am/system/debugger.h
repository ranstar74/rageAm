//
// File: debugger.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <Windows.h>

namespace rageam
{
	class Debugger
	{
	public:
		static void Init()
		{
#ifndef AM_STANDALONE
#endif
		}

		/**
		 * \brief This function has to be used instead of standard win-api one because if RageAm is compiled as hook library,
		 * win-api's IsDebuggerPresent will be hooked to return false.
		 */
		static bool IsPresent()
		{
#ifdef AM_STANDALONE
			return IsDebuggerPresent();
#else
			return IsDebuggerPresent(); // TODO: ...
#endif
		}

		static void Break() { __debugbreak(); }

		static void BreakIfAttached()
		{
			if (IsPresent()) Break();
		}
	};
}
