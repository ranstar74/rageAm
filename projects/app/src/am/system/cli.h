//
// File: cli.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/asserts.h"
#include "common/types.h"

namespace rageam
{
	/**
	 * \brief Console arguments iterator.
	 */
	class ConsoleArguments
	{
		ConstWString* m_Args;
		s32 m_Count;
		s32 m_Current = -1;
	public:
		ConsoleArguments(wchar_t** args, s32 count)
		{
			// Skip first argument (current executable path)
			args++;
			count--;

			// StringBase is wrapper on char* / wchar_t* so we can simply cast it
			m_Args = (const wchar_t**)args;
			m_Count = count;
		}

		ImmutableWString Current() const
		{
			AM_ASSERT(m_Current != -1 && m_Current < m_Count,
				"ConsoleArguments::Current() -> Invalid state! Call Next() and ensure true was returned.");

			return m_Args[m_Current];
		}

		void GoBack()
		{
			AM_ASSERT(--m_Current > 0, "ConsoleArguments::GoBack() -> Gone too far...");
		}

		bool Next()
		{
			return ++m_Current < m_Count;
		}
	};
}
