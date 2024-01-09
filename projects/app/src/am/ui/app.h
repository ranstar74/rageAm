//
// File: app.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"

// Unused here but every app implementation includes it
// ReSharper disable once CppUnusedIncludeDirective
#include <imgui.h>
#include <typeinfo>

namespace rageam::ui
{
	/**
	 * \brief Single-instanced ImGui component.
	 */
	class App
	{
		bool m_Started = false;

	protected:

		// ImGui-related things must be initialized here because
		// app constructor is invoked way before UI is ready to use
		virtual void OnStart() {}
		// Invoked every frame when UI is visible (rendered)
		virtual void OnRender() {}
		// Invoked every frame regardless if UI is visible (rendered) or not
		virtual void OnUpdate() {}

	public:
		virtual ~App() = default;

		// Debug name is used in error / assert dialogs.
		virtual ConstString GetDebugName() { return typeid(*this).name(); }

		void Tick(bool onlyUpdate)
		{
			OnUpdate();
			if (onlyUpdate)
				return;

			if (!m_Started)
			{
				OnStart();
				m_Started = true;
			}

			OnRender();
		}
	};
}
