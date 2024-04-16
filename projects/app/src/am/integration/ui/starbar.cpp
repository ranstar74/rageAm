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
#include "am/uiapps/explorer/explorer.h"
#include "am/integration/memory/hook.h"

namespace
{
	// Disables effect of dark red vertex channel on model, in simple
	// words make model appear a little bit brighter
	bool s_DisableDiffuseShadows = false;
}

// Time cycle value overrides
char* g_timeCycle;
void (*gImpl_Lights_UpdateBaseLights)();
void aImpl_Lights_UpdateBaseLights()
{
	constexpr int TCVAR_LIGHT_NATURAL_PUSH = 20;

	// g_timeCycle.m_frameInfo.m_keyframe.m_vars
	float* vars = (float*)(g_timeCycle + 0x1D60);

	float oldNaturalPush = vars[TCVAR_LIGHT_NATURAL_PUSH];
	if (s_DisableDiffuseShadows) vars[TCVAR_LIGHT_NATURAL_PUSH] = 1.0f;
	gImpl_Lights_UpdateBaseLights();
	if (s_DisableDiffuseShadows) vars[TCVAR_LIGHT_NATURAL_PUSH] = oldNaturalPush;
}

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
	ui::Scene::DefaultSpawnPosition = SCENE_DEFAULT_POS;

	// For overriding time cycle values
	gmAddress updateBaseLights = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"48 81 EC D8 0B 00 00 48 8D", "Lights::UpdateBaseLights");
	g_timeCycle = updateBaseLights.GetAt(7).GetRef(3).To<char*>();
#else
		"40 22 CE 74 02", "Lights::UpdateBaseLights+0x216").GetAt(-0x216);

	gmAddress timeCycle = gmAddress::Scan(
		"48 8D 0D ?? ?? ?? ?? 33 D2 E8 ?? ?? ?? ?? 48 8B D7", 
		"camFramePropagator::ComputeAndPropagateRevisedFarClipFromTimeCycle+0x33");
	g_timeCycle = timeCycle.GetRef(3).To<char*>();
#endif
	Hook::Create(updateBaseLights, aImpl_Lights_UpdateBaseLights, &gImpl_Lights_UpdateBaseLights);
}

void rageam::integration::StarBar::OnRender()
{
	ui::WindowManager* ui = ui::GetUI()->Windows;
	ui::Scene* scene = ui::Scene::GetCurrent();
	ModelScene* sceneEditor = dynamic_cast<ModelScene*>(scene);

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

#ifdef AM_TESTEBED
		// Debug teleport to scene position
		if (ImGui::IsKeyPressed(ImGuiKey_Backslash, false))
		{
			scrWarpPlayer(SCENE_DEFAULT_POS);
		}
#endif
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

		if (!scene) ImGui::BeginDisabled();
		if (SlGui::ToggleButton(ICON_AM_OBJECT"", m_UseIsolatedScene))
		{
			Vec3V pos = m_UseIsolatedScene ? SCENE_ISOLATED_POS : SCENE_DEFAULT_POS;

			scene->SetPosition(pos);
			scene->FocusCamera();

			ui::Scene::DefaultSpawnPosition = pos;
		}
		if (!scene) ImGui::EndDisabled();
		ImGui::ToolTip("Isolates scene model from game world");

		// Separate toggle buttons from actions
		if (!m_CameraEnabled) ImGui::EndDisabled(); // Draw separator without opacity
		ImGui::Separator();
		if (!m_CameraEnabled) ImGui::BeginDisabled();

		if (!scene) ImGui::BeginDisabled();
		if (SlGui::MenuButton(ICON_AM_RESET_VIEW""))
		{
			scene->FocusCamera();
		}
		if (!scene) ImGui::EndDisabled();
		ImGui::ToolTip("Reset camera position to scene");

		if (SlGui::MenuButton(ICON_AM_HOME""))
		{
			// TODO: Need player PED front + position here (or just matrix)
			// m_Camera->SetPosition()
		}
		ImGui::ToolTip("Reset camera position to player");

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

	// Explorer
	ui::WindowPtr explorer = ui->GetExisting<ui::Explorer>();
	bool isExplorerOpen = explorer != nullptr;
	if (SlGui::ToggleButton(ICON_AM_FOLDER_CLOSED_PURPLE" Explorer", isExplorerOpen))
	{
		if (isExplorerOpen)
			ui->Add(new ui::Explorer());
		else
			ui->Close(explorer);
	}

	// Material Editor
	bool isMaterialEditorOpened = sceneEditor && sceneEditor->MaterialEditor.IsOpen;
	if (!sceneEditor) ImGui::BeginDisabled();
	if (SlGui::ToggleButton(ICON_AM_BALL" Material Editor", isMaterialEditorOpened))
		sceneEditor->MaterialEditor.IsOpen = isMaterialEditorOpened;
	if (!sceneEditor) ImGui::EndDisabled();

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

	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.0f);
	if (ImGui::DragFloat("Rot", &m_SceneRotation, 1, 0, 0, "%.1f"))
	{
		// TODO: Wrap function
		m_SceneRotation = fmodf(m_SceneRotation, 360.0f);
		if (m_SceneRotation < 0.0f)
			m_SceneRotation += 360.0f;
	}
	ImGui::ToolTip("Scene rotation around vertical axis.");
	// We do this check to apply rotation even if scene was re-created
	if (scene && !rage::AlmostEquals(scene->GetRotation().Z(), m_SceneRotation))
	{
		scene->SetRotation(Vec3V(0, 0, m_SceneRotation));
	}

	static constexpr ConstString OPTIONS_POPUP = "OPTIONS_POPUP";
	if (SlGui::MenuButton(ICON_AM_FLAGS" Options"))
		ImGui::OpenPopup(OPTIONS_POPUP);
	if (ImGui::BeginPopup(OPTIONS_POPUP))
	{
		GameDrawLists* drawLists = GameDrawLists::GetInstance();

		ImGui::Checkbox("No Diffuse Shadows", &s_DisableDiffuseShadows);
		ImGui::ToolTip(
			"Technically, this disables RED vertex color on mesh, ligthing model up.\n"
			"Under the hood sets TCVAR_LIGHT_NATURAL_PUSH to 1.0");

		// ImGui::SliderFloat("Line Thickness", &DrawList::LineThickness, 0.0f, 0.05f);

		ImGui::Checkbox("Focus camera on scene", &FocusCameraOnScene);
		ImGui::ToolTip("After 3D scene was loaded, moves and points camera to object position");

		ImGui::Checkbox("Debug Overlay", &DrawableRender::IsVisible);

		if (!DrawableRender::IsVisible) ImGui::BeginDisabled();
		SlGui::CategoryText("Light Outlines");
		{
			if (!sceneEditor) ImGui::BeginDisabled();

			LightEditor::OutlineModes outlinesMode = sceneEditor ? sceneEditor->LightEditor.OutlineMode : LightEditor::OutlineMode_None;
			if (ImGui::RadioButton("All", outlinesMode == LightEditor::OutlineMode_All))
				outlinesMode = LightEditor::OutlineMode_All;
			if (ImGui::RadioButton("Only Selected", outlinesMode == LightEditor::OutlineMode_OnlySelected))
				outlinesMode = LightEditor::OutlineMode_OnlySelected;
			if (ImGui::RadioButton("None", outlinesMode == LightEditor::OutlineMode_None))
				outlinesMode = LightEditor::OutlineMode_None;

			if (sceneEditor)
				sceneEditor->LightEditor.OutlineMode = outlinesMode;

			if (!sceneEditor) ImGui::EndDisabled();
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
			if (!DrawableRender::Skeleton) ImGui::BeginDisabled();
			ImGui::Checkbox("Bone Tags", &DrawableRender::BoneTags);
			if (!DrawableRender::Skeleton) ImGui::EndDisabled();
		}
		if (!DrawableRender::IsVisible) ImGui::EndDisabled();

		ImGui::EndPopup();
	}

	SlGui::EndToolWindow();
}

#endif
