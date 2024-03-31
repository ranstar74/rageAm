#include "bvh.h"

rage::phBoundBVH::phBoundBVH()
{
	m_Type = PH_BOUND_BVH;
	m_NumActivePolygons = 0;

	ZeroMemory(m_Pad, sizeof m_Pad);
}

// ReSharper disable once CppPossiblyUninitializedMember
rage::phBoundBVH::phBoundBVH(const datResource& rsc): phBoundGeometry(rsc)
{
			
}

rage::phPrimitive& rage::phBoundBVH::GetPrimitive(int index) const
{
	AM_ASSERTS(index >= 0 && index < m_NumPolygons);
	return m_Polygons[index].GetPrimitive();
}

void rage::phBoundBVH::BuildBVH()
{
	m_BVH = new phOptimizedBvh();
	m_BVH->SetExtents(GetBoundingBox());

	// Added only to polygons
	ScalarV margin = m_Margin;

	// Create BVH primitives
	int primitiveCount = GetPrimitiveCount();
	amPtr<phBvhPrimitiveData[]> primitiveDatas = amPtr<phBvhPrimitiveData[]>(new phBvhPrimitiveData[primitiveCount]);
	for (int i = 0; i < primitiveCount; i++)
	{
		phPrimitive& primitive = GetPrimitive(i);
		phPrimitiveType type = primitive.GetType();

		spdAABB bb(S_MAX, S_MIN);
		Vec3V centroid;
		if (type == PRIM_TYPE_POLYGON)
		{
			phPolygon& polygon = primitive.GetPolygon();
			for (int k = 0; k < 3; k++)
			{
				Vec3V vertex = GetVertex(polygon.GetVertexIndex(k));
				bb.AddPoint(vertex);
				centroid += vertex;
			}
			bb.Min -= margin;
			bb.Max += margin;
			centroid /= 3.0f; // Average
		}
		else if (type == PRIM_TYPE_SPHERE)
		{
			phPrimSphere& sphere = primitive.GetSphere();
			ScalarV radius = sphere.GetRadius();
			Vec3V center = GetVertex(sphere.GetCenterIndex());
			bb.Min = center - radius;
			bb.Max = center + radius;
			centroid = center;
		}
		else if (type == PRIM_TYPE_CAPSULE)
		{
			phPrimCapsule& capsule = primitive.GetCapsule();
			Vec3V endIndex0 = GetVertex(capsule.GetEndIndex0());
			Vec3V endIndex1 = GetVertex(capsule.GetEndIndex1());
			ScalarV radius = capsule.GetRadius();
			bb.Min = bb.Min.Min(endIndex0 - radius);
			bb.Min = bb.Min.Min(endIndex1 - radius);
			bb.Max = bb.Max.Max(endIndex1 + radius);
			bb.Max = bb.Max.Max(endIndex1 + radius);
			centroid = (endIndex0 + endIndex1) * S_HALF; // Average
		}
		else if (type == PRIM_TYPE_BOX)
		{
			phPrimBox& box = primitive.GetBox();
			Vec3V vertex0 = GetVertex(box.GetVertexIndex(0));
			Vec3V vertex1 = GetVertex(box.GetVertexIndex(1));
			Vec3V vertex2 = GetVertex(box.GetVertexIndex(2));
			Vec3V vertex3 = GetVertex(box.GetVertexIndex(3));

			Vec3V boxX = ((vertex1 + vertex3 - vertex0 - vertex2) * S_QUARTER).Abs();
			Vec3V boxY = ((vertex0 + vertex3 - vertex1 - vertex2) * S_QUARTER).Abs();
			Vec3V boxZ = ((vertex2 + vertex3 - vertex0 - vertex1) * S_QUARTER).Abs();
			Vec3V halfExtent = (boxX + boxY + boxZ);

			centroid = (vertex0 + vertex1 + vertex2 + vertex3) * S_QUARTER;
			bb.Min = centroid - halfExtent;
			bb.Max = centroid + halfExtent;
		}
		else if (type == PRIM_TYPE_CYLINDER)
		{
			phPrimCylinder& cylinder = primitive.GetCylinder();
			Vec3V endIndex0 = GetVertex(cylinder.GetEndIndex0());
			Vec3V endIndex1 = GetVertex(cylinder.GetEndIndex1());
			ScalarV radius = cylinder.GetRadius();

			Vec3V dir = (endIndex1 - endIndex0).Normalized();
			Vec3V halfDisc = (S_ONE - dir * dir).Max(S_ZERO).Sqrt() * radius;
			bb.Min = endIndex0.Min(endIndex1) - halfDisc;
			bb.Max = endIndex0.Max(endIndex1) + halfDisc;
		}
		else
		{
			AM_UNREACHABLE("phBoundBVH::BuildBVH() -> Primitive type '%u' is not supported.", type);
		}

		phBvhPrimitiveData& primitiveData = primitiveDatas[i];
		m_BVH->QuantizeMin(primitiveData.AABBMin, bb.Min);
		m_BVH->QuantizeMin(primitiveData.AABBMax, bb.Max);
		m_BVH->QuantizeClosest(primitiveData.Centroid, centroid);
		primitiveData.PrimitiveIndex = i;
	}

	// This is where the tree is built
	m_BVH->BuildFromPrimitiveData(primitiveDatas.get(), primitiveCount, 4);

	// BVH sorts primitives internaly for faster access (they're indexed by start position and count)
	// We have to manually remap bounds:

	amPtr<u16[]> oldToNew = amPtr<u16[]>(new u16[primitiveCount]);
	// Retrieve old inidices from PrimitiveIndex
	for (int i = 0; i < primitiveCount; i++)
		oldToNew[primitiveDatas[i].PrimitiveIndex] = i;

	// Remap all polygon neighbors
	for (int i = 0; i < primitiveCount; i++)
	{
		phPrimitive& primitive = GetPrimitive(i);
		if (!primitive.IsPolygon())
			continue;

		phPolygon& polygon = primitive.GetPolygon();
		for (int k = 0; k < 3; k++)
		{
			u16 neighborIndex = polygon.GetNeighborIndex(i);
			if (neighborIndex == PH_INVALID_INDEX)
				continue;

			polygon.SetNeighborIndex(i, oldToNew[neighborIndex]);
		}
	}

	// Swap primitives and materials 
	for (int lhs = 0; lhs < primitiveCount; lhs++)
	{
		u16 rhs = oldToNew[lhs];
		phPrimitive& primitiveLhs = GetPrimitive(lhs);
		phPrimitive& primitiveRhs = GetPrimitive(rhs);
		std::swap(primitiveLhs, primitiveRhs);
		std::swap(m_PolygonToMaterial[lhs], m_PolygonToMaterial[rhs]);
	}
}