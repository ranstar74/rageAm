#include "outlinerender.h"

#include "color.h"
#include "dx11.h"
#include "common/logger.h"
#include "render.h"
#include "am/integration/memory/address.h"
#include "am/integration/memory/hook.h"
#include "helpers/dx11.h"

void rageam::graphics::OutlineRender::CreateDeviceObjects()
{
	AM_DEBUGF("OutlineRender::CreateDeviceObjects() -> For %ux%u", m_Width, m_Height);

	m_FinalColor = DX11::CreateTexture("Outline Final", CreateTexture_RenderTarget, m_Width, m_Height, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
	m_FinalColorView = DX11::CreateTextureView("Outline Final View", m_FinalColor);
	m_FinalColorRT = DX11::CreateRenderTargetView("Outline Final RT", m_FinalColor);

	m_SobelColor = DX11::CreateTexture("Outline Sobel", CreateTexture_RenderTarget, m_Width, m_Height, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
	m_SobelColorView = DX11::CreateTextureView("Outline Sobel View", m_SobelColor);
	m_SobelColorRT = DX11::CreateRenderTargetView("Outline Sobel RT", m_SobelColor);

	m_GeomColor = DX11::CreateTexture("Outline GBuffer", CreateTexture_RenderTarget, m_Width, m_Height, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
	m_GeomColorView = DX11::CreateTextureView("Outline GBuffer View", m_GeomColor);
	m_GeomColorRT = DX11::CreateRenderTargetView("Outline GBuffer RT", m_GeomColor);

	ColorU32 white = COLOR_WHITE;
	amComPtr<ID3D11Texture2D> whiteTex = DX11::CreateTexture("Outline White", CreateTexture_Default, 1, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &white);
	m_WhiteTexView = DX11::CreateTextureView("Outline White View", whiteTex);
}

// This function is called in grcDevice::DrawIndexed, DrawPrimitive, DrawVertices, so always before drawing vertex buffer
gmAddress sAddr_grcDevice_SetUpPriorToDraw;
void (*gImpl_grcDevice_SetUpPriorToDraw)(int drawMode);
void aImpl_grcDevice_SetUpPriorToDraw(int drawMode)
{
	gImpl_grcDevice_SetUpPriorToDraw(drawMode);

	// Set diffuse texture of whatever we're drawing to white color
	// This will help drawing outlines for decal meshes, so GPU won't cull transparent pixels
	auto outlineRender = rageam::graphics::OutlineRender::GetInstance();
	if (outlineRender->IsRenderingOutline())
	{
		ID3D11ShaderResourceView* overrideView = outlineRender->GetWhiteTexture();;
		DXCONTEXT->PSSetShaderResources(0, 1, &overrideView);
	}
}

rageam::graphics::OutlineRender::OutlineRender()
{
	sAddr_grcDevice_SetUpPriorToDraw = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"89 4C 24 08 48 83 EC 68 E8 ?? ?? ?? ?? C7 44 24 34 00 00 00 00", "rage::grcDevice::SetUpPriorToDraw");
#else
		"E8 ?? ?? ?? ?? 85 DB 74 45", "rage::grcDevice::SetUpPriorToDraw").GetCall();
#endif
	Hook::Create(sAddr_grcDevice_SetUpPriorToDraw, aImpl_grcDevice_SetUpPriorToDraw, &gImpl_grcDevice_SetUpPriorToDraw);
}

rageam::graphics::OutlineRender::~OutlineRender()
{
	Hook::Remove(sAddr_grcDevice_SetUpPriorToDraw);
}

void rageam::graphics::OutlineRender::NewFrame()
{
	if (!m_Initialized)
	{
		m_SobelPS = Shader::CreateFromPath(ShaderPixel, L"im_sobel_ps.hlsl");
		m_GaussPS = Shader::CreateFromPath(ShaderPixel, L"im_gauss_ps.hlsl");

		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		AM_ASSERT_STATUS(DXDEVICE->CreateBlendState(&blendDesc, &m_BlendState));

		// Using this blend state we set alpha of all drawn pixels to 1.0,
		// then in sobel shader we can use it to check if pixel was drawn and ignore the color
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX; // Set alpha to 1.0
		AM_ASSERT_STATUS(DXDEVICE->CreateBlendState(&blendDesc, &m_BlendStateAlpha1));

		DX11::SetDebugName(m_BlendState, "Overlay Blend State");
		DX11::SetDebugName(m_BlendStateAlpha1, "Overlay Blend State Alpha 1");

		m_Initialized = true;
	}

	DX11::ClearRenderTarget(m_FinalColorRT);
	DX11::ClearRenderTarget(m_SobelColorRT);
	DX11::ClearRenderTarget(m_GeomColorRT);
}

void rageam::graphics::OutlineRender::EndFrame()
{
	// Backup state
	DXCONTEXT->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, m_OldRTs, &m_OldDepthStencilView);
	DXCONTEXT->OMGetBlendState(&m_OldBlendState, m_OldBlendFactor, &m_OldSampleMask);

	DXCONTEXT->OMSetBlendState(m_BlendState.Get(), nullptr, 0xFFFFFFFF);
	m_Blit.Bind();

	// Draw outlines
	DX11::SetRenderTarget(m_SobelColorRT);
	m_SobelPS->Bind();
	m_Blit.Draw(m_GeomColorView);

	// Draw with gaussian
	DX11::SetRenderTarget(m_FinalColorRT);
	m_GaussPS->Bind();
	m_Blit.Draw(m_SobelColorView);

	// Draw to the screen
	DXCONTEXT->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, m_OldRTs, nullptr);
	m_Blit.BindDefPS();
	m_Blit.Draw(m_FinalColorView);

	// Restore state
	DXCONTEXT->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, m_OldRTs, m_OldDepthStencilView);
	DXCONTEXT->OMSetBlendState(m_OldBlendState, m_OldBlendFactor, m_OldSampleMask);
	for (auto& rt : m_OldRTs) SAFE_RELEASE(rt);
	SAFE_RELEASE(m_OldBlendState);
	SAFE_RELEASE(m_OldDepthStencilView);
}

bool rageam::graphics::OutlineRender::Begin()
{
	AM_ASSERTS(!m_Began, "OutlineRender::Begin() -> Begin/End mismatch!");
	
	// Create GPU objects if screen was resized or we didn't initialize them yet
	u32 width, height;
	rage::grcDevice::GetScreenSize(width, height);
	if (m_Width != width || m_Height != height)
	{
		m_Width = width;
		m_Height = height;
		CreateDeviceObjects();
	}

	// Backup state
	DXCONTEXT->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, m_OldRTs, &m_OldDepthStencilView);
	DXCONTEXT->OMGetBlendState(&m_OldBlendState, m_OldBlendFactor, &m_OldSampleMask);

	// Set our state
	DX11::SetRenderTarget(m_GeomColorRT); // We don't need depth because we want to draw outlines to be on top of everything
	DXCONTEXT->OMSetBlendState(m_BlendStateAlpha1.Get(), nullptr, 0xFFFFFFFF);

	m_Began = true;
	m_RenderingOutline = true;
	return true;
}

void rageam::graphics::OutlineRender::End()
{
	AM_ASSERTS(m_Began, "OutlineRender::End() -> Begin/End mismatch!");

	// Restore state
	DXCONTEXT->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, m_OldRTs, m_OldDepthStencilView);
	DXCONTEXT->OMSetBlendState(m_OldBlendState, m_OldBlendFactor, m_OldSampleMask);
	for (auto& rt : m_OldRTs) SAFE_RELEASE(rt);
	SAFE_RELEASE(m_OldBlendState);
	SAFE_RELEASE(m_OldDepthStencilView);

	m_RenderingOutline = false;
	m_Began = false;
}
