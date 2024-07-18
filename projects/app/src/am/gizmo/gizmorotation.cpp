#include "gizmorotation.h"

#ifdef AM_INTEGRATED

#include "am/graphics/shapetest.h"
#include "am/integration/im3d.h"
#include "am/ui/extensions.h"

GIZMO_INITIALIZE_INFO(rageam::gizmo::GizmoRotation, "Rotation");

void rageam::gizmo::GizmoRotation::Draw(const GizmoContext& context)
{
	const ScalarV& scale = context.Scale;
	const Mat44V& transform = context.Transform;
	const Mat34V& camera = context.Camera;
	GizmoHandleID hitHandle = context.SelectedHandle;

	DrawCircle(transform, camera.Front, camera.Right, scale, GIZMO_COLOR_GRAY);

	DrawCircleCameraCull(camera, transform, rage::VEC_RIGHT, rage::VEC_FRONT, hitHandle == GizmoRotation_AxisX ? GIZMO_COLOR_YELLOW : GIZMO_COLOR_RED);
	DrawCircleCameraCull(camera, transform, rage::VEC_FRONT, rage::VEC_RIGHT, hitHandle == GizmoRotation_AxisY ? GIZMO_COLOR_YELLOW : GIZMO_COLOR_GREEN);
	DrawCircleCameraCull(camera, transform, rage::VEC_UP,    rage::VEC_RIGHT, hitHandle == GizmoRotation_AxisZ ? GIZMO_COLOR_YELLOW : GIZMO_COLOR_BLUE);
}

void rageam::gizmo::GizmoRotation::Manipulate(const GizmoContext& context, Vec3V& outOffset, Vec3V& outScale, QuatV& outRotation)
{
	EASY_BLOCK("GizmoRotation::Manipulate");

	const Ray&	  startMouseRay = context.StartMouseRay;
	const Mat44V& startTransform = context.StartTransform;
	const Mat44V& transform = context.Transform;
	GizmoHandleID hitHandle = context.SelectedHandle;

	EASY_BLOCK("Select Plane");
	// All gizmo manipulations are done via transforming on a plane
	GizmoPlaneSelector planeSelector = GizmoHandlePlaneSelector[hitHandle];
	Vec3V planeNormal = SelectPlane(context.Camera.Front, planeSelector);
	planeNormal = ConvertNormalOrientation(planeNormal, startTransform, context.Orientation);
	EASY_END_BLOCK;

	EASY_BLOCK("Intersection And Compute Tangent")
	// Find tangent to drag point (direction of rotation)
	// Hit point might be not on circle because we create additional capsule colliders
	Vec3V circlePoint;
	graphics::ShapeTest::RayIntersectsPlane(startMouseRay.Pos, startMouseRay.Dir, transform.Pos, planeNormal, circlePoint);
	Vec3V circleBiNormal = (circlePoint - transform.Pos).Normalized();
	circlePoint = transform.Pos + circleBiNormal * context.Scale; // Make sure that point is exactly on the edge
	Vec3V circleTangent = circleBiNormal.Cross(planeNormal);
	EASY_END_BLOCK;

	// Compute rotation on X and Y axes using tangent
	Vec2S mouseDelta = context.MouseDelta;
	float yFactor = circleTangent.Dot(context.Camera.Up).Get();
	float xFactor = -circleTangent.Dot(context.Camera.Right).Get();
	xFactor = std::clamp(xFactor, -1.0f, 1.0f);
	yFactor = std::clamp(yFactor, -1.0f, 1.0f);
	float delta = -mouseDelta.X * xFactor + -mouseDelta.Y * yFactor;
	delta *= SENSETIVITY;

	outRotation = QuatV::RotationNormal(planeNormal, delta);

	// Draw arrow in direction of rotation and arc
	if (!outRotation.IsIdentity())
	{
		EASY_BLOCK("Draw Arrow And Arc")

		static constexpr float ARROW_ANGLE = 25.0f;
		static constexpr float ARROW_LENGTH = 0.1f;

		u32 arcFillColor;
		switch (hitHandle)
		{
		case GizmoRotation_AxisX: arcFillColor = GIZMO_COLOR_RED;	break;
		case GizmoRotation_AxisY: arcFillColor = GIZMO_COLOR_GREEN;	break;
		case GizmoRotation_AxisZ: arcFillColor = GIZMO_COLOR_BLUE;	break;
		default:				  arcFillColor = 0;					break;
		}

		auto& dl = GetDrawList();
		auto drawArrow = [&](const Vec3V& dir, u32 color)
			{
				Vec3V arrowEnd = circlePoint + dir * context.Scale * 0.65f;
				Vec3V arrow1 = dir.Rotate(context.Camera.Front, rage::DegToRad(ARROW_ANGLE));
				Vec3V arrow2 = dir.Rotate(context.Camera.Front, rage::DegToRad(-ARROW_ANGLE));
				dl.DrawLine(circlePoint, arrowEnd, color);
				dl.DrawLine(arrowEnd, arrowEnd - arrow1 * context.Scale * ARROW_LENGTH, color);
				dl.DrawLine(arrowEnd, arrowEnd - arrow2 * context.Scale * ARROW_LENGTH, color);
			};

		// Draw arrow in direction of rotation
		EASY_BLOCK("Arrow")
		Vec3V arrowDir = delta > 0.0f ? -circleTangent : circleTangent;
		drawArrow(arrowDir, GIZMO_COLOR_YELLOW);
		drawArrow(-arrowDir, GIZMO_COLOR_GRAY);
		EASY_END_BLOCK;

		// Draw rotation arc border
		EASY_BLOCK("Arc")
		Vec3V circlePoint2 = transform.Pos + circleBiNormal.Rotate(outRotation) * context.Scale;
		dl.DrawLine(transform.Pos, circlePoint, arcFillColor);
		dl.DrawLine(transform.Pos, circlePoint2, arcFillColor);
		// Fill arc
		int fillNumRots = rage::Max(1, static_cast<int>(ceil(fabs(delta) / rage::PI2)));
		int fillSteps = 48 * fillNumRots;
		float fillStep = delta / static_cast<float>(fillSteps);
		Vec3V fillPointPrev = circlePoint;
		QuatV fillTheta = DirectX::XMQuaternionRotationNormal(planeNormal, fillStep);
		QuatV fillTotalTheta = rage::QUAT_IDENTITY;
		for (int i = 0; i <= fillSteps; i++)
		{
			Vec3V fillPointCurrent = transform.Pos + circleBiNormal.Rotate(fillTotalTheta) * context.Scale;
			if (i != 0)
			{
				// Slightly transparent color, alpha will accumulate if total rotation is greater than 360 degrees
				graphics::ColorU32 fillCol = arcFillColor;
				fillCol.A = 125;
				dl.DrawTriFill(transform.Pos, fillPointPrev, fillPointCurrent, fillCol);
			}
			fillPointPrev = fillPointCurrent;
			fillTotalTheta = DirectX::XMQuaternionMultiply(fillTotalTheta, fillTheta);
		}
		EASY_END_BLOCK;
	}

	// 3D Text / Debug Drawing
	ConstString displayText;
	if (context.DebugInfo)
	{
		auto& dl = GetDrawList();
		dl.DrawCircle(transform.Pos, planeNormal, planeNormal.Tangent(), context.Scale * 2.0f, graphics::COLOR_ORANGE);

		displayText = ImGui::FormatTemp("%.01f %.01f %.01f %.02f", outRotation.X(), outRotation.Y(), outRotation.Z(), outRotation.W());
	}
	else
	{
		Vec3V eulers = outRotation.ToEuler();
		displayText = ImGui::FormatTemp("X: %.01f Y: %.01f Z: %.01f", eulers.X(), eulers.Y(), eulers.Z());
	}
	Im3D::TextBg(transform.Pos + rage::VEC_DOWN * 0.15f, displayText);
}

rageam::List<rageam::gizmo::HitResult> rageam::gizmo::GizmoRotation::HitTest(const GizmoContext& context)
{
	const ScalarV& scale = context.Scale;
	const Mat44V&  transform = context.Transform;
	const Ray&	   ray = context.MouseRay;

	List<HitResult> hits;
	ScalarV dist;

	ScalarV scale1 = scale * 1.15f;
	ScalarV scale2 = scale1 * 2.4f;

	if (TestDisk(ray, transform, rage::VEC_RIGHT, scale1, dist))
		hits.Construct(dist, GizmoRotation_AxisX);
	if (TestDisk(ray, transform, rage::VEC_FRONT, scale1, dist))
		hits.Construct(dist, GizmoRotation_AxisY);
	if (TestDisk(ray, transform, rage::VEC_UP, scale1, dist))
		hits.Construct(dist, GizmoRotation_AxisZ);

	// For case when view is aligned with the axis, we need additional capsule colliders:

	if (TestCapsule(ray, Mat44V::Translation(rage::VEC_FRONT * 0.1f) * transform, -rage::VEC_UP, rage::VEC_UP, scale2, dist))
		hits.Construct(dist, GizmoRotation_AxisX);
	if (TestCapsule(ray, Mat44V::Translation(-rage::VEC_FRONT * 0.1f) * transform, -rage::VEC_UP, rage::VEC_UP, scale2, dist))
		hits.Construct(dist, GizmoRotation_AxisX);
	if (TestCapsule(ray, Mat44V::Translation(-rage::VEC_UP * 0.1f) * transform, -rage::VEC_FRONT, rage::VEC_FRONT, scale2, dist))
		hits.Construct(dist, GizmoRotation_AxisX);
	if (TestCapsule(ray, Mat44V::Translation(rage::VEC_UP * 0.1f) * transform, -rage::VEC_FRONT, rage::VEC_FRONT, scale2, dist))
		hits.Construct(dist, GizmoRotation_AxisX);

	if (TestCapsule(ray, Mat44V::Translation(rage::VEC_RIGHT * 0.1f) * transform, -rage::VEC_UP, rage::VEC_UP, scale2, dist))
		hits.Construct(dist, GizmoRotation_AxisY);
	if (TestCapsule(ray, Mat44V::Translation(-rage::VEC_RIGHT * 0.1f) * transform, -rage::VEC_UP, rage::VEC_UP, scale2, dist))
		hits.Construct(dist, GizmoRotation_AxisY);
	if (TestCapsule(ray, Mat44V::Translation(-rage::VEC_UP * 0.1f) * transform, -rage::VEC_RIGHT, rage::VEC_RIGHT, scale2, dist))
		hits.Construct(dist, GizmoRotation_AxisY);
	if (TestCapsule(ray, Mat44V::Translation(rage::VEC_UP * 0.1f) * transform, -rage::VEC_RIGHT, rage::VEC_RIGHT, scale2, dist))
		hits.Construct(dist, GizmoRotation_AxisY);

	if (TestCapsule(ray, transform, -rage::VEC_FRONT, rage::VEC_FRONT, scale2, dist))
		hits.Construct(dist, GizmoRotation_AxisZ);
	if (TestCapsule(ray, transform, -rage::VEC_RIGHT, rage::VEC_RIGHT, scale2, dist))
		hits.Construct(dist, GizmoRotation_AxisZ);

	return hits;
}

#endif
