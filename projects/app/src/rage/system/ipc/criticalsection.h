#pragma once

#include <Windows.h>

#include "common/types.h"

namespace rage
{
	// Simple wrapper to be consistent with rage naming

	class sysCriticalSectionToken
	{
		RTL_CRITICAL_SECTION m_Token;
	public:
		sysCriticalSectionToken(u32 spinCount = 1000)
		{
			if (!InitializeCriticalSectionAndSpinCount(&m_Token, spinCount))
				abort();
		}

		~sysCriticalSectionToken() { DeleteCriticalSection(&m_Token); }
		void Enter() { EnterCriticalSection(&m_Token); }
		void Leave() { LeaveCriticalSection(&m_Token); }
		bool IsLocked()
		{
			if (TryEnterCriticalSection(&m_Token))
			{
				Leave();
				return false;
			}
			return true;
		}
	};

	class sysCriticalSectionLock
	{
		sysCriticalSectionToken& tokenRef;
	public:
		sysCriticalSectionLock(sysCriticalSectionToken& token)
			: tokenRef(token)
		{
			tokenRef.Enter();
		}

		~sysCriticalSectionLock()
		{
			tokenRef.Leave();
		}
	};
}
