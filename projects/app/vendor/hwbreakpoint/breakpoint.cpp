#include "breakpoint.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <algorithm>

#include <windows.h>
#include <tlhelp32.h>

namespace HWBreakpoint
{
	namespace
	{
		bool _initialize = false;
		int _countActive;
		void* _address[4];
		int _len[4];
		BPCondition _when[4];

		std::thread _workerThread;
		std::mutex _mutex, _controlMutex;
		std::condition_variable _workerSignal;
		std::atomic<bool> _workerStop;

		volatile DWORD _pendingThread;

		// hook trampoline
		unsigned char* _trampoline;
		unsigned char _orgOpcode[8];
	}

	inline void SetBits(ULONG_PTR& dw, int lowBit, int bits, int newValue)
	{
		int mask = (1 << bits) - 1; // e.g. 1 becomes 0001, 2 becomes 0011, 3 becomes 0111

		dw = (dw & ~(mask << lowBit)) | (newValue << lowBit);
	}
	void Init();
	void UnInit();
	void BuildTrampoline();
	void ThreadDeutor();
	void SetForThreads(std::unique_lock<std::mutex>& lock);
	void RegisterThread(DWORD tid);
	void ToggleThreadHook(bool set);
	void WorkerThreadProc();


	// Interface functions

	bool Set(void* address, BPCondition when, int length)
	{
		std::lock_guard<std::mutex> lock1(_controlMutex);
		if (!_initialize)
			Init();

		std::unique_lock<std::mutex> lock2(_mutex);

		int index = -1;

		// search for this address
		for (int i = 0; i < 4; ++i)
		{
			if (_address[i] == address)
				index = i;
		}

		// find avalible place
		for (int i = 0; index < 0 && i < 4; ++i)
		{
			if (_address[i] == nullptr)
			{
				index = i;
				if (_countActive++ == 0)
					ToggleThreadHook(true);
			}
		}

		if (index >= 0)
		{
			_address[index] = address;
			_len[index] = length; // sizeof(void*)
			_when[index] = when;
			SetForThreads(lock2);
			return true;
		}

		return false;
	}

	void Clear(void* address)
	{
		std::lock_guard<std::mutex> lock1(_controlMutex);
		if (!_initialize)
			return;

		std::unique_lock<std::mutex> lock2(_mutex);
		for (int index = 0; index < 4; ++index)
		{
			if (_address[index] == address)
			{
				_address[index] = nullptr;
				if (--_countActive == 0)
					ToggleThreadHook(false);
				SetForThreads(lock2);
			}
		}
	}

	void ClearAll()
	{
		std::lock_guard<std::mutex> lock(_controlMutex);
		if (!_initialize)
			return;

		UnInit();
	}

	// Internal functions

	void Init()
	{
		if (_initialize)
			return;

		std::memset(_address, 0, sizeof(_address));
		_countActive = 0;

		BuildTrampoline();
		if (!_trampoline)
		{
			std::cout << "[HWBreakpoint] error: failed to build hook function" << std::endl;
			return;
		}

		_workerStop = true;
		_workerThread = std::thread(WorkerThreadProc);
		std::unique_lock<std::mutex> lock(_mutex);
		_workerSignal.wait(lock, [] { return !_workerStop; });

		_initialize = true;
	}

	void UnInit()
	{
		if (!_initialize)
			return;

		ToggleThreadHook(false);
		_workerStop = true;
		_workerSignal.notify_one();
		_workerThread.join();

		if (_trampoline)
			VirtualFree(_trampoline, 0, MEM_RELEASE);

		_initialize = false;
	}

	void BuildTrampoline()
	{
		ULONG_PTR* rtlThreadStartAddress = (ULONG_PTR*)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlUserThreadStart");

		SYSTEM_INFO si;
		GetSystemInfo(&si);

#ifdef _WIN64

		// search for avalible memory in 2GB boundary to host the tampoline function
		ULONG_PTR gMinAddress = (ULONG_PTR)si.lpMinimumApplicationAddress;
		ULONG_PTR gMaxAddress = (ULONG_PTR)si.lpMaximumApplicationAddress;
		ULONG_PTR minAddr = std::max<ULONG_PTR>(gMinAddress, (ULONG_PTR)rtlThreadStartAddress - 0x20000000);
		ULONG_PTR maxAddr = std::min<ULONG_PTR>(gMaxAddress, (ULONG_PTR)rtlThreadStartAddress + 0x20000000);

		const size_t BlockSize = si.dwPageSize;
		intptr_t min = minAddr / BlockSize;
		intptr_t max = maxAddr / BlockSize;
		int rel = 0;
		_trampoline = nullptr;
		MEMORY_BASIC_INFORMATION mi = { 0 };
		for (int i = 0; i < (max - min + 1); ++i)
		{
			rel = -rel + (i & 1);
			void* pQuery = reinterpret_cast<void*>(((min + max) / 2 + rel) * BlockSize);
			VirtualQuery(pQuery, &mi, sizeof(mi));
			if (mi.State == MEM_FREE)
			{
				_trampoline = (unsigned char*)VirtualAlloc(pQuery, BlockSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
				if (_trampoline != nullptr)
					break;
			}
		}

		if (!_trampoline)
			return;

		// save prologe hooked function
		*(ULONG64*)_orgOpcode = *(ULONG64*)rtlThreadStartAddress;

		*(unsigned char*)&_trampoline[0] = 0x51;                // push rcx
		*(unsigned char*)&_trampoline[1] = 0x52;                // push rdx
		*(unsigned char*)&_trampoline[2] = 0x52;                // push rdx
		*(unsigned short*)&_trampoline[3] = 0x15FF;             // call
		*(DWORD*)&_trampoline[5] = 0x00000018;                  //       ThreadDeutor
		*(unsigned char*)&_trampoline[9] = 0x5A;                // pop rdx
		*(unsigned char*)&_trampoline[10] = 0x5A;               // pop rdx
		*(unsigned char*)&_trampoline[11] = 0x59;               // pop rcx

		*(DWORD*)&_trampoline[12] = 0x48EC8348;                 // sub rsp, 0x48  (2 instruction from prologe of target hook)
		*(unsigned short*)&_trampoline[16] = 0x8B4C;            // mov r9,
		*(unsigned char*)&_trampoline[18] = 0xC9;               //         rcx
		*(short*)&_trampoline[19] = 0x25FF;                     // jmp
		*(DWORD*)&_trampoline[21] = 0x00000000;                 //      rtlThreadStartAddress + 7

		// address data for call & jump
		*(DWORD64*)&_trampoline[25] = (DWORD64)((unsigned char*)rtlThreadStartAddress + 7);
		*(DWORD64*)&_trampoline[33] = (DWORD64)ThreadDeutor;

#else

		if (((unsigned char*)rtlThreadStartAddress)[0] != 0x89 || ((unsigned char*)rtlThreadStartAddress)[4] != 0x89 || ((unsigned char*)rtlThreadStartAddress)[8] != 0xE9)
			return;

		_trampoline = (unsigned char*)VirtualAlloc(NULL, si.dwPageSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (!_trampoline)
			return;

		// save prologe hooked function
		*(ULONG64*)_orgOpcode = *(ULONG64*)rtlThreadStartAddress;

		*(unsigned char*)&_trampoline[0] = 0x50;                            // push eax
		*(unsigned char*)&_trampoline[1] = 0x53;                            // push ebx
		*(unsigned char*)&_trampoline[2] = 0xE8;                            // call
		*(unsigned long*)&_trampoline[3] = (ULONG_PTR)ThreadDeutor - (ULONG_PTR)_trampoline - 7;	//	ThreadDeutor
		*(unsigned char*)&_trampoline[7] = 0x5B;                            // pop ebx
		*(unsigned char*)&_trampoline[8] = 0x58;                            // pop eax

		// execute 2 instruction from prologe of hooked function
		*(unsigned long*)&_trampoline[9] = *rtlThreadStartAddress;
		*(unsigned long*)&_trampoline[13] = *(rtlThreadStartAddress + 1);
		*(unsigned char*)&_trampoline[17] = 0xE9;                           // jmp rtlThreadStartAddress + 8
		*(unsigned long*)&_trampoline[18] = (ULONG_PTR)rtlThreadStartAddress - (ULONG_PTR)_trampoline - 14;


#endif
	}

	void ThreadDeutor()
	{
		std::unique_lock<std::mutex> lock(_mutex);

		_pendingThread = GetCurrentThreadId();
		_workerSignal.notify_one();
		_workerSignal.wait(lock, [] { return _pendingThread == -1; });
	}

	void SetForThreads(std::unique_lock<std::mutex>& lock)
	{
		const DWORD pid = GetCurrentProcessId();

		HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
		THREADENTRY32 te32;
		hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (hThreadSnap == INVALID_HANDLE_VALUE)
			return;

		te32.dwSize = sizeof(THREADENTRY32);
		if (!Thread32First(hThreadSnap, &te32))
		{
			CloseHandle(hThreadSnap);
			return;
		}

		do
		{
			if (te32.th32OwnerProcessID == pid)
			{
				_pendingThread = te32.th32ThreadID;
				_workerSignal.notify_one();
				_workerSignal.wait(lock, [] { return _pendingThread == -1; });
			}
		} while (Thread32Next(hThreadSnap, &te32));
	}

	void RegisterThread(DWORD tid)
	{
		// this function supposed to be called only from worker thread
		if (GetCurrentThreadId() == tid)
			return;

		HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, tid);
		if (!hThread)
			return;

		do
		{
			CONTEXT cxt;
			cxt.ContextFlags = CONTEXT_DEBUG_REGISTERS;

			if (SuspendThread(hThread) == -1)
				break;

			if (!GetThreadContext(hThread, &cxt))
				break;

			for (int index = 0; index < 4; ++index)
			{
				const bool isSet = _address[index] != nullptr;
				SetBits(cxt.Dr7, index * 2, 1, isSet);

				if (isSet)
				{
					switch (index)
					{
					case 0: cxt.Dr0 = (DWORD_PTR)_address[index]; break;
					case 1: cxt.Dr1 = (DWORD_PTR)_address[index]; break;
					case 2: cxt.Dr2 = (DWORD_PTR)_address[index]; break;
					case 3: cxt.Dr3 = (DWORD_PTR)_address[index]; break;
					}

					SetBits(cxt.Dr7, 16 + (index * 4), 2, (int)_when[index]);
					SetBits(cxt.Dr7, 18 + (index * 4), 2, (int)_len[index]);
				}
			}

			if (!SetThreadContext(hThread, &cxt))
				break;

			if (ResumeThread(hThread) == -1)
				break;

			//std::cout << "[HWBreakpoint] Set/Reset BP for thread: " << std::hex << tid << std::endl;

		} while (false);

		CloseHandle(hThread);
	}

	void ToggleThreadHook(bool set)
	{
		if (!_trampoline)
			return;

		//TODO: replacing opcode in system dll might be dangeruos because another thread may invokes and run throw the code while we still replacing it.
		//		the right solution is to do it from another process which first suspend the target process, inject the changes to his memory, and resume it.

		DWORD oldProtect;
		ULONG_PTR* rtlThreadStartAddress = (ULONG_PTR*)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlUserThreadStart");

		if (set)
		{
			VirtualProtect(rtlThreadStartAddress, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
			((unsigned char*)rtlThreadStartAddress)[0] = 0xE9;
			*(DWORD*)&(((unsigned char*)rtlThreadStartAddress)[1]) = (DWORD)_trampoline - (DWORD)rtlThreadStartAddress - 5;

			VirtualProtect(rtlThreadStartAddress, 5, oldProtect, &oldProtect);
		}
		else if (*rtlThreadStartAddress != *(ULONG_PTR*)_orgOpcode)
		{
			VirtualProtect(rtlThreadStartAddress, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
			*(ULONG64*)rtlThreadStartAddress = *(ULONG64*)_orgOpcode;
			VirtualProtect(rtlThreadStartAddress, 5, oldProtect, &oldProtect);
		}
	}

	void WorkerThreadProc()
	{
		_pendingThread = -1;
		_workerStop = false;
		_workerSignal.notify_one();

		while (true)
		{
			std::unique_lock<std::mutex> lock(_mutex);
			_workerSignal.wait(lock, [] { return _pendingThread != -1 || _workerStop; });
			if (_workerStop)
				return;

			if (_pendingThread != -1)
			{
				RegisterThread(_pendingThread);
				_pendingThread = -1;
				_workerSignal.notify_one();
			}
		}
	}
}
