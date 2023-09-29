#include "assetwindow.h"

#include "imgui_internal.h"
#include "am/task/worker.h"
#include "am/ui/font_icons/icons_am.h"
#include "rage/math/math.h"

void rageam::ui::assetview::AssetWindow::SaveAsync()
{
	Undo.SetSavePoint();
	m_IsSaving = true;
	BackgroundWorker::Run([this]
	{
		SaveChanges();
		bool saved = m_Asset->SaveConfig();
		m_IsSaving = false;
		return saved;
	}, L"Save %ls", m_Asset->GetAssetName());
}

void rageam::ui::assetview::AssetWindow::CompileAsync()
{
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

void rageam::ui::assetview::AssetWindow::OnMenuRender()
{
	if (!ImGui::BeginMenuBar())
		return;

	if (ImGui::MenuItem(ICON_AM_BUILD_SOLUTION" Export"))
	{
		CompileAsync();
	}

	if (ImGui::MenuItem(ICON_AM_SAVE" Save"))
	{
		SaveAsync();
	}

	// Draw export dialog
	ImGui::SetNextWindowSize(ImVec2(460, 300));
	if (m_IsCompiling && ImGui::BeginPopupModal(SAVE_POPUP_NAME))
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

			if(rage::Math::AlmostEquals(ImGui::GetScrollY(), ImGui::GetScrollMaxY()))
				ImGui::SetScrollY(ImGui::GetScrollMaxY());

			ImGui::EndChild();
		}

		// Align progress bar to bottom of window
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		window->DC.CursorPos = window->WorkRect.GetBL() - ImVec2(0, ImGui::GetFrameHeight());
		ImGui::ProgressBar(static_cast<float>(m_Progress));

		ImGui::EndPopup();
	}

	OnAssetMenuRender();

	ImGui::EndMenuBar();
}
