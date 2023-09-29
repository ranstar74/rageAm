#pragma once

#include <Windows.h>
#include <tlhelp32.h>

#include "common/logger.h"

namespace hooks
{
	class AntiDebug
	{
		static bool CheckIsBogousThreadAndKill(DWORD threadId)
		{
			bool terminated = false;

			// We have to identify is thread is the right one by checking if instruction pointer points to invalid address
			HANDLE threadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);
			if (threadHandle != INVALID_HANDLE_VALUE)
			{
				CONTEXT ctx;
				GetThreadContext(threadHandle, &ctx);

				if (IsBadReadPtr((LPVOID)ctx.Rip, 8))
				{
					AM_DEBUGF("AntiDebug -> Found and killed bogous thread (handle - %p).", threadHandle);
					TerminateThread(threadHandle, 0);
					terminated = true;
				}

				CloseHandle(threadHandle);
			}
			return terminated;
		}

		static void ScanAndKillForBogousThread()
		{
			// Go through every thread in this process
			HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetCurrentProcessId());
			if (snapshot != INVALID_HANDLE_VALUE)
			{
				THREADENTRY32 threadEntry;
				threadEntry.dwSize = sizeof THREADENTRY32;

				if (Thread32First(snapshot, &threadEntry))
				{
					do
					{
						/*if (CheckIsBogousThreadAndKill(threadEntry.th32ThreadID))
							break;*/
						CheckIsBogousThreadAndKill(threadEntry.th32ThreadID);

					} while (Thread32Next(snapshot, &threadEntry));
				}
			}
			CloseHandle(snapshot);
		}

	public:
		static void Init()
		{
			// TODO: Merge scylla here

			// 1) This this really nasty one - game creates thread with invalid starting address
			// and it generates tons of exceptions, but they use exception filter
			// that prevents those exception to pop up when game is not debugged
			// the issue is that exception filter stops working as soon as debugger is attached
			//ScanAndKillForBogousThread(); // TODO: Crashes PC
		}
	};
}
