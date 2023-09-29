#include "device.h"

#include "am/desktop/window.h"
#include "am/integration/memory/address.h"
#include "am/system/asserts.h"
#include "rage/paging/ref.h"

void rageam::render::Device::Create(bool useWindow)
{
#ifdef AM_STANDALONE
	DXGI_SWAP_CHAIN_DESC scDesc{};

	if (useWindow)
	{
		scDesc.BufferCount = 2;
		scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scDesc.BufferDesc.RefreshRate.Numerator = 60;
		scDesc.BufferDesc.RefreshRate.Denominator = 1;
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		scDesc.SampleDesc.Count = 1;

		scDesc.OutputWindow = WindowFactory::GetWindow()->GetHandle();
		scDesc.Windowed = TRUE;
	}

	D3D_FEATURE_LEVEL featureLevel;

	UINT creationFlags = 0;
#if _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	constexpr D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
	HRESULT result = D3D11CreateDeviceAndSwapChain(
		NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, creationFlags,
		featureLevels, 1, D3D11_SDK_VERSION,
		useWindow ? &scDesc : NULL, &m_Swapchain, &m_Factory, &featureLevel, &m_Context);

	AM_ASSERT(result == S_OK, "Failed to create DirectX device, result: %x", result);
#else
	gmAddress gAddr_CreateWindowAndDevice = gmAddress::Scan(
		"48 8D 05 ?? ?? ?? ?? 45 33 C9 48 89 44 24 58 48 8D 85 D0 08 00 00", "rage::grcSetup::CreateWindowAndDevice");

	m_Context = *gAddr_CreateWindowAndDevice.GetRef(3).To<ID3D11DeviceContext**>();
	m_Factory = *gAddr_CreateWindowAndDevice.GetRef(33).To<ID3D11Device**>();
	m_Swapchain = *gAddr_CreateWindowAndDevice.GetRef(47).To<IDXGISwapChain**>();
#endif
}

void rageam::render::Device::Destroy()
{
	// TODO: We can add ref in create function and simply use com ptr

	// We don't release objects in integration mode because we don't own them (game does)
#ifdef AM_STANDALONE
	SAFE_RELEASE(m_Factory);
	SAFE_RELEASE(m_Context);
	SAFE_RELEASE(m_Swapchain);
#endif
}
