//
// File: engine.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <mutex>

#include "am/desktop/window.h"
#include "am/graphics/render/thread.h"
#include "am/graphics/render/device.h"

void D3DAssertHandler(HRESULT status, ConstString fmt, ...);
#define AM_ASSERT_D3D(status, fmt, ...) D3DAssertHandler(status, fmt, __VA_ARGS__)
#define AM_ASSERT_D3D(status, msg) D3DAssertHandler(status, msg)
#define AM_ASSERT_D3D(status) D3DAssertHandler(status, "")

namespace rageam::render
{
	using TRenderFn = std::function<bool()>;

	/**
	 * \brief The root of rendering pipeline.
	 * \n Pumped by RenderThreadStandalone / RenderThreadIntegrated depending on build config.
	 */
	class Engine : EventAwareBase
	{
		amUniquePtr<Thread> m_RenderThread;
		Device m_Device;
		TRenderFn m_RenderFn;
		std::mutex m_Mutex;
		bool m_UseWindow;

		static inline Engine* sm_Instance = nullptr;

		static bool InvokeRenderFunction();
	public:
		Engine(bool useWindow);
		~Engine();

		void WaitRenderDone() const { m_RenderThread->WaitRenderDone(); }
		void WaitExecutingDone() const { m_RenderThread->WaitExecutingDone(); }
		void SetRenderFunction(const TRenderFn& fn);

		void Stop();

		ID3D11Device* GetFactory() const { return m_Device.GetFactory(); }
		ID3D11DeviceContext* GetDeviceContext() const { return m_Device.GetContext(); }
		IDXGISwapChain* GetSwapchain() const { return m_Device.GetSwapchain(); }

		static void SetInstance(Engine* instance) { sm_Instance = instance; }
		static Engine* GetInstance() { return sm_Instance; }
	};

	inline ID3D11Device* GetDevice() { return Engine::GetInstance()->GetFactory(); }
	inline ID3D11DeviceContext* GetDeviceContext() { return Engine::GetInstance()->GetDeviceContext(); }
	inline IDXGISwapChain* GetSwapchain() { return Engine::GetInstance()->GetSwapchain(); }
}
