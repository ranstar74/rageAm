#include "lighteditor.h"

#include "ImGuizmo.h"
#include "am/graphics/shapetest.h"
#include "am/ui/im3d.h"
#include "am/ui/font_icons/icons_am.h"

void rageam::integration::LightEditor::Render(gtaDrawable* drawable, const rage::Mat44V& entityMtx)
{
	static bool s_EditingFallof = false;

	u16 lightCount = drawable->GetLightCount();

	m_LightWorldMatrices.Clear();
	m_LightWorldMatrices.Resize(lightCount);

	// Compute light matrices
	for (u16 i = 0; i < lightCount; i++)
	{
		CLightAttr* light = drawable->GetLight(i);
		rage::crSkeletonData* skeleton = drawable->GetSkeletonData();

		// If there's no skeleton, light transform is defined by CLightAttr.Position * EntityMatrix
		rage::Mat44V lightModel = rage::Mat44V::Translation(light->Position);
		if (skeleton)
		{
			// Parent light to bone
			rage::crBoneData* bone = skeleton->GetBoneFromTag(light->BoneTag);
			rage::Mat44V lightBoneMtx = skeleton->GetBoneWorldTransform(bone);
			lightModel *= lightBoneMtx;
		}

		// Transform to light world space
		m_LightWorldMatrices[i] = lightModel * entityMtx;
	}

	// Picking light using cursor
	s32 hoverLightIndex = -1;
	if (!s_EditingFallof)
	{
		// Get world mouse ray for picking lights
		rage::Vec3V mousePos, mouseDir;
		CViewport::GetWorldMouseRay(mousePos, mouseDir);

		// Find the closest hovered light to the camera
		rage::ScalarV hoverLightDist = rage::S_MAX;
		for (u16 i = 0; i < drawable->m_Lights.GetSize(); i++)
		{
			CLightAttr& light = drawable->m_Lights[i];
			const rage::Mat44V& lightWorld = m_LightWorldMatrices[i];

			rage::ScalarV hitDistance;
			bool didHit = false;

			// Point light
			if (graphics::ShapeTest::RayIntersectsSphere(mousePos, mouseDir, lightWorld.Pos, light.Falloff, &hitDistance))
				didHit = true;

			// New closest light found
			if (didHit && hitDistance < hoverLightDist)
			{
				hoverLightDist = hitDistance.Get();
				hoverLightIndex = i;
			}
		}

		// Not hovering any ImGui window or gizmo
		bool canSelectLight = GImGui->HoveredWindow == nullptr && !ImGuizmo::IsOver();
		// Handle light selection using mouse
		if (canSelectLight && ImGui::IsKeyPressed(ImGuiKey_MouseLeft, false))
			m_SelectedLight = hoverLightIndex;
	}

	if (m_SelectedLight == -1)
		s_EditingFallof = false;

	rage::Vec3V camFront, camRight, camUp;
	CViewport::GetCamera(camFront, camRight, camUp);

	// Draw light gizmos
	for (u16 i = 0; i < drawable->m_Lights.GetSize(); i++)
	{
		CLightAttr& light = drawable->m_Lights[i];
		const rage::Mat44V& lightWorld = m_LightWorldMatrices[i];

		// Draw edit gizmos for selected light
		float fallofEditRadius = 0.0f;
		if (m_SelectedLight == i)
		{
			// Try to start editing falloff
			bool canStartFalloffEdit = false;
			if (!s_EditingFallof)
			{
				// We compute radius of point light sphere in screen units & distance to sphere center from mouse cursor
				// If its almost equal then we're hovering sphere edge
				rage::Vec3V sphereEdge = lightWorld.Pos + camRight * light.Falloff;
				ImVec2 screenSphereCenter;
				ImVec2 screenSphereEdge;
				if (Im3D::WorldToScreen(lightWorld.Pos, screenSphereCenter) &&
					Im3D::WorldToScreen(sphereEdge, screenSphereEdge))
				{
					float screenSphereRadius = ImGui::Distance(screenSphereCenter, screenSphereEdge);
					float distanceToSphere = ImGui::Distance(screenSphereCenter, ImGui::GetMousePos());

					// Check if mouse cursor is on the edge of sphere
					constexpr float edgeWidth = 5.0f;
					float distanceFromEdge = abs(screenSphereRadius - distanceToSphere);
					if (distanceFromEdge < edgeWidth)
					{
						canStartFalloffEdit = true;
					}
				}
			}

			// Update editing falloff
			if (s_EditingFallof || canStartFalloffEdit)
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

				rage::Vec3V dragDelta, startDragPos, dragPos;
				rage::Vec3V spherePlaneNormal = camFront; // We are editing falloff on camera plane
				bool isDragging;
				bool dragFinished = Im3D::GizmoBehaviour(isDragging, lightWorld.Pos, spherePlaneNormal, dragDelta, startDragPos, dragPos);

				// Compute new falloff radius
				if (isDragging)
				{
					fallofEditRadius = dragPos.DistanceTo(lightWorld.Pos).Get();
					light.Falloff = fallofEditRadius;

					Im3D::TextBg(dragPos + rage::VEC_UP * 0.25f, "%.02f", dragDelta.Length().Get());

					// GRenderContext->OverlayRender.DrawLine(startDragPos, dragPos, graphics::ColorU32(140, 140, 140, 140));
				}

				if (dragFinished)
				{
					// TODO: ...
				}

				s_EditingFallof = isDragging;
			}

			// Move light position using regular gizmo
			if (!s_EditingFallof)
			{
				rage::Mat44V delta;
				if (Im3D::Gizmo(lightWorld, delta, ImGuizmo::TRANSLATE))
				{
					light.Position += rage::Vec3V(delta.Pos);
				}
			}
		}

		graphics::ColorU32 lightGizmoColor = i == hoverLightIndex || i == m_SelectedLight ? graphics::COLOR_YELLOW : graphics::COLOR_ORANGE;
		lightGizmoColor.A = m_SelectedLight == i ? 210 : 145;

		// Draw light shape gizmos
		switch (light.Type)
		{
		case LIGHT_POINT:
		{
			GRenderContext->OverlayRender.DrawSphere(lightWorld, lightGizmoColor, light.Falloff);
			// Outer ring
			graphics::ColorU32 lightBorderColor = graphics::COLOR_YELLOW;
			GRenderContext->OverlayRender.DrawCricle(
				lightWorld.Pos, camFront, camRight,
				light.Falloff, rage::Mat44V::Identity(), lightBorderColor, lightBorderColor);
		}
		break;
		default: break;
		}

		Im3D::CenterNext();
		Im3D::TextBgColored(lightWorld.Pos, graphics::COLOR_YELLOW, "Point Light #%u", i);
	}

	// Draw editor for selected light
	if (m_SelectedLight == -1)
		return;

	ImGui::Begin(ICON_AM_LIGHT" Light Editor");
	{
		CLightAttr* light = drawable->GetLight(m_SelectedLight);

		ImGui::Text("Light #%i", m_SelectedLight);

		graphics::ColorU32 color(light->ColorR, light->ColorG, light->ColorB);
		rage::Vector4 colorF = color.ToVec4();
		if (ImGui::ColorPicker3("Color", (float*)&colorF))
		{
			color = graphics::ColorU32::FromVec4(colorF);
			light->ColorR = color >> IM_COL32_R_SHIFT & 0xFF;
			light->ColorG = color >> IM_COL32_G_SHIFT & 0xFF;
			light->ColorB = color >> IM_COL32_B_SHIFT & 0xFF;
		}

		ImGui::DragFloat("Falloff", &light->Falloff, 0.1f, 0, 25, "%.1f");
		ImGui::DragFloat("Falloff Exponent", &light->FallofExponent, 1.0f, 0, 500, "%.1f");
	}
	ImGui::End();
}
