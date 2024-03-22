#include "assetwindow.h"

#include "imgui_internal.h"
#include "am/system/worker.h"
#include "am/ui/extensions.h"
#include "am/ui/imglue.h"
#include "am/ui/font_icons/icons_am.h"
#include "rage/math/math.h"
#include "assetasynccompiler.h"

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

	AssetAsyncCompiler::GetInstance()->CompileAsync(m_Asset->GetDirectoryPath());
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

	OnAssetMenuRender();

	ImGui::EndMenuBar();
}
