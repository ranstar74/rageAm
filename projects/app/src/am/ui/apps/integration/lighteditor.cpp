#include "lighteditor.h"

#include "ImGuizmo.h"
#include "am/graphics/shapetest.h"
#include "am/ui/im3d.h"
#include "am/ui/font_icons/icons_am.h"
#include "am/ui/styled/slwidgets.h"

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
		rage::Mat44V lightModel;
		/*rage::Vec3V lightUp = light->Direction;
		rage::Vec3V lightFront = lightUp.Cross(rage::VEC_RIGHT);
		lightModel.Up = lightUp.Cross(lightUp);
		lightModel.Front = lightFront;
		lightModel.Right = lightFront.Cross(lightUp);*/
		lightModel.Pos = rage::Vec3V(light->Position);
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
		{
			m_SelectedLight = hoverLightIndex;

			// Clicked outside any light, reset gizmo mode
			if (hoverLightIndex == -1)
				m_GizmoMode = GIZMO_None;
		}
	}

	// Handle gizmo mode selection
	if (m_SelectedLight != -1)
	{
		// TODO: Toggling will break switching between gizmos
		if (ImGui::IsKeyPressed(ImGuiKey_G, false)) m_GizmoMode ^= GIZMO_Translate;
		//// Rotation is only supported on spot light
		//if (drawable->GetLight(m_SelectedLight)->Type == LIGHT_SPOT)
		//	if (ImGui::IsKeyPressed(ImGuiKey_R, false)) m_GizmoMode ^= GIZMO_Rotate;
	}

	if (m_SelectedLight == -1)
		s_EditingFallof = false;

	rage::Vec3V camFront, camRight, camUp;
	CViewport::GetCamera(camFront, camRight, camUp);

	// Draw light gizmos
	for (u16 i = 0; i < drawable->GetLightCount(); i++)
	{
		CLightAttr* light = drawable->GetLight(i);
		const rage::Mat44V& lightWorld = m_LightWorldMatrices[i];

		// Draw edit gizmos only for selected light
		if (m_SelectedLight == i)
		{
			// We support falloff drag editing only for point light
			if (light->Type == LIGHT_POINT)
			{
				// Try to start editing falloff
				bool canStartFalloffEdit = false;
				if (!s_EditingFallof)
				{
					// We compute radius of point light sphere in screen units & distance to sphere center from mouse cursor
					// If its almost equal then we're hovering sphere edge
					rage::Vec3V sphereEdge = lightWorld.Pos + camRight * light->Falloff;
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
						float fallofEditRadius = dragPos.DistanceTo(lightWorld.Pos).Get();
						light->Falloff = fallofEditRadius;

						Im3D::TextBg(dragPos + rage::VEC_UP * 0.25f, "%.02f", dragDelta.Length().Get());

						// GRenderContext->OverlayRender.DrawLine(startDragPos, dragPos, graphics::ColorU32(140, 140, 140, 140));
					}

					if (dragFinished)
					{
						// TODO: ...
					}

					s_EditingFallof = isDragging;
				}
			}

			// Move light position using regular gizmo
			if (!s_EditingFallof && m_GizmoMode != GIZMO_None)
			{
				rage::Mat44V delta;
				ImGuizmo::OPERATION op;
				switch (m_GizmoMode)
				{
				case GIZMO_Translate:	op = ImGuizmo::TRANSLATE;	break;
				case GIZMO_Rotate:		op = ImGuizmo::ROTATE;		break;
				default: break;
				}

				if (Im3D::Gizmo(lightWorld, delta, op))
				{
					// Accept gizmo delta
					switch (m_GizmoMode)
					{
					case GIZMO_Translate:
					{
						light->Position += rage::Vec3V(delta.Pos);
						break;
					}
					//case GIZMO_Rotate:
					//{
					//	rage::QuatV deltaRot;
					//	delta.Decompose(0, 0, &deltaRot);
					//	light->Direction = rage::Vec3V(light->Direction).Rotate(deltaRot);
					//	break;
					//}
					default: break;
					}
				}
			}
		}

		bool highlightGizmo = i == hoverLightIndex || i == m_SelectedLight;
		graphics::ColorU32 lightGizmoColor = highlightGizmo ? graphics::COLOR_YELLOW : graphics::COLOR_ORANGE;
		graphics::ColorU32 lightGizmoSecondaryColor = highlightGizmo ? graphics::COLOR_YELLOW : graphics::COLOR_ORANGE;
		lightGizmoColor.A = m_SelectedLight == i ? 210 : 145;
		lightGizmoSecondaryColor.A = m_SelectedLight == i ? 180 : 125;

		// Draw light shape gizmos
		switch (light->Type)
		{
		case LIGHT_POINT:
		{
			GRenderContext->OverlayRender.DrawSphere(lightWorld, lightGizmoColor, light->Falloff);
			// Outer ring
			graphics::ColorU32 lightBorderColor = graphics::ColorU32(0, 157, 255);
			GRenderContext->OverlayRender.DrawCircle(
				lightWorld.Pos, camFront, camRight,
				light->Falloff, rage::Mat44V::Identity(), lightBorderColor, lightBorderColor);
			break;
		}
		case LIGHT_SPOT:
		{
			// Spot light is in fact half sphere
			// Draw outer edges
			float outRads = rage::Math::DegToRad(light->ConeOuterAngle);
			rage::Vec3V rotationNormal = camRight.Cross(rage::VEC_UP);
			const rage::Vec3V coneLine = rage::VEC_DOWN * light->Falloff;
			rage::Vec3V coneLeft = coneLine.Rotate(rotationNormal, -outRads);
			rage::Vec3V coneRight = coneLine.Rotate(rotationNormal, outRads);
			GRenderContext->OverlayRender.DrawLine(rage::VEC_ORIGIN, coneLeft, lightWorld, lightGizmoColor);
			GRenderContext->OverlayRender.DrawLine(rage::VEC_ORIGIN, coneRight, lightWorld, lightGizmoColor);
			// Inner edges
			rage::Vec3V coneInnerLeft = coneLine.Rotate(rotationNormal, rage::Math::DegToRad(-light->ConeInnerAngle));
			rage::Vec3V coneInnerRight = coneLine.Rotate(rotationNormal, rage::Math::DegToRad(light->ConeInnerAngle));
			GRenderContext->OverlayRender.DrawLine(rage::VEC_ORIGIN, coneInnerLeft, lightWorld, lightGizmoSecondaryColor);
			GRenderContext->OverlayRender.DrawLine(rage::VEC_ORIGIN, coneInnerRight, lightWorld, lightGizmoSecondaryColor);
			// Draw bottom arc
			constexpr int stepCount = 16;
			float step = outRads * 2 / stepCount;
			rage::Vec3V prevSegmentPos;
			for (int k = 0; k < stepCount + 1; k++)
			{
				float theta = -outRads + static_cast<float>(k) * step;
				rage::Vec3V segmentPos = coneLine.Rotate(rotationNormal, theta);
				if (k != 0) GRenderContext->OverlayRender.DrawLine(prevSegmentPos, segmentPos, lightWorld, lightGizmoColor);
				prevSegmentPos = segmentPos;
			}
			// Half sphere
			rage::Vec3V circleCenter = (coneLeft + coneRight) * rage::S_HALF;
			GRenderContext->OverlayRender.DrawCircle(
				circleCenter, rage::VEC_UP, rage::VEC_RIGHT, circleCenter.DistanceTo(coneLeft), lightWorld, lightGizmoColor, lightGizmoColor);
			break;
		}
		default: break;
		}

		Im3D::CenterNext();
		Im3D::TextBgColored(lightWorld.Pos, graphics::COLOR_YELLOW, "%s Light #%u", GetLightTypeName(light->Type), i);
	}

	// Draw editor for selected light
	if (m_SelectedLight == -1)
		return;

	ImGui::Begin(ICON_AM_LIGHT" Light Editor");
	{
		CLightAttr* light = drawable->GetLight(m_SelectedLight);

		ImGui::Text("Light #%i", m_SelectedLight);

		// Type picker
		static ConstString s_LightNames[] = { "Point", "Spot (Cone)", "Capsule" };
		static eLightType s_LightTypes[] = { LIGHT_POINT, LIGHT_SPOT, LIGHT_CAPSULE };
		static int s_LightTypeToIndex[] = { -1, 0, 1, -1, 2 };
		int lightTypeIndex = s_LightTypeToIndex[light->Type];
		if (ImGui::Combo("Type", &lightTypeIndex, s_LightNames, 3))
			light->Type = s_LightTypes[lightTypeIndex];

		if (SlGui::CollapsingHeader(ICON_AM_PICKER" Color###LIGHT_COLOR"))
		{
			graphics::ColorU32 color(light->ColorR, light->ColorG, light->ColorB);
			rage::Vector4 colorF = color.ToVec4();
			if (ImGui::ColorPicker3("Color", (float*)&colorF))
			{
				color = graphics::ColorU32::FromVec4(colorF);
				light->ColorR = color.R;
				light->ColorG = color.G;
				light->ColorB = color.B;
			}
		}

		ImGui::DragFloat("Falloff", &light->Falloff, 0.1f, 0, 25, "%.1f");
		ImGui::DragFloat("Falloff Exponent", &light->FallofExponent, 1.0f, 0, 500, "%.1f");
		if (light->Type == LIGHT_SPOT)
		{
			bool snapInnerAngle = rage::Math::AlmostEquals(light->ConeOuterAngle, light->ConeInnerAngle);
			if (ImGui::DragFloat("Cone Outer Angle", &light->ConeOuterAngle, 1, SPOT_LIGHT_MIN_CONE_ANGLE, SPOT_LIGHT_MAX_CONE_ANGLE, "%.1f"))
			{
				// Make sure that inner angle is not bigger than outer after editing
				if (light->ConeInnerAngle > light->ConeOuterAngle)
					light->ConeInnerAngle = light->ConeOuterAngle;

				if (snapInnerAngle)
					light->ConeInnerAngle = light->ConeOuterAngle;
			}
			ImGui::DragFloat("Cone Inner Angle", &light->ConeInnerAngle, 1, SPOT_LIGHT_MIN_CONE_ANGLE, light->ConeOuterAngle, "%.1f");

			if (SlGui::CollapsingHeader(ICON_AM_SPOT_LIGHT" Volume"))
			{
				graphics::ColorU32 volumeColor(light->VolumeOuterColorR, light->VolumeOuterColorG, light->VolumeOuterColorB);
				rage::Vector4 volumeColorF = volumeColor.ToVec4();
				if (SlGui::CollapsingHeader(ICON_AM_PICKER" Color###VOLUME_COLOR"))
				{
					if (ImGui::ColorPicker3("Outer Color", (float*)&volumeColorF))
					{
						volumeColor = graphics::ColorU32::FromVec4(volumeColorF);
						light->VolumeOuterColorR = volumeColor.R;
						light->VolumeOuterColorG = volumeColor.G;
						light->VolumeOuterColorB = volumeColor.B;
					}
				}

				ImGui::DragFloat("Intensity", &light->VolumeIntensity, 0.1f, 0.0f, 1.0f, "%.1f");
				ImGui::DragFloat("Size Scale", &light->VolumeSizeScale, 0.1f, 0.0f, 1.0f, "%.1f");
			}
		}
	}
	ImGui::End();
}
