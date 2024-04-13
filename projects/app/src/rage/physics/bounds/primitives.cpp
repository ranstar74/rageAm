#include "primitives.h"
#include "rage/paging/resource.h"

void rage::phPrimitiveBase::SetType(phPrimitiveType type)
{
	return reinterpret_cast<phPrimitive*>(this)->SetType(type);
}

rage::phPrimitiveType rage::phPrimitiveBase::GetType() const
{
	return reinterpret_cast<const phPrimitive*>(this)->GetType();
}

rage::phPrimitive& rage::phPrimitiveBase::GetPrimitive()
{
	return *reinterpret_cast<phPrimitive*>(this);
}

rage::phPolygon::phPolygon()
{
	if (datResource::IsBuilding())
		return;

	m_NeighboringPolygons[0] = PH_INVALID_INDEX;
	m_NeighboringPolygons[1] = PH_INVALID_INDEX;
	m_NeighboringPolygons[2] = PH_INVALID_INDEX;
	m_VertexIndices[0] = PH_INVALID_INDEX;
	m_VertexIndices[1] = PH_INVALID_INDEX;
	m_VertexIndices[2] = PH_INVALID_INDEX;
	m_Area = 0.0f;
	SetType(PRIM_TYPE_POLYGON);
}

rage::phPolygon::phPolygon(u16 vi1, u16 vi2, u16 vi3)
{
	m_NeighboringPolygons[0] = PH_INVALID_INDEX;
	m_NeighboringPolygons[1] = PH_INVALID_INDEX;
	m_NeighboringPolygons[2] = PH_INVALID_INDEX;
	m_VertexIndices[0] = vi1;
	m_VertexIndices[1] = vi2;
	m_VertexIndices[2] = vi3;
	m_Area = 0.0f;
	SetType(PRIM_TYPE_POLYGON);
}

bool rage::phPolygon::UsesVertexIndex(u32 index) const
{
	if (GetVertexIndex(0) == index) return true;
	if (GetVertexIndex(1) == index) return true;
	if (GetVertexIndex(2) == index) return true;
	return false;
}

void rage::phPolygon::ResetNeighboors()
{
	for (u16& m_NeighboringPolygon : m_NeighboringPolygons)
		m_NeighboringPolygon = PH_INVALID_INDEX;
}

void rage::phPolygon::SetArea(float v)
{
	AM_ASSERTS(GetType() == PRIM_TYPE_POLYGON);
	// See m_AreaAndType in phPrimitive for more details
	// We have to manually re-set primitive type because overwriting area will trash it
	m_Area = v;
	SetType(PRIM_TYPE_POLYGON);
}

rage::phPrimSphere::phPrimSphere()
{
	if (datResource::IsBuilding())
		return;

	m_CenterIndex = PH_INVALID_INDEX;
	m_Radius = 0;
	ZeroMemory(m_Pad0, sizeof m_Pad0);
	ZeroMemory(m_Pad1, sizeof m_Pad1);
	SetType(PRIM_TYPE_SPHERE);
}

rage::phPrimCapsule::phPrimCapsule()
{
	if (datResource::IsBuilding())
		return;

	m_EndIndex0 = PH_INVALID_INDEX;
	m_EndIndex1 = PH_INVALID_INDEX;
	m_Radius = 0;
	ZeroMemory(m_Pad0, sizeof m_Pad0);
	ZeroMemory(m_Pad1, sizeof m_Pad1);
	SetType(PRIM_TYPE_CAPSULE);
}

rage::phPrimBox::phPrimBox()
{
	if (datResource::IsBuilding())
		return;

	m_VertexIndices[0] = PH_INVALID_INDEX;
	m_VertexIndices[1] = PH_INVALID_INDEX;
	m_VertexIndices[2] = PH_INVALID_INDEX;
	m_VertexIndices[3] = PH_INVALID_INDEX;
	ZeroMemory(m_Pad0, sizeof m_Pad0);
	ZeroMemory(m_Pad1, sizeof m_Pad1);
	SetType(PRIM_TYPE_BOX);
}

rage::phPrimCylinder::phPrimCylinder()
{
	if (datResource::IsBuilding())
		return;

	m_EndIndex0 = PH_INVALID_INDEX;
	m_EndIndex1 = PH_INVALID_INDEX;
	m_Radius = 0;
	ZeroMemory(m_Pad0, sizeof m_Pad0);
	ZeroMemory(m_Pad1, sizeof m_Pad1);
	SetType(PRIM_TYPE_CYLINDER);
}

rage::phPrimitive::phPrimitive(phPrimitiveType type)
{
	if (datResource::IsBuilding())
		return;

	m_AreaAndType = 0;
	ZeroMemory(m_Pad, sizeof m_Pad);

	SetType(type);
}

void rage::phPrimitive::SetType(phPrimitiveType type)
{
	m_AreaAndType &= ~TYPE_MASK;
	m_AreaAndType |= type & TYPE_MASK;
}

rage::spdAABB rage::phPrimitive::ComputeBoundingBox(Vector3* vertices)
{
	spdAABB bb(S_MAX, S_MIN);
	switch (GetType())
	{
	case PRIM_TYPE_POLYGON:
	{
		phPolygon& poly = GetPolygon();
		bb = bb.AddPoint(vertices[poly.GetVertexIndex(0)]);
		bb = bb.AddPoint(vertices[poly.GetVertexIndex(1)]);
		bb = bb.AddPoint(vertices[poly.GetVertexIndex(2)]);
		break;
	}
	case PRIM_TYPE_SPHERE:
	{
		phPrimSphere& sphere = GetSphere();
		Vec3V center = vertices[sphere.GetCenterIndex()];
		Vec3V extent = VEC_EXTENT * sphere.GetRadius();
		bb = bb.AddPoint(center - extent);
		bb = bb.AddPoint(center + extent);
		bb = bb.AddPoint(vertices[sphere.GetCenterIndex()]);
		break;
	}
	case PRIM_TYPE_CAPSULE:
	{
		phPrimCapsule& capsule = GetCapsule();
		float radius = capsule.GetRadius();
		Vec3V end0 = vertices[capsule.GetEndIndex0()];
		Vec3V end1 = vertices[capsule.GetEndIndex1()];
		Vec3V extent = VEC_EXTENT * radius;
		bb = bb.AddPoint(end0 - extent);
		bb = bb.AddPoint(end0 + extent);
		bb = bb.AddPoint(end1 - extent);
		bb = bb.AddPoint(end1 + extent);
		break;
	}
	case PRIM_TYPE_BOX:
	{
		phPrimBox& box = GetBox();
		Vec3V vertex0 = vertices[box.GetVertexIndex(0)];
		Vec3V vertex1 = vertices[box.GetVertexIndex(1)];
		Vec3V vertex2 = vertices[box.GetVertexIndex(2)];
		Vec3V vertex3 = vertices[box.GetVertexIndex(3)];
		Vec3V boxX = ((vertex1 + vertex3 - vertex0 - vertex2) * S_QUARTER).Abs();
		Vec3V boxY = ((vertex0 + vertex3 - vertex1 - vertex2) * S_QUARTER).Abs();
		Vec3V boxZ = ((vertex2 + vertex3 - vertex0 - vertex1) * S_QUARTER).Abs();
		Vec3V halfExtent = (boxX + boxY + boxZ);
		Vec3V centroid = (vertex0 + vertex1 + vertex2 + vertex3) * S_QUARTER;
		bb.Min = centroid - halfExtent;
		bb.Max = centroid + halfExtent;
		break;
	}
	case PRIM_TYPE_CYLINDER:
	{
		phPrimCylinder& cylinder = GetCylinder();
		Vec3V endIndex0 = vertices[cylinder.GetEndIndex0()];
		Vec3V endIndex1 = vertices[cylinder.GetEndIndex1()];
		ScalarV radius = cylinder.GetRadius();
		Vec3V dir = (endIndex1 - endIndex0).Normalized();
		Vec3V halfDisc = (S_ONE - dir * dir).Max(S_ZERO).Sqrt() * radius;
		bb.Min = endIndex0.Min(endIndex1) - halfDisc;
		bb.Max = endIndex0.Max(endIndex1) + halfDisc;
		break;
	}

	default:
		break;
	}
	return bb;
}
