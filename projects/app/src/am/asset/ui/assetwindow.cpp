#include "assetwindow.h"

#include "imgui_internal.h"
#include "am/system/worker.h"
#include "am/ui/extensions.h"
#include "am/ui/font_icons/icons_am.h"
#include "rage/math/math.h"

void rageam::ui::AssetWindow::DoSave()
{
	SaveChanges();
	if (m_Asset->SaveConfig())
	{
		Undo.SetSavePoint();
	}
}

void rageam::ui::AssetWindow::DoRefresh()
{
	// Refresh invalidates undo, we have to reset it
	Undo.Clear();

	m_Asset->Refresh();
	OnRefresh();
}

void rageam::ui::AssetWindow::CompileAsync()
{
	DoSave();

	// Open popup window and subscribe on asset compile callback to report progress to user
	m_Progress = 0;
	m_ProgressMessages.Clear();
	ImGui::OpenPopup(SAVE_POPUP_NAME);
	m_Asset->CompileCallback = [this](ConstWString msg, double progress)
		{
			std::unique_lock lock(m_ProgressMutex);

			m_Progress = progress;
			m_ProgressMessages.Construct(String::ToUtf8Temp(msg));
		};

	m_IsCompiling = true;
	BackgroundWorker::Run([this]
		{
			bool success = m_Asset->CompileToFile();
			m_Asset->CompileCallback = nullptr;
			m_IsCompiling = false;
			return success;
		}, L"Compile asset");
}

void rageam::ui::AssetWindow::OnMenuRender()
{
	if (!ImGui::BeginMenuBar())
		return;

	if (ImGui::MenuItem(ICON_AM_BUILD_STYLE" Export") || ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_E))
	{
		if (!m_IsCompiling)
			CompileAsync();
	}
	ImGui::ToolTip("Compiles asset to native binary format.");

	if (ImGui::MenuItem(ICON_AM_SAVE" Save") || ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S))
	{
		DoSave();
	}

	if (ImGui::MenuItem(ICON_AM_REFRESH" Refresh") || ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_R))
	{
		// Only for hotkey, menu button cannot be pressed while modal dialog is open
		bool canRefresh = !ImGui::IsPopupOpen(REFRESH_POPUP_NAME);

		if (canRefresh)
		{
			if (!Undo.HasChanges())
				DoRefresh();
			else
				ImGui::OpenPopup(REFRESH_POPUP_NAME);
		}
	}
	ImGui::ToolTip("Scans asset directory for changes.");

	if(ImGui::MenuItem(ICON_AM_CLEAN_DATA" Reset"))
	{
		ImGui::OpenPopup(RESET_POPUP_NAME);
	}
	ImGui::ToolTip("Removes all settings, this action can't be undone.");

	// Draw reset dialog
	if (ImGui::BeginPopupModal(RESET_POPUP_NAME, 0, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("All existing settings will be reset to default, do you want to proceed?");
		ImGui::Text("This action can't be undone. History will be erased.");

		if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("OK"))
		{
			Reset();
			DoRefresh();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	// Draw refresh dialog
	if (ImGui::BeginPopupModal(REFRESH_POPUP_NAME, 0, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("There are unsaved changes, how do you want to proceed?");
		ImGui::Text("This action can't be undone. History will be erased.");

		if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::Button("Revert changes and refresh"))
		{
			DoRefresh();
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::Button("Save and refresh"))
		{
			DoSave();
			DoRefresh();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	// Draw export dialog
	if (m_IsCompiling)
	{
		ImGui::SetNextWindowSize(ImVec2(460, 300));
		if (ImGui::BeginPopupModal(SAVE_POPUP_NAME))
		{
			std::unique_lock lock(m_ProgressMutex);

			ImGuiStyle& style = ImGui::GetStyle();
			float childHeight = 300 - ImGui::GetFrameHeight() * 3 - style.FramePadding.y;

			if (ImGui::BeginChild("##SAVE_POPUP_MESSAGES", ImVec2(0, childHeight), true))
			{
				for (ConstString msg : m_ProgressMessages)
				{
					ImGui::Text(msg);
				}

				if (rage::AlmostEquals(ImGui::GetScrollY(), ImGui::GetScrollMaxY()))
					ImGui::SetScrollY(ImGui::GetScrollMaxY());

			}
			ImGui::EndChild();

			// Align progress bar to bottom of window
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			window->DC.CursorPos = window->WorkRect.GetBL() - ImVec2(0, ImGui::GetFrameHeight());
			ImGui::ProgressBar(static_cast<float>(m_Progress));

			ImGui::EndPopup();
		}
	}

	OnAssetMenuRender();

	ImGui::EndMenuBar();
}
