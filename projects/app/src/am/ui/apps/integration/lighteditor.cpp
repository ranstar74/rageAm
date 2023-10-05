#include "lighteditor.h"

#include "ImGuizmo.h"
#include "am/graphics/shapetest.h"
#include "am/ui/im3d.h"
#include "am/ui/font_icons/icons_am.h"
#include "am/ui/styled/slwidgets.h"

u32 rageam::integration::LightEditor::GetOutlinerColor(bool isSelected, bool isHovered, bool isPrimary) const
{
	graphics::ColorU32 col = isPrimary ? graphics::COLOR_YELLOW : graphics::ColorU32(0, 160, 255);
	if (!isSelected) col.A -= 50;
	if (!isHovered) col.A -= 50;
	return col;
}

rageam::graphics::ShapeHit rageam::integration::LightEditor::ProbeLightSphere(const LightDrawContext& ctx) const
{
	graphics::ShapeHit shapeHit;
	shapeHit.DidHit = graphics::ShapeTest::RayIntersectsSphere(
		ctx.WorldMouseRay.Pos, ctx.WorldMouseRay.Dir, ctx.LightWorld.Pos, ctx.Light->Falloff, &shapeHit.Distance);
	return shapeHit;
}

rageam::graphics::ShapeHit rageam::integration::LightEditor::DrawOutliner_Point(const LightDrawContext& ctx) const
{
	// 3 Axis sphere
	GRenderContext->OverlayRender.DrawSphere(ctx.PrimaryColor, ctx.Light->Falloff);

	// Outer ring for falloff drag resizing, aligned to camera
	GRenderContext->OverlayRender.DrawCircle(
		rage::VEC_ORIGIN, ctx.CamFront, ctx.CamRight, ctx.Light->Falloff, ctx.SecondaryColor);

	return ProbeLightSphere(ctx);
}

rageam::graphics::ShapeHit rageam::integration::LightEditor::DrawOutliner_Spot(const LightDrawContext& ctx) const
{
	// Hit Test:
	// Spot light outline is a little bit complex, first do cheap bounding sphere test
	graphics::ShapeHit shapeHit = ProbeLightSphere(ctx);
	bool anyHit = false; // Fact of hitting bounding sphere doesn't guarantee that we're going to hit any other test

	// Advanced outline hit test:
	// Outline is composed of 3 parts - base cone triangle, a circle defining top of half sphere and half sphere arc
	// All parts are shown on this diagram: https://i.imgur.com/43qoAg2.png ( note that this is side view but not orthographic )
	auto testTriangle = [&](const rage::Vec3V& v1, const rage::Vec3V& v2, const rage::Vec3V& v3)
		{
			// Don't continue hit testing if previous test failed
			if (!shapeHit.DidHit)
				return;

			// We got hit already, next can be skipped
			if (anyHit)
				return;

			float hitDistance; // We just use distance from sphere bound instead
			if (graphics::ShapeTest::RayIntersectsTriangle(
				ctx.WorldMouseRay.Pos, ctx.WorldMouseRay.Dir, v1, v2, v3, hitDistance))
			{
				anyHit = true;
			}
		};

	// Spot light shape is a little bit different from regular cone light in blender,
	// the difference is that instead of straight bottom there's a half sphere (or arc if looking from side)
	// Side view diagram available here: https://i.imgur.com/cBYN3gJ.png

	// Maximum extent of light in light direction ( world down is aligned with light direction in light world matrix )
	const rage::Vec3V coneLine = rage::VEC_DOWN * ctx.Light->Falloff;

	// Outline must always face camera
	rage::Vec3V frontNormal = ctx.CamRight.Cross(rage::VEC_UP);

	// Rotates 2 lines with falloff length to -coneAngle and +coneAngle
	auto getLeftRight = [&](float theta, rage::Vec3V& coneLeft, rage::Vec3V& coneRight)
		{
			coneLeft = coneLine.Rotate(frontNormal, -theta);
			coneRight = coneLine.Rotate(frontNormal, theta);
		};

	// Outer & Inner edges
	float outerAngle = rage::Math::DegToRad(ctx.Light->ConeOuterAngle);
	float innerAngle = rage::Math::DegToRad(ctx.Light->ConeInnerAngle);
	rage::Vec3V coneOuterLeft, coneOuterRight, coneInnerLeft, coneInnerRight;
	getLeftRight(outerAngle, coneOuterLeft, coneOuterRight);
	getLeftRight(innerAngle, coneInnerLeft, coneInnerRight);
	GRenderContext->OverlayRender.DrawLine(rage::VEC_ORIGIN, coneOuterLeft, ctx.PrimaryColor);
	GRenderContext->OverlayRender.DrawLine(rage::VEC_ORIGIN, coneOuterRight, ctx.PrimaryColor);
	GRenderContext->OverlayRender.DrawLine(rage::VEC_ORIGIN, coneInnerLeft, ctx.SecondaryColor);
	GRenderContext->OverlayRender.DrawLine(rage::VEC_ORIGIN, coneInnerRight, ctx.SecondaryColor);

	// Test #1: Base cone triangle
	rage::Vec3V coneOuterLeftWorld = coneOuterLeft.Transform(ctx.LightWorld);
	rage::Vec3V coneOuterRightWorld = coneOuterRight.Transform(ctx.LightWorld);
	// The first vertex is located at light position ( emitting position ), which is basically top of the cone
	testTriangle(ctx.LightWorld.Pos, coneOuterLeftWorld, coneOuterRightWorld);

	// Half sphere top circle
	rage::Vec3V circleCenter = (coneOuterLeft + coneOuterRight) * rage::S_HALF;
	rage::ScalarV circleRadius = circleCenter.DistanceTo(coneOuterLeft); // TODO: Is there better way to compute this?
	GRenderContext->OverlayRender.DrawCircle(circleCenter, rage::VEC_UP, ctx.CamRight, circleRadius, ctx.PrimaryColor);

	// Test #2: Top of half sphere
	if (shapeHit.DidHit && !anyHit)
	{
		rage::Vec3V circleCenterWorld = circleCenter.Transform(ctx.LightWorld);
		if (graphics::ShapeTest::RayIntersectsCircle(
			ctx.WorldMouseRay.Pos, ctx.WorldMouseRay.Dir, circleCenterWorld, rage::VEC_UP, circleRadius))
		{
			anyHit = true;
		}
	}

	// Draw bottom arc for outer angle
	auto drawArc = [&](const rage::Vec3V& normal)
		{
			constexpr int arcSegmentCount = 16;
			float arcStep = outerAngle * 2 / arcSegmentCount;
			rage::Vec3V prevSegmentPos;
			rage::Vec3V prevSegmentPosWorld;
			for (int i = 0; i < arcSegmentCount + 1; i++)
			{
				// From -angle to +angle
				float arcTheta = -outerAngle + static_cast<float>(i) * arcStep;
				rage::Vec3V segmentPos = coneLine.Rotate(normal, arcTheta);
				rage::Vec3V segmentPosWorld;
				if (!anyHit) // There was hit already... skip
					segmentPosWorld = segmentPos.Transform(ctx.LightWorld);
				if (i != 0)
				{
					GRenderContext->OverlayRender.DrawLine(prevSegmentPos, segmentPos, ctx.PrimaryColor);

					// Test #3: Half sphere arc
					testTriangle(prevSegmentPosWorld, segmentPosWorld, coneOuterLeftWorld);
				}
				prevSegmentPos = segmentPos;
				prevSegmentPosWorld = segmentPosWorld;
			}
		};

	// World aligned
	//drawArc(rage::VEC_FORWARD);
	//drawArc(rage::VEC_RIGHT);
	drawArc(frontNormal);

	if (!anyHit)
		shapeHit.DidHit = false;

	return shapeHit;
}

rageam::graphics::ShapeHit rageam::integration::LightEditor::DrawOutliner(const LightDrawContext& ctx) const
{
	GRenderContext->OverlayRender.SetCurrentMatrix(ctx.LightWorld);

	graphics::ShapeHit shapeHit = {};
	switch (ctx.Light->Type)
	{
	case LIGHT_POINT:	shapeHit = DrawOutliner_Point(ctx); break;
	case LIGHT_SPOT:	shapeHit = DrawOutliner_Spot(ctx); break;
	default: break;
	}

	GRenderContext->OverlayRender.ResetCurrentMatrix();

	return shapeHit;
}

void rageam::integration::LightEditor::ComputeLightWorldMatrix(
	gtaDrawable* drawable, const rage::Mat44V& entityMtx, u16 lightIndex,
	rage::Mat44V& lightWorld, rage::Mat44V& lightBind) const
{
	CLightAttr* light = drawable->GetLight(lightIndex);

	// Create orientation matrix
	rage::Mat44V lightLocal = light->GetMatrix();

	lightBind = rage::Mat44V::Identity();
	// If there's no skeleton, light transform is defined by CLightAttr.Position * EntityMatrix
	rage::crSkeletonData* skeleton = drawable->GetSkeletonData();
	if (skeleton)
	{
		// Parent light to bone
		rage::crBoneData* bone = skeleton->GetBoneFromTag(light->BoneTag);
		rage::Mat44V lightBoneMtx = skeleton->GetBoneWorldTransform(bone);
		lightBind *= lightBoneMtx;
	}

	lightBind *= entityMtx;
	// Transform to light world space
	lightWorld = lightLocal * lightBind;
	// Inverse bind matrix after we used it to transform local light to world
	lightBind = lightBind.Inverse();
}

void rageam::integration::LightEditor::RenderLightUI(CLightAttr* light) const
{
	if (ImGui::Begin(ICON_AM_LIGHT" Light Editor"))
	{
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
		ImGui::DragFloat("Falloff Exponent", &light->FallofExponent, 1.0f, 0, 500, "%.2f", ImGuiSliderFlags_Logarithmic);

		// ImGui::DragU8("Flashiness", &light->Flashiness, 1, 0, 255);

		struct Flashiness
		{
			float TimeOn, TimeOff, FadeInTime, FadeOutTime;
		};

		// TODO: Better blink picker. Can we simulate blinking pattern in UI itself for preview?
		ImGui::Text("Flashiness (blinking)");
		ImGui::DragU8("###FLASHINESS", &light->Flashiness, 1, 0, 20);
		bool noInc = light->Flashiness == 20;
		bool noSub = light->Flashiness == 0;
		ImGui::SameLine();
		ImGui::BeginDisabled(noSub);
		if (ImGui::Button("<")) light->Flashiness--;
		ImGui::EndDisabled();
		ImGui::BeginDisabled(noInc);
		ImGui::SameLine();
		if (ImGui::Button(">")) light->Flashiness++;
		ImGui::EndDisabled();

		ImGui::DragFloat("Intensity###LIGHT_INTENSITY", &light->Intensity, 1.0f, 0.0f, 1000, "%.1f");
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
				ImGui::DragFloat("Intensity###VOLUME_INTENSITY", &light->VolumeIntensity, 0.005f, 0.0f, 1.0f, "%.3f");
				ImGui::DragFloat("Size Scale", &light->VolumeSizeScale, 0.01f, 0.0f, 1.0f, "%.2f");

				graphics::ColorU32 volumeColor(light->VolumeOuterColorR, light->VolumeOuterColorG, light->VolumeOuterColorB);
				rage::Vector4 volumeColorF = volumeColor.ToVec4();
				if (SlGui::CollapsingHeader(ICON_AM_PICKER" Outer Color###VOLUME_COLOR"))
				{
					if (ImGui::ColorPicker3("Outer Color", (float*)&volumeColorF))
					{
						volumeColor = graphics::ColorU32::FromVec4(volumeColorF);
						light->VolumeOuterColorR = volumeColor.R;
						light->VolumeOuterColorG = volumeColor.G;
						light->VolumeOuterColorB = volumeColor.B;
					}
				}
				ImGui::DragFloat("Outer Exponent", &light->VolumeOuterExponent, 1.0f, 0, 500, "%.2f", ImGuiSliderFlags_Logarithmic);
				ImGui::DragFloat("Outer Intensity", &light->VolumeOuterIntensity, 0.005f, 0.0f, 1.0f, "%.3f");

				// Fade distance 0 - disabled, handle this with checkbox
				bool fadeEnabled = light->VolumetricFadeDistance > 0;
				if (ImGui::Checkbox("Fade", &fadeEnabled))
				{
					if (fadeEnabled)	light->VolumetricFadeDistance = 255;
					else				light->VolumetricFadeDistance = 0;
				}
				if (fadeEnabled)
				{
					ImGui::DragU8("Fade Distance", &light->VolumetricFadeDistance, 1, 1, 255);
				}
			}
		}
	}
	ImGui::End();
}

void rageam::integration::LightEditor::RenderLightTransformGizmo(CLightAttr* light, const rage::Mat44V& lightWorld, const rage::Mat44V& lightBind) const
{
	if (m_GizmoMode != GIZMO_None)
	{
		ImGuizmo::OPERATION op;
		switch (m_GizmoMode)
		{
		case GIZMO_Translate:	op = ImGuizmo::TRANSLATE;	break;
		case GIZMO_Rotate:		op = ImGuizmo::ROTATE;		break;
		default: break;
		}

		rage::Mat44V editDelta;
		rage::Mat44V editedLightWorld = lightWorld;
		if (Im3D::Gizmo(editedLightWorld, editDelta, op))
		{
			rage::Mat44V local = editedLightWorld * lightBind;
			light->SetMatrix(local);
		}
	}
}

void rageam::integration::LightEditor::SelectGizmoMode(gtaDrawable* drawable)
{
	if (m_SelectedLight == -1)
		return;

	// Handle gizmo mode toggling
	// G - Translation
	// R - Rotation
	int newModeMask = 0;

	if (ImGui::IsKeyPressed(ImGuiKey_G, false)) newModeMask = GIZMO_Translate;
	// Rotation is only supported on spot light
	if (drawable->GetLight(m_SelectedLight)->Type == LIGHT_SPOT)
		if (ImGui::IsKeyPressed(ImGuiKey_R, false)) newModeMask = GIZMO_Rotate;

	if (newModeMask != 0) // Some Mode was toggled
	{
		m_GizmoMode &= newModeMask; // Clear other modes
		m_GizmoMode ^= newModeMask; // Toggle pressed mode
	}
}

void rageam::integration::LightEditor::Render(gtaDrawable* drawable, const rage::Mat44V& entityMtx)
{
	u16 lightCount = drawable->GetLightCount();
	if (lightCount == 0)
		return;

	SelectGizmoMode(drawable);

	static bool s_EditingFallof = false;
	if (m_SelectedLight == -1)
		s_EditingFallof = false;

	// Prepare context for drawing lights
	LightDrawContext drawContext;
	CViewport::GetCamera(&drawContext.CamFront, &drawContext.CamRight, &drawContext.CamUp);
	CViewport::GetWorldMouseRay(drawContext.WorldMouseRay.Pos, drawContext.WorldMouseRay.Dir);

	rage::Mat44V selectedLightWorld;
	rage::Mat44V selectedLightBind;

	auto hoveredLightDistance = rage::S_MAX;
	s32 hoveredLightIndex = -1;
	for (u16 i = 0; i < drawable->GetLightCount(); i++)
	{
		CLightAttr* light = drawable->GetLight(i);
		rage::Mat44V lightWorld;
		rage::Mat44V lightBind;
		ComputeLightWorldMatrix(drawable, entityMtx, i, lightWorld, lightBind);

		bool isSelected = m_SelectedLight == i;
		bool isHovered = m_HoveredLight == i;

		if (isSelected)
		{
			selectedLightWorld = lightWorld;
			selectedLightBind = lightBind;
		}

		drawContext.LightWorld = lightWorld;
		drawContext.IsSelected = m_SelectedLight == i;
		drawContext.Light = light;
		drawContext.PrimaryColor = GetOutlinerColor(isSelected, isHovered, true);
		drawContext.SecondaryColor = GetOutlinerColor(isSelected, isHovered, false);
		drawContext.CulledColor = graphics::ColorU32(140, 140, 140, 140);

		//// Draw edit gizmos only for selected light
		//if (isSelected)
		//{
		//	// We support falloff drag editing only for point light
		//	if (light->Type == LIGHT_POINT)
		//	{
		//		// Try to start editing falloff
		//		bool canStartFalloffEdit = false;
		//		if (!s_EditingFallof)
		//		{
		//			// We compute radius of point light sphere in screen units & distance to sphere center from mouse cursor
		//			// If its almost equal then we're hovering sphere edge
		//			rage::Vec3V sphereEdge = lightWorld.Pos + camRight * light->Falloff;
		//			ImVec2 screenSphereCenter;
		//			ImVec2 screenSphereEdge;
		//			if (Im3D::WorldToScreen(lightWorld.Pos, screenSphereCenter) &&
		//				Im3D::WorldToScreen(sphereEdge, screenSphereEdge))
		//			{
		//				float screenSphereRadius = ImGui::Distance(screenSphereCenter, screenSphereEdge);
		//				float distanceToSphere = ImGui::Distance(screenSphereCenter, ImGui::GetMousePos());

		//				// Check if mouse cursor is on the edge of sphere
		//				constexpr float edgeWidth = 5.0f;
		//				float distanceFromEdge = abs(screenSphereRadius - distanceToSphere);
		//				if (distanceFromEdge < edgeWidth)
		//				{
		//					canStartFalloffEdit = true;
		//				}
		//			}
		//		}

		//		// Update editing falloff
		//		if (s_EditingFallof || canStartFalloffEdit)
		//		{
		//			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

		//			rage::Vec3V dragDelta, startDragPos, dragPos;
		//			rage::Vec3V spherePlaneNormal = camFront; // We are editing falloff on camera plane
		//			bool isDragging;
		//			bool dragFinished = Im3D::GizmoBehaviour(isDragging, lightWorld.Pos, spherePlaneNormal, dragDelta, startDragPos, dragPos);

		//			// Compute new falloff radius
		//			if (isDragging)
		//			{
		//				float fallofEditRadius = dragPos.DistanceTo(lightWorld.Pos).Get();
		//				light->Falloff = fallofEditRadius;

		//				Im3D::TextBg(dragPos + rage::VEC_UP * 0.25f, "%.02f", dragDelta.Length().Get());

		//				// GRenderContext->OverlayRender.DrawLine(startDragPos, dragPos, graphics::ColorU32(140, 140, 140, 140));
		//			}

		//			if (dragFinished)
		//			{
		//				// TODO: ...
		//			}

		//			s_EditingFallof = isDragging;
		//		}
		//	}

		//	// Move light position using regular gizmo
		//	if (!s_EditingFallof && m_GizmoMode != GIZMO_None)
		//	{
		//		rage::Mat44V delta;
		//		ImGuizmo::OPERATION op;
		//		switch (m_GizmoMode)
		//		{
		//		case GIZMO_Translate:	op = ImGuizmo::TRANSLATE;	break;
		//		case GIZMO_Rotate:		op = ImGuizmo::ROTATE;		break;
		//		default: break;
		//		}

		//		if (Im3D::Gizmo(lightWorld, delta, op))
		//		{
		//			// Accept gizmo delta
		//			switch (m_GizmoMode)
		//			{
		//			case GIZMO_Translate:
		//			{
		//				light->Position += rage::Vec3V(delta.Pos);
		//				break;
		//			}
		//			//case GIZMO_Rotate:
		//			//{
		//			//	rage::QuatV deltaRot;
		//			//	delta.Decompose(0, 0, &deltaRot);
		//			//	light->Direction = rage::Vec3V(light->Direction).Rotate(deltaRot);
		//			//	break;
		//			//}
		//			default: break;
		//			}
		//		}
		//	}
		//}
		//

		// Render light outlines and find the closest one to the camera
		graphics::ShapeHit shapeHit = DrawOutliner(drawContext);
		if (shapeHit.DidHit && shapeHit.Distance < hoveredLightDistance)
		{
			hoveredLightDistance = shapeHit.Distance;
			hoveredLightIndex = i;
		}

		// Light name in 3D
		// TODO: Use name from DrawableAssetMap
		Im3D::CenterNext();
		Im3D::TextBgColored(lightWorld.Pos, graphics::COLOR_YELLOW, "%s Light #%u", GetLightTypeName(light->Type), i);
	}

	m_HoveredLight = hoveredLightIndex;

	// Select light if covered & left clicked
	bool canSelectLights = !ImGuizmo::IsOver() && !ImGui::IsAnyWindowHovered();
	if (canSelectLights && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		m_SelectedLight = m_HoveredLight;

	// Draw editor for selected light
	if (m_SelectedLight != -1)
	{
		CLightAttr* light = drawable->GetLight(m_SelectedLight);

		RenderLightTransformGizmo(light, selectedLightWorld, selectedLightBind);
		RenderLightUI(light);
	}
}
