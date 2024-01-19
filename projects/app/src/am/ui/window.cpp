#include "window.h"

#include "extensions.h"
#include "slwidgets.h"

void rageam::ui::WindowManager::OnRender()
{
	// Add pending windows
	for (WindowPtr& window : m_WindowsToAdd)
	{
		m_Windows.Emplace(std::move(window));
	}
	m_WindowsToAdd.Clear();

	// Update existing windows
	rage::atArray<WindowPtr> windowsToRemove;
	for (const WindowPtr& window : m_Windows)
	{
		bool hasMenu = window->HasMenu();
		bool isDisabled = window->IsDisabled();
		bool isOpen = true;

		ImGuiWindowFlags windowFlags = 0;
		if (hasMenu)
			windowFlags |= ImGuiWindowFlags_MenuBar;

		if (isDisabled) ImGui::BeginDisabled();

		// Setup window start size and position
		ImVec2 defaultSize = window->GetDefaultSize();
		ImGui::SetNextWindowSize(defaultSize, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

		// Format window title with optional '*' for unsaved documents
		bool unsaved = window->ShowUnsavedChangesInTitle() && window->Undo.HasChanges();
		ConstString title = ImGui::FormatTemp("%s%s###%s",
			window->GetTitle(), unsaved ? "*" : "", window->GetTitle());

		if (SlGui::Begin(title, &isOpen, windowFlags))
		{
			UndoStack::Push(window->Undo);
			if (!window->m_Started)
			{
				window->OnStart();
				window->m_Started = true;
			}
			if (hasMenu)
				window->OnMenuRender();
			window->OnRender();

			ImGui::HandleUndoHotkeys();

			UndoStack::Pop();
		}
		ImGui::End();

		if (isDisabled) ImGui::EndDisabled();

		if (!isOpen)
			windowsToRemove.Add(window);
	}

	// Remove windows that been closed
	for (WindowPtr& window : windowsToRemove)
		m_Windows.Remove(window);
}

rageam::ui::WindowPtr rageam::ui::WindowManager::Add(Window* window)
{
	return m_Windows.Emplace(amPtr<Window>(window));
}

void rageam::ui::WindowManager::Close(const WindowPtr& ptr)
{
	if (!ptr)
		return;

	m_Windows.Remove(ptr);
}

void rageam::ui::WindowManager::Focus(const WindowPtr& window) const
{
	ConstString title = ImGui::FormatTemp("###%s", window->GetTitle());
	ImGui::SetWindowFocus(title);
}

rageam::ui::WindowPtr rageam::ui::WindowManager::FindByTitle(ConstString title) const
{
	WindowPtr* ppWindow = m_Windows.TryGetAt(rage::atStringHash(title));
	if (ppWindow)
		return *ppWindow;
	return nullptr;
}
