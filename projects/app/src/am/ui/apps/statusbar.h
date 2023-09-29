#pragma once

#include "imgui.h"
#include "am/task/worker.h"
#include "rage/atl/string.h"
#include "am/ui/app.h"

namespace rageam::ui
{
	class StatusBar : public App
	{
		static constexpr double MAX_STATUS_TIME = 2.0; // 2 seconds

		rage::sysCriticalSectionToken m_Mutex;
		rage::atString m_Status;

		double m_LastStatusTime = 0;
		bool m_StatusSet = false;

		void OnRender() override
		{
#ifdef AM_STANDALONE // works bad in integration
			const ImGuiViewport* viewport = ImGui::GetMainViewport();

			// Set position to the bottom of the viewport
			ImGui::SetNextWindowPos(
				ImVec2(viewport->Pos.x,
					viewport->Pos.y + viewport->Size.y - ImGui::GetFrameHeight()));

			// Extend width to viewport width
			ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ImGui::GetFrameHeight()));

			// Add menu bar flag and disable everything else
			ImGuiWindowFlags flags =
				ImGuiWindowFlags_NoDecoration |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoScrollWithMouse |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoBackground |
				ImGuiWindowFlags_MenuBar;

			if (ImGui::Begin("StatusBar#STATUS_BAR_WINDOW", nullptr, flags)) {
				if (ImGui::BeginMenuBar())
				{
					ImGui::Text("%s", m_Status.GetCStr());
					ImGui::EndMenuBar();
				}
				ImGui::End();
			}

			if (m_StatusSet && ImGui::GetTime() > m_LastStatusTime + MAX_STATUS_TIME)
			{
				m_Status = "Ready";
			}
#endif
		}
	public:
		StatusBar()
		{
			BackgroundWorker::TaskCallback = [this](const wchar_t* status)
			{
				PushStatus(String::ToUtf8Temp(status));
			};

			m_Status = "Ready";
		}

		~StatusBar() override
		{
			BackgroundWorker::TaskCallback = nullptr;
		}

		void PushStatus(const char* status)
		{
			rage::sysCriticalSectionLock lock(m_Mutex);

			m_Status = status;
			m_StatusSet = true;
			m_LastStatusTime = ImGui::GetTime();
		}
	};
}
