#ifdef AM_INTEGRATED

#include "lighteditor.h"
#include "modelscene.h"
#include "game/viewport.h"
#include "rage/math/math.h"
#include "am/graphics/shapetest.h"
#include "am/integration/im3d.h"
#include "am/ui/slwidgets.h"
#include "am/ui/font_icons/icons_am.h"
#include "am/integration/gamedrawlists.h"
#include "am/gizmo/gizmotranslation.h"
#include "am/integration/script/core.h"

#include <stb_sprintf.h>

GIZMO_INITIALIZE_INFO(rageam::integration::LightGizmo_Point, "Point Light");
GIZMO_INITIALIZE_INFO(rageam::integration::LightGizmo_Spot, "Spot Light");
GIZMO_INITIALIZE_INFO(rageam::integration::LightGizmo_Capsule, "Capsule Light");

void rageam::integration::LightGizmo::OnStart(const gizmo::GizmoContext& context)
{
	m_StartLightState = *GetLightContext(context).Light;
}

void rageam::integration::LightGizmo::OnCancel(const gizmo::GizmoContext& context)
{
	*GetLightContext(context).Light = m_StartLightState;
}

void rageam::integration::LightGizmo_Point::Draw(const gizmo::GizmoContext& context)
{
	LightDrawContext& lightContext = GetLightContext(context);
	float radius = lightContext.Light->Falloff;
	Mat44V transform = Mat44V::Scale(radius) * lightContext.LightWorld;

	u32 color = 
		context.IsSelected ? COLOR_SELECTED :
		context.SelectedHandle == PointLight_Sphere ? COLOR_HOVERED : COLOR_DEFAULT;

	gizmo::DrawCircleCameraCull(context.Camera, transform, rage::VEC_RIGHT, rage::VEC_UP, color);
	gizmo::DrawCircleCameraCull(context.Camera, transform, rage::VEC_FRONT, rage::VEC_UP, color);
	gizmo::DrawCircleCameraCull(context.Camera, transform, rage::VEC_UP, rage::VEC_RIGHT, color);

	gizmo::DrawCircle(transform, context.Camera.Front, context.Camera.Right, radius * 1.05f, 
		context.HoveredHandle == PointLight_Edge ? gizmo::GIZMO_COLOR_GRAY : gizmo::GIZMO_COLOR_GRAY2);
}

void rageam::integration::LightGizmo_Point::Manipulate(const gizmo::GizmoContext& context, Vec3V& outOffset, Vec3V& outScale, QuatV& outRotation)
{
	LightDrawContext& lightContext = GetLightContext(context);
	Vec3V pos = lightContext.LightWorld.Pos;
	if (context.SelectedHandle == PointLight_Edge)
	{
		Vec3V dragFrom, dragTo;
		ManipulateOnPlane(
			context.StartMouseRay, context.MouseRay, Mat44V::Identity(), context.Camera, pos, context.Camera.Front, dragFrom, dragTo);
		float newRadius = dragTo.DistanceTo(pos).Get();
		lightContext.Light->Falloff = newRadius;

		Im3D::TextBg(pos, "Falloff: %.02f", newRadius);
	}
}

rageam::List<rageam::gizmo::HitResult> rageam::integration::LightGizmo_Point::HitTest(const gizmo::GizmoContext& context)
{
	LightDrawContext& lightContext = GetLightContext(context);
	Vec3V pos = lightContext.LightWorld.Pos;
	float radius = lightContext.Light->Falloff;
	ScalarV dist;
	if (TestCircle(context.MouseRay, Mat44V::Translation(pos), context.Camera.Front, radius, gizmo::ROT_CIRCLE_BORDER_WIDTH, dist))
		return { gizmo::HitResult(dist, PointLight_Edge) };
	if (graphics::ShapeTest::RayIntersectsSphere(context.MouseRay.Pos, context.MouseRay.Dir, pos, radius, &dist))
		return { gizmo::HitResult(dist, PointLight_Sphere) };
	return {};
}

void rageam::integration::LightGizmo_Spot::Draw(const gizmo::GizmoContext& context)
{
	LightDrawContext& lightContext = GetLightContext(context);
	CLightAttr*		  light = lightContext.Light;
	Mat44V&			  lightWorld = lightContext.LightWorld;

	bool selected = context.IsHandleSelectedOrHovered(SpotLight_HemisphereCone);
	u32 color = selected ? COLOR_HOVERED : COLOR_DEFAULT;

	// Spot light shape is a little bit different from regular cone light in blender,
	// the difference is that instead of straight bottom there's a half sphere (or arc if looking from side)
	// Side view diagram available here: https://i.imgur.com/cBYN3gJ.png

	// Maximum extent of light in light direction ( world down is aligned with light direction in light world matrix )
	const rage::Vec3V coneLine = rage::VEC_DOWN * light->Falloff;

	// Outline must always face camera
	rage::Vec3V frontNormal = lightContext.CamRight.Cross(rage::VEC_UP);

	// Rotates 2 lines with falloff length to -coneAngle and +coneAngle
	auto getLeftRight = [&](float theta, rage::Vec3V& coneLeft, rage::Vec3V& coneRight)
		{
			coneLeft = coneLine.Rotate(frontNormal, -theta);
			coneRight = coneLine.Rotate(frontNormal, theta);
		};

	auto& dl = gizmo::GetDrawList();
	dl.SetTransform(lightWorld);

	// Outer & Inner edges
	float outerAngle = rage::DegToRad(light->ConeOuterAngle);
	float innerAngle = rage::DegToRad(light->ConeInnerAngle);
	rage::Vec3V coneOuterLeft, coneOuterRight, coneInnerLeft, coneInnerRight;
	getLeftRight(outerAngle, coneOuterLeft, coneOuterRight);
	getLeftRight(innerAngle, coneInnerLeft, coneInnerRight);
	dl.DrawLine(rage::VEC_ORIGIN, coneOuterLeft, color);
	dl.DrawLine(rage::VEC_ORIGIN, coneOuterRight, color);
	if (light->ConeInnerAngle != light->ConeOuterAngle)
	{
		dl.DrawLine(rage::VEC_ORIGIN, coneInnerLeft, gizmo::GIZMO_COLOR_CYAN);
		dl.DrawLine(rage::VEC_ORIGIN, coneInnerRight, gizmo::GIZMO_COLOR_CYAN);
	}

	// Half sphere top circle
	rage::Vec3V circleCenter = (coneOuterLeft + coneOuterRight) * rage::S_HALF;
	rage::ScalarV circleRadius = circleCenter.DistanceTo(coneOuterLeft);
	dl.DrawCircle(circleCenter, rage::VEC_UP, lightContext.CamRight, circleRadius, color);

	// Draw bottom arc for outer angle
	auto drawArc = [&](const rage::Vec3V& normal)
		{
			constexpr int arcSegmentCount = 16;
			float arcStep = outerAngle * 2 / arcSegmentCount;
			rage::Vec3V prevSegmentPos;
			for (int i = 0; i < arcSegmentCount + 1; i++)
			{
				// From -angle to +angle
				float arcTheta = -outerAngle + static_cast<float>(i) * arcStep;
				rage::Vec3V segmentPos = coneLine.Rotate(normal, arcTheta);
				if (i != 0)
					dl.DrawLine(prevSegmentPos, segmentPos, color);
				prevSegmentPos = segmentPos;
			}
		};

	// Draw sphere arcs
	// World aligned
	drawArc(rage::VEC_FORWARD);
	drawArc(rage::VEC_RIGHT);
	// drawArc(frontNormal);

	// Draw target manipulator
	Vec3V target = GetTargetPointLocal(lightContext);
	dl.DrawLine(rage::VEC_ORIGIN, target, gizmo::GIZMO_COLOR_YELLOW2);
	dl.DrawAlignedBox(target, TARGET_BOX_EXTENT,
		context.IsHandleSelectedOrHovered(SpotLight_Target) ? gizmo::GIZMO_COLOR_WHITE : gizmo::GIZMO_COLOR_YELLOW2);

	dl.ResetTransform();
}

void rageam::integration::LightGizmo_Spot::Manipulate(const gizmo::GizmoContext& context, Vec3V& outOffset, Vec3V& outScale, QuatV& outRotation)
{

}

rageam::List<rageam::gizmo::HitResult> rageam::integration::LightGizmo_Spot::HitTest(const gizmo::GizmoContext& context)
{
	// Spot light diagram: https://imgur.com/ecfPRRZ

	const gizmo::Ray& ray = context.MouseRay;
	LightDrawContext& lightContext = GetLightContext(context);
	CLightAttr*       light = lightContext.Light;
	Mat44V&           lightWorld = lightContext.LightWorld;

	List<gizmo::HitResult> hitResults;
	ScalarV dist;

	// Test target manipulator, higher priority
	Vec3V target = lightWorld.Pos + GetTargetPointWorld(lightContext);
	if (graphics::ShapeTest::RayIntersectsCircle(ray.Pos, ray.Dir, target, context.Camera.Front, TARGET_BOX_EXTENT_RADIUS, &dist))
	{
		hitResults.Add(gizmo::HitResult(dist, SpotLight_Target));
		return hitResults;
	}

	// Test whole sphere furst
	if (!context.IsSelected && // Don't hit test if light is selected to allow manipulating direction (Target)
		graphics::ShapeTest::RayIntersectsSphere(ray.Pos, ray.Dir, lightWorld.Pos, light->Falloff, &dist))
	{
		// Now we have to find out if ray landed on on hemisphere or cone
		Vec3V point = ray.Pos + ray.Dir * dist - lightWorld.Pos;
		float coneAngleRad = rage::DegToRad(light->ConeOuterAngle);

		// Distance from top of the cone to upper cut of the hemisphere
		float coneHeight = light->Falloff * cosf(coneAngleRad);
		float coneBaseRadius = light->Falloff * sinf(coneAngleRad);

		ScalarV projectedPointDistanceToLightPos = point.Dot(light->Direction); // Project point on light normal
		if (projectedPointDistanceToLightPos <= coneHeight)
		{
			// Point lies somewhere on the cone, not on the hemisphere
			Vec3V coneBase = Vec3V(lightWorld.Pos) + Vec3V(light->Direction) * coneHeight;
			Vec3V coneToTip = -light->Direction;
			if (graphics::ShapeTest::RayIntersectsCone(
				ray.Pos, ray.Dir, coneBase, coneToTip, coneBaseRadius, coneHeight, &dist))
			{
				hitResults.Add(gizmo::HitResult(dist, SpotLight_HemisphereCone));
			}
		}
		else
		{
			// Point lies on the hemisphere
			hitResults.Add(gizmo::HitResult(dist, SpotLight_HemisphereCone));
		}
	}

	return hitResults;
}

rageam::Vec3V rageam::integration::LightGizmo_Spot::GetTargetPointLocal(const LightDrawContext& lightContext)
{
	return rage::VEC_DOWN * (lightContext.Light->Falloff + TARGET_BOX_OFFSET);
}

rageam::Vec3V rageam::integration::LightGizmo_Spot::GetTargetPointWorld(const LightDrawContext& lightContext)
{
	return lightContext.Light->Direction * (lightContext.Light->Falloff + TARGET_BOX_OFFSET);
}

void rageam::integration::LightGizmo_Spot::SetTargetPointLocal(const LightDrawContext& lightContext, const Vec3V& targetPoint)
{
	lightContext.Light->SetSpotTarget(targetPoint - Vec3V(lightContext.Light->Direction) * TARGET_BOX_OFFSET);
}

void rageam::integration::LightGizmo_Capsule::Draw(const gizmo::GizmoContext& context)
{
	LightDrawContext& lightContext = GetLightContext(context);
	CLightAttr*		  light = lightContext.Light;

	// Capsule light diagram: https://i.imgur.com/IdXEw2a.png

	DrawList& dl = gizmo::GetDrawList();
	dl.SetTransform(lightContext.LightWorld);

	bool selected = context.IsHandleSelectedOrHovered(CapsuleLight_Capsule);
	u32  color = selected ? COLOR_HOVERED : COLOR_DEFAULT;

	rage::ScalarV halfExtent = light->Extent.X * 0.5f;
	rage::Vec3V extentFrom = rage::VEC_UP * halfExtent;
	rage::Vec3V extentTo = rage::VEC_DOWN * halfExtent;

	float radius = light->Falloff;

	dl.DrawLine(extentFrom, extentTo, gizmo::GIZMO_COLOR_CYAN);

	// Half sphere tops
	dl.DrawCircle(extentTo, rage::VEC_UP, rage::VEC_RIGHT, radius, color);
	dl.DrawCircle(extentFrom, rage::VEC_UP, rage::VEC_RIGHT, radius, color);

	// Draw two half spheres (arcs)
	dl.DrawCircle(extentTo, rage::VEC_FRONT, rage::VEC_RIGHT, radius, color, 0, rage::PI);
	dl.DrawCircle(extentTo, -rage::VEC_RIGHT, rage::VEC_FRONT, radius, color, 0, rage::PI);
	dl.DrawCircle(extentFrom, -rage::VEC_FRONT, rage::VEC_RIGHT, radius, color, 0, rage::PI);
	dl.DrawCircle(extentFrom, rage::VEC_RIGHT, rage::VEC_FRONT, radius, color, 0, rage::PI);

	// Draw lines connecting half spheres (arcs)
	dl.DrawLine(extentTo + rage::VEC_FRONT * radius, extentFrom + rage::VEC_FRONT * radius, color);
	dl.DrawLine(extentTo + rage::VEC_RIGHT * radius, extentFrom + rage::VEC_RIGHT * radius, color);
	dl.DrawLine(extentTo - rage::VEC_FRONT * radius, extentFrom - rage::VEC_FRONT * radius, color);
	dl.DrawLine(extentTo - rage::VEC_RIGHT * radius, extentFrom - rage::VEC_RIGHT * radius, color);

	// ExtentFrom / ExtentTo manipulators
	dl.DrawAlignedBox(
		extentFrom, TARGET_BOX_EXTENT, context.IsHandleSelectedOrHovered(CapsuleLight_Extent1) ? gizmo::GIZMO_COLOR_WHITE : gizmo::GIZMO_COLOR_YELLOW2);
	dl.DrawAlignedBox(
		extentTo, TARGET_BOX_EXTENT, context.IsHandleSelectedOrHovered(CapsuleLight_Extent2) ? gizmo::GIZMO_COLOR_WHITE : gizmo::GIZMO_COLOR_YELLOW2);

	dl.ResetTransform();
}

void rageam::integration::LightGizmo_Capsule::Manipulate(const gizmo::GizmoContext& context, Vec3V& outOffset, Vec3V& outScale, QuatV& outRotation)
{

}

rageam::List<rageam::gizmo::HitResult> rageam::integration::LightGizmo_Capsule::HitTest(const gizmo::GizmoContext& context)
{
	const gizmo::Ray& ray = context.MouseRay;
	LightDrawContext& lightContext = GetLightContext(context);
	CLightAttr*		  light = lightContext.Light;
	Mat44V&           lightWorld = lightContext.LightWorld;

	ScalarV halfExtent = light->Extent.X * 0.5f;
	Vec3V   extentTo   = Vec3V(lightWorld.Pos) + Vec3V(light->Direction) * halfExtent;
	Vec3V   extentFrom = Vec3V(lightWorld.Pos) - Vec3V(light->Direction) * halfExtent;
	float   radius = light->Falloff;

	ScalarV dist;

	// Extent manipulator, higher priority
	if (graphics::ShapeTest::RayIntersectsCircle(ray.Pos, ray.Dir, extentFrom, context.Camera.Front, TARGET_BOX_EXTENT_RADIUS, &dist))
		return { gizmo::HitResult(dist, CapsuleLight_Extent1) };
	if (graphics::ShapeTest::RayIntersectsCircle(ray.Pos, ray.Dir, extentTo, context.Camera.Front, TARGET_BOX_EXTENT_RADIUS, &dist))
		return { gizmo::HitResult(dist, CapsuleLight_Extent2) };

	// Sphere capsule
	if (graphics::ShapeTest::RayIntersectsCapsule(ray.Pos, ray.Dir, extentFrom, extentTo, radius, &dist))
		return { gizmo::HitResult(dist, CapsuleLight_Capsule) };

	return {};
}

void rageam::integration::LightEditor::SetCullPlaneFromLight(const LightDrawContext& ctx)
{
	// See diagram: https://i.imgur.com/SyBQ591.png
	rage::Vec3V planeNormal = ctx.Light->CullingPlaneNormal;
	rage::ScalarV planeOffset = ctx.Light->CullingPlaneOffset;
	rage::Vec3V cullingNormal = -planeNormal; // Everything behind the plane is culled
	rage::Vec3V planePos = cullingNormal * planeOffset;
	m_CullPlane = rage::Mat44V::FromNormalPos(planePos, planeNormal);
}

void rageam::integration::LightEditor::DrawCullPlaneEditGizmo(const LightDrawContext& ctx)
{
	DrawList& dl = gizmo::GetDrawList();

	// See diagram: https://i.imgur.com/SyBQ591.png

	rage::Vec4V lightPos(ctx.Light->Position, 0.0f);

	rage::Mat44V planeWorld = m_CullPlane * ctx.LightBoneWorld;
	planeWorld.Pos += lightPos; // Cull plane ignores light orientation, use only position

	dl.DrawQuad(planeWorld.Pos, planeWorld.Front, planeWorld.Right, rage::S_TWO, rage::S_ONE, graphics::COLOR_WHITE);

	// Draw line that indicates culling area
	dl.DrawLine(planeWorld.Pos, planeWorld.Pos - planeWorld.Front, graphics::COLOR_RED);

	// Debug cull info
	//Im3D::TextBg(planeWorld.Pos, "<Cull Plane> Normal: %.2f %.2f %.2f Offset: %.2f",
	//	ctx.Light->CullingPlaneNormal.X, ctx.Light->CullingPlaneNormal.Y, ctx.Light->CullingPlaneNormal.Z,
	//	ctx.Light->CullingPlaneOffset);

	if (GIZMO->ManipulateDefault("Cull Plane", planeWorld))
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

void rageam::integration::LightEditor::DrawLightUI(const LightDrawContext& ctx)
{
	CLightAttr* light = ctx.Light;

	ConstString windowName = ImGui::FormatTemp(ICON_AM_LIGHT_BULB" Light Editor (%s #%i)###LIGHT_EDITOR_WINDOW",
		GetLightTypeName(light->Type), m_SelectedLight);

	if (ImGui::Begin(windowName))
	{
		if (m_EditingCullPlane) ImGui::BeginDisabled();
		ImGui::Checkbox("Freeze Selection", &m_SelectionFreezed);
		if (m_EditingCullPlane) ImGui::EndDisabled();

#if APP_BUILD_2699_16_RELEASE_NO_OPT
		static bool* s_DisableArtWorldLights = gmAddress::Scan(
			"0F B6 05 ?? ?? ?? ?? 85 C0 0F 84 B4 00 00 00 0F B6 05 ?? ?? ?? ?? 85 C0 75 6C", "Lights::AddSceneLight+0x16")
			.GetRef(3).To<bool*>();
#else
		static bool* s_DisableArtWorldLights = gmAddress::Scan("44 38 25 ?? ?? ?? ?? 45 8A F8", "Lights::AddSceneLight+0x2E")
			.GetRef(3).To<bool*>();
#endif
		ImGui::Checkbox("Disable world lights", s_DisableArtWorldLights);
		ImGui::SameLine();
		ImGui::HelpMarker("To see current light, flag 'Ignore EMP' must be enabled.");

		ImGui::Dummy(ImVec2(0, GImGui->FontSize * 0.25f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0, GImGui->FontSize * 0.25f));

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
					// ImGui::Indent();
					ImGui::DragU8(ImGui::FormatTemp("Distance###%s", name), &fadeDistance, 1, 1, 255);
					// ImGui::Unindent();
				}
			};

		if (SlGui::Category(ICON_AM_OBJECT_DATA" Shape And Color"))
		{
			// Type picker
			static ConstString s_LightNames[] = { "Point", "Spot", "Capsule", "Directional", "AO Volume" };
			static eLightType s_LightTypes[] = { LIGHT_TYPE_POINT, LIGHT_TYPE_SPOT, LIGHT_TYPE_CAPSULE, LIGHT_TYPE_DIRECTIONAL, LIGHT_TYPE_AO_VOLUME };
			static int s_LightTypeToIndex[] =
			{
				// Light types are 1 2 4 8 16
				-1, 0, 1, -1, 2, -1, -1, -1, 3, -1, -1, -1, -1, -1, -1, -1, 4
			};
			int lightTypeIndex = s_LightTypeToIndex[light->Type];
			if (ImGui::Combo("Type", &lightTypeIndex, s_LightNames, 3 /* 5 - Last 2 types seems to be not supported by CLightAttr */))
				light->Type = s_LightTypes[lightTypeIndex];

			graphics::ColorU32 color(light->ColorR, light->ColorG, light->ColorB);
			rage::Vector4 colorF = color.ToVec4();
			// if (ImGui::ColorPicker3("Color", (float*)&colorF))
			if (ImGui::ColorEdit3("Color", (float*)&colorF))
			{
				color = graphics::ColorU32::FromVec4(colorF);
				light->ColorR = color.R;
				light->ColorG = color.G;
				light->ColorB = color.B;
			}

			ImGui::DragFloat("Falloff", &light->Falloff, 0.1f, 0, 25, "%.1f");
			ImGui::DragFloat("Exponent", &light->FallofExponent, 1.0f, 0, 500, "%.2f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
			ImGui::DragFloat("Intensity###LIGHT_INTENSITY", &light->Intensity, 1.0f, 0.0f, 1000, "%.1f");
			editFadeDistance("Fade", light->LightFadeDistance);
			editFadeDistance("Specular Fade", light->SpecularFadeDistance);

			SlGui::CategoryText(ICON_AM_META_CAPSULE" Capsule");
			ImGui::DragFloat("Length", &light->Extent.X, 0.1f, 0.0f, 25.0f, " %.1f");

			SlGui::CategoryText(ICON_AM_LIGHT_SPOT" Spot");
			bool snapInnerAngle = rage::AlmostEquals(light->ConeOuterAngle, light->ConeInnerAngle);
			if (ImGui::DragFloat("Outer Angle", &light->ConeOuterAngle, 1, SPOT_LIGHT_MIN_CONE_ANGLE, SPOT_LIGHT_MAX_CONE_ANGLE, "%.1f"))
			{
				// Make sure that inner angle is not bigger than outer after editing
				if (light->ConeInnerAngle > light->ConeOuterAngle)
					light->ConeInnerAngle = light->ConeOuterAngle;

				if (snapInnerAngle)
					light->ConeInnerAngle = light->ConeOuterAngle;
			}
			ImGui::DragFloat("Inner Angle", &light->ConeInnerAngle, 1, SPOT_LIGHT_MIN_CONE_ANGLE, light->ConeOuterAngle, "%.1f");
		}

		if (SlGui::Category(ICON_AM_MODIFIER" Flags", false))
		{
			ImGui::HelpMarker(
				"Flags like 'USE_CULL_PLANE' been moved to tabs with related features (e.g. cull plane edit tab).", 
				"Why not all flags are shown");

			// TODO: LIGHTFLAG_TEXTURE_PROJECTION

			SlGui::BeginCompact();
			if (ImGui::BeginTable("SHADER_CATEGORIES_TABLE", 2))
			{
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Interior Only", &light->Flags, LIGHTFLAG_INTERIOR_ONLY);
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Exterior Only", &light->Flags, LIGHTFLAG_EXTERIOR_ONLY);
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Ignore in cutscene", &light->Flags, LIGHTFLAG_DONT_USE_IN_CUTSCENE);
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Show underground", &light->Flags, LIGHTFLAG_RENDER_UNDERGROUND);
				ImGui::ToolTip("LIGHTFLAG_VEHICLE - Draw light under map (in tunnels)");
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Ignore EMP", &light->Flags, LIGHTFLAG_IGNORE_ARTIFICIAL_LIGHT_STATE);
				ImGui::ToolTip("Light will ignore SET_ARTIFICIAL_LIGHTS_STATE(FALSE). Artificial light state is toggled by the game to simulate EMP effect.");
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Ignore Time", &light->Flags, LIGHTFLAG_IGNORE_TIME);
				ImGui::ToolTip("LIGHTFLAG_CALC_FROM_SUN - Don't use light intensity based on game time (brighter at night, dimmer at day)");
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Enable buzzing", &light->Flags, LIGHTFLAG_ENABLE_BUZZING);
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Force buzzing", &light->Flags, LIGHTFLAG_FORCE_BUZZING);
				ImGui::ToolTip("Plays electrical buzz sound near the light, note that volume is pretty low");
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("No Specular", &light->Flags, LIGHTFLAG_NO_SPECULAR);
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Interior and exterior", &light->Flags, LIGHTFLAG_BOTH_INTERIOR_AND_EXTERIOR);
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Not in reflection", &light->Flags, LIGHTFLAG_NOT_IN_REFLECTION);
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Only in reflection", &light->Flags, LIGHTFLAG_ONLY_IN_REFLECTION);
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Don't light alpha", &light->Flags, LIGHTFLAG_DONT_LIGHT_ALPHA);
				ImGui::ToolTip("Light won't affect transparent surfaces (such as glass)");
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Cutscene", &light->Flags, LIGHTFLAG_CUTSCENE);
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Use vehicle twin", &light->Flags, LIGHTFLAG_USE_VEHICLE_TWIN);
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Far LOD", &light->Flags, LIGHTFLAG_FAR_LOD_LIGHT);
				ImGui::TableNextColumn(); ImGui::CheckboxFlags("Medium LOD", &light->Flags, LIGHTFLAG_FORCE_MEDIUM_LOD_LIGHT);

				ImGui::EndTable();
			}
			SlGui::EndCompact();
		}

		if (SlGui::Category(ICON_AM_VOLUME" Volume", false))
		{
			ImGui::TextDisabled(ICON_AM_ERROR" Volume is not supported on capsule lights");

			ImGui::CheckboxFlags("Draw volume always", &light->Flags, LIGHTFLAG_DRAW_VOLUME);
			ImGui::SameLine();
			ImGui::HelpMarker("Enables volume ignoring timecycle. By default volume is only visible in foggy weather.");

			ImGui::CheckboxFlags("Draw only Volume", &light->Flags, LIGHTFLAG_DELAY_RENDER);

			ImGui::DragFloat("Intensity###VOLUME_INTENSITY", &light->VolumeIntensity, 0.001f, 0.0f, 1.0f, "%.4f",
				ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
			ImGui::DragFloat("Size Scale", &light->VolumeSizeScale, 0.01f, 0.0f, 1.0f, "%.2f");

			editFadeDistance("Fade", light->VolumetricFadeDistance);

			ImGui::CheckboxFlags("Outer Color", &light->Flags, LIGHTFLAG_USE_VOLUME_OUTER_COLOR);
			if (light->Flags & LIGHTFLAG_USE_VOLUME_OUTER_COLOR)
			{
				ImGui::Indent();
				graphics::ColorU32 volumeColor(light->VolumeOuterColorR, light->VolumeOuterColorG, light->VolumeOuterColorB);
				rage::Vector4 outerColor = volumeColor.ToVec4();
				ImGui::DragFloat("Exponent###VOLUME_OUTER_EXPONENT", &light->VolumeOuterExponent, 1.0f, 0, 500, "%.2f", ImGuiSliderFlags_Logarithmic);
				ImGui::DragFloat("Intensity###VOLUME_OUTER_INTENSITY", &light->VolumeOuterIntensity, 0.005f, 0.0f, 1.0f, "%.3f");
				//if (ImGui::ColorPicker3("Color###VOLUME_OUTER_COLOR", (float*)&outerColor))
				if (ImGui::ColorEdit3("Color###VOLUME_OUTER_COLOR", (float*)&outerColor))
				{
					volumeColor = graphics::ColorU32::FromVec4(outerColor);
					light->VolumeOuterColorR = volumeColor.R;
					light->VolumeOuterColorG = volumeColor.G;
					light->VolumeOuterColorB = volumeColor.B;
				}
				ImGui::Unindent();
			}
		}

		if (SlGui::Category(ICON_AM_SHARP_CURVE" Flashiness / Blinking", false))
		{
			ImVec2 tilePadding = ImVec2(0, ImGui::GetFrameHeight() * 0.25f);

			ImGui::HelpMarker(
				"#0 is the default 'always on' state.\n"
				"#11, #12, #13 use XY light position as the seed for randomizing.\n"
				"#14 is synced with #3 unless there is a radio audio emitter nearby.\n", "Notes");
			{
				ConstString flashinessName = LightFlashinessName[light->Flashiness];
				ConstString flashinessDesc = LightFlashinessDesc[light->Flashiness];
				ImGui::HelpMarker(flashinessDesc, ImGui::FormatTemp("Selected: %s", flashinessName));
			}

			// Additional simple slider with buttons
			ImGui::SliderU8("###FLASHINESS", &light->Flashiness, 0, FL_COUNT - 1, "%u");
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
				.ToFunc<void(u8 flashiness, rage::Mat34V* transform, float& outIntensity, bool& outIsDrawn)>();

			ImGui::Dummy(tilePadding); // Add extra padding before flashiness tiles
			ImGui::Indent();

			for (int i = 0; i < FL_COUNT; i++)
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
				{
					ConstString flashinessName = LightFlashinessName[i];
					ConstString flashinessDesc = LightFlashinessDesc[i];
					ImGui::ToolTip(ImGui::FormatTemp("%s - %s", flashinessName, flashinessDesc));
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

		if (SlGui::Category(ICON_AM_TIME" Time Options", false))
		{
			ImGui::CheckboxFlags("Always", &light->TimeFlags, LIGHT_TIME_ALWAYS_MASK);
			ImGui::CheckboxFlags("Night (20:00 - 06:00)", &light->TimeFlags, LIGHT_TIME_NIGHT_MASK);
			ImGui::CheckboxFlags("Day (06:00 - 20:00)", &light->TimeFlags, LIGHT_TIME_DAY_MASK);
			ImGui::CheckboxFlags("First Half (00:00 - 12:00)", &light->TimeFlags, LIGHT_TIME_FIRST_HALF_MASK);
			ImGui::CheckboxFlags("Second Half (12:00 - 00:00)", &light->TimeFlags, LIGHT_TIME_SECOND_HALF_MASK);

			if (SlGui::CollapsingHeader("Show All"))
			{
				ImGui::Indent();
				SlGui::BeginCompact();
				if (ImGui::BeginTable("LIGHT_FLAGS_TABLE", 2, 0/*ImGuiTableFlags_BordersInnerV*/))
				{
					//ImGui::TableSetupColumn("Night", ImGuiTableColumnFlags_WidthFixed);
					//ImGui::TableSetupColumn("Day", ImGuiTableColumnFlags_WidthFixed);

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
					SlGui::EndCompact();
				}
				ImGui::Unindent();
			}
		}

		if (SlGui::Category(ICON_AM_ORIENTATION_NORMAL" Cull Plane", false))
		{
			ImGui::CheckboxFlags("Enable###ENABLE_CULL_PLANE", &light->Flags, LIGHTFLAG_USE_CULL_PLANE);
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
		}

		if (SlGui::Category(ICON_AM_SHADING_RENDERED" Shadows", false))
		{
			ImGui::CheckboxFlags("Default###ENABLE_SHADOWS", &light->Flags, LIGHTFLAG_CAST_SHADOWS);
			ImGui::CheckboxFlags("Static Geometry", &light->Flags, LIGHTFLAG_CAST_STATIC_GEOM_SHADOWS);
			ImGui::CheckboxFlags("Dynamic Geometry", &light->Flags, LIGHTFLAG_CAST_DYNAMIC_GEOM_SHADOWS);

			ImGui::Text("Resolution:");
			ImGui::SameLine();
			ImGui::HelpMarker("High resolution only increases light priority, it will more likely be rendered hi-res in scene with lot of lights.");
			static constexpr u32 DEFAULT_RES_MASK = ~(LIGHTFLAG_CAST_LOWRES_SHADOWS | LIGHTFLAG_CAST_HIGHER_RES_SHADOWS);
			bool lowRes = light->Flags & LIGHTFLAG_CAST_LOWRES_SHADOWS;
			bool hiRes = light->Flags & LIGHTFLAG_CAST_HIGHER_RES_SHADOWS;
			bool defaultRes = !(lowRes | hiRes);
			if (ImGui::RadioButton("Low", lowRes))
			{
				light->Flags &= DEFAULT_RES_MASK;
				light->Flags |= LIGHTFLAG_CAST_LOWRES_SHADOWS;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Default", defaultRes))
			{
				light->Flags &= DEFAULT_RES_MASK;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("High", hiRes))
			{
				light->Flags &= DEFAULT_RES_MASK;
				light->Flags |= LIGHTFLAG_CAST_HIGHER_RES_SHADOWS;
			}

			//bool shadowsEnabled = light->Flags & LIGHTFLAG_USE_SHADOWS;
			//if (!shadowsEnabled) ImGui::BeginDisabled();
			{
				ImGui::DragU8("Blur", &light->ShadowBlur, 1, 0, 255);
				ImGui::DragFloat("Near Clip", &light->ShadowNearClip, 0.1f, 0, 300, "%.1f");
				editFadeDistance("Fade###SHADOW_FADE", light->ShadowFadeDistance);
			}
			//if (!shadowsEnabled) ImGui::EndDisabled();

		}

		if (SlGui::Category(ICON_AM_LIGHT_SUN" Corona", false))
		{
			ImGui::TextDisabled(ICON_AM_ERROR" Supported only on point lights");

			ImGui::DragFloat("Intensity###CORONA_INTENSITY", &light->CoronaIntensity, 0.05f, 0.0f, 100, "%.4f",
				ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
			ImGui::DragFloat("Size###CORONA_SIZE", &light->CoronaSize, 0.01, 0.0f, 10, "%.3f");
			ImGui::DragFloat("ZBias###CORONA_ZBIAS", &light->CoronaZBias, 1.0f, 0.0f, 100, "%.1f");
			ImGui::CheckboxFlags("Only Corona", &light->Flags, LIGHTFLAG_CORONA_ONLY);
			ImGui::CheckboxFlags("Only Corona LOD", &light->Flags, LIGHTFLAG_CORONA_ONLY_LOD_LIGHT);
		}
	}
	ImGui::End();
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

	// TODO: Is there any performance impact?
	scrUpdateLightsOnEntity(scrObjectIndex(m_SceneContext->EntityHandle));

	// Prepare context for drawing lights
	LightDrawContext drawContext;
	CViewport::GetCamera(&drawContext.CamFront, &drawContext.CamRight, &drawContext.CamUp);
	CViewport::GetWorldMouseRay(drawContext.WorldMouseRay.Pos, drawContext.WorldMouseRay.Dir);

	for (u16 i = 0; i < drawable->GetLightCount(); i++)
	{
		CLightAttr* light = drawable->GetLight(i);
		u16 lightNodeIndex = m_SceneContext->DrawableAsset->CompiledDrawableMap->LightAttrToSceneNode[i];
		graphics::SceneNode* lightNode = m_SceneContext->DrawableAsset->GetScene()->GetNode(lightNodeIndex);

		bool canSelect = m_SceneContext->SceneNodeToCanBeSelectedInViewport->Get(lightNodeIndex);
		bool isSelected = m_SelectedLight == i;

		ComputeLightMatrices(i,
			drawContext.LightWorld,
			drawContext.LightBind,
			drawContext.LightLocal,
			drawContext.LightBoneWorld);

		drawContext.Light = light;

		// Lights don't really have transform so we pass identity and manipulate lights directly
		if (!m_EditingCullPlane)
		{
			if (!canSelect) GIZMO->PushCanSelect(false);

			// Main light gizmo
			gizmo::GizmoInfo* gizmoType = nullptr;
			switch (light->Type)
			{
			case LIGHT_TYPE_POINT:	 gizmoType = GIZMO_GET_INFO(LightGizmo_Point);	 break;
			case LIGHT_TYPE_SPOT:	 gizmoType = GIZMO_GET_INFO(LightGizmo_Spot);  	 break;
			case LIGHT_TYPE_CAPSULE: gizmoType = GIZMO_GET_INFO(LightGizmo_Capsule); break;
			default: AM_UNREACHABLE("LightEditor::Render() -> No gizmo set for light type %i", light->Type);
			}
			char lightID[16];
			stbsp_snprintf(lightID, sizeof lightID, LIGHT_OUTLINE_GIZMO_FORMAT, i);
			Mat44V lightTransform = Mat44V::Identity();
			gizmo::GizmoState gizmoState;
			GIZMO->Manipulate(lightID, lightTransform, gizmoType, gizmoState, &drawContext);
			 
			// Move/Rotate gizmo
			if (gizmoState.SelectedHandle == LightGizmo_Point::PointLight_Sphere ||
				gizmoState.SelectedHandle == LightGizmo_Spot::SpotLight_HemisphereCone ||
				gizmoState.SelectedHandle == LightGizmo_Spot::CapsuleLight_Capsule)
			{
				Mat44V lightWorld = drawContext.LightWorld;
				char lightTransformID[16];
				stbsp_snprintf(lightTransformID, sizeof lightTransformID, LIGHT_TRANSFORM_GIZMO_FORMAT, i);
				if (GIZMO->ManipulateDefault(lightTransformID, lightWorld, &drawContext, lightID))
				{
					rage::Mat44V lightLocal = lightWorld * drawContext.LightBind;
					light->SetMatrix(lightLocal);
				}
			}
			
			// Move Spot Light Direction
			if (gizmoState.SelectedHandle == LightGizmo_Spot::SpotLight_Target)
			{
				Mat44V targetWorld = drawContext.LightWorld * Mat44V::Translation(LightGizmo_Spot::GetTargetPointWorld(drawContext));
				char lightTargetTransformID[16];
				stbsp_snprintf(lightTargetTransformID, sizeof lightTargetTransformID, LIGHT_TARGET_TRANSFORM_GIZMO_FORMAT, i);
				if (GIZMO->Manipulate(lightTargetTransformID, targetWorld, GIZMO_GET_INFO(gizmo::GizmoTranslation), nullptr, &drawContext, lightID))
				{
					Vec3V relativeTarget = Vec3V(targetWorld.Pos) - Vec3V(drawContext.LightWorld.Pos);
					LightGizmo_Spot::SetTargetPointLocal(drawContext, relativeTarget);
				}
			}

			// Move Capsule Light Extents
			if (gizmoState.SelectedHandle == LightGizmo_Spot::CapsuleLight_Extent1 || 
				gizmoState.SelectedHandle == LightGizmo_Spot::CapsuleLight_Extent2)
			{
				float halfExtent = light->Extent.X * 0.5f;
				Mat44V extent1 = Mat44V::Translation(rage::VEC_UP * halfExtent) * drawContext.LightWorld;
				Mat44V extent2 = Mat44V::Translation(rage::VEC_DOWN * halfExtent) * drawContext.LightWorld;

				// Extent to manipulate
				Mat44V& extentTarget = gizmoState.SelectedHandle == LightGizmo_Spot::CapsuleLight_Extent1 ? extent1 : extent2;

				char lightExtentTransformID[16];
				stbsp_snprintf(lightExtentTransformID, sizeof lightExtentTransformID, "Light_ExtentTransform%i", i);
				if (GIZMO->Manipulate(lightExtentTransformID, extentTarget, GIZMO_GET_INFO(gizmo::GizmoTranslation), nullptr, &drawContext, lightID))
				{
					// Transform to light local space
					extent1 *= drawContext.LightBind;
					extent2 *= drawContext.LightBind;

					light->SetCapsulePoints(extent1.Pos, extent2.Pos);
				}
			}

			if (!m_SelectionFreezed)
			{
				if (gizmoState.JustUnselected && m_SelectedLight == i)
				{
					m_SelectedLight = -1;
				}

				if (gizmoState.JustSelected)
				{
					m_SelectedLight = i;
				}
			}

			// Light name in 3D
			if (DrawableRender::LightNames)
			{
				Im3D::CenterNext();
				Im3D::TextBgColored(
					drawContext.LightWorld.Pos,
					gizmo::GIZMO_COLOR_WHITE, "%s [%s Light #%u]",
					lightNode->GetName(),
					GetLightTypeName(drawContext.Light->Type),
					i);
			}

			if (!canSelect) GIZMO->PopCanSelect();
		}

		if (isSelected)
		{
			if (m_EditingCullPlane)
				DrawCullPlaneEditGizmo(drawContext);

			DrawLightUI(drawContext);
		}
	}
}

void rageam::integration::LightEditor::SelectLight(s32 index)
{
	if (!m_SelectionFreezed)
	{
		// Select outline gizmo
		if (index != -1)
		{
			char lightID[16];
			stbsp_snprintf(lightID, sizeof lightID, LIGHT_OUTLINE_GIZMO_FORMAT, index);
			GIZMO->SetSelectedID(lightID);
		}

		// Light is selected and we want to unset any selected light
		if (index == -1 && m_SelectedLight != -1)
		{
			GIZMO->ResetSelection();
		}

		m_SelectedLight = index;
	}
}

void rageam::integration::LightEditor::Reset()
{
	SelectLight(-1);
	scrUpdateLightsOnEntity(scrObjectIndex(m_SceneContext->EntityHandle));
}

#endif
