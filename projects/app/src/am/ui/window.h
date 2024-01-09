//
// File: window.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/undo.h"
#include "am/types.h"
#include "app.h"

namespace rageam::ui
{
	// We use window system when we have to manage multiple/single windows,
	// this was primarily designed for asset system, we want each asset
	// to be linked with its own opened window instance

	class Window
	{
		friend class WindowManager;

		bool m_Started = false;

	public:
		virtual ~Window() = default;

		virtual bool HasMenu() const { return false; }
		virtual void OnMenuRender() {}
		virtual void OnStart() {}
		virtual void OnRender() = 0;
		virtual bool IsDisabled() const { return false; }
		virtual bool ShowUnsavedChangesInTitle() const { return false; }
		virtual ConstString GetTitle() const = 0;
		virtual ImVec2 GetDefaultSize() { return { 0, 0 }; }

		UndoStack Undo;
	};

	using WindowPtr = amPtr<Window>;
	struct WindowPtrHashFn
	{
		u32 operator()(const WindowPtr& window) const { return Hash(window->GetTitle()); }
	};

	class WindowManager : public App
	{
		// We allow window to open child windows and in order to prevent them opening during
		// update cycle we add them in temporary array that will be processed next frame
		List<WindowPtr>						m_WindowsToAdd;
		HashSet<WindowPtr, WindowPtrHashFn> m_Windows;

		void OnRender() override;

	public:
		WindowPtr Add(Window* window);
		void Close(const WindowPtr& ptr);
		void Focus(const WindowPtr& window) const;
		WindowPtr FindByTitle(ConstString title) const;

		template<typename T>
		WindowPtr GetExisting() const
		{
			for (WindowPtr& window : m_Windows)
			{
				if (amPtr<T> candidate = std::dynamic_pointer_cast<T>(window))
					return candidate;
			}
			return nullptr;
		}

		template<typename T>
		WindowPtr GetExistingOrCreateNew()
		{
			WindowPtr explorer = GetExisting<T>();
			if (explorer) return explorer;
			return Add(new T());
		}
	};
}
