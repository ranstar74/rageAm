#include "assetasynccompiler.h"
#include "am/asset/factory.h"

#include <imgui_internal.h>

bool rageam::ui::AssetAsyncCompiler::ProcessActiveRequest()
{
	if (!m_ActiveCompileTask)
		return false;

	// There's already active task and it's not finished yet
	if (!m_ActiveCompileTask->IsFinished())
		return true;

	// Task finished, clean it up
	m_ActiveCompileTask = nullptr;
	m_ActiveAsset = nullptr;
	m_Progress = 0;
	m_ProgressMessages.Clear();
	return false;
}

bool rageam::ui::AssetAsyncCompiler::ProcessNewRequest()
{
	if (m_CompileRequests.empty())
		return false;

	// We had either no active task or active task just finished, in either
	// case we can process new request
	file::WPath requestPath = m_CompileRequests.front();
	m_CompileRequests.pop();

	// Load the asset from requested path
	m_ActiveAsset = asset::AssetFactory::LoadFromPath(requestPath);
	if (!m_ActiveAsset)
		return false;

	// Subscribe on asset compile callback to report progress to user
	m_ActiveAsset->CompileCallback = [this] (ConstWString msg, double progress)
	{
		std::unique_lock lock(m_ProgressMutex);

		m_Progress = progress;
		m_ProgressMessages.Construct(String::ToUtf8Temp(msg));
	};

	// Start the background compilation task
	m_ActiveCompileTask = BackgroundWorker::Run([this]
	{
		return m_ActiveAsset->CompileToFile();
	}, L"Compile %ls", requestPath.GetCStr());

	// Finally, open UI dialog
	ImGui::OpenPopup(SAVE_POPUP_NAME);
	return true;
}

void rageam::ui::AssetAsyncCompiler::OnRender()
{
	bool hasTaskToProcess = ProcessActiveRequest();
	// Try to start new task...
	if (!hasTaskToProcess && ProcessNewRequest())
	{
		hasTaskToProcess = true;
	}

	ImGui::SetNextWindowSize(ImVec2(460, 300));
	if (ImGui::BeginPopupModal(SAVE_POPUP_NAME))
	{
		// Active task was finished, close popup and leave
		if (!hasTaskToProcess)
		{
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return;
		}

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

rageam::ui::AssetAsyncCompiler::~AssetAsyncCompiler()
{
	if (m_ActiveCompileTask)
		m_ActiveCompileTask->Wait();
}

void rageam::ui::AssetAsyncCompiler::CompileAsync(ConstWString assetPath)
{
	m_CompileRequests.push(assetPath);
}
