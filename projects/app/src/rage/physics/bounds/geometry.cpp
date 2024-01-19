#include "geometry.h"

#include "am/graphics/buffereditor.h"
#include "am/graphics/shapetest.h"
#include "am/integration/memory/address.h"
#include "rage/atl/string.h"
#include "rage/math/math.h"
#include "rage/math/mathv.h"

void rage::phBoundPolyhedron::ComputeBoundingBoxCenter()
{
	m_BoundingBoxCenter = (m_BoundingBoxMax + m_BoundingBoxMin) * 0.5f;
}

void rage::phBoundPolyhedron::ComputeNeighbors() const
{
	// Refer to diagram in header to understand things easier

	amUniquePtr<atArray<u32>[]> vertexToPolys =
		std::make_unique<atArray<u32>[]>(m_NumVertices);

	// Generate vertex to polygons map so we can quickly look up all polygons that share vertex at index X
	for (u32 i = 0; i < m_NumPolygons; i++)
	{
		phPrimTriangle& poly = GetPolygon(i);

		if (m_Type == PH_BOUND_BVH && !poly.IsTriangle())
			continue;

		poly.ResetNeighboors();

		// Map each vertex to polygon
		for (u32 k = 0; k < 3; k++)
			vertexToPolys[poly.GetVertexIndex(k)].Add(i);
	}

	auto getNextIndexL = [](u16 index) { return (index + 1) % 3; };
	// Relatively to polygon 1, adjacent polygon 2 indices are incrementing in opposite direction
	// Refer to diagram in header...
	auto getNextIndexR = [](u16 index) { return index == 0 ? 2 : index - 1; };

	for (u32 lhsPolyIdx = 0; lhsPolyIdx < m_NumPolygons; lhsPolyIdx++)
	{
		phPrimTriangle& lhsPoly = GetPolygon(lhsPolyIdx);

		if (m_Type == PH_BOUND_BVH && !lhsPoly.IsTriangle())
			continue;

		for (u32 lhsPolyVertIdx = 0; lhsPolyVertIdx < 3; lhsPolyVertIdx++)
		{
			u16 lhsVertexIdx = lhsPoly.GetVertexIndex(lhsPolyVertIdx);
			u16 lhsVertexIdxNext = lhsPoly.GetVertexIndex(getNextIndexL(lhsPolyVertIdx));

			atArray<u32>& adjacentPolygons = vertexToPolys[lhsVertexIdx];

			for (u32 rhsPolyIdx : adjacentPolygons)
			{
				// Don't check against itself
				if (lhsPolyIdx == rhsPolyIdx)
					continue;

				// Instead of checking polygons with indices 0:7 and 7:0 (lhs:rhs)
				// we only check 0:7 and link both polygons together
				if (rhsPolyIdx <= lhsPolyIdx)
					continue;

				phPrimTriangle& rhsPoly = GetPolygon(rhsPolyIdx);

				// Neighbor polygon must share edge that is defined by
				// current vertex index + next vertex index in phPolygon::VertexIndices
				for (u32 rhsPolyVertIdx = 0; rhsPolyVertIdx < 3; rhsPolyVertIdx++)
				{
					// Check if first index matches
					if (lhsVertexIdx != rhsPoly.GetVertexIndex(rhsPolyVertIdx))
						continue;

					u16 rhsPolyVertIdxNext = getNextIndexR(rhsPolyVertIdx);

					// Check if second edge index matches
					if (lhsVertexIdxNext != rhsPoly.GetVertexIndex(rhsPolyVertIdxNext))
						continue;

					// Match found, link vertex with polygon
					lhsPoly.SetNeighborIndex(lhsPolyVertIdx, rhsPolyIdx);
					rhsPoly.SetNeighborIndex(rhsPolyVertIdxNext, lhsPolyIdx);

					// In order to quickly escape both loops
					goto found;
				}
			}
		found:;
			// Process next vertex in polygon...
		}
	}
}

void rage::phBoundPolyhedron::ComputeUnQuantizeFactor()
{
	Vector3 halfSize = (m_BoundingBoxMax - m_BoundingBoxMin) * 0.5f;
	m_UnQuantizeFactor = halfSize / INT16_MAX;
}

void rage::phBoundPolyhedron::ComputePolyArea() const
{
	for (u32 i = 0; i < m_NumPolygons; i++)
	{
		phPrimTriangle& poly = GetPolygon(i);

		Vec3V v1, v2, v3;
		DecompressPoly(poly, v1, v2, v3);

		Vec3V cross = GetTriCross(v1, v2, v3);
		ScalarV area = cross.Length() * S_HALF;

		poly.SetArea(area.Get());
	}
}

void rage::phBoundPolyhedron::SetOctantMap(phOctantMap* map)
{
	m_OctantIndexCounts = map->IndexCounts;
	m_OctantIndices = map->Indices;
}

void rage::phBoundPolyhedron::OctantMapDelete()
{
	phOctantMap* map = GetOctantMap();
	delete map;

	m_OctantIndexCounts = nullptr;
	m_OctantIndices = nullptr;
}

void rage::phBoundPolyhedron::RecomputeOctantMap()
{
	OctantMapDelete();
	ComputeOctantMap();
}

void rage::phBoundPolyhedron::ComputeOctantMap()
{
	if (m_Type == PH_BOUND_BVH)
		return;

	if (m_NumShrunkVertices == 0)
		return;

	Vec3V* shrunkVertices = new Vec3V[m_NumShrunkVertices];  // TODO: Delete
	for (u32 i = 0; i < m_NumShrunkVertices; i++)
		shrunkVertices[i] = DecompressVertex(i, true);

	// Originally was done using bit masks but converted into simpler form for readability
	static const Vec3V OctantMasks[8] =
	{
		{  1.0f,  1.0f,  1.0f },
		{ -1.0f,  1.0f,  1.0f },
		{  1.0f, -1.0f,  1.0f },
		{ -1.0f, -1.0f,  1.0f },
		{  1.0f,  1.0f, -1.0f },
		{ -1.0f,  1.0f, -1.0f },
		{  1.0f, -1.0f, -1.0f },
		{ -1.0f, -1.0f, -1.0f },
	};

	u32* indexBuffer = new u32[m_NumShrunkVertices]; // TODO: Delete
	u32* indices = indexBuffer;

	u32 octantIndexCounts[8];
	u32* octantIndices[8];

	u32 totalIndexCount = 0;
	for (u32 octantIndex = 0; octantIndex < phOctantMap::MAX_OCTANTS; octantIndex++)
	{
		u32 octantIndexCount = 0;
		const Vec3V& octantDir = OctantMasks[octantIndex];

		for (u32 vertexIndex = 0; vertexIndex < m_NumShrunkVertices; vertexIndex++)
		{
			Vec3V vertex = shrunkVertices[vertexIndex];

			u32 v23 = 0;
			u32 v24 = 0;

			// Add the first vertex
			if (octantIndexCount == 0)
			{
			LABEL_17:
				if (v24 + totalIndexCount >= m_NumShrunkVertices)
					return;
				octantIndexCount = v24 + 1;
				indices[v24] = vertexIndex;
				continue;
			}

			while (true)
			{
				Vec3V toVertex = vertex - shrunkVertices[indices[v23]];
				toVertex *= octantDir;

				bool behind = toVertex <= S_ZERO;
				if (behind)
					break;

				if (toVertex >= S_ZERO == false)
				{
					indices[v24++] = indices[v23];
				}

				++v23;
				if (v23 >= octantIndexCount)
					goto LABEL_17;
			}
		}

		if (octantIndexCount == 0)
			return;

		octantIndices[octantIndex] = indices;
		octantIndexCounts[octantIndex] = octantIndexCount;
		totalIndexCount += octantIndexCount;

		// Move cursor to next octant
		indices += octantIndexCount;
	}
	OctantMapAllocateAndCopy(octantIndexCounts, octantIndices);
}

void rage::phBoundPolyhedron::OctantMapAllocateAndCopy(const u32* indexCounts, u32** indices)
{
	rage::phOctantMap* newMap; // rax
	int64_t octantIndex; // rbx
	u32* pItems; // rdi
	__int64 i; // rsi
	u32 count; // ecx
	__int64 count_; // r8

	newMap = (rage::phOctantMap*)operator new[](
		4i64 * (*indexCounts + indexCounts[1] + indexCounts[2] + indexCounts[3] + indexCounts[4] + indexCounts[5] + indexCounts[6] + indexCounts[7])
		+ 0x80, 8ui64);
	octantIndex = 0i64;
	m_OctantIndexCounts = (u32*)newMap;
	pItems = newMap->IndexBuffer;
	m_OctantIndices = newMap->Indices;
	for (i = 0i64;
		i < 8;
		++i)
	{
		count = indexCounts[i];
		this->m_OctantIndexCounts[i] = count;
		this->m_OctantIndices[octantIndex] = pItems;
		count_ = count;
		pItems = (u32*)(((unsigned __int64)&pItems[count_] + 3) & ~3ui64);
		memmove(this->m_OctantIndices[octantIndex], indices[octantIndex], count_ * 4);
		++octantIndex;
	}
}

rage::phBoundPolyhedron::phBoundPolyhedron()
{
	m_VerticesPad = 0;

	m_OctantIndexCounts = nullptr;
	m_OctantIndices = nullptr;

	m_NumShrunkVertices = 0;
	m_NumVertices = 0;
	m_NumPolygons = 0;

	m_Unknown80 = 0;
	m_Unknown82 = 0;
	m_UnknownB8 = 0;
	m_UnknownD8 = 0;
	m_UnknownE0 = 0;
	m_UnknownE8 = 0;
}

// ReSharper disable once CppPossiblyUninitializedMember
rage::phBoundPolyhedron::phBoundPolyhedron(const datResource& rsc) : phBound(rsc)
{
	if (m_OctantIndexCounts != nullptr)
	{
		rsc.Fixup(m_OctantIndexCounts);
		rsc.Fixup(m_OctantIndices);
		for (u32 i = 0; i < phOctantMap::MAX_OCTANTS; i++)
			rsc.Fixup(m_OctantIndices[i]);
	}
}

void rage::phBoundPolyhedron::DecompressPoly(const phPrimTriangle& poly, Vec3V& v1, Vec3V& v2, Vec3V& v3, bool shrunk) const
{
	v1 = DecompressVertex(poly.GetVertexIndex(0), shrunk);
	v2 = DecompressVertex(poly.GetVertexIndex(1), shrunk);
	v3 = DecompressVertex(poly.GetVertexIndex(2), shrunk);
}

rage::Vec3V rage::phBoundPolyhedron::DecompressVertex(const CompressedVertex& vertex) const
{
	Vec3V packedVec(vertex.X, vertex.Y, vertex.Z);
	return packedVec * m_UnQuantizeFactor + m_BoundingBoxCenter;
}

rage::Vec3V rage::phBoundPolyhedron::DecompressVertex(u32 index, bool shrunked) const
{
	CompressedVertex* verts = shrunked ? m_CompressedShrunkVertices.Get() : m_CompressedVertices.Get();
	return DecompressVertex(verts[index]);
}

rage::phBoundPolyhedron::CompressedVertex rage::phBoundPolyhedron::CompressVertex(const Vec3V& vertex) const
{
	Vec3V quantizeFactor = m_UnQuantizeFactor.Reciprocal();
	Vec3V packedVec = (vertex - m_BoundingBoxCenter) * quantizeFactor;
	return CompressedVertex(
		(s16)packedVec.X(),
		(s16)packedVec.Y(),
		(s16)packedVec.Z());
}

rage::Vec3V rage::phBoundPolyhedron::ComputePolygonNormalEstimate(const phPrimTriangle& poly) const
{
	Vec3V v1 = DecompressVertex(poly.GetVertexIndex(0));
	Vec3V v2 = DecompressVertex(poly.GetVertexIndex(1));
	Vec3V v3 = DecompressVertex(poly.GetVertexIndex(2));
	Vec3V normalSq = GetTriCross(v1, v2, v3);
	float length = poly.GetArea() * 2;
	float lengthInv = 1.0f / length;
	return normalSq * lengthInv;
}

rage::Vec3V rage::phBoundPolyhedron::ComputePolygonNormal(const phPrimTriangle& poly) const
{
	Vec3V v1 = DecompressVertex(poly.GetVertexIndex(0));
	Vec3V v2 = DecompressVertex(poly.GetVertexIndex(1));
	Vec3V v3 = DecompressVertex(poly.GetVertexIndex(2));
	return GetTriNormal(v1, v2, v3);
}

void rage::phBoundPolyhedron::ComputeVertexNormals(Vec3V* outNormals) const
{
	// We are computing average normal by accumulating normals from adjacent to vertex polygons
	// So make sure we aren't accumulating garbage and set all initial normals to zero
	memset(outNormals, 0, m_NumVertices * sizeof Vec3V);

	for (u32 i = 0; i < m_NumPolygons; i++)
	{
		phPrimTriangle& poly = GetPolygon(i);

		u16 vi1 = poly.GetVertexIndex(0);
		u16 vi2 = poly.GetVertexIndex(1);
		u16 vi3 = poly.GetVertexIndex(2);

		Vec3V v1 = DecompressVertex(vi1);
		Vec3V v2 = DecompressVertex(vi2);
		Vec3V v3 = DecompressVertex(vi3);

		Vec3V normal = GetTriNormal(v1, v2, v3);

		outNormals[vi1] += normal;
		outNormals[vi2] += normal;
		outNormals[vi3] += normal;
	}

	// After adding bunch of normals we have to make sure that resulting normal is ... normalized
	for (u32 i = 0; i < m_NumVertices; i++)
	{
		outNormals[i].Normalize();
	}
}

void rage::phBoundPolyhedron::DecompressVertices(Vec3V* outVertices) const
{
	for (u32 i = 0; i < m_NumVertices; i++)
		outVertices[i] = DecompressVertex(i);
}

void rage::phBoundPolyhedron::GetIndices(u16* outIndices) const
{
	for (u32 i = 0; i < m_NumPolygons; i++)
	{
		const phPrimTriangle& poly = m_Polygons[i];
		outIndices[i * 3 + 0] = poly.GetVertexIndex(0);
		outIndices[i * 3 + 1] = poly.GetVertexIndex(1);
		outIndices[i * 3 + 2] = poly.GetVertexIndex(2);
	}
}

rage::phOctantMap* rage::phBoundPolyhedron::GetOctantMap() const
{
	// m_OctantIndexCounts point to phOctantMap::IndexCounts
	phOctantMap* map = (phOctantMap*)m_OctantIndexCounts;
	return map;
}

void rage::phBoundGeometry::ShrinkVerticesByMargin(float margin, Vec3V* outVertices) const
{
	Vec3V* normals = new Vec3V[m_NumVertices];
	ComputeVertexNormals(normals);

	// Shift all vertices in opposite to normal direction by margin distance
	ScalarV negMargin = -margin;
	for (u32 i = 0; i < m_NumVertices; i++)
	{
		outVertices[i] = DecompressVertex(i) + normals[i] * negMargin;
	}

	delete[] normals;
}

void rage::phBoundGeometry::ShrinkPolysByMargin(float margin, Vec3V* outVertices) const
{
	// First one is normal of current polygon and the remaining are normals of all neighbors,
	// for now we assuming that 63 neighbors is enough
	Vec3V normals[64];
	Vec3V* neighborNormals = &normals[1];

	// Pre-decompress all vertices
	for (u32 i = 0; i < m_NumVertices; i++)
		outVertices[i] = DecompressVertex(i);

	u32 processedVertices[2048]{}; // 2048 * 32 = 65536 vertices

	ScalarV negMargin = -margin;
	for (u32 polyIndex = 0; polyIndex < m_NumPolygons; polyIndex++)
	{
		phPrimTriangle& polygon = GetPolygon(polyIndex);
		for (u32 polyVertexIndex = 0; polyVertexIndex < 3; polyVertexIndex++)
		{
			u16 vertexIndex = polygon.GetVertexIndex(polyVertexIndex);

			// Check if this vertex was already processed,
			// can be simplified with set but this is faster
			u32 bucket = vertexIndex >> 5;
			u32 mask = 1 << (polygon.GetVertexID(polyVertexIndex) & 0x1F);
			if (processedVertices[bucket] & mask)
				continue;
			processedVertices[bucket] |= mask;

			Vec3V vertex = outVertices[vertexIndex];

			// Compute normal
			Vec3V normal = ComputePolygonNormal(polygon);
			normals[0] = normal;

			// Compute average normal from all surrounding polygons (that share at least one vertex)
			// and all them in normal list for further weighted normal computation
			Vec3V averageNormal = normal;
			u32 prevNeighbor = polyIndex;
			u32 normalCount = 1;
			u32 neighborCount = 0;
			{
				// Find starting neighbor index
				u16 polyNeighborIndex = (polyVertexIndex + 2) % 3;
				u16 neighbor = polygon.m_NeighboringPolygons[polyNeighborIndex];
				if (neighbor == INVALID_NEIGHBOR)
				{
					neighbor = polygon.GetNeighborIndex(polyVertexIndex);
					polyNeighborIndex = polyVertexIndex;
				}

				// Search for neighbors and accumulate normal...
				while (neighbor != INVALID_NEIGHBOR)
				{
					phPrimTriangle& neighborPoly = GetPolygon(neighbor);
					Vec3V neighborNormal = ComputePolygonNormal(neighborPoly);
					averageNormal += neighborNormal;
					neighborNormals[neighborCount] = neighborNormal;

					normalCount++;
					neighborCount++;

					// Lookup for new neighbor
					u16 newNeighbor = INVALID_NEIGHBOR;
					for (u32 j = 0; j < 3; j++)
					{
						u32 nextIndex = (j + 1) % 3;
						if (neighborPoly.GetVertexIndex(nextIndex) == vertexIndex)
						{
							newNeighbor = neighborPoly.GetNeighborIndex(j);
							if (newNeighbor == prevNeighbor)
								newNeighbor = neighborPoly.GetNeighborIndex(nextIndex);
							prevNeighbor = neighbor;
							neighbor = newNeighbor;
							break;
						}
					}

					// Check if we've closed circle and iterated through all neighbors
					if (newNeighbor == polyIndex)
						break;
				}

				// After adding bunch of normals together we have to re-normalize it
				averageNormal.Normalize();
			}

			if (normalCount == 1)
			{
				outVertices[vertexIndex] = vertex + normal * negMargin;
			}
			else if (normalCount == 2)
			{
				Vec3V cross = normal.Cross(neighborNormals[0]);
				ScalarV crossMagSq = cross.LengthSquared();

				// Very small angle between two normals, just shrink using base normal
				static const ScalarV minMag = { 0.1f };
				if (crossMagSq < minMag)
				{
					outVertices[vertexIndex] = vertex + normal * negMargin;
					continue;
				}

				// Insert new normal in weighted set
				ScalarV lengthInv = crossMagSq.ReciprocalSqrt();
				neighborNormals[1] = cross * lengthInv;
				normalCount = 3;
			}

			if (normalCount < 3)
				continue;

			// The first normal is the base polygon one
			u32 neighborNormalCount = normalCount - 1;

			// Default shrink by average normal
			Vec3V shrunk = vertex + averageNormal * negMargin;

			// Traverse all normal & compute weighted normals
			for (u32 i = 0; i < neighborNormalCount - 1; i++)
			{
				for (u32 j = 0; j < neighborNormalCount - i - 1; j++)
				{
					for (u32 k = 0; k < neighborNormalCount - j - i - 1; k++)
					{
						const Vec3V& normal1 = normals[i];
						const Vec3V& normal2 = normals[i + j + 1];
						const Vec3V& normal3 = normals[i + j + k + 2];

						Vec3V cross23 = normal2.Cross(normal3);
						ScalarV dot = normal1.Dot(cross23);

						// Check out neighbors whose normals direction is too similar (small angle between neighbor normals & polygon normal)
						if (dot.Abs() > S_QUARTER)
						{
							// More neighbors normals are aligned with polygon normal, less weight will be applied
							// Normals with higher angle (closer to 0.25) will contribute more to weighted normal
							ScalarV dotInv = dot.Reciprocal();

							// Compute new weighted normal
							Vec3V cross31 = normal3.Cross(normal1);
							Vec3V cross12 = normal1.Cross(normal2);
							Vec3V newNormal = (cross23 + cross31 + cross12) * dotInv;
							Vec3V newShrink = vertex + newNormal * negMargin;

							// Pick shrunk vertex that's more distant from original vertex
							Vec3V toOld = shrunk - vertex;
							Vec3V toNew = newShrink - vertex;
							if (toNew.LengthSquared() > toOld.LengthSquared())
							{
								shrunk = newShrink;
							}
						}
					}
				}
			}

			// Insert final shrunk vertex in out buffer
			outVertices[vertexIndex] = shrunk;
		}
	}
}

void rage::phBoundGeometry::ShrinkPolysOrVertsByMargin(float margin, float t, Vec3V* outVertices) const
{
	if (t == 0.0f)
	{
		ShrinkPolysByMargin(margin, outVertices);
		return;
	}

	if (t == 1.0f)
	{
		ShrinkVerticesByMargin(margin, outVertices);
		return;
	}

	// We have to allocate another buffer to compute both poly and vertex shrinks
	Vec3V* shrunkVerts = new Vec3V[m_NumVertices];
	ShrinkVerticesByMargin(margin, shrunkVerts);
	ShrinkPolysByMargin(margin, outVertices);
	for (u32 i = 0; i < m_NumVertices; i++)
	{
		outVertices[i].Lerp(shrunkVerts[i], t); // Interpolate between polygons & vertices by T
	}
	delete[] shrunkVerts;
}

bool rage::phBoundGeometry::TryShrinkByMargin(float margin, float t, Vec3V* outShrunkVertices) const
{
	ShrinkPolysOrVertsByMargin(margin, t, outShrunkVertices);

	// In order to do success shrinking, we have to make sure that no polygons collide with each other
	for (u32 vertexIndex = 0; vertexIndex < m_NumVertices; vertexIndex++)
	{
		Vec3V vertex = DecompressVertex(vertexIndex);
		Vec3V shrunkVertex = outShrunkVertices[vertexIndex];

		Vec3V segmentPos = shrunkVertex;
		Vec3V segmentDir = vertex - shrunkVertex;
		ScalarV segmentLength = segmentDir.Length();
		segmentDir /= segmentLength;

		// Line segment intersection
		float maxDistance = segmentLength.Get();
		auto intersectTest = [&](const Vec3V& v1, const Vec3V& v2, const Vec3V& v3)
			{
				float distance;
				if (!rageam::graphics::ShapeTest::RayIntersectsTriangle(segmentPos, segmentDir, v1, v2, v3, distance))
					return false;

				return distance <= maxDistance;
			};

		for (u32 polygonIndex = 0; polygonIndex < m_NumPolygons; polygonIndex++)
		{
			phPrimTriangle& poly = GetPolygon(polygonIndex);

			u16 vi1 = poly.GetVertexIndex(0);
			u16 vi2 = poly.GetVertexIndex(1);
			u16 vi3 = poly.GetVertexIndex(2);

			// Intersection test is done against other polygons, so we must exclude polygons that share current vertex
			if (vi1 == vertexIndex || vi2 == vertexIndex || vi3 == vertexIndex)
				continue;

			// Check intersection against non-shrunk polygon
			Vec3V v1 = DecompressVertex(vi1);
			Vec3V v2 = DecompressVertex(vi2);
			Vec3V v3 = DecompressVertex(vi3);
			if (intersectTest(v1, v2, v3))
				return false;

			// Against shrunk polygon
			Vec3V vs1 = outShrunkVertices[vi1];
			Vec3V vs2 = outShrunkVertices[vi2];
			Vec3V vs3 = outShrunkVertices[vi3];
			if (intersectTest(vs1, vs2, vs3))
				return false;
		}
	}
	return true;
}

rage::phBoundGeometry::phBoundGeometry()
{
	m_Type = PH_BOUND_GEOMETRY;

	m_Materials = nullptr;
	m_MaterialIds = nullptr;
	m_NumMaterials = 0;

	m_UnknownF8 = 0;
	m_Unknown100 = 0;
	m_Unknown108 = 0;
	m_Unknown110 = 0;
	m_Unknown121 = 0;
	m_Unknown122 = 0;
	m_Unknown124 = 0;
	m_Unknown128 = 0;
	m_Unknown12C = 0;
}

// ReSharper disable once CppPossiblyUninitializedMember
rage::phBoundGeometry::phBoundGeometry(const datResource& rsc): phBoundPolyhedron(rsc)
{

}

void rage::phBoundGeometry::PostLoadCompute()
{
	CalculateExtents();
}

void rage::phBoundGeometry::SetCentroidOffset(const Vec3V& offset)
{
	Vec3V shift = offset - Vec3V(m_CentroidOffset);
	if (shift != S_ZERO)
		ShiftCentroidOffset(shift);
}

void rage::phBoundGeometry::ShiftCentroidOffset(const Vec3V& offset)
{
	m_BoundingBoxCenter += offset;
	m_BoundingBoxMin += offset;
	m_BoundingBoxMax += offset;
	m_CentroidOffset += offset;

	CalculateExtents();
}

rage::phMaterial* rage::phBoundGeometry::GetMaterial(int partIndex) const
{
#ifdef AM_STANDALONE
	return nullptr;
#else
	static auto fn = gmAddress::Scan("48 8B 81 F0 00 00 00 48 63").ToFunc<phMaterial * (int)>();
	return fn(partIndex);
#endif
}

void rage::phBoundGeometry::SetMaterial(u64 materialId, int partIndex)
{
	if (partIndex == BOUND_PARTS_ALL || partIndex >= m_NumMaterials)
	{
		for (u32 i = 0; i < m_NumMaterials; i++)
		{
			m_Materials[i] = materialId;
		}
		return;
	}
	m_Materials[partIndex] = materialId;
}

u64 rage::phBoundGeometry::GetMaterialIdFromPartIndex(int partIndex, int boundIndex) const
{
	if (partIndex >= m_NumPolygons)
		return 0;

	u8 materialIdx = m_MaterialIds[partIndex];
	if (materialIdx >= m_NumMaterials)
		return 0;

	return m_Materials[materialIdx];
}

void rage::phBoundGeometry::CalculateExtents()
{
	// TODO: BHV Related code

	CalculateBoundingSphere();
	CalculateVolumeDistribution();
	ComputeBoundingBoxCenter();
}

u64 rage::phBoundGeometry::GetMaterialID(int partIndex)
{
	if (partIndex < m_NumMaterials)
		return m_Materials[partIndex];
	return m_Materials[0];
}

u64 rage::phBoundGeometry::GetPolygonMaterialIndex(int polygon)
{
	return m_MaterialIds[polygon];
}

void rage::phBoundGeometry::SetPolygonMaterialIndex(int polygon, int materialIndex)
{
	m_MaterialIds[polygon] = materialIndex;
}

ConstString rage::phBoundGeometry::GetMaterialName(int materialIndex)
{
#ifdef AM_STANDALONE
	return "";
#else
	static auto fn = gmAddress::Scan("48 8B C4 48 89 58 10 48 89 68 18 48 89 70 20 57 48 83 EC 30 49")
		.ToFunc<ConstString(u64)>();
	return fn(m_Materials[materialIndex]);
#endif
}

void rage::phBoundGeometry::SetMarginAndShrink(float margin, float t)
{
	// We have to make sure that margin won't exceed bounding box dimensions
	Vec3V size = Vec3V(m_BoundingBoxMax) - Vec3V(m_BoundingBoxMin);
	size *= 0.5f; // We must shrink size from both directions so take half of it
	margin = Min(margin, size.X());
	margin = Min(margin, size.Y());
	margin = Min(margin, size.Z());

	m_NumShrunkVertices = m_NumVertices;

	Vec3V* shrunkVertices = new Vec3V[m_NumShrunkVertices];
	while (margin > 0.000001f)
	{
		if (TryShrinkByMargin(margin, t, shrunkVertices))
			break;

		margin /= 2.0f;
	}

	if (m_CompressedShrunkVertices == nullptr)
		m_CompressedShrunkVertices = new CompressedVertex[m_NumShrunkVertices];

	m_Margin = Max(margin, 0.025f);

	Vec3V shrunkBoundingMin = Vec3V(m_BoundingBoxMin) + m_Margin;
	Vec3V shrunkBoundingMax = Vec3V(m_BoundingBoxMax) - m_Margin;

	// Compress & set shrunked vertices
	for (u32 i = 0; i < m_NumShrunkVertices; i++)
	{
		Vec3V vertex = shrunkVertices[i];

		// Although this never should happen, clamp vertex to bounding box
		vertex = vertex.Min(shrunkBoundingMax);
		vertex = vertex.Max(shrunkBoundingMin);

		m_CompressedShrunkVertices[i] = CompressVertex(vertex);
	}

	delete shrunkVertices;
}

void rage::phBoundGeometry::SetMesh(const spdAABB& bb, const Vector3* vertices, const u16* indices, u32 vertexCount, u32 indexCount)
{
	AM_ASSERT(indexCount % 3 == 0, "phBoundGeometry::SetMesh() -> Non triangle mesh with %u indices", indexCount);

	// TODO: Concave check
	// TODO: Init function

	u32 polyCount = indexCount / 3;

	m_CompressedVertices = new CompressedVertex[indexCount];
	m_Polygons = new phPrimTriangle[polyCount];
	m_NumPolygons = polyCount;
	m_NumVertices = vertexCount;

	// TODO: Materials...
	m_NumMaterials = 1;
	m_Materials = new u64[1]{ 56 };
	m_MaterialIds = new u8[m_NumPolygons]{ 0 };

	// TODO: Test...
	m_VolumeDistribution = { 0.333f, 0.333f, 0.333f, 5.0f };

	SetBoundingBox(bb.Min, bb.Max);

	// Compress & set vertices
	for (u32 i = 0; i < vertexCount; i++)
		m_CompressedVertices[i] = CompressVertex(vertices[i]);

	// Create polygons from indices
	for (u32 i = 0; i < polyCount; i++)
	{
		u16 vi1 = indices[i * 3 + 0];
		u16 vi2 = indices[i * 3 + 1];
		u16 vi3 = indices[i * 3 + 2];

		m_Polygons[i] = phPrimTriangle(vi1, vi2, vi3);
	}
	ComputeNeighbors();
	ComputePolyArea();

	SetMarginAndShrink();
	RecomputeOctantMap();
	//auto addr = gmAddress::Scan("E8 ?? ?? ?? ?? 48 03 DE").GetCall();
	//auto ro = addr.To<void(*)(phBoundGeometry*)>();
	//ro(this);

	CalculateVolumeDistribution();
}

void rage::phBoundGeometry::WriteObj(const fiStreamPtr& stream, bool shrunkedVertices) const
{
	stream->WriteLinef("# Bound Geometry; %u Vertices, %u Faces\n", m_NumVertices, m_NumPolygons);

	// Vertices
	stream->WriteLinef("\n# Vertices\n");
	for (u32 i = 0; i < m_NumVertices; i++)
	{
		Vec3V vtx = DecompressVertex(i, shrunkedVertices);
		stream->WriteLinef("v %f %f %f\n", vtx.X(), vtx.Y(), vtx.Z());
	}

	// Faces
	stream->WriteLinef("\n# Faces\n");
	for (u32 i = 0; i < m_NumPolygons; i++)
	{
		phPrimTriangle& poly = GetPolygon(i);

		if (!poly.IsTriangle())
			continue;

		stream->WriteLinef("f %u %u %u\n",
			poly.GetVertexIndex(0) + 1,
			poly.GetVertexIndex(1) + 1,
			poly.GetVertexIndex(2) + 1);
	}
}

void rage::phBoundGeometry::WriteObj(ConstString path, bool shrunkedVertices) const
{
	fiStreamPtr stream = fiStream::Create(path);
	WriteObj(stream, shrunkedVertices);
}

amUniquePtr<rage::Vector3> rage::phBoundGeometry::GetVertices(bool shrunkedVertices) const
{
	Vector3* vertices = new Vector3[m_NumVertices];
	for (u32 i = 0; i < m_NumVertices; i++)
		vertices[i] = DecompressVertex(i, shrunkedVertices);
	return amUniquePtr<Vector3>(vertices);
}

amUniquePtr<u16> rage::phBoundGeometry::GetIndices() const
{
	u16* indices = new u16[GetIndexCount()];
	for (u32 i = 0; i < m_NumPolygons; i++)
	{
		phPrimTriangle& poly = GetPolygon(i);

		indices[i * 3 + 0] = poly.GetVertexIndex(0);
		indices[i * 3 + 1] = poly.GetVertexIndex(1);
		indices[i * 3 + 2] = poly.GetVertexIndex(2);
	}
	return amUniquePtr<u16>(indices);
}

amUniquePtr<char> rage::phBoundGeometry::ComposeVertexBuffer(const grcVertexFormatInfo& vertexInfo) const
{
	rageam::graphics::VertexBufferEditor bufferEditor(vertexInfo);
	bufferEditor.Init(m_NumVertices);

	auto normals = amUniquePtr<Vec3V>(new Vec3V[m_NumVertices]);
	auto vertices = amUniquePtr<Vec3V>(new Vec3V[m_NumVertices]);

	ComputeVertexNormals(normals.get());
	DecompressVertices(vertices.get());

	bufferEditor.SetPositions(vertices.get());
	bufferEditor.SetNormals(normals.get());
	bufferEditor.SetColorSingle(0, 0xFFFFFFFF); // White
	bufferEditor.SetTexcordSingle(0, Vector2(0, 0));

	return amUniquePtr<char>((char*)bufferEditor.MoveBuffer());
}

void rage::phBoundGeometry::PrintOctantMap(bool printIndices) const
{
	if (!GetOctantMap())
	{
		AM_TRACEF("Octant map is not present.");
		return;
	}

	// Print index counts
	AM_TRACEF("Octant | Index Count");
	for (u32 i = 0; i < phOctantMap::MAX_OCTANTS; i++)
	{
		u32 indexCount = m_OctantIndexCounts[i];
		AM_TRACEF("[%u] - %u", i, indexCount);
	}

	// Print indices
	if (printIndices)
	{
		AM_TRACEF("Octant | Indices");
		for (u32 i = 0; i < phOctantMap::MAX_OCTANTS; i++)
		{
			atString indices;
			u32 indexCount = m_OctantIndexCounts[i];
			for (u32 k = 0; k < indexCount; k++)
			{
				u32 shrunkVertexIndex = m_OctantIndices[i][k];
				indices.AppendFormat("%u ", shrunkVertexIndex);
			}
			AM_TRACEF("%s", indices.GetCStr());
		}
	}
}
