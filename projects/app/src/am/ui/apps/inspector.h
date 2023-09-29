#pragma once

#include "common/types.h"
#include "am/ui/app.h"
#include "am/task/undo.h"

namespace rageam::ui
{
	class IInspectable
	{

	public:
		virtual ~IInspectable() = default;

		virtual ConstString GetName() = 0;

		virtual void RenderProperties() {}
		virtual void RenderPreview() {}

		virtual bool HasPendingChanges() { return false; }
		virtual void Save() {}
	};

	class InspectorApp : public App
	{
		using TCancelCallback = std::function<void()>;

		static constexpr ConstString InspectorUnsavedPopup = "INSPECTOR_UNSAVED_POPUP";

		TCancelCallback m_CancelCallback;
		amPtr<IInspectable> m_PendingInspectable;
		amPtr<IInspectable> m_Inspectable;
		// UndoContext m_UndoContext;

		void UnsavedDialog();
		void OnRender() override;
	public:
		InspectorApp();

		template<typename T>
		void SetInspectable(amPtr<T> inspectable, TCancelCallback cancelCallback = nullptr /* TODO - Cancel callback is deprecated */)
		{
			amPtr<IInspectable> newInspectable = std::dynamic_pointer_cast<IInspectable>(inspectable);

			if (m_Inspectable == newInspectable)
				return;

			// TODO: We don't need this dialog, just add * to project name in explorer
			//if (m_UndoContext.Any())
			//{
			//	m_PendingInspectable = std::move(newInspectable);
			//	ImGui::OpenPopup(InspectorUnsavedPopup);
			//	return;
			//}

			m_Inspectable = std::move(newInspectable);
			m_CancelCallback = cancelCallback;
		}
	};
}
