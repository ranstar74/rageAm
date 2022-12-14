#pragma once
#include "ImGuiApp.h"
#include <vector>
#include <memory>

#include "imgui_internal.h"

namespace imgui_rage
{
	class ImGuiAppMgr;
}
extern imgui_rage::ImGuiAppMgr g_ImGuiAppMgr;

namespace imgui_rage
{
	class ImGuiAppMgr
	{
		static inline std::vector<std::unique_ptr<ImGuiApp>> sm_apps;

		bool m_isBackground = false;
		bool m_isVisible = true;

		void UpdateAll()
		{
			if (!g_ImGui.IsInitialized())
				return;

			auto ctx = ImGui::GetCurrentContext();

			// New frame must be called no matter whether we actually rendering or not,
			// otherwise things like input wont work correctly.

			g_ImGui.NewFrame();

			if (ImGui::IsKeyReleased(ImGuiKey_F9))
				m_isVisible = !m_isVisible;

			bool isFocused = true;
			if (m_isVisible)
			{
				if (ImGui::IsKeyReleased(ImGuiKey_F10))
					m_isBackground = !m_isBackground;

				if (m_isBackground)
					isFocused = false;
				else
					rh::GameInput::DisableAllControlsThisFrame();

				if (GetForegroundWindow() != rh::PlatformWin32Impl::GetHwnd())
					isFocused = false;
			}

			if (!isFocused)
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			for (auto& app : sm_apps)
			{
				if (m_isVisible || app->bRenderAlways)
					app->Update();
			}
			if (!isFocused)
				ImGui::PopItemFlag();

			ctx->IO.MouseDrawCursor = m_isVisible && isFocused;

			g_ImGui.Render();
		}

		static void UpdateAll_Wrapper(ImGuiAppMgr* inst)
		{
			inst->UpdateAll();
		}

		static void OnRender()
		{
			if (!CrashHandler::ExecuteSafe(UpdateAll_Wrapper, &g_ImGuiAppMgr))
			{
				g_ImGui.Shutdown();
			}
		}
	public:
		void Init() const
		{
			rh::D3D::AddRenderTask(OnRender);
		}

		template<typename T>
		void RegisterApp()
		{
			sm_apps.push_back(std::make_unique<T>());
		}
	};
}

inline imgui_rage::ImGuiAppMgr g_ImGuiAppMgr;
