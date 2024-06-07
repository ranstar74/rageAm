#include "gizmobase.h"

#include "am/graphics/shapetest.h"
#include "am/integration/im3d.h"
#include "am/ui/extensions.h"
#include "am/ui/imglue.h"
#include "game/viewport.h"

#include <easy/arbitrary_value.h>

bool rageam::gizmo::TestAxis(const Ray& ray, const Mat44V& transform, const Vec3V& normal, const ScalarV& scale, ScalarV& outDist)
{
	Vec3V   p1 = transform.Pos;
	Vec3V   p2 = p1 + normal.TransformNormal(transform) * scale;
	ScalarV radius = AXIS_CAP_HIT_RADIUSV * scale; // Cheaper than matrix multiplication
	return graphics::ShapeTest::RayIntersectsCapsule(ray.Pos, ray.Dir, p1, p2, radius, &outDist);
}

bool rageam::gizmo::TestCapsule(const Ray& ray, const Mat44V& transform, const Vec3V& p1, const Vec3V& p2, const ScalarV& scale, ScalarV& outDist)
{
	Vec3V   pt1 = p1.Transform(transform);
	Vec3V   pt2 = p2.Transform(transform);
	ScalarV radius = AXIS_CAP_HIT_RADIUSV * scale; // Cheaper than matrix multiplication
	return graphics::ShapeTest::RayIntersectsCapsule(ray.Pos, ray.Dir, pt1, pt2, radius, &outDist);
}

bool rageam::gizmo::TestPlane(const Ray& ray, const Mat44V& transform, const Vec3V& normal, const Vec3V& tangent, ScalarV& outDist)
{
	Vec3V p1 = transform.Pos;
	Vec3V p2 = normal * AXIS_PLANE_EXTENTV;
	Vec3V p3 = tangent * AXIS_PLANE_EXTENTV;
	Vec3V p4 = p2 + p3;
	p2 = p2.Transform(transform);
	p3 = p3.Transform(transform);
	p4 = p4.Transform(transform);
	float dist;
	if (graphics::ShapeTest::RayIntersectsTriangle(ray.Pos, ray.Dir, p1, p2, p4, dist))
	{
		outDist = dist;
		return true;
	}
	if (graphics::ShapeTest::RayIntersectsTriangle(ray.Pos, ray.Dir, p1, p3, p4, dist))
	{
		outDist = dist;
		return true;
	}
	outDist = -1.0f;
	return false;
}

bool rageam::gizmo::TestDisk(const Ray& ray, const Mat44V& transform, const Vec3V& normal, const ScalarV& scale, ScalarV& outDist)
{
	Vec3V n = normal.TransformNormal(transform);
	return graphics::ShapeTest::RayIntersectsCircle(ray.Pos, ray.Dir, transform.Pos, n, scale, &outDist);
}

bool rageam::gizmo::TestQuad(const Ray& ray, const Mat44V& transform, const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, const Vec3V& p4, ScalarV& outDist)
{
	Vec3V pt1 = p1.Transform(transform);
	Vec3V pt2 = p2.Transform(transform);
	Vec3V pt3 = p3.Transform(transform);
	Vec3V pt4 = p4.Transform(transform);
	float dist;
	if (graphics::ShapeTest::RayIntersectsTriangle(ray.Pos, ray.Dir, pt1, pt2, pt3, dist))
	{
		outDist = dist;
		return true;
	}
	if (graphics::ShapeTest::RayIntersectsTriangle(ray.Pos, ray.Dir, pt1, pt3, pt4, dist))
	{
		outDist = dist;
		return true;
	}
	return false;
}

bool rageam::gizmo::TestCircle(const Ray& ray, const Mat44V& transform, const Vec3V& normal, const ScalarV& scale, const ScalarV& borderWidth, ScalarV& outDist)
{
	ScalarV b = borderWidth * scale;
	// Adjust scale so hit test works a bit outside circle too
	ScalarV s = scale + b;
	if (!TestDisk(ray, transform, normal, s, outDist))
		return false;
	Vec3V   point = ray.Pos + ray.Dir * outDist;
	ScalarV dist = point.DistanceToEstimate(transform.Pos);
	ScalarV excludeDist = scale - b;
	return dist - excludeDist >= rage::S_ZERO;
}

void rageam::gizmo::DrawCircleCameraCull(const Mat34V& camera, const Mat44V& transform, const rage::Vec3V& normal, const rage::Vec3V& tangent, u32 color)
{
	EASY_BLOCK("gizmo::DrawCircleCameraCull");

	auto& dl = GetDrawList();

	static constexpr int   NUM_SEGMENTS = 48;
	static constexpr int   NUM_SUB_SEGMENTS = NUM_SEGMENTS / 4;
	static constexpr float SEGMENT_STEP = rage::PI2 / NUM_SEGMENTS;

	Vec3V cameraToTransform = Vec3V(transform.Pos - camera.Pos).Normalized();
	bool isParallel = cameraToTransform.Dot(normal.TransformNormal(transform)).Abs() >= 0.98f;

	// We need tangent and binormal for orientation of the circle
	rage::Vec3V biNormal = normal.Cross(tangent);
	rage::Vec3V prevPoint;
	bool        prevVisible = true;
	// We drawing + 1 more segment to connect last & first point
	for (int i = 0; i < NUM_SEGMENTS + 1; i++)
	{
		rage::Vec3V point;

		bool visible = false;

		// Try to find last point that's visible
		for (int k = 0; k < NUM_SUB_SEGMENTS; k++)
		{
			float theta =
					SEGMENT_STEP * static_cast<float>(i) + (prevVisible ? -1.0f : 1.0f) *
					SEGMENT_STEP * static_cast<float>(k) / static_cast<float>(NUM_SUB_SEGMENTS);
			float cos = cosf(theta);
			float sin = sinf(theta);

			// Transform coordinate on circle plane
			rage::Vec3V to = tangent * cos + biNormal * sin;
			rage::Vec3V toTransformed = to.Transform(transform);
			point = toTransformed;

			if (isParallel || camera.Front.Dot(to.TransformNormal(transform)) <= 0.0f)
			{
				if (i != 0 && prevVisible)
					dl.DrawLineFast(prevPoint, toTransformed, color);
				visible = true;
				break;
			}

			// Previous point wasn't visible, this point either, there's no point to add sub-divide
			// line because all in-between points are not visible too
			//if (!prevVisible) // TODO: FIX THIS...
			//	break;
		}
		prevPoint = point;
		prevVisible = visible;
	}
}

void rageam::gizmo::DrawCircle(const Mat44V& transform, const rage::Vec3V& normal, const rage::Vec3V& tangent, const ScalarV& scale, u32 color)
{
	auto& dl = GetDrawList();
	dl.DrawCircle(transform.Pos, normal, tangent, scale, color);
}

void rageam::gizmo::DrawPlane(const Mat44V& transform, const Vec3V& normal, const Vec3V& tangent, u32 color1, u32 color2, bool hovered)
{
	auto& dl = GetDrawList();
	Vec3V p1 = rage::VEC_ORIGIN;
	Vec3V p2 = normal * AXIS_PLANE_EXTENTV;
	Vec3V p3 = tangent * AXIS_PLANE_EXTENTV;
	Vec3V p4 = p2 + p3;
	dl.DrawLine(p2, p4, transform, color1);
	dl.DrawLine(p3, p4, transform, color2);
	if (hovered)
		dl.DrawQuadFill(p1, p2, p4, p3, GIZMO_COLOR_HOVER, transform);
}

void rageam::gizmo::DrawQuad(const Mat44V& transform, const Vec3V& normal, const Vec3V& tangent, const ScalarV& extent, u32 color)
{
	auto& dl = GetDrawList();
	dl.SetTransform(transform);
	dl.DrawQuad(rage::VEC_ORIGIN, normal, tangent, extent, extent, color);
	dl.ResetTransform();
}

void rageam::gizmo::DrawAxis(const Mat44V& transform, const Vec3V& normal, const Vec3V& tangent, u32 color, bool hovered, bool cap)
{
	auto& dl = GetDrawList();

	u32 sliderColor = hovered ? GIZMO_COLOR_YELLOW : color;

	Vec3V biNormal = normal.Cross(tangent);
	Vec3V p1 = rage::VEC_ORIGIN;
	Vec3V p2 = normal;

	// Slider
	dl.DrawLine(p1 + normal * AXIS_CENTER_OFFSETV, p2, transform, sliderColor);

	// Cap
	if (!cap)
		return;
	Vec3V cap0 = normal - normal * AXIS_CAP_EXTENTV; // For easier hit testing, cap ends with slider
	Vec3V cap1 = normal;
	Vec3V prevBasePoint;
	for (int i = 0; i <= AXIS_CAP_SEGMENTS; i++)
	{
		float t = AXIS_CAP_STEP * static_cast<float>(i);
		float x = cosf(t);
		float y = sinf(t);

		// Point on cap (cone) circle base
		Vec3V basePoint = cap0 + (tangent * x + biNormal * y) * AXIS_CAP_RADIUSV;
		if (i > 0)
		{
			// Bottom
			dl.DrawTriFill(basePoint, prevBasePoint, cap0, color, color, color, transform);
			// Side
			dl.DrawTriFill(prevBasePoint, basePoint, cap1, color, color, color, transform);
		}
		prevBasePoint = basePoint;
	}
}

rageam::Vec3V rageam::gizmo::SelectPlane(const Vec3V& cameraFront, GizmoPlaneSelector selector)
{
	// Find the plane normal
	Vec3V planeNormal;
	switch (selector)
	{
	case GizmoPlaneSelector_AxisX:	planeNormal = rage::VEC_RIGHT;	break;
	case GizmoPlaneSelector_AxisY:	planeNormal = rage::VEC_FRONT;	break;
	case GizmoPlaneSelector_AxisZ:	planeNormal = rage::VEC_UP;		break;
	case GizmoPlaneSelector_View:	planeNormal = cameraFront;		break;
	// For axes it is a bit more complicated because we have to find the best plane depending on view,
	// which is axis that has the smallest angle with camera direction ('most' parallel)
	case GizmoPlaneSelector_BestMatchX:
		{
			ScalarV yDot = cameraFront.Dot(rage::VEC_FRONT).Abs();
			ScalarV zDot = cameraFront.Dot(rage::VEC_UP).Abs();
			planeNormal = yDot > zDot ? rage::VEC_FRONT : rage::VEC_UP;
		}
		break;
	case GizmoPlaneSelector_BestMatchY:
		{
			ScalarV xDot = cameraFront.Dot(rage::VEC_RIGHT).Abs();
			ScalarV zDot = cameraFront.Dot(rage::VEC_UP).Abs();
			planeNormal = xDot > zDot ? rage::VEC_RIGHT : rage::VEC_UP;
		}
		break;
	case GizmoPlaneSelector_BestMatchZ:
		{
			ScalarV xDot = cameraFront.Dot(rage::VEC_RIGHT).Abs();
			ScalarV yDot = cameraFront.Dot(rage::VEC_FRONT).Abs();
			planeNormal = xDot > yDot ? rage::VEC_RIGHT : rage::VEC_FRONT;
		}
		break;

	default:
		AM_UNREACHABLE("gizmo::SelectPlane() -> Selector %i is not implemented.", selector);
	}
	return planeNormal;
}

rageam::Vec3V rageam::gizmo::ManipulateOnPlane(
	const Ray& startRay, const Ray& currentRay, const Mat44V& transform, const Mat34V& camera, 
	const Vec3V& planePos, const Vec3V& planeNormal, Vec3V& outFrom, Vec3V& outTo, GizmoConstraint constraint)
{
	// Project initial and current mouse rays on manipulation plane
	// If raycast fails then ray goes too far... this is the case when camera is perpendicular to the plane normal
	Vec3V startProjected, endProjected;
	if (!graphics::ShapeTest::RayIntersectsPlane(startRay.Pos, startRay.Dir, planePos, planeNormal, startProjected))
		startProjected = planePos;
	if (!graphics::ShapeTest::RayIntersectsPlane(currentRay.Pos, currentRay.Dir, planePos, planeNormal, endProjected))
		endProjected = planePos;

	endProjected = ConstraintPoint(startProjected, endProjected, transform, constraint);

	// Offset is just difference between positions where mouse ray first landed and now
	Vec3V dragOffset = endProjected - startProjected;

	// If camera is perpendicular to the plane, we can't really do transformation, because
	// at such extremes single pixel move will result offset
	Vec3V expectedPos = transform.Pos + dragOffset;
	ScalarV perpendicularFactor = camera.Front.Dot(planeNormal).Abs();
	perpendicularFactor / camera.Pos.DistanceToEstimate(expectedPos); // Approximate perspective depth factor

	// Going to extreme, reset. The same behaviour is implememnted in 3Ds Max
	if (perpendicularFactor < 0.0001f)
		dragOffset = {};

	outFrom = startProjected;
	outTo = endProjected;

	return dragOffset;
}

rageam::Vec3V rageam::gizmo::ConstraintPoint(const Vec3V& p1, const Vec3V& p2, const Mat44V& transform, GizmoConstraint constraint)
{
	Vec3V to = p2 - p1;
	switch (constraint)
	{
	case GizmoConstraint_None: return p2;
	case GizmoConstraint_X: return p1 + to.Project(rage::VEC_RIGHT.TransformNormal(transform));
	case GizmoConstraint_Y: return p1 + to.Project(rage::VEC_FRONT.TransformNormal(transform));
	case GizmoConstraint_Z: return p1 + to.Project(rage::VEC_UP.TransformNormal(transform));
	}
	AM_UNREACHABLE("gizmo::ConstraintPoint() -> Constraint %i is not implemented!", constraint);
}

rageam::gizmo::GizmoManager::GizmoManager()
{
	m_Gizmos.InitAndAllocate(59);
}

rageam::gizmo::GizmoID rageam::gizmo::GizmoManager::GetID(ConstString stringID) const
{
	// Base seed is hash-sum of all pushed IDs
	u32 seed = 0;
	if (m_IDStack.Any())
		seed = DataHash(m_IDStack.begin(), m_IDStack.GetSize() * sizeof(GizmoID));

	return Hash(stringID, seed);
}

void rageam::gizmo::GizmoManager::PushID(ConstString id)
{
	m_IDStack.Add(GetID(id));
}

void rageam::gizmo::GizmoManager::PopID()
{
	AM_ASSERT(m_IDStack.Any(), "GizmoManager::PopID() -> Stack is empty!");
	m_IDStack.RemoveLast();
}

bool rageam::gizmo::GizmoManager::CanInteract() const
{
	return !ui::GetUI()->IsDisabled && !ImGui::IsAnyWindowHovered();
}

void rageam::gizmo::GizmoManager::BeginFrame()
{
	// Cache cursor ray and camera
	Vec3V rayPos, rayDir;
	CViewport::GetWorldMouseRay(rayPos, rayDir);
	m_Camera = Mat34V(CViewport::GetCameraMatrix());
	m_MouseRay = Ray(rayPos, rayDir);
}

void rageam::gizmo::GizmoManager::EndFrame()
{
	EASY_BLOCK("GizmoManager::EndFrame");
	AM_ASSERT(!m_IDStack.Any(), "GizmoManager::EndFrame() -> ID stack is not empty!");

	GizmoEntry* closestHitGizmo = nullptr;
	bool closestHitGizmoTopMost = false;
	List<GizmoID> gizmosToRemove;
	for (GizmoEntry& gizmo : m_Gizmos)
	{
		// Remove gizmos that weren't manipulated for at least one frame
		if (abs(ImGui::GetFrameCount() - gizmo.LastFrame) != 0)
		{
			gizmosToRemove.Add(gizmo.ID);
			continue;
		}

		// Only top most gizmo can shadow previously set top most gizmo
		bool isTopMost = gizmo.Impl->TopMost();
		bool canShadow = isTopMost || !closestHitGizmoTopMost;

		// Find the closest hit gizmo to the camera
		if (canShadow && gizmo.DidHit)
		{
			// This is the first gizmo that we check, simply initialize result
			if (!closestHitGizmo)
			{
				closestHitGizmo = &gizmo;
				closestHitGizmoTopMost = isTopMost;
			}
			// Either gizmo is closer or it's set as top most, and previous gizmo isn't top most
			else if (gizmo.HitDistance < closestHitGizmo->HitDistance || (isTopMost && !closestHitGizmoTopMost))
			{
				closestHitGizmo = &gizmo;
				closestHitGizmoTopMost = isTopMost;
			}
		}
	}

	for (GizmoID id : gizmosToRemove)
		m_Gizmos.RemoveAt(id);

	bool clicked = CanInteract() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	bool selectionChanged = false;
	auto oldSelectedID = m_SelectedID;

	// Set closest hovered gizmo & handle selection
	if (closestHitGizmo)
	{
		m_ClosestHoveredGizmo = closestHitGizmo->ID;

		bool isSelected = 
			m_SelectedID.HasValue() && 
			m_SelectedID == closestHitGizmo->ID && 
			m_SelectedHandle == closestHitGizmo->HitHandle;

		// Select gizmo on click
		if (!isSelected && closestHitGizmo->Impl->CanSelect(closestHitGizmo->HitHandle) && clicked)
		{
			m_SelectedHandle = closestHitGizmo->HitHandle;
			m_SelectedID = closestHitGizmo->ID;
			// We're in end frame, increment frame index so it will be valid on next update
			closestHitGizmo->SelectedFrame = ImGui::GetFrameCount() + 1;
			selectionChanged = true;
		}
	}
	else
	{
		m_ClosestHoveredGizmo.Reset();
		// Clicked nowhere, deselect
		if (clicked)
		{
			m_SelectedHandle = GizmoHandleID_Invalid;
			m_SelectedID.Reset();
			selectionChanged = true;
		}
	}

	if (selectionChanged && oldSelectedID.HasValue())
	{
		// Same as on selection, set it to next frame
		GizmoEntry* oldSelectedEntry = m_Gizmos.TryGetAt(oldSelectedID.GetValue());
		if (oldSelectedEntry)
			oldSelectedEntry->UnselectedFrame = ImGui::GetFrameCount() + 1;
	}
}

void rageam::gizmo::GizmoManager::SetSelectedID(ConstString id)
{
	GizmoID intID = GetID(id);
	m_SelectedID = intID;
}

void rageam::gizmo::GizmoManager::ResetSelection()
{
	m_SelectedID.Reset();
}

rageam::Vec2S rageam::gizmo::GizmoEntry::GetMousePos() const
{
	return gizmo::GetMousePos() + MousePosOffset;
}

rageam::gizmo::GizmoEntry& rageam::gizmo::GizmoManager::ManipulateInternal(GizmoID id, Mat44V& inOutTransform, GizmoInfo* info, pVoid userData, ConstString parentID)
{
	EASY_BLOCK("GizmoManager::ManipulateInternal");

	bool firstFrame = false;

	// Convert transform to selected space orientation
	Mat44V orientedTransform = ConvertTransformOrientation(inOutTransform, Orientation);

	// Find existing gizmo (if it was manipulated for more than one frame) or create new one
	GizmoEntry* entry;
	GizmoEntry* existingEntry = m_Gizmos.TryGetAt(id);
	if (existingEntry && existingEntry->Info->NameHash == info->NameHash) // Also make sure that gizmo type still matches, otherwise we have to re-create it
	{
		entry = existingEntry;
	}
	else
	{
		GizmoEntry newEntry = {};
		newEntry.ID = id;
		newEntry.Impl = std::move(info->CreateFn());
		newEntry.Info = info;
		entry = &m_Gizmos.EmplaceAt(id, std::move(newEntry));
		firstFrame = true;
	}

	// Finish canceling and make gizmo manipulable again
	if (ImGui::IsKeyReleased(ImGuiKey_Escape) || ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		entry->Canceled = false;

	// Scaling gizmo on distance to camera gives us the same scale on any distance from actual gizmo position
	ScalarV gizmoScale = rage::S_ONE;
	if (entry->Impl->DynamicScale())
	{
		gizmoScale = GIZMO_SCALE * m_Camera.Pos.DistanceTo(orientedTransform.Pos);
		orientedTransform = Mat44V::Scale(gizmoScale) * orientedTransform;
	}

	// Update frame information
	entry->LastFrame = ImGui::GetFrameCount();

	GizmoContext context = {};
	context.Transform = orientedTransform;
	context.Scale = gizmoScale;
	context.Camera = m_Camera;
	context.MouseRay = m_MouseRay;
	context.DebugInfo = DebugInfo;
	context.Orientation = Orientation;
	context.MouseDelta = entry->GetMouseDelta();
	context.UserData = userData;

	if (firstFrame)
		entry->Impl->OnStart(context);
	entry->Impl->Update(context);

	// Don't do hit test if we already manipulating, otherwise we're looking for new gizmo to manipulate
	bool canManipulate = ImGui::IsMouseDown(ImGuiMouseButton_Left) && !entry->Canceled;
	if (!canManipulate)
	{
		entry->Manipulated = false;
		entry->DidHit = false;
		entry->HitDistance = -1.0f;
		entry->HitHandle = GizmoHandleID_Invalid;
		entry->MousePosOffset = Vec2S(0, 0);

		bool canDoHitTest = GetCanSelect() && CanInteract();
		if (canDoHitTest)
		{
			EASY_BLOCK("Hit Test");
			// Find closest hovered handle to the camera
			List<HitResult> hits = entry->Impl->HitTest(context);
			ScalarV closestHit = rage::S_MAX;
			for (HitResult hit : hits)
			{
				if (hit.Distance >= closestHit)
					continue;

				closestHit = hit.Distance;

				entry->HitDistance = hit.Distance;
				entry->HitHandle = hit.Handle;
				entry->DidHit = true;
			}
		}
	}

	bool isHovered = m_ClosestHoveredGizmo == id || entry->Manipulated;
	// Set mouse cursor hand
	if (isHovered)
	{
		// TODO: Mouse cursor switch doesn't seem to work correctly in our ImGui integration
		// ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
	}

	// Perform actual manipulation if gizmo is hovered, this is done only for one gizmo per frame
	if (isHovered && canManipulate)
	{
		EASY_BLOCK("Manipulate");

		bool wasManipulating = entry->Manipulated;
		entry->Manipulated = true;

		POINT cursorPos = {};
		GetCursorPos(&cursorPos);

		// Save initial transform on beginning of drag
		if (!wasManipulating)
		{
			entry->StartTransform = inOutTransform;
			entry->StartTransformOriented = orientedTransform;
			entry->StartMouseRay = m_MouseRay;
			entry->StartMousePos = GetMousePos();
			// Update mouse delta in context because start mouse pos was changed
			context.MouseDelta = entry->GetMouseDelta();

			// Store start monitor work rect for cursor wrapping
			HMONITOR monitor = MonitorFromPoint(cursorPos, MONITOR_DEFAULTTOPRIMARY);
			MONITORINFO monitorInfo;
			monitorInfo.cbSize = sizeof MONITORINFO;
			GetMonitorInfo(monitor, &monitorInfo);
			entry->StartMonitorWorkRect = monitorInfo.rcWork;
		}

		// Cancel manipulation on ESC press / right click
		if (ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			entry->DoCancel(context);
			if (parentID)
			{
				GizmoEntry* parentEntry = m_Gizmos.TryGetAt(GetID(parentID));
				AM_ASSERT(parentEntry, "GizmoManager::ManipulateInternal() -> Parent gizmo ID '%s' is not valid!", parentID);
				AM_ASSERT(parentEntry != entry, "GizmoManager::ManipulateInternal() -> Parent ID matches self ID!");
				parentEntry->DoCancel(context);
			}
			inOutTransform = entry->StartTransform;
			return *entry;
		}

		// Do manipulation itself
		Vec3V scale = Vec3V(1, 1, 1);
		Vec3V translation = Vec3V(0, 0, 0);
		QuatV rotation = rage::QUAT_IDENTITY;
		context.StartTransform = entry->StartTransform;
		context.StartTransformOriented = entry->StartTransformOriented;
		context.StartMouseRay = entry->StartMouseRay;
		context.SelectedHandle = entry->HitHandle;
		entry->Impl->Manipulate(context, translation, scale, rotation);

		// Do cursor wrapping
		if (entry->Impl->WrapCursor())
		{
			RECT& rect = entry->StartMonitorWorkRect;
			POINT oldCursorPos = cursorPos;

			int width = rect.right - rect.left;
			int height = rect.bottom - rect.top;
			if (cursorPos.x >= rect.right - 1)	cursorPos.x -= width - 2;
			if (cursorPos.y >= rect.bottom - 1) cursorPos.y -= height - 1;
			if (cursorPos.x <= rect.left)		cursorPos.x += width - 1;
			if (cursorPos.y <= rect.top)		cursorPos.y += height - 1;

			int deltaX = oldCursorPos.x- cursorPos.x;
			int deltaY = oldCursorPos.y - cursorPos.y;
			if (deltaX || deltaY)
			{
				SetCursorPos(cursorPos.x, cursorPos.y);
				entry->MousePosOffset += Vec2S(static_cast<float>(deltaX), static_cast<float>(deltaY));
			}
		}

		// Compose final transform
		Mat44V outTransform = entry->StartTransform;
		outTransform.Pos = rage::VEC_ZERO;
		outTransform *= Mat44V::Transform(scale, rotation, rage::VEC_ZERO);
		outTransform.Pos = Vec3V(entry->StartTransform.Pos) + translation;
		inOutTransform = outTransform;
	}

	context.SelectedHandle = GizmoHandleID_Invalid;
	if (m_SelectedID == entry->ID)
	{
		// Pass handle of selected gizmo to draw code
		context.SelectedHandle = m_SelectedHandle;
		// Don't pass hovered handle for gizmos that are not actually hovered
		context.HoveredHandle = isHovered ? entry->HitHandle : GizmoHandleID_Invalid;
		context.IsSelected = true;
	}
	else if (isHovered) // This checks for closest hovered gizmo!
	{
		context.SelectedHandle = entry->HitHandle;
		context.HoveredHandle = entry->HitHandle;
	}

	EASY_BLOCK("Draw");
	entry->Impl->Draw(context);
	EASY_END_BLOCK;

	//if (DebugInfo)
	//{
	//	Vec2S offset = entry->MousePosOffset;
	//	Vec2S pos = entry->GetMousePos();
	//	Im3D::TextBg(
	//		inOutTransform.Pos,  ImGui::FormatTemp("Mouse Offset: %.01f %.01f Mouse Pos: %.01f %.01f ", offset.X, offset.Y, pos.X, pos.Y));
	//}

	return *entry;
}

void rageam::gizmo::GizmoManager::Manipulate(ConstString id, Mat44V& inOutTransform, GizmoInfo* info, GizmoState& state, pVoid userData, ConstString parentID)
{
	GizmoEntry& gizmo = ManipulateInternal(GetID(id), inOutTransform, info, userData, parentID);
	state.Manipulated = gizmo.Manipulated;
	state.Hovered = gizmo.HitHandle != GizmoHandleID_Invalid;
	state.Selected = m_SelectedID == gizmo.ID;
	state.JustSelected = gizmo.SelectedFrame == ImGui::GetFrameCount();
	state.JustUnselected = gizmo.UnselectedFrame == ImGui::GetFrameCount();
	state.SelectedHandle = m_SelectedID == gizmo.ID ? m_SelectedHandle : GizmoHandleID_Invalid;
}

bool rageam::gizmo::GizmoManager::Manipulate(ConstString id, Mat44V& inOutTransform, GizmoInfo* info, bool* outIsHovered, pVoid userData, ConstString parentID)
{
	GizmoEntry& gizmo = ManipulateInternal(GetID(id), inOutTransform, info, userData, parentID);
	if (outIsHovered) *outIsHovered = gizmo.HitHandle != GizmoHandleID_Invalid;
	return gizmo.Manipulated;
}
