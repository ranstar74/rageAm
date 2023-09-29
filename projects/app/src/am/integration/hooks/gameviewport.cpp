#include "gameviewport.h"

#include "common/logger.h"
#include "am/integration/memory/address.h"
#include "am/integration/memory/hook.h"
#include "am/system/ptr.h"

static bool s_GameWindowResizing = false;
static u32 s_GameWidth = 0;
static u32 s_GameHeight = 0;
//
//BOOL(*gImpl_GetClientRect)(HWND, LPRECT);
//BOOL aImpl_GetClientRect(HWND hWnd, LPRECT lpRect)
//{
//	if (s_GameWindowResizing && lpRect)
//	{
//		lpRect->left = 0;
//		lpRect->top = 0;
//		lpRect->right = (LONG)s_GameWidth;
//		lpRect->bottom = (LONG)s_GameHeight;
//		return true;
//	}
//	return gImpl_GetClientRect(hWnd, lpRect);
//}
//
//bool(*gImpl_UpdateResize)();
//bool aImpl_UpdateResize()
//{
//	s_GameWindowResizing = true;
//	bool result = gImpl_UpdateResize();
//	s_GameWindowResizing = false;
//
//	return result;
//}
//
//HRESULT(STDMETHODCALLTYPE* gImpl_GetBuffer)(IDXGISwapChain* inst, UINT Buffer, REFIID riid, void** ppSurface);
//HRESULT STDMETHODCALLTYPE GetBuffer(IDXGISwapChain* inst, UINT Buffer, REFIID riid, void** ppSurface)
//{
//	if (!s_GameWindowResizing)
//		return gImpl_GetBuffer(inst, Buffer, riid, ppSurface);
//
//	// Return our texture where game will be drawn instead of SwapChain back buffer
//	*ppSurface = hooks::GameViewport::GetGameTexture();
//	return S_OK;
//}
//
//HRESULT(STDMETHODCALLTYPE* gImpl_IDXGISwapChain_ResizeBuffers)(IDXGISwapChain* inst, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
//HRESULT STDMETHODCALLTYPE aImpl_IDXGISwapChain_ResizeBuffers(IDXGISwapChain* inst, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
//{
//	if (!s_GameWindowResizing)
//		return gImpl_IDXGISwapChain_ResizeBuffers(inst, BufferCount, Width, Height, NewFormat, SwapChainFlags);
//
//	// Nothing to resize here, game buffer was resized already in ::SetResolution
//	return S_OK;
//}

void hooks::GameViewport::CreateGameBuffer()
{
	//DestroyGameBuffer();

	//IDXGISwapChain* swapchain = rageam::RenderDevice::GetSwapchain();
	//ID3D11Device* device = rageam::RenderDevice::GetDevice();

	//D3D11_TEXTURE2D_DESC gameTextureDesc;

	//// Use original back buffer desc for our custom one
	//ID3D11Texture2D* pBackBuffer;
	//swapchain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	//pBackBuffer->GetDesc(&gameTextureDesc);
	//pBackBuffer->Release();

	//// Override resolution
	//gameTextureDesc.Width = s_GameWidth;
	//gameTextureDesc.Height = s_GameHeight;
	//gameTextureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	//device->CreateTexture2D(&gameTextureDesc, NULL, &sm_GameTexture);

	//D3D11_SHADER_RESOURCE_VIEW_DESC gameTextureViewDesc = {};
	//gameTextureViewDesc.Format = gameTextureDesc.Format;
	//gameTextureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	//gameTextureViewDesc.Texture2D.MipLevels = 1;
	//device->CreateShaderResourceView(sm_GameTexture, &gameTextureViewDesc, &sm_GameShaderView);
}

void hooks::GameViewport::DestroyGameBuffer()
{
	//SAFE_RELEASE(sm_GameShaderView);
	//SAFE_RELEASE(sm_GameTexture);
}

void hooks::GameViewport::DoUpdate()
{
	if (!sm_Enabled)
		return;

	//CreateGameBuffer();
	//aImpl_UpdateResize();
}

void hooks::GameViewport::Init()
{
	//IDXGISwapChain* swapchain = rageam::RenderDevice::GetSwapchain();

	//// Hook IDXGISwapChain::GetBuffers && IDXGISwapChain::Resize virtual methods
	//gmAddress swapChain_GetBuffers = *(u64*)(*(u64*)swapchain + 0x48);
	//gmAddress swapChain_Resize = *(u64*)(*(u64*)swapchain + 0x68);
	//Hook::Create(swapChain_Resize, aImpl_IDXGISwapChain_ResizeBuffers, &gImpl_IDXGISwapChain_ResizeBuffers);
	//Hook::Create(swapChain_GetBuffers, GetBuffer, &gImpl_GetBuffer);

	//Hook::Create(GetClientRect, aImpl_GetClientRect, &gImpl_GetClientRect);

	//gmAddress gAddr_UpdateResize = gmAddress::Scan("48 8B C4 55 53 56 57 41 54 41 56 41 57 48 8D A8 F8");
	//Hook::Create(gAddr_UpdateResize, aImpl_UpdateResize, &gImpl_UpdateResize);
}

void hooks::GameViewport::Enable()
{
	CreateGameBuffer();

	sm_Enabled = true;
}

void hooks::GameViewport::Disable()
{
	// Resize back to original window size
	//gImpl_UpdateResize();

	//DestroyGameBuffer();
}

void hooks::GameViewport::SetResolution(u32 width, u32 height)
{
	s_GameWidth = width;
	s_GameHeight = height;

	DoUpdate();
}
