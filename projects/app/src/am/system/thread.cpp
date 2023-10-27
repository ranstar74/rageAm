#include "thread.h"

#include "common/logger.h"
#include "am/system/asserts.h"

DWORD rageam::Thread::ThreadEntry(LPVOID lpParam)
{
	ThreadContext* ctx = (ThreadContext*)lpParam;

	ctx->Thread->m_Running = true;
	u32 retCode = ctx->EntryPoint(ctx);
	ctx->Thread->m_Running = false;

	delete ctx;

	return retCode;
}

rageam::Thread::Thread(ConstString debugName, ThreadEntryPoint entryPoint, pVoid param, bool paused)
{
	ThreadContext* ctx = new ThreadContext();

	m_DebugName = debugName;
	m_Handle = CreateThread(NULL, 0, ThreadEntry, ctx, CREATE_SUSPENDED, NULL);

	AM_ASSERT(m_Handle != INVALID_HANDLE_VALUE, "Failed to create thread %s", debugName);
	AM_DEBUGF("Thread [%s] started", debugName);

	// Set thread name so it will be displayed in debugger
	wchar_t wName[64];
	swprintf_s(wName, 64, L"[RAGEAM] %hs", debugName);
	SetThreadDescription(m_Handle, wName);

	// Fill thread context before resuming thread
	ctx->Param = param;
	ctx->EntryPoint = entryPoint;
	ctx->Thread = this;

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

void rageam::CurrentThreadSleep(u32 ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
