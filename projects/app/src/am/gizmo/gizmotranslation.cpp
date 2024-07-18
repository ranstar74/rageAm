#include "gizmotranslation.h"

#ifdef AM_INTEGRATED

#include "am/graphics/shapetest.h"
#include "am/integration/im3d.h"
#include "am/ui/extensions.h"

GIZMO_INITIALIZE_INFO(rageam::gizmo::GizmoTranslation, "Translation");

void rageam::gizmo::GizmoTranslation::Draw(const GizmoContext& context)
{
	const ScalarV& scale = context.Scale;
	const Mat44V&  transform = context.Transform;
	const Mat34V&  camera = context.Camera;
	GizmoHandleID  hitHandle = context.SelectedHandle;

	Mat44V viewTransform = Mat44V::Transform(scale, rage::QUAT_IDENTITY, transform.Pos); // Without rotation
	DrawQuad(viewTransform, camera.Front, camera.Right, AXIS_SCREEN_PLANE_EXTENTV, hitHandle == GizmoTranslation_View ? GIZMO_COLOR_YELLOW : GIZMO_COLOR_HOVER);

	DrawAxis(transform, rage::VEC_RIGHT, rage::VEC_FRONT, GIZMO_COLOR_RED,   hitHandle == GizmoTranslation_AxisX);
	DrawAxis(transform, rage::VEC_FRONT, rage::VEC_RIGHT, GIZMO_COLOR_GREEN, hitHandle == GizmoTranslation_AxisY);
	DrawAxis(transform, rage::VEC_UP,    rage::VEC_RIGHT, GIZMO_COLOR_BLUE,  hitHandle == GizmoTranslation_AxisZ);

	DrawPlane(transform, rage::VEC_RIGHT, rage::VEC_FRONT, GIZMO_COLOR_RED,   GIZMO_COLOR_GREEN, hitHandle == GizmoTranslation_PlaneXY);
	DrawPlane(transform, rage::VEC_RIGHT, rage::VEC_UP,    GIZMO_COLOR_RED,   GIZMO_COLOR_BLUE,  hitHandle == GizmoTranslation_PlaneXZ);
	DrawPlane(transform, rage::VEC_FRONT, rage::VEC_UP,    GIZMO_COLOR_GREEN, GIZMO_COLOR_BLUE,  hitHandle == GizmoTranslation_PlaneYZ);
}

void rageam::gizmo::GizmoTranslation::Manipulate(const GizmoContext& context, Vec3V& outOffset, Vec3V& outScale, QuatV& outRotation)
{
	const Mat44V& startTransform = context.StartTransformOriented;
	const Mat44V& transform = context.Transform;
	GizmoHandleID hitHandle = context.SelectedHandle;
	
	// DOF is only limited for axes because we manipulating on 2 axis plane, but need only 1 axis
	GizmoConstraint constraint;
	switch (hitHandle)
	{
	case GizmoTranslation_AxisX: constraint = GizmoConstraint_X;	 break;
	case GizmoTranslation_AxisY: constraint = GizmoConstraint_Y;	 break;
	case GizmoTranslation_AxisZ: constraint = GizmoConstraint_Z;	 break;
	default:					 constraint = GizmoConstraint_None;  break;
	}

	// All gizmo manipulations are done via transforming on a plane
	GizmoPlaneSelector planeSelector = GizmoHandlePlaneSelector[hitHandle];
	Vec3V planeNormal = SelectPlane(context.Camera.Front, planeSelector);
	Vec3V planePos = transform.Pos;	
	planeNormal = ConvertNormalOrientation(planeNormal, startTransform, context.Orientation); // Transform plane to gizmo transform
	Vec3V dragStart, dragEnd;
	Vec3V dragOffset = ManipulateOnPlane(
		context.StartMouseRay, context.MouseRay, startTransform, context.Camera, planePos, planeNormal, dragStart, dragEnd, constraint);
	outOffset = dragOffset;

	// 3D Text / Debug Drawing
	auto& dl = GetDrawList();
	ConstString displayText;
	if (context.DebugInfo)
	{
		dl.DrawLine(dragStart, dragEnd, graphics::COLOR_YELLOW, graphics::COLOR_RED);
		dl.DrawCircle(transform.Pos, planeNormal, planeNormal.Tangent(), context.Scale, graphics::COLOR_ORANGE);

		displayText = ImGui::FormatTemp("Offset: %.01f %.01f %.01f Plane: %.01f %.01f %.01f",
			dragOffset.X(), dragOffset.Y(), dragOffset.Z(), planeNormal.X(), planeNormal.Y(), planeNormal.Z());
	}
	else
	{
		displayText = ImGui::FormatTemp("X: %.01f Y: %.01f Z: %.01f", dragOffset.X(), dragOffset.Y(), dragOffset.Z());
	}
	Im3D::TextBg(transform.Pos + rage::VEC_DOWN * 0.15f, displayText);
	// Direction lines
	{
		static const ScalarV DIRECTION_LINE_LENGTH = 1000.0f;
		if (hitHandle == GizmoTranslation_AxisX || hitHandle == GizmoTranslation_AxisY || hitHandle == GizmoTranslation_AxisZ)
		{
			Vec3V dragDir;
			if (hitHandle == GizmoTranslation_AxisX)      dragDir = rage::VEC_RIGHT;
			else if (hitHandle == GizmoTranslation_AxisY) dragDir = rage::VEC_FRONT;
			else if (hitHandle == GizmoTranslation_AxisZ) dragDir = rage::VEC_UP;
			dragDir = dragDir.TransformNormal(startTransform);
			dl.DrawLine(dragStart, dragStart + dragDir * DIRECTION_LINE_LENGTH, GIZMO_COLOR_GRAY2);
			dl.DrawLine(dragStart, dragStart - dragDir * DIRECTION_LINE_LENGTH, GIZMO_COLOR_GRAY2);
		}
		else
		{
			Vec3V dragDir1;
			Vec3V dragDir2;
			if (hitHandle == GizmoTranslation_PlaneXY)		{ dragDir1 = rage::VEC_RIGHT; dragDir2 = rage::VEC_FRONT; }
			else if (hitHandle == GizmoTranslation_PlaneXZ) { dragDir1 = rage::VEC_RIGHT; dragDir2 = rage::VEC_UP; }
			else if (hitHandle == GizmoTranslation_PlaneYZ) { dragDir1 = rage::VEC_FRONT; dragDir2 = rage::VEC_UP; }
			dragDir1 = dragDir1.TransformNormal(startTransform);
			dragDir2 = dragDir2.TransformNormal(startTransform);
			dl.DrawLine(dragStart, dragStart + dragDir1 * DIRECTION_LINE_LENGTH, GIZMO_COLOR_GRAY2);
			dl.DrawLine(dragStart, dragStart - dragDir1 * DIRECTION_LINE_LENGTH, GIZMO_COLOR_GRAY2);
			dl.DrawLine(dragStart, dragStart + dragDir2 * DIRECTION_LINE_LENGTH, GIZMO_COLOR_GRAY2);
			dl.DrawLine(dragStart, dragStart - dragDir2 * DIRECTION_LINE_LENGTH, GIZMO_COLOR_GRAY2);
		}
	}
}

rageam::List<rageam::gizmo::HitResult> rageam::gizmo::GizmoTranslation::HitTest(const GizmoContext& context)
{
	const ScalarV& scale = context.Scale;
	const Mat44V&  transform = context.Transform;
	const Ray&     ray = context.MouseRay;

	List<HitResult> hits;
	ScalarV dist;

	// Screen space plane in center of gizmo, it has priority over other handles so
	// don't test other handles if it was hovered
	if (graphics::ShapeTest::RayIntersectsSphere(ray.Pos, ray.Dir, transform.Pos, AXIS_SCREEN_PLANE_EXTENTV * scale * 1.5f, &dist))
	{
		hits.Construct(dist, GizmoTranslation_View);
		return hits;
	}

	// Planes
	if (TestPlane(ray, transform, rage::VEC_RIGHT, rage::VEC_FRONT, dist))
		hits.Construct(dist, GizmoTranslation_PlaneXY);
	if (TestPlane(ray, transform, rage::VEC_RIGHT, rage::VEC_UP, dist))
		hits.Construct(dist, GizmoTranslation_PlaneXZ);
	if (TestPlane(ray, transform, rage::VEC_FRONT, rage::VEC_UP, dist))
		hits.Construct(dist, GizmoTranslation_PlaneYZ);

	// Don't test axes if any plane was hit, axis collider is quite big
	if (hits.Any())
		return hits;

	// Axes
	if (TestAxis(ray, transform, rage::VEC_RIGHT, scale, dist))
		hits.Construct(dist, GizmoTranslation_AxisX);
	if (TestAxis(ray, transform, rage::VEC_FRONT, scale, dist))
		hits.Construct(dist, GizmoTranslation_AxisY);
	if (TestAxis(ray, transform, rage::VEC_UP, scale, dist))
		hits.Construct(dist, GizmoTranslation_AxisZ);

	return hits;
}

#endif
