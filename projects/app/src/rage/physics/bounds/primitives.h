//
// File: primitives.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "am/system/asserts.h"
#include "rage/spd/aabb.h"

namespace rage
{
	class phPrimitive;

	static constexpr u16 PH_INVALID_INDEX = u16(-1);

	enum phPrimitiveType : u8
	{
		PRIM_TYPE_POLYGON,
		PRIM_TYPE_SPHERE,
		PRIM_TYPE_CAPSULE,
		PRIM_TYPE_BOX,
		PRIM_TYPE_CYLINDER,

		PRIM_TYPE_COUNT
	};

	// All phPrimitive classes are structured like this:
	// Bytes ... Meaning
	// 0-3		 Type
	// 3-128	 Derived class data
	//
	// All classes have the same size and live in one memory block,
	// so structures are hacked together scary way:
	// phPrimitive		- Contains type and padding for derived classes data
	// phPrimitiveBase	- Actual 'base' class (c++ inheritance wise)

	// Bytes reserved by base phPrimitive fields - type and area (24 bits)
	// We expose only GetArea from phPrimitive
	class phPrimitiveBase
	{
	public:
		bool IsPolygon()  const { return GetType() == PRIM_TYPE_POLYGON; }
		bool IsSphere()   const { return GetType() == PRIM_TYPE_SPHERE; }
		bool IsCapsule()  const { return GetType() == PRIM_TYPE_CAPSULE; }
		bool IsBox()      const	{ return GetType() == PRIM_TYPE_BOX; }
		bool IsCylinder() const { return GetType() == PRIM_TYPE_CYLINDER; }

		void SetType(phPrimitiveType type);
		phPrimitiveType GetType() const;
		phPrimitive& GetPrimitive();
	};

	class phPolygon : public phPrimitiveBase
	{
		float m_Area;
		u16   m_VertexIndices[3];
		u16	  m_NeighboringPolygons[3];
	public:
		phPolygon();
		phPolygon(u16 vi1, u16 vi2, u16 vi3);

		bool  GetNormalCode(u32 i) const { return m_VertexIndices[i] & 0x8000; } // Highest bit; TODO: Is normal out?
		u16   GetVertex(u32 i) const { return m_VertexIndices[i]; } // Including normal flag
		u16   GetVertexIndex(u32 i) const { return m_VertexIndices[i] & 0x7FFF; } // All bits except highest
		u16   GetNeighborIndex(u32 i) const { return m_NeighboringPolygons[i]; }
		void  SetNeighborIndex(u32 i, u32 neighbor) { m_NeighboringPolygons[i] = static_cast<u16>(neighbor); }
		bool  UsesVertexIndex(u32 index) const; // Whether any vertex index in this polygon is equals to given one
		void  ResetNeighboors(); // Sets all neighbor indices to invalid
		float GetArea() const { return m_Area; }
		void  SetArea(float v);
	};

	class phPrimSphere : phPrimitiveBase
	{
		u8	  m_Pad0[2];
		u16   m_CenterIndex;
		float m_Radius;
		u8    m_Pad1[8];
	public:
		phPrimSphere();

		void  SetCenter(u16 index) { m_CenterIndex = index; }
		void  SetRadius(float radius) { m_Radius = radius; }
		u16   GetCenterIndex() const { return m_CenterIndex; }
		float GetRadius() const { return m_Radius; }
	};

	class phPrimCapsule : phPrimitiveBase
	{
		u8	  m_Pad0[2];
		u16   m_EndIndex0;
		float m_Radius;
		u16   m_EndIndex1;
		u8    m_Pad1[6];
	public:
		phPrimCapsule();

		u16   GetEndIndex0() const { return m_EndIndex0; }
		u16   GetEndIndex1() const { return m_EndIndex1; }
		float GetRadius() const { return m_Radius; }
		void  SetEndIndex0(u16 index) { m_EndIndex0 = index; }
		void  SetEndIndex1(u16 index) { m_EndIndex1 = index; }
		void  SetRadius(float r) { m_Radius = r; }
	};

	// Oriented bounding box (OBB)
	class phPrimBox : phPrimitiveBase
	{
		u8  m_Pad0[3];
		u16 m_VertexIndices[4];
		u8  m_Pad1[4];
	public:
		phPrimBox();

		u16  GetVertexIndex(int i) const { AM_ASSERTS(i >= 0 && i < 4); return m_VertexIndices[i]; }
		void SetVertexIndex(int i, u16 vi) { AM_ASSERTS(i >= 0 && i < 4); m_VertexIndices[i] = vi; }
	};

	class phPrimCylinder : phPrimitiveBase
	{
		u8	  m_Pad0[2];
		u16   m_EndIndex0;
		float m_Radius;
		u16   m_EndIndex1;
		u8    m_Pad1[6];
	public:
		phPrimCylinder();

		u16   GetEndIndex0() const { return m_EndIndex0; }
		u16   GetEndIndex1() const { return m_EndIndex1; }
		float GetRadius() const { return m_Radius; }
		void  SetEndIndex0(u16 index) { m_EndIndex0 = index; }
		void  SetEndIndex1(u16 index) { m_EndIndex1 = index; }
		void  SetRadius(float r) { m_Radius = r; }
	};

	/**
	 * \brief Base class for physics bounds elements - polygons, boxes, spheres, etc.
	 */
	class phPrimitive
	{
		// This is quite retarded, but in phPolygon area is stored at offset 0 and takes
		// the same offset as the type, and because of that, type takes 3 lowest bits of the area
		static constexpr u32 TYPE_MASK = 0x7;

		u32	m_AreaAndType;
		u16 m_Pad[6]; // Type-specific data (actually starts from 24th bit)

	public:
		phPrimitive(phPrimitiveType type);

		phPrimitiveType GetType() const { return phPrimitiveType(m_AreaAndType & TYPE_MASK); }
		void SetType(phPrimitiveType type);

		// Polygons (triangles) are used only on geometry bound, other types are used on BVH
		bool IsPolygon() const { return GetType() == PRIM_TYPE_POLYGON; }

		phPolygon&		GetPolygon()  { AM_ASSERTS(GetType() == PRIM_TYPE_POLYGON);  return *reinterpret_cast<phPolygon*>(this); }
		phPrimSphere&	GetSphere()	  { AM_ASSERTS(GetType() == PRIM_TYPE_SPHERE);   return *reinterpret_cast<phPrimSphere*>(this); }
		phPrimCapsule&	GetCapsule()  { AM_ASSERTS(GetType() == PRIM_TYPE_CAPSULE);  return *reinterpret_cast<phPrimCapsule*>(this); }
		phPrimBox&		GetBox()	  { AM_ASSERTS(GetType() == PRIM_TYPE_BOX);      return *reinterpret_cast<phPrimBox*>(this); }
		phPrimCylinder& GetCylinder() { AM_ASSERTS(GetType() == PRIM_TYPE_CYLINDER); return *reinterpret_cast<phPrimCylinder*>(this); }
	};

	// Size of all types must match
	static_assert(sizeof(phPrimitive) == 16);
	static_assert(sizeof(phPolygon) == 16);
	static_assert(sizeof(phPrimSphere) == 16);
	static_assert(sizeof(phPrimCapsule) == 16);
	static_assert(sizeof(phPrimBox) == 16);
	static_assert(sizeof(phPrimCylinder) == 16);
}
