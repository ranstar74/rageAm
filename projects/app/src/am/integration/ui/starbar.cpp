#ifdef AM_INTEGRATED

#include "starbar.h"

#include "am/integration/im3d.h"
#include "am/integration/integration.h"
#include "am/ui/font_icons/icons_am.h"
#include "game/viewport.h"
#include "modelscene.h"
#include "am/ui/imglue.h"
#include "am/ui/slwidgets.h"
#include "am/integration/gamedrawlists.h"
#include "am/integration/script/extensions.h"

void rageam::integration::StarBar::UpdateCamera()
{
	if (!m_CameraEnabled)
	{
		m_Camera = nullptr;
		return;
	}

	// Save previous camera position if switching from one type to another
	Nullable<Mat44V> cameraMatrix;
	if (m_Camera)
		cameraMatrix = m_Camera->GetMatrix();

	if (m_UseOrbitCamera)	m_Camera.Create<OrbitCamera>();
	else					m_Camera.Create<FreeCamera>();

	// Restore saved matrix if saved
	if (cameraMatrix.HasValue())
	{
		m_Camera->SetMatrix(cameraMatrix.GetValue());
	}
	else
	{
		// Use current viewport camera matrix as default one
		m_Camera->SetMatrix(CViewport::GetCameraMatrix());
	}

	// Reset origin for orbit camera
	if (m_UseOrbitCamera)
		m_Camera->LookAt(m_Camera->GetPosition() + m_Camera->GetFront() * 3.0f);

	m_Camera->SetActive(true);
}

void rageam::integration::StarBar::OnStart()
{
	m_ModelScene = ui::GetUI()->FindAppByType<ModelScene>();
}

void rageam::integration::StarBar::OnRender()
{
	if (Im3D::IsViewportFocused())
	{
		// Switch camera with ']'
		if (ImGui::IsKeyPressed(ImGuiKey_RightBracket, false))
		{
			m_CameraEnabled = !m_CameraEnabled;
			UpdateCamera();
		}

		// Switch orbit / regular camera on hotkey press
		if (ImGui::IsKeyPressed(ImGuiKey_O, false))
		{
			m_UseOrbitCamera = !m_UseOrbitCamera;

			UpdateCamera();
		}

		// Debug teleport to scene position
		if (ImGui::IsKeyPressed(ImGuiKey_Backslash, false))
		{
			scrWarpPlayer(SCENE_POS);
		}
	}

	if (!SlGui::BeginToolWindow(ICON_AM_STAR" StarBar"))
	{
		SlGui::EndToolWindow();
		return;
	}

	if (SlGui::ToggleButton(ICON_AM_CAMERA_GIZMO"", m_CameraEnabled))
	{
		UpdateCamera();
	}
	ImGui::ToolTip("Camera");

	// Camera options
	ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.2f);
	if (!m_CameraEnabled) ImGui::BeginDisabled();
	{
		if (SlGui::ToggleButton(ICON_AM_ORBIT"", m_UseOrbitCamera))
		{
			UpdateCamera();
		}
		ImGui::ToolTip("Orbit camera rotates around the object");

		bool isolatedSceneOn = m_ModelScene->IsIsolatedSceneOn();
		if (SlGui::ToggleButton(ICON_AM_OBJECT"", isolatedSceneOn))
		{
			m_ModelScene->SetIsolatedSceneOn(isolatedSceneOn);
		}
		ImGui::ToolTip("Isolates scene model from game world");

		// Separate toggle buttons from actions
		if (!m_CameraEnabled) ImGui::EndDisabled(); // Draw separator without opacity
		ImGui::Separator();
		if (!m_CameraEnabled) ImGui::BeginDisabled();

		if (SlGui::MenuButton(ICON_AM_HOME""))
		{
			m_ModelScene->ResetCameraPosition();
		}
		ImGui::ToolTip("Reset camera position");

		if (SlGui::MenuButton(ICON_AM_PED_ARROW""))
		{
			scrWarpPlayer(m_Camera->GetPosition());
		}
		ImGui::ToolTip("Teleports player ped to camera position");
	}

	if (!m_CameraEnabled) ImGui::EndDisabled();
	ImGui::PopStyleVar(1); // DisabledAlpha

	// World / Local gizmo switch
	// ImGui::Separator();
	bool        useWorld = Im3D::GetGizmoUseWorld();
	ConstString worldStr = ICON_AM_WORLD" World";
	ConstString localStr = ICON_AM_LOCAL" World";
	if (SlGui::ToggleButton(useWorld ? worldStr : localStr, useWorld))
		Im3D::SetGizmoUseWorld(useWorld);
	ImGui::ToolTip("Show edit gizmos in world or local space");
	if (ImGui::IsKeyPressed(ImGuiKey_Period, false)) // Hotkey switch
		Im3D::SetGizmoUseWorld(!useWorld);

	SlGui::ToggleButton(ICON_AM_BALL" Material Editor", m_ModelScene->MaterialEditor.IsOpen);

	// Game time
	float time = scrGetTimeFloat();
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.0f);
	if (ImGui::DragFloat("Time", &time, 0.0015f, -1.0f, 1.0f, "%.3f", ImGuiSliderFlags_NoRoundToFormat))
	{
		// We allow negative time to allow natural wrapping
		if (time < 0.0f)
			time += 1.0f;

		scrSetTimeFloat(time);
	}

	static constexpr ConstString OVERLAY_POPUP = "OVERLAY_POPUP";
	if (SlGui::MenuButton(ICON_AM_VISIBILITY" Overlay"))
		ImGui::OpenPopup(OVERLAY_POPUP);
	if (ImGui::BeginPopup(OVERLAY_POPUP))
	{
		GameDrawLists* drawLists = GameDrawLists::GetInstance();

		// ImGui::SliderFloat("Line Thickness", &DrawList::LineThickness, 0.0f, 0.05f);

		SlGui::CategoryText("Light Outlines");
		{
			LightEditor::OutlineModes& outlinesMode = m_ModelScene->LightEditor.OutlineMode;
			if (ImGui::RadioButton("All", outlinesMode == LightEditor::OutlineMode_All))
				outlinesMode = LightEditor::OutlineMode_All;
			if (ImGui::RadioButton("Only Selected", outlinesMode == LightEditor::OutlineMode_OnlySelected))
				outlinesMode = LightEditor::OutlineMode_OnlySelected;
		}
		SlGui::CategoryText("Collision");
		{
			ImGui::Checkbox("Visible", &DrawableRender::Collision);
			bool disabled = !DrawableRender::Collision;
			if (disabled) ImGui::BeginDisabled();
			// ImGui::Checkbox("Draw On Top###DRAW_COLLISION_ON_TOP", &drawLists->CollisionMesh.NoDepth);
			ImGui::Checkbox("Wireframe", &drawLists->CollisionMesh.Wireframe);
			if (disabled) ImGui::EndDisabled();

			ImVec4 color = ImGui::ColorConvertU32ToFloat4(DrawableRender::CollisionBoundColor);
			if (ImGui::ColorEdit4("Color", (float*)&color, ImGuiColorEditFlags_NoInputs))
				DrawableRender::CollisionBoundColor = ImGui::ColorConvertFloat4ToU32(color);
		}
		SlGui::CategoryText("Drawable");
		{
			ImVec4 color = ImGui::ColorConvertU32ToFloat4(DrawableRender::BoundingOutlineColor);
			if (ImGui::ColorEdit4("Outline Color", (float*)&color, ImGuiColorEditFlags_NoInputs))
				DrawableRender::BoundingOutlineColor = ImGui::ColorConvertFloat4ToU32(color);

			ImGui::Checkbox("Draw On Top###DRAW_BOUNDS_ON_TOP", &DrawableRender::DrawBoundsOnTop);
			ImGui::Checkbox("Bounding Box", &DrawableRender::BoundingBox);
			ImGui::Checkbox("Bounding Sphere", &DrawableRender::BoundingSphere);
			ImGui::Checkbox("Skeleton", &DrawableRender::Skeleton);
		}

		ImGui::EndPopup();
	}

	SlGui::EndToolWindow();
}

#endif
