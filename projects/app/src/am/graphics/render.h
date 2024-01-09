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
	class Render : public Singleton<Render>
	{
#ifdef AM_STANDALONE
		amComPtr<ID3D11RenderTargetView>	m_RenderTarget;
		int									m_RenderWidth;
		int									m_RenderHeight;
#endif

		void CreateDevice();
		void CreateRT();

	public:
		Render();
		~Render() override;

#ifdef AM_STANDALONE
		void SetRenderSize(int width, int height);
#endif

		// Must be called on loop after handling window events, this function
		// renders draw list and calls present (only in standalone) mode
		bool DoRender();
		// Called from main (update) thread, builds UI draw list
		void BuildDrawLists();
		// In case of integration mode hooks render thread and immediately returns
		void EnterRenderLoop();
		// Pauses caller thread until frame render is finished and pauses render
		// thread until Unlock() is called
		// If was called during frame execution, frame is first finished and only then paused
		void Lock() const;
		void Unlock() const;

		amComPtr<ID3D11Device>			Device;
		amComPtr<ID3D11DeviceContext>	Context;
		amComPtr<IDXGISwapChain>		Swapchain;
	};

	inline auto RenderGetDevice() { return Render::GetInstance()->Device.Get(); }
	inline auto RenderGetContext() { return Render::GetInstance()->Context.Get(); }
}
