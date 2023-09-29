#include "inspector.h"

#include "imgui.h"
#include "imgui_internal.h"

//void rage::IInspectable::Render()
//{
//	// TODO: Handle preview container properly in this function
//
//	RenderProperties();
//	RenderPreview();
//}

void rageam::ui::InspectorApp::UnsavedDialog()
{
	/*ImGui::BeginPopupModal(InspectorUnsavedPopup, 0, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Text("Inspectable %s has unsaved properties", m_Inspectable->GetName());
	ImGui::Separator();

	if (ImGui::Button("Save", ImVec2(120, 0)))
	{
		m_UndoContext.Clear();

		m_Inspectable->Save();
		m_Inspectable.reset();
		m_Inspectable = std::move(m_PendingInspectable);

		ImGui::CloseCurrentPopup();
	}
	ImGui::SameLine();
	if (ImGui::Button("Discard", ImVec2(120, 0)))
	{
		m_UndoContext.Clear();

		m_Inspectable.reset();
		m_Inspectable = std::move(m_PendingInspectable);

		ImGui::CloseCurrentPopup();
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel", ImVec2(120, 0)))
	{
		m_PendingInspectable.reset();
		m_CancelCallback();

		ImGui::CloseCurrentPopup();
	}
	ImGui::SetItemDefaultFocus();

	ImGui::EndPopup();*/
}

void rageam::ui::InspectorApp::OnRender()
{
	ImGuiWindowFlags windowFlags = 0;

	//if (m_UndoContext.Any())
	if (m_Inspectable && m_Inspectable->HasPendingChanges())
		windowFlags |= ImGuiWindowFlags_UnsavedDocument;

	ImGui::Begin("Inspector", 0, windowFlags);

	if (m_PendingInspectable)
	{
		UnsavedDialog();
	}

	if (m_Inspectable)
	{
		//ImGui::BeginToolBar("INSPECTOR_TOOLBAR");
		//ImGui::Text("%s", m_Inspectable->GetName());
		//ImGui::EndToolBar();

		m_Inspectable->RenderProperties();
		m_Inspectable->RenderPreview();

		if (ImGui::Shortcut(ImGuiKey_S | ImGuiMod_Ctrl, 0, ImGuiInputFlags_RouteGlobal))
		{
			m_Inspectable->Save();
		}
	}
	else
	{
		ImGui::Text("No active object.");
	}

	ImGui::End();
}

rageam::ui::InspectorApp::InspectorApp() //: m_UndoContext("Inspector")
{

}
