#include "render.h"

#include "am/asset/types/hotdrawable.h"
#include "am/graphics/window.h"
#include "am/integration/integration.h"
#include "am/system/system.h"
#include "am/ui/imglue.h"

#ifdef AM_INTEGRATED
#include "am/integration/memory/address.h"
#include "am/integration/memory/hook.h"
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
#elif APP_BUILD_2699_16
	gmAddress addr = gmAddress::Scan(
		"48 8D 05 ?? ?? ?? ?? 45 33 C9 48 89 44 24 58 48 8D 85 D0 08 00 00", "rage::grcDevice::InitClass");
	Context = amComPtr(*addr.GetRef(3).To<ID3D11DeviceContext**>());
	Device = amComPtr(*addr.GetRef(30 + 3).To<ID3D11Device**>());
	Swapchain = amComPtr(*addr.GetRef(44 + 3).To<IDXGISwapChain**>());
#else // APP_BUILD_3095_0
	gmAddress addr = gmAddress::Scan( // 4th byte from the end is different comparing to APP_BUILD_2699_16
		"48 8D 05 ?? ?? ?? ?? 45 33 C9 48 89 44 24 58 48 8D 85 E0 08 00 00", "rage::grcDevice::InitClass");
	Context = amComPtr(*addr.GetRef(3).To<ID3D11DeviceContext**>());
	Device = amComPtr(*addr.GetRef(30 + 3).To<ID3D11Device**>());
	Swapchain = amComPtr(*addr.GetRef(44 + 3).To<IDXGISwapChain**>());
#endif

	Context->AddRef();
	Device->AddRef();
	Swapchain->AddRef();

#endif
}

#ifdef AM_STANDALONE
void rageam::graphics::Render::CreateRT()
{
	ID3D11Texture2D* backBuffer = nullptr;
	ID3D11RenderTargetView* backBufferRT = nullptr;
	AM_ASSERT_STATUS(Swapchain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
	AM_ASSERT_STATUS(Device->CreateRenderTargetView(backBuffer, nullptr, &backBufferRT));
	backBuffer->Release();
	m_RenderTarget = amComPtr(backBufferRT);
}
#endif

rageam::graphics::Render::Render()
{
	CreateDevice();
#ifdef AM_STANDALONE
	CreateRT();
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

void rageam::graphics::Render::EnterRenderLoop() AM_INTEGRATED_ONLY(const)
{
	Window* window = Window::GetInstance();
	AM_INTEGRATED_ONLY(window->UpdateInit());

#ifdef AM_STANDALONE
	auto ui = ui::GetUI();
	while (true)
	{
		if (!window->Update())
			break;

		// Apply window resize
		int newWidth, newHeight;
		if (WindowGetNewSize(newWidth, newHeight))
		{
			SetRenderSize(newWidth, newHeight);
		}

		static constexpr float CLEAR_COLOR[] = { 0.0f, 0.0f, 32.0f / 255.0f, 1.0f };
		ID3D11RenderTargetView* rt = m_RenderTarget.Get();
		Context->OMSetRenderTargets(1, &rt, nullptr);
		Context->ClearRenderTargetView(rt, CLEAR_COLOR);

		ui->BeginFrame();
		ui->UpdateApps();
		ui->EndFrame();

		(void) Swapchain->Present(1, 0);
	}

#else

#endif
}
#endif
