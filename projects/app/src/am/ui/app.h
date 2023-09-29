//
// File: app.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <typeinfo>

#include "am/desktop/input.h"
#include "common/types.h"

namespace rageam::ui
{
	/**
	 * \brief App has one running instance and has update function that is called on tick.
	 */
	class App
	{
		bool m_Started = false;
	protected:

		// ImGui-related things must be done in here because
		// when module is constructed im gui might be not initialized.
		virtual void OnStart() {}

		// Invoked every frame when window is visible.
		virtual void OnRender() = 0;

	public:
		virtual ~App() = default;

		// Debug name is used in error / assert dialogs.
		virtual ConstString GetDebugName() { return typeid(*this).name(); }

		void Update()
		{
			if (!m_Started)
			{
				OnStart();
				m_Started = true;
			}

			OnRender();
		}

		InputState& GetInput() const;
	};
}
