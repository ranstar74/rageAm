#ifdef AM_STANDALONE
#include "thread_standalone.h"
#include "common/types.h"

u32 rageam::render::ThreadStandalone::RenderThreadEntry(const ThreadContext* ctx)
{
	ThreadStandalone* renderThread = static_cast<ThreadStandalone*>(ctx->Param);

	bool needExit;
	do
	{
		needExit = renderThread->UpdateAndCallRenderFn() == false;

		renderThread->m_SwapChain->Present(1, 0);

		renderThread->m_RenderDoneCondition.notify_all();
	} while (!needExit);

	return 0;
}

rageam::render::ThreadStandalone::ThreadStandalone(IDXGISwapChain* swapchain, const TRenderFn& renderFn)
	: Thread(renderFn), m_Thread("Render Thread", RenderThreadEntry, this)
{
	m_SwapChain = swapchain;
}

void rageam::render::ThreadStandalone::WaitRenderDone()
{
	std::unique_lock lock(m_RenderDoneMutex);
	m_RenderDoneCondition.wait(lock);
}
#endif
