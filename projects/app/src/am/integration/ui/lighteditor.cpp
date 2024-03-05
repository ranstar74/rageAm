#ifdef AM_INTEGRATED

#include "lighteditor.h"
#include "ImGuizmo.h"
#include "modelscene.h"
#include "am/graphics/shapetest.h"
#include "am/integration/im3d.h"
#include "am/ui/slwidgets.h"
#include "am/ui/font_icons/icons_am.h"
#include "game/viewport.h"
#include "rage/math/math.h"
#include "am/integration/gamedrawlists.h"

void rageam::integration::LightEditor::SetCullPlaneFromLight(const LightDrawContext& ctx)
{
	// See diagram: https://i.imgur.com/SyBQ591.png
	rage::Vec3V planeNormal = ctx.Light->CullingPlaneNormal;
	rage::ScalarV planeOffset = ctx.Light->CullingPlaneOffset;
	rage::Vec3V cullingNormal = -planeNormal; // Everything behind the plane is culled
	rage::Vec3V planePos = cullingNormal * planeOffset;
	m_CullPlane = rage::Mat44V::FromNormalPos(planePos, planeNormal);
}

u32 rageam::integration::LightEditor::GetOutlinerColor(bool isSelected, bool isHovered, bool isPrimary) const
{
	graphics::ColorU32 col = isPrimary ? graphics::COLOR_YELLOW : graphics::ColorU32(0, 160, 255);
	if (!isSelected) col.A -= 65;
	if (!isSelected && !isHovered) col.A -= 40;
	if (m_EditingCullPlane) col.A -= 150;
	return col;
}

rageam::integration::DrawList& rageam::integration::LightEditor::GetDrawList() const
{
	return GameDrawLists::GetInstance()->Overlay;
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
	DrawList& drawList = GetDrawList();

	// 3 Axis sphere
	drawList.DrawSphere(ctx.PrimaryColor, ctx.Light->Falloff);

	// Outer ring for falloff drag resizing, aligned to camera
	drawList.DrawCircle(
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

	DrawList& drawList = GetDrawList();

	// Outer & Inner edges
	float outerAngle = rage::DegToRad(ctx.Light->ConeOuterAngle);
	float innerAngle = rage::DegToRad(ctx.Light->ConeInnerAngle);
	rage::Vec3V coneOuterLeft, coneOuterRight, coneInnerLeft, coneInnerRight;
	getLeftRight(outerAngle, coneOuterLeft, coneOuterRight);
	getLeftRight(innerAngle, coneInnerLeft, coneInnerRight);
	drawList.DrawLine(rage::VEC_ORIGIN, coneOuterLeft, ctx.PrimaryColor);
	drawList.DrawLine(rage::VEC_ORIGIN, coneOuterRight, ctx.PrimaryColor);
	drawList.DrawLine(rage::VEC_ORIGIN, coneInnerLeft, ctx.SecondaryColor);
	drawList.DrawLine(rage::VEC_ORIGIN, coneInnerRight, ctx.SecondaryColor);

	// Test #1: Base cone triangle
	rage::Vec3V coneOuterLeftWorld = coneOuterLeft.Transform(ctx.LightWorld);
	rage::Vec3V coneOuterRightWorld = coneOuterRight.Transform(ctx.LightWorld);
	// The first vertex is located at light position ( emitting position ), which is basically top of the cone
	testTriangle(ctx.LightWorld.Pos, coneOuterLeftWorld, coneOuterRightWorld);

	// Half sphere top circle
	rage::Vec3V circleCenter = (coneOuterLeft + coneOuterRight) * rage::S_HALF;
	rage::ScalarV circleRadius = circleCenter.DistanceTo(coneOuterLeft); // TODO: Is there better way to compute this?
	drawList.DrawCircle(circleCenter, rage::VEC_UP, ctx.CamRight, circleRadius, ctx.PrimaryColor);

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
					drawList.DrawLine(prevSegmentPos, segmentPos, ctx.PrimaryColor);

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

rageam::graphics::ShapeHit rageam::integration::LightEditor::DrawOutliner_Capsule(const LightDrawContext& ctx) const
{
	DrawList& drawList = GetDrawList();

	// Capsule light diagram: https://i.imgur.com/IdXEw2a.png

	rage::ScalarV halfExtent = ctx.Light->Extent.X * 0.5f;
	rage::Vec3V extentFrom = rage::VEC_UP * halfExtent;
	rage::Vec3V extentTo = rage::VEC_DOWN * halfExtent;

	drawList.DrawLine(extentFrom, extentTo, ctx.SecondaryColor);

	float radius = ctx.Light->Falloff;

	// Half sphere tops
	drawList.DrawCircle(extentTo, rage::VEC_UP, rage::VEC_RIGHT, radius, ctx.PrimaryColor);
	drawList.DrawCircle(extentFrom, rage::VEC_UP, rage::VEC_RIGHT, radius, ctx.PrimaryColor);

	// Draw two half spheres (arcs)
	drawList.DrawCircle(extentTo, rage::VEC_FRONT, rage::VEC_RIGHT, radius, ctx.PrimaryColor, 0, rage::PI);
	drawList.DrawCircle(extentTo, -rage::VEC_RIGHT, rage::VEC_FRONT, radius, ctx.PrimaryColor, 0, rage::PI);
	drawList.DrawCircle(extentFrom, -rage::VEC_FRONT, rage::VEC_RIGHT, radius, ctx.PrimaryColor, 0, rage::PI);
	drawList.DrawCircle(extentFrom, rage::VEC_RIGHT, rage::VEC_FRONT, radius, ctx.PrimaryColor, 0, rage::PI);

	// Draw lines connecting half spheres (arcs)
	drawList.DrawLine(extentTo + rage::VEC_FRONT * radius, extentFrom + rage::VEC_FRONT * radius, ctx.PrimaryColor);
	drawList.DrawLine(extentTo + rage::VEC_RIGHT * radius, extentFrom + rage::VEC_RIGHT * radius, ctx.PrimaryColor);
	drawList.DrawLine(extentTo - rage::VEC_FRONT * radius, extentFrom - rage::VEC_FRONT * radius, ctx.PrimaryColor);
	drawList.DrawLine(extentTo - rage::VEC_RIGHT * radius, extentFrom - rage::VEC_RIGHT * radius, ctx.PrimaryColor);

	// Hit test
	graphics::ShapeHit shapeHit;
	shapeHit.DidHit = graphics::ShapeTest::RayIntersectsCapsule(ctx.WorldMouseRay.Pos, ctx.WorldMouseRay.Dir,
		extentFrom.Transform(ctx.LightWorld), extentTo.Transform(ctx.LightWorld), radius, &shapeHit.Distance);
	return shapeHit;
}

rageam::graphics::ShapeHit rageam::integration::LightEditor::DrawOutliner(const LightDrawContext& ctx) const
{
	DrawList& drawList = GetDrawList();

	if (OutlineMode == OutlineMode_None || 
		(OutlineMode == OutlineMode_OnlySelected && !ctx.IsSelected))
	{
		return graphics::ShapeHit(0, false);
	}

	drawList.SetTransform(ctx.LightWorld);

	graphics::ShapeHit shapeHit = {};
	switch (ctx.Light->Type)
	{
	case LIGHT_TYPE_SPOT:		shapeHit = DrawOutliner_Spot(ctx);		break;
	case LIGHT_TYPE_CAPSULE:	shapeHit = DrawOutliner_Capsule(ctx);	break;
	case LIGHT_TYPE_POINT:
	case LIGHT_TYPE_DIRECTIONAL:
	case LIGHT_TYPE_AO_VOLUME:	shapeHit = DrawOutliner_Point(ctx);		break;
	default: break;
	}

	drawList.ResetTransform();

	// Light name in 3D
	// TODO: Use name from DrawableAssetMap
	Im3D::CenterNext();
	Im3D::TextBgColored(
		ctx.LightWorld.Pos, ctx.PrimaryColor, "%s Light #%u", GetLightTypeName(ctx.Light->Type), ctx.LightIndex);

	return shapeHit;
}

bool rageam::integration::LightEditor::DrawPointLightFalloffGizmo(const LightDrawContext& ctx)
{
	bool canStartEdit =
		!ImGuizmo::IsOver() &&
		!ImGui::IsAnyWindowHovered() &&
		!ImGui::IsAnyPopUpOpen();

	bool canEdit = false;
	// Try to start editing
	if (!m_UsingPointFalloffGizmo && canStartEdit)
	{
		// We compute radius of point light sphere in screen units & distance to sphere center from mouse cursor
		// If its almost equal then we're hovering sphere edge
		rage::Vec3V sphereEdge = ctx.LightWorld.Pos + ctx.CamRight * ctx.Light->Falloff;
		ImVec2 screenSphereCenter;
		ImVec2 screenSphereEdge;
		if (Im3D::WorldToScreen(ctx.LightWorld.Pos, screenSphereCenter) &&
			Im3D::WorldToScreen(sphereEdge, screenSphereEdge))
		{
			float screenSphereRadius = ImGui::Distance(screenSphereCenter, screenSphereEdge);
			float distanceToSphere = ImGui::Distance(screenSphereCenter, ImGui::GetMousePos());

			// Check if mouse cursor is on the edge of sphere
			constexpr float edgeWidth = 5.0f;
			float distanceFromEdge = abs(screenSphereRadius - distanceToSphere);
			if (distanceFromEdge < edgeWidth)
			{
				canEdit = true;
			}
		}
	}

	// Not editing & cant' start... Skip
	if (!canEdit && !m_UsingPointFalloffGizmo)
		return false;

	// Update editing falloff
	ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

	rage::Vec3V dragDelta, startDragPos, dragPos;
	rage::Vec3V spherePlaneNormal = ctx.CamFront; // We are editing falloff on camera plane
	bool isDragging;
	bool dragFinished = Im3D::GizmoBehaviour(isDragging, ctx.LightWorld.Pos, spherePlaneNormal, dragDelta, startDragPos, dragPos);

	// Compute new falloff radius
	if (isDragging)
	{
		float fallofEditRadius = dragPos.DistanceTo(ctx.LightWorld.Pos).Get();
		ctx.Light->Falloff = fallofEditRadius;

		Im3D::TextBg(dragPos + rage::VEC_UP * 0.25f, "%.02f", dragDelta.Length().Get());
	}

	m_UsingPointFalloffGizmo = isDragging;

	return true;
}

void rageam::integration::LightEditor::DrawCullPlaneEditGizmo(const LightDrawContext& ctx)
{
	DrawList& drawList = GetDrawList();

	// See diagram: https://i.imgur.com/SyBQ591.png

	rage::Vec4V lightPos(ctx.Light->Position, 0.0f);

	rage::Mat44V delta;
	rage::Mat44V planeWorld = m_CullPlane * ctx.LightBoneWorld;
	planeWorld.Pos += lightPos; // Cull plane ignores light orientation, use only position

	drawList.DrawQuad(
		planeWorld.Pos, planeWorld.Front, planeWorld.Right, rage::S_TWO, rage::S_ONE, graphics::COLOR_WHITE);

	// Draw line that indicates culling area
	drawList.DrawLine(
		planeWorld.Pos, planeWorld.Pos - planeWorld.Front, graphics::COLOR_RED);

	// Debug cull info
	//Im3D::TextBg(planeWorld.Pos, "<Cull Plane> Normal: %.2f %.2f %.2f Offset: %.2f",
	//	ctx.Light->CullingPlaneNormal.X, ctx.Light->CullingPlaneNormal.Y, ctx.Light->CullingPlaneNormal.Z,
	//	ctx.Light->CullingPlaneOffset);

	if (Im3D::Gizmo(planeWorld, delta, ImGuizmo::OPERATION(GetImGuizmoOperation())))
	{
		planeWorld.Pos -= lightPos;
		m_CullPlane = planeWorld * ctx.LightBoneWorld.Inverse();

		rage::Vec3V pos = m_CullPlane.Pos;
		rage::Vec3V normal = m_CullPlane.Front;
		rage::ScalarV offset;
		graphics::ShapeTest::ClosestPointOnPlane(rage::VEC_ORIGIN, -normal, pos, normal, offset);

		ctx.Light->CullingPlaneNormal = normal;
		ctx.Light->CullingPlaneOffset = offset.Get();
	}
}

void rageam::integration::LightEditor::ComputeLightMatrices(
	u16 lightIndex,
	rage::Mat44V& lightWorld,
	rage::Mat44V& lightBind,
	rage::Mat44V& lightLocal,
	rage::Mat44V& lightBoneWorld) const
{
	CLightAttr* light = m_SceneContext->Drawable->GetLight(lightIndex);

	// Create orientation matrix
	lightBoneWorld = rage::Mat44V::Identity();
	lightLocal = light->GetMatrix();

	// If there's no skeleton, light transform is defined by CLightAttr.Position * EntityMatrix
	rage::crSkeletonData* skeleton = m_SceneContext->Drawable->GetSkeletonData().Get();
	if (skeleton)
	{
		// Parent light to bone
		rage::crBoneData* bone = skeleton->GetBoneFromTag(light->BoneTag);
		rage::Mat44V lightBoneMtx = skeleton->GetBoneWorldTransform(bone);
		lightBoneWorld *= lightBoneMtx;
	}

	lightBoneWorld *= m_SceneContext->EntityWorld;
	lightBind = lightBoneWorld;
	// Transform to light world space
	lightWorld = lightLocal * lightBind;
	// Inverse bind matrix after we used it to transform local light to world
	lightBind = lightBind.Inverse();
}

int rageam::integration::LightEditor::GetImGuizmoOperation() const
{
	ImGuizmo::OPERATION op = ImGuizmo::OPERATION(0);
	switch (m_GizmoMode)
	{
	case GIZMO_Translate:	op = ImGuizmo::TRANSLATE;	break;
	case GIZMO_Rotate:		op = ImGuizmo::ROTATE;		break;
	default: break;
	}
	return op;
}

void rageam::integration::LightEditor::DrawLightUI(const LightDrawContext& ctx)
{
	CLightAttr* light = ctx.Light;

	ConstString windowName = ImGui::FormatTemp(ICON_AM_LIGHT" Light Editor (%s #%i)###LIGHT_EDITOR_WINDOW",
		GetLightTypeName(light->Type), m_SelectedLight);

	if (ImGui::Begin(windowName))
	{
		// Type picker
		static ConstString s_LightNames[] = { "Point", "Spot (Cone)", "Capsule", "Directional", "AO Volume"};
		static eLightType s_LightTypes[] = { LIGHT_TYPE_POINT, LIGHT_TYPE_SPOT, LIGHT_TYPE_CAPSULE, LIGHT_TYPE_DIRECTIONAL, LIGHT_TYPE_AO_VOLUME};
		static int s_LightTypeToIndex[] = 
		{
			// Light types are 1 2 4 8 16
			-1, 0, 1, -1, 2, -1, -1, -1, 3, -1, -1, -1, -1, -1, -1, -1, 4
		};
		int lightTypeIndex = s_LightTypeToIndex[light->Type];
		if (ImGui::Combo("Type", &lightTypeIndex, s_LightNames, 5))
			light->Type = s_LightTypes[lightTypeIndex];

		if (m_EditingCullPlane) ImGui::BeginDisabled();
		ImGui::Checkbox("Freeze Selection", &m_SelectionFreezed);
		if (m_EditingCullPlane) ImGui::EndDisabled();

		static bool s_EnableArtificialLights = true;
		if (ImGui::Checkbox("Enable world lights", &s_EnableArtificialLights))
		{
			//scrInvoke([]
			//	{
			//		// SHV::GRAPHICS::SET_ARTIFICIAL_LIGHTS_STATE(!s_EnableArtificialLights);
			//	});
		}
		ImGui::SameLine();
		ImGui::HelpMarker("In order to see current light, flag 'Ignore Artificial Light State' must be enabled.");

		// Helper to edit fade distance (there are multiple ones for light, shadow, volume)
		auto editFadeDistance = [&](ConstString name, u8& fadeDistance)
			{
				bool fadeEnabled = fadeDistance > 0;
				if (ImGui::Checkbox(name, &fadeEnabled))
				{
					if (fadeEnabled)	fadeDistance = 255;
					else				fadeDistance = 0;
				}
				if (fadeEnabled)
				{
					ImGui::Indent();
					ImGui::DragU8(ImGui::FormatTemp("Distance###%s", name), &fadeDistance, 1, 1, 255);
					ImGui::Unindent();
				}
			};

		if (ImGui::BeginTabBar("LIGHT_EDITOR_TAB_BAR"))
		{
			if (ImGui::BeginTabItem("General"))
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

				ImGui::DragFloat("Falloff", &light->Falloff, 0.1f, 0, 25, "%.1f");
				ImGui::DragFloat("Falloff Exponent", &light->FallofExponent, 1.0f, 0, 500, "%.2f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
				ImGui::DragFloat("Intensity###LIGHT_INTENSITY", &light->Intensity, 1.0f, 0.0f, 1000, "%.1f");
				editFadeDistance("Fade", light->LightFadeDistance);

				if (light->Type == LIGHT_TYPE_CAPSULE)
				{
					SlGui::CategoryText(ICON_AM_CAPSULE" Capsule");
					ImGui::DragFloat("Length", &light->Extent.X, 0.1f, 0.0f, 25.0f, " %.1f");
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(ICON_AM_FLAGS" Flags"))
			{
				static bool s_ShowAllFlags = false;
				ImGui::Checkbox("Show All", &s_ShowAllFlags);
				ImGui::SameLine();
				ImGui::HelpMarker("Show all flags including unknown, for research purposes.");
				ImGui::Separator();

				if (!s_ShowAllFlags)
				{
					ImGui::CheckboxFlags("Disable Light", &light->Flags, LF_DISABLE_LIGHT);
					ImGui::ToolTip("Only volume will be rendered");

					ImGui::CheckboxFlags("Ignore Time", &light->Flags, LF_IGNORE_TIME);
					ImGui::ToolTip("Don't use light intensity based on game time (brighter at night, dimmer at day)");

					ImGui::CheckboxFlags("Electrical buzz/hum", &light->Flags, LF_ELECTRICAL_HUM);
					ImGui::ToolTip("Plays electrical buzz sound near the light, note that volume is pretty low");

					ImGui::CheckboxFlags("Render Underground", &light->Flags, LF_RENDER_UNDERGROUND);
					ImGui::ToolTip("Draw light under map (in tunnels)");

					ImGui::CheckboxFlags("Ignore Artificial Light State", &light->Flags, LF_IGNORE_ARTIFICIAL_LIGHT_STATE);
					ImGui::ToolTip("Light will ignore SET_ARTIFICIAL_LIGHTS_STATE(FALSE) and keep rendering");

					ImGui::CheckboxFlags("No Reflection", &light->Flags, LF_NO_REFLECTION);
					ImGui::CheckboxFlags("Ignore Glass", &light->Flags, LF_IGNORE_GLASS);
					ImGui::ToolTip("Light won't affect glass");
				}
				else
				{
					for (int i = 0; i < 32; i++)
					{
						eLightFlags flag = eLightFlags(1 << i);
						ConstString name = Enum::GetName(flag);
						if (!name)
							name = ImGui::FormatTemp("LF_UNKNOWN_%i", i);
						ImGui::CheckboxFlags(name, &light->Flags, flag);
					}
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Misc"))
			{
				ImVec2 tilePadding = ImVec2(0, ImGui::GetFrameHeight() * 0.25f);
				if (SlGui::CollapsingHeader("Flashiness (blinking)"))
				{
					ImGui::HelpMarker(
						"#0 is the default 'always on' state.\n"
						"#11, #12, #13 use XY light position as the seed for randomizing.\n"
						"#14 is synced with #3 unless there is a radio audio emitter nearby.\n", "Help");

					// Additional simple slider with buttons
					ImGui::SliderU8("###FLASHINESS", &light->Flashiness, 0, 20, "%u");
					bool noInc = light->Flashiness == 20;
					bool noSub = light->Flashiness == 0;
					ImGui::SameLine(0, 1);
					ImGui::BeginDisabled(noSub);
					if (ImGui::Button("<")) light->Flashiness--;
					ImGui::EndDisabled();
					ImGui::BeginDisabled(noInc);
					ImGui::SameLine(0, 1);
					if (ImGui::Button(">")) light->Flashiness++;
					ImGui::EndDisabled();

					static auto getFlashinessState = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
						"80 7C 24 30 07", "CLightEntity::ApplyEffectsToLightCommon+0x31").GetAt(-0x31)
#else
						"8D 41 F5 3C 02", "CLightEntity::ApplyEffectsToLightCommon+0x53").GetAt(-0x53)
#endif
						.ToFunc<void(u8 flashiness, rage::Mat34V * transform, float& outIntensity, bool& outIsDrawn)>();

					ImGui::Dummy(tilePadding); // Add extra padding before flashiness tiles
					ImGui::Indent();

					for (int i = 0; i <= 20; i++)
					{
						// 4 items per row
						if (i % 4 != 0)
							ImGui::SameLine();

						ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;

						// Flashiness picker button
						float frameHeight = ImGui::GetFrameHeight();
						ImVec2 itemSize(frameHeight * 2.25f, frameHeight * 1.25f);
						bool selected = light->Flashiness == i;
						if (ImGui::Selectable(ImGui::FormatTemp("#%u###FLASHINESS_SELECTABLE_%u", i, i), selected, 0, itemSize))
						{
							light->Flashiness = i;
						}
						const ImRect& selectableRect = GImGui->LastItemData.Rect;
						// Selectable border
						drawList->AddRect(selectableRect.GetTL(), selectableRect.GetBR(),
							selected ? ImGui::GetColorU32(ImGuiCol_ButtonActive) : ImGui::GetColorU32(ImGuiCol_Border));

						// Get flashiness state from game
						float flashinessIntensity = 1.0f;
						bool flashinessIsDrawn = false;
						getFlashinessState(i, (rage::Mat34V*)&ctx.LightWorld, flashinessIntensity, flashinessIsDrawn);
						if (!flashinessIsDrawn)
							flashinessIntensity = 0.0f;

						// Draw it in UI
						ImU32 flashinessColor = // Also can use light color here but looks a bit weird
							IM_COL32(230, 230, 230, static_cast<u8>(flashinessIntensity * 255.0f));
						float lightPadding = ImGui::GetStyle().FramePadding.x * 2.5f;
						// Position on top right corner
						float lightRadius = frameHeight * 0.475f;
						ImVec2 lightSize(lightRadius, lightRadius);
						ImVec2 lightPos = selectableRect.GetTR();
						lightPos += ImVec2(-lightRadius * 0.5f, lightRadius * 0.5f);
						lightPos += ImVec2(-lightPadding, lightPadding);
						drawList->AddCircleFilled(lightPos, lightSize.x, flashinessColor);
					}
					ImGui::Unindent();
					ImGui::Dummy(tilePadding); // Add extra padding after flashiness tiles
				}

				editFadeDistance("Specular Fade", light->LightFadeDistance);

				SlGui::CategoryText("Time Options - When light is on");
				{
					ImGui::CheckboxFlags("Always", &light->TimeFlags, LIGHT_TIME_ALWAYS_MASK);
					ImGui::SameLine();
					ImGui::CheckboxFlags("Day (12:00 - 00:00)", &light->TimeFlags, LIGHT_TIME_DAY_MASK);
					ImGui::SameLine();
					ImGui::CheckboxFlags("Night (00:00 - 12:00)", &light->TimeFlags, LIGHT_TIME_NIGHT_MASK);

					if (SlGui::CollapsingHeader("Show All"))
					{
						ImGui::Indent();
						if (ImGui::BeginTable("LIGHT_FLAGS_TABLE", 2, ImGuiTableFlags_BordersInnerV))
						{
							ImGui::TableSetupColumn("Night", ImGuiTableColumnFlags_WidthFixed);
							ImGui::TableSetupColumn("Day", ImGuiTableColumnFlags_WidthFixed);

							auto timeFlagsCheckBox = [&](int hour)
								{
									ConstString timeName = ImGui::FormatTemp("%02i:00 - %02i:00", hour, hour + 1);
									ImGui::CheckboxFlags(timeName, &light->TimeFlags, 1 << hour);
								};

							for (int i = 0; i < 12; i++)
							{
								ImGui::TableNextRow();

								ImGui::TableNextColumn();
								timeFlagsCheckBox(i);		// Night
								ImGui::TableNextColumn();
								timeFlagsCheckBox(i + 12);	// Day
							}

							ImGui::EndTable();
						}
						ImGui::Unindent();
					}
				}

				SlGui::CategoryText("Cull Plane");
				ImGui::CheckboxFlags("Enable###ENABLE_CULL_PLANE", &light->Flags, LF_ENABLE_CULLING_PLANE);
				ImGui::SameLine();
				if (SlGui::ToggleButton("Edit Mode", m_EditingCullPlane))
				{
					// Froze selection while editing cull plane
					if (m_EditingCullPlane)
					{
						m_SelectionWasFreezed = m_SelectionFreezed;
						m_SelectionFreezed = true;
						SetCullPlaneFromLight(ctx);
					}
					else
					{
						m_SelectionFreezed = m_SelectionWasFreezed;
					}
				}
				ImGui::SameLine();
				ImGui::HelpMarker("Cull (clip) plane allows to completely block light. Usually they are used to prevent light emitting through a wall.");

				SlGui::CategoryText(ICON_AM_SHADOW" Shadows");
				ImGui::CheckboxFlags("Enable###ENABLE_SHADOWS", &light->Flags, LF_ENABLE_SHADOWS);
				bool shadowsEnabled = light->Flags & LF_ENABLE_SHADOWS;
				if (!shadowsEnabled) ImGui::BeginDisabled();
				{
					ImGui::DragU8("Blur", &light->ShadowBlur, 1, 0, 255);
					ImGui::DragFloat("Near Clip", &light->ShadowNearClip, 0.1f, 0, 300, "%.1f");
					editFadeDistance("Fade", light->ShadowFadeDistance);
				}
				if (!shadowsEnabled) ImGui::EndDisabled();

				SlGui::CategoryText(ICON_AM_CORONA" Corona");
				ImGui::DragFloat("Intensity###CORONA_INTENSITY", &light->CoronaIntensity, 0.05f, 0.0f, 100, "%.4f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
				ImGui::DragFloat("Size###CORONA_SIZE", &light->CoronaSize, 0.01, 0.0f, 10, "%.3f");
				ImGui::DragFloat("ZBias###CORONA_ZBIAS", &light->CoronaZBias, 1.0f, 0.0f, 100, "%.1f");

				ImGui::EndTabItem();
			}

			if (light->Type == LIGHT_TYPE_SPOT)
			{
				if (ImGui::BeginTabItem(ICON_AM_SPOT_LIGHT" Volume"))
				{
					ImGui::CheckboxFlags("Draw volume always", &light->Flags, LF_ENABLE_VOLUME);
					ImGui::SameLine();
					ImGui::HelpMarker("Enables volume ignoring timecycle. By default volume is only visible in foggy weather.");

					ImGui::DragFloat("Intensity###VOLUME_INTENSITY", &light->VolumeIntensity, 0.001f, 0.0f, 1.0f, "%.4f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
					ImGui::DragFloat("Size Scale", &light->VolumeSizeScale, 0.01f, 0.0f, 1.0f, "%.2f");

					bool snapInnerAngle = rage::AlmostEquals(light->ConeOuterAngle, light->ConeInnerAngle);
					if (ImGui::DragFloat("Cone Outer Angle", &light->ConeOuterAngle, 1, SPOT_LIGHT_MIN_CONE_ANGLE, SPOT_LIGHT_MAX_CONE_ANGLE, "%.1f"))
					{
						// Make sure that inner angle is not bigger than outer after editing
						if (light->ConeInnerAngle > light->ConeOuterAngle)
							light->ConeInnerAngle = light->ConeOuterAngle;

						if (snapInnerAngle)
							light->ConeInnerAngle = light->ConeOuterAngle;
					}
					ImGui::DragFloat("Cone Inner Angle", &light->ConeInnerAngle, 1, SPOT_LIGHT_MIN_CONE_ANGLE, light->ConeOuterAngle, "%.1f");

					editFadeDistance("Fade", light->VolumetricFadeDistance);

					ImGui::CheckboxFlags("Outer Color", &light->Flags, LF_ENABLE_VOLUME_OUTER_COLOR);
					if (light->Flags & LF_ENABLE_VOLUME_OUTER_COLOR)
					{
						ImGui::Indent();
						graphics::ColorU32 volumeColor(light->VolumeOuterColorR, light->VolumeOuterColorG, light->VolumeOuterColorB);
						rage::Vector4 outerColor = volumeColor.ToVec4();
						ImGui::DragFloat("Exponent###VOLUME_OUTER_EXPONENT", &light->VolumeOuterExponent, 1.0f, 0, 500, "%.2f", ImGuiSliderFlags_Logarithmic);
						ImGui::DragFloat("Intensity###VOLUME_OUTER_INTENSITY", &light->VolumeOuterIntensity, 0.005f, 0.0f, 1.0f, "%.3f");
						if (ImGui::ColorPicker3("Color###VOLUME_OUTER_COLOR", (float*)&outerColor))
						{
							volumeColor = graphics::ColorU32::FromVec4(outerColor);
							light->VolumeOuterColorR = volumeColor.R;
							light->VolumeOuterColorG = volumeColor.G;
							light->VolumeOuterColorB = volumeColor.B;
						}
						ImGui::Unindent();
					}

					ImGui::EndTabItem();
				}
			}

			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

void rageam::integration::LightEditor::DrawLightTransformGizmo(const LightDrawContext& ctx) const
{
	if (m_GizmoMode != GIZMO_None)
	{
		rage::Mat44V editDelta;
		rage::Mat44V editedLightWorld = ctx.LightWorld;
		if (Im3D::Gizmo(editedLightWorld, editDelta, ImGuizmo::OPERATION(GetImGuizmoOperation())))
		{
			rage::Mat44V local = editedLightWorld * ctx.LightBind;
			ctx.Light->SetMatrix(local);
		}
	}
}

void rageam::integration::LightEditor::DrawCustomGizmos(const LightDrawContext& ctx)
{
	if (ctx.Light->Type == LIGHT_TYPE_POINT)
	{
		DrawPointLightFalloffGizmo(ctx);
	}

	if (m_EditingCullPlane)
	{
		DrawCullPlaneEditGizmo(ctx);
	}
}

void rageam::integration::LightEditor::SelectGizmoMode()
{
	if (m_SelectedLight == -1)
		return;

	// Handle gizmo mode toggling
	// G - Translation
	// R - Rotation
	int newModeMask = 0;

	if (ImGui::IsKeyPressed(ImGuiKey_G, false)) newModeMask = GIZMO_Translate;
	if (ImGui::IsKeyPressed(ImGuiKey_R, false)) newModeMask = GIZMO_Rotate;

	if (newModeMask != 0) // Some Mode was toggled
	{
		m_GizmoMode &= newModeMask; // Clear other modes
		m_GizmoMode ^= newModeMask; // Toggle pressed mode
	}
}

rageam::integration::LightEditor::LightEditor(ModelSceneContext* sceneContext)
{
	m_SceneContext = sceneContext;
}

void rageam::integration::LightEditor::Render()
{
	gtaDrawable* drawable = m_SceneContext->Drawable.Get();

	u16 lightCount = drawable->GetLightCount();
	if (lightCount == 0)
		return;

	SelectGizmoMode();

	// Prepare context for drawing lights
	LightDrawContext drawContext;
	CViewport::GetCamera(&drawContext.CamFront, &drawContext.CamRight, &drawContext.CamUp);
	CViewport::GetWorldMouseRay(drawContext.WorldMouseRay.Pos, drawContext.WorldMouseRay.Dir);

	auto hoveredLightDistance = rage::S_MAX;
	s32 hoveredLightIndex = -1;
	for (u16 i = 0; i < drawable->GetLightCount(); i++)
	{
		CLightAttr* light = drawable->GetLight(i);

		bool isSelected = m_SelectedLight == i;
		bool isHovered = m_HoveredLight == i;

		ComputeLightMatrices(i,
			drawContext.LightWorld,
			drawContext.LightBind,
			drawContext.LightLocal,
			drawContext.LightBoneWorld);

		drawContext.IsSelected = m_SelectedLight == i;
		drawContext.Light = light;
		drawContext.LightIndex = i;
		drawContext.PrimaryColor = GetOutlinerColor(isSelected, isHovered, true);
		drawContext.SecondaryColor = GetOutlinerColor(isSelected, isHovered, false);
		drawContext.CulledColor = graphics::ColorU32(140, 140, 140, 140);

		// Render light outlines and find the closest one to the camera
		graphics::ShapeHit shapeHit = DrawOutliner(drawContext);
		if (shapeHit.DidHit && shapeHit.Distance < hoveredLightDistance)
		{
			hoveredLightDistance = shapeHit.Distance;
			hoveredLightIndex = i;
		}

		if (isSelected)
		{
			DrawCustomGizmos(drawContext);
			DrawLightUI(drawContext);

			// Transform gizmo
			if (!m_UsingPointFalloffGizmo && !m_EditingCullPlane)
			{
				DrawLightTransformGizmo(drawContext);
			}
		}
	}

	// Popup blocks whole screen so we block hovering too
	if (!ImGui::IsAnyPopUpOpen())
	{
		m_HoveredLight = hoveredLightIndex;
	}

	if (!m_SelectionFreezed)
	{
		// Select light if hovered & left clicked
		bool canUseGizmo =
			!ImGuizmo::IsOver() &&
			!ImGui::IsAnyWindowHovered() &&
			!ImGui::IsAnyPopUpOpen();

		if (canUseGizmo && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			m_SelectedLight = m_HoveredLight;

			// Reset selection
			if (m_HoveredLight == -1)
			{
				m_UsingPointFalloffGizmo = false;
			}
		}
	}
}

void rageam::integration::LightEditor::SelectLight(s32 index)
{
	if (!m_SelectionFreezed)
		m_SelectedLight = index;
}

#endif
