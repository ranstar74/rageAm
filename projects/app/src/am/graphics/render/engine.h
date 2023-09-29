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
		void SetRenderFunction(const TRenderFn& fn);

		void Stop();

		ID3D11Device* GetFactory() const { return m_Device.GetFactory(); }
		ID3D11DeviceContext* GetDeviceContext() const { return m_Device.GetContext(); }
		IDXGISwapChain* GetSwapchain() const { return m_Device.GetSwapchain(); }

		static void SetInstance(Engine* instance) { sm_Instance = instance; }
		static Engine* GetInstance() { return sm_Instance; }
	};
}
