#include "am/integration/gamehooks.h"
#include "common/logger.h"
#ifndef AM_STANDALONE
#include "thread_integrated.h"

#include "am/integration/memory/address.h"
#include "am/integration/memory/hook.h"

static rageam::render::ThreadIntegrated* s_RenderThreadInstance;

static gmAddress s_gAddr_PresentImage;

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

	Hook::Create(s_gAddr_PresentImage, PresentCallback, &gImpl_grcSetup_PresentImage);

	s_RenderThreadInstance = this;
}

void rageam::render::ThreadIntegrated::WaitRenderDone()
{
	std::unique_lock lock(m_RenderDoneMutex);
	m_RenderDoneCondition.wait(lock);
}
#endif
