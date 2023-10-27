#include "engine.h"

#include "am/desktop/window.h"

#ifdef AM_STANDALONE
#include "thread_standalone.h"
#else
#include "thread_integrated.h"
#endif

rageam::render::Engine::Engine(bool useWindow)
{
	m_UseWindow = useWindow;
	m_Device.Create(useWindow);

	// Non-window render has no render thread
	if (!useWindow)
		return;

#ifdef AM_STANDALONE
	m_RenderThread = std::make_unique<ThreadStandalone>(m_Device.GetSwapchain(), InvokeRenderFunction);
#else
	m_RenderThread = std::make_unique<ThreadIntegrated>(InvokeRenderFunction);
#endif

	if (m_UseWindow)
	{
		Window* window = WindowFactory::GetWindow();
		window->OnResizeEnd.Listen(this, [this](WindowSize size)
			{
				// IDXGISwapChain::ResizeBuffers is invoked by game in integration mode
#ifdef AM_STANDALONE
				// I had to disable this to support imgui viewports but this may cause weird issues
				//WaitRenderDone();

				IDXGISwapChain* swapchain = m_Device.GetSwapchain();
				swapchain->ResizeBuffers(0, size.Width, size.Height, DXGI_FORMAT_UNKNOWN, 0);
#endif
			});
	}
}

rageam::render::Engine::~Engine()
{
	Stop();
}

void rageam::render::Engine::SetRenderFunction(const TRenderFn& fn)
{
	std::unique_lock lock(m_Mutex);

	m_RenderFn = fn;
}

void rageam::render::Engine::Stop()
{
	if (m_UseWindow)
	{
		m_RenderThread->RequestStop();
		while (m_RenderThread->IsRunning())
		{
			// Wait until render thread exits	
		}
	}

	m_Device.Destroy();
}

bool rageam::render::Engine::InvokeRenderFunction()
{
	Engine* instance = GetInstance();
	if (!instance)
		return true;

	std::unique_lock lock(instance->m_Mutex);

	if (!instance->m_RenderFn)
		return true;

	return instance->m_RenderFn();
}

void D3DAssertHandler(HRESULT status, ConstString fmt, ...)
{
	if (status == S_OK)
		return;

	char buffer[256];
	va_list args;
	va_start(args, fmt);
	vsprintf_s(buffer, 256, fmt, args);
	va_end(args);

	AM_UNREACHABLE("%s ; Error Code: %u", buffer, status);
}
