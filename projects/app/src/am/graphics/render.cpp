#include "render.h"

#include "am/system/asserts.h"
#include "am/graphics/window.h"
#include "am/ui/imglue.h"

#ifdef AM_INTEGRATED
#include "am/integration/memory/address.h"
#include "am/integration/memory/hook.h"
#endif

namespace
{
#ifdef AM_INTEGRATED
	gmAddress			s_Addr_PresentImage = 0;
	gmAddress			s_Addr_IdleSection = 0;
	std::atomic_bool	s_BgThreadNeedToStop;
	std::atomic_bool	s_BgThreadStopped = true;
	std::atomic_bool	s_DoneRender = false;
#endif
	// We are not locking thread for long duration, there is
	// no point to use condition variables here
	std::atomic_bool	s_Locked;
	std::atomic_bool	s_Rendering;
}

#ifdef AM_INTEGRATED
void(*gImpl_PresentImage)();
void aImpl_PresentImage()
{
	static bool s_Initialized = false;
	if (!s_Initialized)
	{
		(void)SetThreadDescription(GetCurrentThread(), L"[RAGE] Render Thread");
		s_Initialized = true;
	}

	if (s_Locked) {} // Wait until render is unlocked...

	s_Rendering = true;
	if (!s_DoneRender) // Game might not call present function every frame, we have to sync it ourselfs
	{
		// Dispatch our calls right before game present
		rageam::graphics::Render::GetInstance()->DoRender();
		s_DoneRender = true;
	}
	gImpl_PresentImage();
	s_Rendering = false;

	if (s_BgThreadNeedToStop)
	{
		s_BgThreadStopped = true;
		Hook::Remove(s_Addr_PresentImage);
	}
}
#endif

void rageam::graphics::Render::CreateDevice()
{
#ifdef AM_STANDALONE
	HWND windowHandle = graphics::WindowGetHWND();

	DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
	DXGI_SWAP_CHAIN_DESC* pSwapchainDesc = windowHandle != NULL ? &swapchainDesc : NULL;

	if (windowHandle)
	{
		swapchainDesc.BufferCount = 2;
		swapchainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapchainDesc.SampleDesc.Count = 1;
		swapchainDesc.OutputWindow = windowHandle;
		swapchainDesc.Windowed = TRUE;
	}

	D3D_FEATURE_LEVEL featureLevel;

	UINT creationFlags = 0;
#if DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	constexpr D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

	ID3D11Device* device;
	ID3D11DeviceContext* context;
	IDXGISwapChain* swapchain;

	HRESULT result = D3D11CreateDeviceAndSwapChain(
		NULL, D3D_DRIVER_TYPE_HARDWARE,
		NULL, creationFlags, featureLevels, 1, D3D11_SDK_VERSION,
		pSwapchainDesc, &swapchain, &device, &featureLevel, &context);
	AM_ASSERT_STATUS(result, "Render::Create() -> Failed to create DX11 Device; Code: %x", result);

	Device = amComPtr(device);
	Context = amComPtr(context);
	Swapchain = amComPtr(swapchain);

	// Get dimensions of the back buffer
	ID3D11Texture2D* backBuffer = nullptr;
	D3D11_TEXTURE2D_DESC backBufferDesc = {};
	AM_ASSERT_STATUS(Swapchain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
	backBuffer->GetDesc(&backBufferDesc);
	backBuffer->Release();

	m_RenderWidth = static_cast<int>(backBufferDesc.Width);
	m_RenderHeight = static_cast<int>(backBufferDesc.Height);
#else

#if APP_BUILD_2699_16_RELEASE_NO_OPT
	gmAddress addr = gmAddress::Scan(
		"45 33 C0 8B 94 24 04 01 00 00 33 C9", "rage::grcDevice::InitClass").GetAt(-95);
	Context = amComPtr(*addr.GetRef(3).To<ID3D11DeviceContext**>());
	Device = amComPtr(*addr.GetRef(25 + 3).To<ID3D11Device**>());
	Swapchain = amComPtr(*addr.GetRef(37 + 3).To<IDXGISwapChain**>());
#elif APP_BUILD_2699_16_RELEASE
	gmAddress addr = gmAddress::Scan(
		"48 8D 05 ?? ?? ?? ?? BE 07 00 00 00", "rage::grcDevice::InitClass");
	Context = amComPtr(*addr.GetRef(3).To<ID3D11DeviceContext**>());
	Device = amComPtr(*addr.GetRef(32 + 3).To<ID3D11Device**>());
	Swapchain = amComPtr(*addr.GetRef(47 + 3).To<IDXGISwapChain**>());
#else
	gmAddress addr = gmAddress::Scan(
		"48 8D 05 ?? ?? ?? ?? 45 33 C9 48 89 44 24 58 48 8D 85 D0 08 00 00", "rage::grcDevice::InitClass");
	Context = amComPtr(*addr.GetRef(3).To<ID3D11DeviceContext**>());
	Device = amComPtr(*addr.GetRef(33).To<ID3D11Device**>());
	Swapchain = amComPtr(*addr.GetRef(47).To<IDXGISwapChain**>());
#endif

	Context->AddRef();
	Device->AddRef();
	Swapchain->AddRef();
#endif
}

void rageam::graphics::Render::CreateRT()
{
#ifdef AM_STANDALONE
	ID3D11Texture2D* backBuffer = nullptr;
	ID3D11RenderTargetView* backBufferRT = nullptr;
	AM_ASSERT_STATUS(Swapchain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
	AM_ASSERT_STATUS(Device->CreateRenderTargetView(backBuffer, nullptr, &backBufferRT));
	backBuffer->Release();
	m_RenderTarget = amComPtr(backBufferRT);
#endif
}

rageam::graphics::Render::Render()
{
	CreateDevice();
	CreateRT();
}

rageam::graphics::Render::~Render()
{
#ifdef AM_INTEGRATED
	s_BgThreadNeedToStop = true;
	while (!s_BgThreadStopped) {}

	// Present image hook is removed in aImpl_PresentImage
	Hook::Remove(s_Addr_IdleSection);
#endif
}

#ifdef AM_STANDALONE
void rageam::graphics::Render::SetRenderSize(int width, int height)
{
	if (m_RenderWidth == width && m_RenderHeight == height)
		return;

	m_RenderTarget = nullptr;
	AM_ASSERT_STATUS(Swapchain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0));
	CreateRT();

	m_RenderWidth = width;
	m_RenderHeight = height;
}
#endif

bool rageam::graphics::Render::DoRender()
{
	AM_ASSERT(Swapchain != nullptr, "Render::DoRender() -> Window was not created, can't render frame.");

	// We still want to see the game, right? RT is already bound in game too
#ifdef AM_STANDALONE
	static constexpr float CLEAR_COLOR[] = { 0.0f, 0.0f, 32.0f / 255.0f, 1.0f };
	ID3D11RenderTargetView* rt = m_RenderTarget.Get();
	Context->OMSetRenderTargets(1, &rt, nullptr);
	Context->ClearRenderTargetView(rt, CLEAR_COLOR);
#endif

	ui::GetUI()->Render();

	// In integrated mode present is called by native render system
#ifdef AM_STANDALONE
	(void)Swapchain->Present(1, 0);
#endif

	return true;
}

void rageam::graphics::Render::BuildDrawLists()
{
#ifdef AM_INTEGRATED
	// Wait until render is finished... game might skip some frames
	if (!s_DoneRender)
		return;
#endif

	ui::GetUI()->BuildDrawList();

#ifdef AM_INTEGRATED
	s_DoneRender = false;
#endif
}

void rageam::graphics::Render::EnterRenderLoop()
{
	Window* window = Window::GetInstance();
	window->UpdateInit();

	auto ui = ui::GetUI();

#ifdef AM_STANDALONE
	ui->SetEnabled(true);
	while (true)
	{
		while (s_Locked) {}

		s_Rendering = true;

		if (!window->Update())
			break;

		// Apply window resize
		int newWidth, newHeight;
		if (WindowGetNewSize(newWidth, newHeight))
		{
			SetRenderSize(newWidth, newHeight);
		}

		// In standalone there's only single thread
		ui->BuildDrawList();
		if (!DoRender())
			break;

		s_Rendering = false;
}
	s_Rendering = false;

#else
	s_BgThreadStopped = false;
	s_Addr_PresentImage = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"48 89 4C 24 08 48 81 EC F8 00 00 00 E8 ?? ?? ?? ?? 8B 05",
#elif APP_BUILD_2699_16_RELEASE
		"40 55 53 56 57 41 54 41 56 41 57 48 8B EC 48 83 EC 70 48",
#else
		"40 55 53 56 57 41 54 41 56 41 57 48 8B EC 48 83 EC 40 48 8B 0D",
#endif
		"rage::grcDevice::EndFrame");

	Hook::Create(s_Addr_PresentImage, aImpl_PresentImage, &gImpl_PresentImage);

	// Render is now initialized, we can enable UI
	ui->SetEnabled(true);
#endif
}

void rageam::graphics::Render::Lock() const
{
	AM_ASSERT(!s_Locked, "Render::Lock() -> Rendering is already locked!");
	s_Locked = true;
	while (s_Rendering) {}
}

void rageam::graphics::Render::Unlock() const
{
	AM_ASSERT(s_Locked, "Render::Unlock() -> Rendering is not locked!");
	s_Locked = false;
}
