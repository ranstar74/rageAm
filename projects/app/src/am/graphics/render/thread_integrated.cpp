#include "am/integration/gamehooks.h"
#include "common/logger.h"
#ifndef AM_STANDALONE
#include "thread_integrated.h"

#include "am/integration/memory/address.h"
#include "am/integration/memory/hook.h"

namespace
{
	rageam::render::ThreadIntegrated* s_RenderThreadInstance;
	gmAddress s_gAddr_PresentImage;
	gmAddress s_ExeñuteAndRemoveAllAddr;
	std::atomic_bool s_IsExecuting = false;
}

void(*gImpl_grcSetup_PresentImage)();

void rageam::render::ThreadIntegrated::PresentCallback()
{
	static bool initialized = false;
	if (!initialized)
	{
		SetThreadDescription(GetCurrentThread(), L"[RAGE] Render Thread");

		// Initialize game hooks that require rage thread with tls
		GameHooks::InitFromGtaThread();

		initialized = true;
	}

	s_RenderThreadInstance->Render();
}

static void(*gImpl_ExecuteAndRemoveAll)(u64 arg);
void aImpl_ExecuteAndRemoveAll(u64 arg)
{
	s_IsExecuting = true;
	gImpl_ExecuteAndRemoveAll(arg);
	s_IsExecuting = false;
}

void rageam::render::ThreadIntegrated::Render()
{
	if (m_Stopped)
		return;

	bool needToStop = !UpdateAndCallRenderFn();

	// We must present image in either case because otherwise render thread will deadlock
	gImpl_grcSetup_PresentImage();
	m_RenderDoneCondition.notify_all();

	if (needToStop)
	{
		AM_DEBUG("RenderThreadIntegrated::Render() -> Stopping...");
		Hook::Remove(s_gAddr_PresentImage);
		m_Stopped = true;
	}
}

rageam::render::ThreadIntegrated::ThreadIntegrated(const TRenderFn& renderFn) : Thread(renderFn)
{
	s_gAddr_PresentImage = gmAddress::Scan(
		"40 55 53 56 57 41 54 41 56 41 57 48 8B EC 48 83 EC 40 48 8B 0D", "rage::grcSetup::Present");
	s_ExeñuteAndRemoveAllAddr = gmAddress::Scan("E8 ?? ?? ?? ?? 48 8B 4E 08 48 8B 01 FF 50").GetCall();

	Hook::Create(s_ExeñuteAndRemoveAllAddr, aImpl_ExecuteAndRemoveAll, &gImpl_ExecuteAndRemoveAll, true);
	Hook::Create(s_gAddr_PresentImage, PresentCallback, &gImpl_grcSetup_PresentImage);

	s_RenderThreadInstance = this;
}

rageam::render::ThreadIntegrated::~ThreadIntegrated()
{
	Hook::Remove(s_ExeñuteAndRemoveAllAddr);
}

void rageam::render::ThreadIntegrated::WaitRenderDone()
{
	std::unique_lock lock(m_RenderDoneMutex);
	m_RenderDoneCondition.wait(lock);
}

void rageam::render::ThreadIntegrated::WaitExecutingDone()
{
	while(s_IsExecuting) {}
}
#endif
