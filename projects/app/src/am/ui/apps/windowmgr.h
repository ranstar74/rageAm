#pragma once

#include "imgui.h"
#include "am/system/ptr.h"
#include "am/ui/app.h"
#include "am/ui/window.h"
#include "rage/atl/set.h"

namespace rageam::ui
{
	using WindowPtr = amPtr<Window>;
	struct WindowPtrHashFn
	{
		u32 operator()(const WindowPtr& window) const { return rage::joaat(window->GetTitle()); }
	};

	class WindowManager : public App
	{
		// We allow window to open child windows and in order to prevent them opening during
		// update cycle we add them in temporary array that will be processed next frame
		rage::atArray<WindowPtr> m_WindowsToAdd;

		rage::atSet<WindowPtr, WindowPtrHashFn> m_Windows;

		void OnStart() override;
		void OnRender() override;

	public:
		WindowPtr Add(Window* window);
		void Close(const WindowPtr& ptr);
		WindowPtr FindByTitle(ConstString title) const;

		void Focus(const WindowPtr& window) const;

		template<typename T>
		WindowPtr FindByType() const
		{
			for (WindowPtr& window : m_Windows)
			{
				if (amPtr<T> tWindow = std::dynamic_pointer_cast<T>(window))
					return tWindow;
			}
			return nullptr;
		}

		// Opens window of given type if such is not opened already.
		template<typename T>
		WindowPtr EnsureOpened()
		{
			WindowPtr explorer = FindByType<T>();
			if (explorer) return explorer;
			return Add(new T());
		}
	};
}
