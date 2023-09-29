#pragma once

#include "am/ui/app.h"
#include "am/ui/extensions.h"
#include "am/ui/styled/slwidgets.h"
#include "am/integration/hooks/gameviewport.h"
#include "am/integration/hooks/GameInput.h"

namespace rageam
{
	class GameApp : public ui::Module
	{
		// Should we move that somewhere else?
		POINT m_CursorPosBackup = {};

		ImVec2 m_PrevGameSize;
		bool m_WasResized = false;
		bool m_IsFocused = false;
		bool m_FocusChanged = false;
	public:
		void OnRender() override
		{
			m_FocusChanged = false;

			ConstString title = ImGui::FormatTemp("Game (%s)###GAME_WINDOW", m_IsFocused ? "Active" : "Inactive");

			SlGui::Begin(title);

			// Render game viewport
			ImVec2 gameSize = ImGui::GetCurrentWindow()->ClipRect.GetSize();
			gameSize -= { 1, 1}; // Shrink a little because drawing with clip rect size we're drawing over window border

			ImGui::Image(ImTextureID(hooks::GameViewport::GetGameShaderView()), gameSize);

			bool viewportClicked = ImGui::IsItemClicked();
			if (!m_IsFocused && viewportClicked) // Clicked on viewport
			{
				GetCursorPos(&m_CursorPosBackup);
				m_IsFocused = true;
				m_FocusChanged = true;

				// Maybe move it back...
				//AppWindow::SetCursorVisible(false);
			}
			//else if (m_IsFocused && !viewportClicked && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) // Clicked outside viewport
			//	m_IsFocused = false;

			// Leave viewport by pressing ctrl + alt and restore cursor pos
			if (m_IsFocused && ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyDown(ImGuiKey_LeftAlt))
			{
				//AppWindow::SetCursorVisible(true);
				SetCursorPos(m_CursorPosBackup.x, m_CursorPosBackup.y);
				m_IsFocused = false;
				m_FocusChanged = true;
				  
			}

			if (!m_IsFocused)
			{
				hooks::GameInput::DisableAllControlsThisFrame();
			}

			//m_WasResized = !AlmostEquals(gameSize.x, m_PrevGameSize.x) || !AlmostEquals(gameSize.y, m_PrevGameSize.y);
			m_PrevGameSize = gameSize;

			ImGui::End();
		}

		bool WasResized() const { return m_WasResized; }
		bool IsFocused() const { return m_IsFocused; }
		bool FocusChanged() const { return m_FocusChanged; }

		u32 GetGameWidth() const { return (u32)m_PrevGameSize.x; }
		u32 GetGameHeight() const { return (u32)m_PrevGameSize.y; }
	};
}
