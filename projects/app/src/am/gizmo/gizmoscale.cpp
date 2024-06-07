#include "gizmoscale.h"

GIZMO_INITIALIZE_INFO(rageam::gizmo::GizmoScale, "Scale");

void rageam::gizmo::GizmoScale::Draw(const GizmoContext& context)
{
	const ScalarV& scale = context.Scale;
	const Mat44V&  transform = context.Transform;
	const Mat34V&  camera = context.Camera;
	GizmoHandleID  hitHandle = context.SelectedHandle;

	// Mat44V viewTransform = Mat44V::Transform(scale, rage::QUAT_IDENTITY, transform.Pos); // Without rotation
	// DrawQuad(viewTransform, camera.Front, camera.Right, AXIS_SCREEN_PLANE_EXTENTV, hitHandle == GizmoScale_All ? GIZMO_COLOR_YELLOW : GIZMO_COLOR_HOVER);

	DrawAxis(transform, rage::VEC_RIGHT, rage::VEC_FRONT, GIZMO_COLOR_RED, hitHandle == GizmoScale_AxisX, false);
	DrawAxis(transform, rage::VEC_FRONT, rage::VEC_RIGHT, GIZMO_COLOR_GREEN, hitHandle == GizmoScale_AxisY, false);
	DrawAxis(transform, rage::VEC_UP, rage::VEC_RIGHT, GIZMO_COLOR_BLUE, hitHandle == GizmoScale_AxisZ, false);

	auto& dl = GetDrawList();
	dl.SetTransform(transform);
	dl.DrawAlignedBoxFill(rage::VEC_ORIGIN, SCALE_BOX_EXTENTV, GIZMO_COLOR_GRAY2);
	dl.DrawAlignedBoxFill(rage::VEC_RIGHT, SCALE_BOX_EXTENTV, GIZMO_COLOR_RED);
	dl.DrawAlignedBoxFill(rage::VEC_FRONT, SCALE_BOX_EXTENTV, GIZMO_COLOR_GREEN);
	dl.DrawAlignedBoxFill(rage::VEC_UP, SCALE_BOX_EXTENTV, GIZMO_COLOR_BLUE);
	dl.ResetTransform();

	//DrawPlane(transform, rage::VEC_RIGHT, rage::VEC_FRONT, GIZMO_COLOR_RED, GIZMO_COLOR_GREEN, hitHandle == GizmoTranslation_PlaneXY);
	//DrawPlane(transform, rage::VEC_RIGHT, rage::VEC_UP, GIZMO_COLOR_RED, GIZMO_COLOR_BLUE, hitHandle == GizmoTranslation_PlaneXZ);
	//DrawPlane(transform, rage::VEC_FRONT, rage::VEC_UP, GIZMO_COLOR_GREEN, GIZMO_COLOR_BLUE, hitHandle == GizmoTranslation_PlaneYZ);
}

void rageam::gizmo::GizmoScale::Manipulate(const GizmoContext& context, Vec3V& outOffset, Vec3V& outScale, QuatV& outRotation)
{
	const Mat44V& startTransform = context.StartTransform;
	const Mat44V& transform = context.Transform;
	GizmoHandleID hitHandle = context.SelectedHandle;

	// DOF is only limited for axes because we manipulating on 2 axis plane, but need only 1 axis
	GizmoConstraint constraint;
	switch (hitHandle)
	{
	case GizmoScale_AxisX: constraint = GizmoConstraint_X;	 break;
	case GizmoScale_AxisY: constraint = GizmoConstraint_Y;	 break;
	case GizmoScale_AxisZ: constraint = GizmoConstraint_Z;	 break;
	default:			   constraint = GizmoConstraint_None;  break;
	}

	// All gizmo manipulations are done via transforming on a plane
	GizmoPlaneSelector planeSelector = GizmoHandlePlaneSelector[hitHandle];
	Vec3V planeNormal = SelectPlane(context.Camera.Front, planeSelector);
	Vec3V planePos = transform.Pos;
	planeNormal = ConvertNormalOrientation(planeNormal, startTransform, context.Orientation); // Transform plane to gizmo transform
	Vec3V dragStart, dragEnd;
	Vec3V dragOffset = ManipulateOnPlane(
		context.StartMouseRay, context.MouseRay, startTransform, context.Camera, planePos, planeNormal, dragStart, dragEnd, constraint);

	ScalarV scaleFactor = dragOffset.Length();

	if (hitHandle != GizmoScale_All)
	{
		Vec3V dragDir;
		if (hitHandle == GizmoScale_AxisX)      dragDir = rage::VEC_RIGHT;
		else if (hitHandle == GizmoScale_AxisY) dragDir = rage::VEC_FRONT;
		else if (hitHandle == GizmoScale_AxisZ) dragDir = rage::VEC_UP;
		dragDir = dragDir.TransformNormal(startTransform);
		outScale = rage::S_ONE + dragDir * scaleFactor;
	}
}

rageam::List<rageam::gizmo::HitResult> rageam::gizmo::GizmoScale::HitTest(const GizmoContext& context)
{
	return {};
}
