//
// File: apps.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/atl/array.h"
#include "app.h"

#include <mutex>

#include "am/system/ptr.h"

namespace rageam::ui
{
	/**
	 * \brief Encapsulates list of UI apps.
	 */
	class Apps
	{
		rage::atArray<amUniquePtr<App>> m_Apps;
		App* m_LastApp = nullptr;

		std::recursive_mutex m_Mutex;

		// Static method for exception handler.
		static void UpdateModule(App* module) { module->Update(); }
	public:
		void RegisterSystemApps();

		// Returns True if no exceptions occurred during executing; otherwise False.
		bool UpdateAll();

		void AddApp(App* app);

		// Gets first found instance (in added order) of app with given type.
		// Normally there should be just one instance.
		template<typename T>
		T* FindAppByType()
		{
			std::unique_lock lock(m_Mutex);

			for (amUniquePtr<App>& app : m_Apps)
			{
				T* obj = dynamic_cast<T*>(app.get());
				if (obj) return obj;
			}
			return nullptr;
		}

		App* GetLastUpdated() const { return m_LastApp; }
	};
}
