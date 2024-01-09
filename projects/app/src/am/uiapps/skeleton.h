//
// File: skeleton.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/integration/integration.h"
#include "am/ui/app.h"
#include "am/ui/extensions.h"
#include "am/ui/imglue.h"

namespace rageam::ui
{
	/**
	 * \brief Handles visibility & activation shortcuts (in integration mode), dock space and those kind of things.
	 */
	class Skeleton : public App
	{
		void OnUpdate() override
		{
			ImGlue* ui = GetUI();

#ifdef AM_INTEGRATED
			auto integration = integration::GameIntegration::GetInstance();
#endif

			if (ImGui::IsKeyPressed(ImGuiKey_F9))
				ui->IsVisible = !ui->IsVisible;

			// We display UI disabled if game viewport is focused (active) in integration mode
#ifdef AM_INTEGRATED
			if (ImGui::IsKeyPressed(ImGuiKey_F10))
			{
				ui->IsDisabled = !ui->IsDisabled;
				// We clip cursor (keep it within game window) when UI is in background
				bool cursorClipped = ui->IsDisabled;
				graphics::Window::GetInstance()->SetMouseClipped(cursorClipped);
			}

			// We don't want game controls to mess up with the UI
			if (!ui->IsDisabled)
			{
				integration->DisableAllControlsThisFrame();
			}

			// TODO: Use OS cursor
			GImGui->IO.MouseDrawCursor = !ui->IsDisabled && !integration->IsPauseMenuActive();
#endif

			// Dock space in integration mode will cover all game viewport, we don't want that
			// Title bar covers part of viewport, we have to disable it for now (until we implement custom game viewports)
#ifdef AM_STANDALONE
			ImGui::BeginDockSpace();

			bool openAbout = false;
			static ConstString ABOUT_POPUP_ID = "About##ABOUT_POPUP_ID";
			/*if (ImGui::BeginMenuBar())
			{
				static WindowManager* windowMgr = FindByType<WindowManager>();

				ImGui::MenuItem("File");

				if (ImGui::BeginMenu("View"))
				{
					if (ImGui::MenuItem(ICON_AM_COLOR_PALETTE "  Style Editor"))
						windowMgr->EnsureOpened<StyleEditor>();

					if (ImGui::MenuItem(ICON_AM_PERFORMANCE_REPORT"  System Performance"))
						windowMgr->EnsureOpened<SystemMonitor>();

					if (ImGui::MenuItem(ICON_AM_FOLDER_BOTTOM_PANEL"  Explorer"))
						windowMgr->EnsureOpened<Explorer>();

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Help"))
				{
					if (ImGui::MenuItem(ICON_AM_ABOUT_BOX"  About"))
					{
						openAbout = true;
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}*/

			if (openAbout)
				ImGui::OpenPopup(ABOUT_POPUP_ID);

			//if (ImGui::BeginPopupModal(ABOUT_POPUP_ID, 0, ImGuiWindowFlags_AlwaysAutoResize))
			//{
			//	static AsyncImage* amIcon = Gui->Icons.GetIcon("am", IconSize_256);
			//	static int currentYear = DateTime::Now().Year();

			//	SlGui::PushFont(SlFont_Medium);

			//	amIcon->Render(64);
			//	ImGui::SameLine();
			//	ImGui::Text("The 'rageAm' RAGE Research Project");
			//	ImGui::SnapToPrevious();
			//	ImGui::Text(U8("\u00a9")); // Trademark
			//	ImGui::SameLine(0, 0);
			//	ImGui::Text(" %i ranstar74.", currentYear);

			//	ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * 1.75f));

			//	if (ImGui::Button("Continue"))
			//		ImGui::CloseCurrentPopup();

			//	ImGui::PopFont();

			//	ImGui::EndPopup();
			//}

			ImGui::End(); // DockSpace
#endif
		}
	};
}
