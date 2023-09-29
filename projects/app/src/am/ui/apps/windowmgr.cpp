#include "windowmgr.h"

#include "am/task/undo.h"
#include "am/ui/extensions.h"
#include "am/ui/windows/explorer/explorer.h"

void rageam::ui::WindowManager::OnStart()
{
#ifdef AM_STANDALONE
	// Open default windows
	EnsureOpened<Explorer>();
#endif
}

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
			if (hasMenu)
				window->OnMenuRender();
			window->OnRender();
			UndoStack::Pop();

			// Handle undo hot-keys
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
			{
				if (window->Undo.CanUndo() &&
					ImGui::Shortcut(ImGuiKey_Z | ImGuiMod_Ctrl, 0, ImGuiInputFlags_RouteGlobal))
				{
					window->Undo.Undo();
				}

				if (window->Undo.CanRedo() &&
					ImGui::Shortcut(ImGuiKey_Y | ImGuiMod_Ctrl, 0, ImGuiInputFlags_RouteGlobal))
				{
					window->Undo.Redo();
				}
			}
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

rageam::ui::WindowPtr rageam::ui::WindowManager::FindByTitle(ConstString title) const
{
	WindowPtr* ppWindow = m_Windows.TryGetAt(rage::joaat(title));
	if (ppWindow)
		return *ppWindow;
	return nullptr;
}

void rageam::ui::WindowManager::Focus(const WindowPtr& window) const
{
	// ### are required by ImGui
	ConstString title = ImGui::FormatTemp("###%s", window->GetTitle());
	ImGui::SetWindowFocus(title);
}
