#include "am/integration/script/extensions.h"
#ifdef AM_INTEGRATED

#include "starbar.h"

#include "am/integration/im3d.h"
#include "am/integration/integration.h"
#include "am/ui/font_icons/icons_am.h"
#include "game/viewport.h"
#include "modelscene.h"
#include "am/ui/imglue.h"
#include "am/ui/slwidgets.h"
#include "am/integration/script/core.h"

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
	auto integration = GameIntegration::GetInstance();

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

	if (SlGui::ToggleButton(ICON_AM_CAMERA_GIZMO" Camera", m_CameraEnabled))
	{
		UpdateCamera();
	}

	// Camera options
	ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.2f);
	if (!m_CameraEnabled) ImGui::BeginDisabled();
	{
		if (SlGui::ToggleButton(ICON_AM_ORBIT" Orbit", m_UseOrbitCamera))
		{
			UpdateCamera();
		}
		ImGui::ToolTip("Use orbit camera instead of free");

		bool isolatedSceneOn = false;/* m_ModelScene->IsIsolatedSceneOn();*/
		if (SlGui::ToggleButton(ICON_AM_OBJECT " Isolate", isolatedSceneOn))
		{
			m_ModelScene->SetIsolatedSceneOn(isolatedSceneOn);
		}
		ImGui::ToolTip("Isolates scene model from game world");

		// Separate toggle buttons from actions
		if (!m_CameraEnabled) ImGui::EndDisabled(); // Draw separator without opacity
		ImGui::Separator();
		if (!m_CameraEnabled) ImGui::BeginDisabled();

		if (SlGui::MenuButton(ICON_AM_HOME" Reset Cam"))
		{
			m_ModelScene->ResetCameraPosition();
		}

		if (SlGui::MenuButton(ICON_AM_PED_ARROW" Warp Ped"))
		{
			scrWarpPlayer(m_Camera->GetPosition());
		}
		ImGui::ToolTip("Teleports player ped on surface to camera position.");
	}

	if (!m_CameraEnabled) ImGui::EndDisabled();
	ImGui::PopStyleVar(1); // DisabledAlpha

	// World / Local gizmo switch
	ImGui::Separator();
	bool        useWorld = Im3D::GetGizmoUseWorld();
	ConstString worldStr = ICON_AM_WORLD" World";
	ConstString localStr = ICON_AM_LOCAL" World";
	if (SlGui::ToggleButton(useWorld ? worldStr : localStr, useWorld))
		Im3D::SetGizmoUseWorld(useWorld);
	ImGui::ToolTip("Show edit gizmos in world or local space.");
	ImGui::Separator();
	if (ImGui::IsKeyPressed(ImGuiKey_Period, false)) // Hotkey switch
		Im3D::SetGizmoUseWorld(!useWorld);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
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
		SlGui::CategoryText("Lights");
		{
			SlGui::Checkbox("Outlines", &m_ModelScene->LightEditor.ShowLightOutlines);
			SlGui::Checkbox("Only Selected", &m_ModelScene->LightEditor.ShowOnlySelectedLightOutline);
		}
		SlGui::CategoryText("Collision");
		{
			SlGui::Checkbox("Visible", &DrawableRender::Collision);
			if (SlGui::Checkbox("Wireframe", &integration->DrawListCollision.Wireframe))
			{
				// Wireframe must be unlit
				integration->DrawListCollision.Unlit = integration->DrawListCollision.Wireframe;
			}
		}
		SlGui::CategoryText("Drawable");
		{
			SlGui::Checkbox("Bounding Box", &DrawableRender::BoundingBox);
			SlGui::Checkbox("Bounding Sphere", &DrawableRender::BoundingSphere);
			SlGui::Checkbox("Skeleton", &DrawableRender::Skeleton);
		}
		ImGui::EndMenu();
	}
	ImGui::PopStyleVar(2); // WindowPadding, WindowBorderSize

	SlGui::EndToolWindow();
}

#endif
