//
// File: render.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/singleton.h"
#include "am/system/ptr.h"

#include <d3d11.h>

namespace rageam::graphics
{
	// For integration mode device is hooked up here, however render thread present hook
	// is located in GameIntegration (integration.h), in the same place with other update hooks

	class Render : public Singleton<Render>
	{
#ifdef AM_STANDALONE
		amComPtr<ID3D11RenderTargetView> m_RenderTarget;
		int								 m_RenderWidth;
		int								 m_RenderHeight;
#endif

		void CreateDevice();
#ifdef AM_STANDALONE
		void CreateRT();
#endif

	public:
		Render();

#ifdef AM_STANDALONE
		void SetRenderSize(int width, int height);

		void EnterRenderLoop() AM_INTEGRATED_ONLY(const);
#endif

		amComPtr<ID3D11Device>        Device;
		amComPtr<ID3D11DeviceContext> Context;
		amComPtr<IDXGISwapChain>      Swapchain;
	};

	inline auto RenderGetDevice() { return Render::GetInstance()->Device.Get(); }
	inline auto RenderGetContext() { return Render::GetInstance()->Context.Get(); }
}
