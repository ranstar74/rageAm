#include "primitives.h"

#include "geometry.h"
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
