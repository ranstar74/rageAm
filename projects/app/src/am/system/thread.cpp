#include "thread.h"

#include "common/logger.h"
#include "am/system/asserts.h"

rage::atFixedArray<rageam::ThreadContext, rageam::Thread::MAX_THREADS> rageam::Thread::sm_ThreadContexts;

DWORD rageam::Thread::ThreadEntry(LPVOID lpParam)
{
	s32 index = reinterpret_cast<s32>(lpParam); // NOLINT(clang-diagnostic-void-pointer-to-int-cast)

	ThreadContext* ctx = &sm_ThreadContexts[index];
	ctx->Thread->m_Running = true;
	u32 retCode = ctx->EntryPoint(ctx);
	ctx->Thread->m_Running = false;

	return retCode;
}

rageam::Thread::Thread(ConstString debugName, ThreadEntryPoint entryPoint, pVoid param, bool paused)
{
	m_DebugName = debugName;

	s32 threadIndex = sm_ThreadContexts.GetSize();

	m_Handle = CreateThread(
		NULL, 0, ThreadEntry, reinterpret_cast<LPVOID>(threadIndex), CREATE_SUSPENDED, NULL);

	AM_ASSERT(m_Handle != INVALID_HANDLE_VALUE, "Failed to create thread %s", debugName);
	AM_DEBUGF("Thread [%s] started", debugName);

	// Set thread name so it will be displayed in debugger
	wchar_t wName[64];
	swprintf_s(wName, 64, L"[RAGEAM] %hs", debugName);
	SetThreadDescription(m_Handle, wName);

	ThreadContext& ctx = sm_ThreadContexts.Construct();
	ctx.Param = param;
	ctx.EntryPoint = entryPoint;
	ctx.Thread = this;

	m_Suspended = paused;
	if (!paused)
	{
		// Now context is set, we can start thread
		ResumeThread(m_Handle);
	}
}

rageam::Thread::~Thread()
{
	RequestExitAndWait();
}

void rageam::Thread::RequestExitAndWait()
{
	RequestExit();
	WaitExit();
}

void rageam::Thread::WaitExit()
{
	if (m_Handle == INVALID_HANDLE_VALUE)
		return;

	u32 code = WaitForSingleObject(m_Handle, INFINITE);
	CloseHandle(m_Handle);
	m_Handle = INVALID_HANDLE_VALUE;

	AM_DEBUGF("Thread [%s] exited with code %u", m_DebugName, code);
}
