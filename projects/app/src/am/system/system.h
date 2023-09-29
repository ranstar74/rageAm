//
// File: system.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/desktop/window.h"
#include "am/graphics/render/context.h"
#include "am/graphics/render/engine.h"
#include "am/task/worker.h"
#include "am/ui/context.h"
#include "exception/handler.h"
#include "rage/grcore/fvf.h"

#ifdef AM_INTEGRATED
#include "am/integration/gamehooks.h"
#include "am/integration/integration.h"
#endif

namespace rageam
{
	/**
	 * \brief Cares of core system components, such as - memory allocator, exception handler, rendering.
	 */
	class System
	{
		amUniquePtr<render::Engine>	m_RenderEngine;

		bool m_UseWindowRender = false;
		bool m_Initialized = false;
	public:
		System() = default;
		~System()
		{
			Destroy();
		}

		void Destroy()
		{
			if (!m_Initialized)
				return;

			// RAGE-Specific
			{
				rage::grcVertexDeclaration::CleanUpCache();
			}

			if (m_UseWindowRender)
			{
				WindowFactory::DestroyRenderWindow();
			}

			if (m_RenderEngine)
				m_RenderEngine->SetRenderFunction(nullptr);

			DestroyUIContext();
			DestroyRenderContext();

			m_RenderEngine.reset();
			render::Engine::SetInstance(nullptr);

			// This must be called after all hook-dependent things are released
			// - render::Engine hooks game render thread
#ifdef AM_INTEGRATED
			GameHooks::Shutdown();
			DestroyIntegrationContext();
#endif

			asset::AssetFactory::Shutdown();

			BackgroundWorker::Shutdown();
			ExceptionHandler::Shutdown();

			// Any still allocated memory after this point is considered as leaked.
			// No relying on atexit, it's totally unsafe to use.
			rage::SystemHeap::Shutdown();

			m_Initialized = false;
			AM_DEBUGF("System::Destroy() -> Shutted down");
		}

		// Has to be called after all required sub-systems are initialized.
		void Finalize()
		{
#ifdef AM_INTEGRATED
			GameHooks::EnableAll();
			CreateIntegrationContext();
#endif

			m_Initialized = true;
		}

		// Initializes exception handler and core syb-systems, must be done before doing anything
		void InitCore() const
		{
			ExceptionHandler::Init();
			asset::AssetFactory::Init();

#ifdef AM_INTEGRATED
			GameHooks::Init();
#endif

			AM_DEBUGF("System::InitCore() -> Done");
		}

		// Used only in standalone window mode.
		void EnterWindowUpdateLoop() const
		{
			AM_ASSERT(m_UseWindowRender,
				"System::EnterWindowUpdateLoop() -> Render was not initialized or was initialized without window mode!");

			Window* window = WindowFactory::GetWindow();
			while (window->Update())
			{
				// ...
			}
		}

		// Initializes rendering backend, non-window mode is used when we processing command line arguments,
		// such as compile texture dictionary project without launching whole winded application.
		void InitRender(bool useWindow)
		{
			if (useWindow)
			{
				WindowFactory::CreateRenderWindow();
				Input::InitClass();
				m_UseWindowRender = true;
			}

			m_RenderEngine = std::make_unique<render::Engine>(useWindow);
			render::Engine::SetInstance(m_RenderEngine.get());

			CreateRenderContext();

			AM_DEBUGF("System::InitRender() -> Render Started");
		}

		// Creates UI context and ImGui, sets render function.
		void InitUI() const
		{
			AM_ASSERT(m_RenderEngine != nullptr, "System::InitUI() -> Render Engine must be initialized for UI!");

			CreateUIContext();

			m_RenderEngine->SetRenderFunction([this]
				{
					return Gui->Update();
				});

			AM_DEBUGF("System::InitUI() -> UI Started");
		}
	};
}
