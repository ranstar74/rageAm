#include "boundbase.h"

#include "box.h"
#include "bvh.h"
#include "composite.h"
#include "geometry.h"
#include "am/integration/memory/address.h"

rage::phBound::phBound()
{
	m_Type = PH_BOUND_UNKNOWN;
	m_Flags = 0;
	m_PartIndex = 0;
	m_RefCount = 1;

	m_RadiusAroundCentroid = 0.0f;
	m_Margin = PH_DEFAULT_MARGIN;

	m_VolumeDistribution = S_ONE;
	m_CentroidOffset = S_ZERO;
	m_BoundingBoxMin = S_ZERO;
	m_BoundingBoxMax = S_ZERO;
	m_CGOffset = S_ZERO;

	m_MaterialID0 = 0;
	m_MaterialID1 = 0;
}

rage::Vec3V rage::phBound::ComputeAngularInertia(float mass) const
{
	return m_VolumeDistribution * mass;
}

float rage::phBound::GetVolume() const
{
	return m_VolumeDistribution.W();
}

void rage::phBound::SetCGOffset(const Vec3V& offset)
{
	m_CGOffset = offset;
}

void rage::phBound::SetCentroidOffset(const Vec3V& offset)
{
	m_CentroidOffset = offset;

	// Recompute bounding box at new center
	Vec3V halfDiagonal = (Vec3V(m_BoundingBoxMax) - Vec3V(m_BoundingBoxMin)) * S_HALF;
	m_BoundingBoxMin = offset - halfDiagonal;
	m_BoundingBoxMax = offset + halfDiagonal;
}

void rage::phBound::ShiftCentroidOffset(const Vec3V& offset)
{
	SetCentroidOffset(Vec3V(m_CentroidOffset) + offset);
}

rage::phMaterial* rage::phBound::GetMaterial(int partIndex) const
{
#ifdef AM_STANDALONE
	return nullptr;
#else
	static auto fn = gmAddress::Scan("8B 41 4C 48 8B 15").ToFunc<phMaterial * (int)>();
	return fn(partIndex);
#endif
}

void rage::phBound::SetMaterial(u64 materialId, int partIndex)
{
	m_MaterialID0 = materialId;
	m_MaterialID1 = materialId >> 32;
}

void rage::phBound::Copy(const phBound* from)
{
	m_Type = from->m_Type;
	m_Flags = from->m_Flags;
	m_PartIndex = from->m_PartIndex;

	m_BoundingBoxMin = from->m_BoundingBoxMin;
	m_BoundingBoxMax = from->m_BoundingBoxMax;
	m_Margin = from->m_Margin;

	m_RadiusAroundCentroid = from->m_RadiusAroundCentroid;
	m_CentroidOffset = from->m_CentroidOffset;
	m_VolumeDistribution = from->m_VolumeDistribution;

	m_RefCount = 1;
}

rage::phBound* rage::phBound::Clone() const
{
	phBound* clone = CreateOfType(m_Type);
	clone->Copy(this);
	return clone;
}

rage::phBound* rage::phBound::Place(const datResource& rsc, phBound* that)
{
	switch (that->m_Type)
	{
	case PH_BOUND_BOX:			return new (that) phBoundBox(rsc);
	case PH_BOUND_GEOMETRY:		return new (that) phBoundGeometry(rsc);
		//case PH_BOUND_BVH:			return new (that) phBoundBVH(rsc);
	case PH_BOUND_COMPOSITE:	return new (that) phBoundComposite(rsc);
	default:
		AM_WARNINGF("phBound::Place() -> Type %u is not supported.", that->m_Type);
		//return nullptr;
		return new (that) phBoundBox(rsc); // TO PREVENT ERROR SPAM
		//AM_UNREACHABLE("phBound::Place() -> Type %u is not supported.", that->m_Type);
	}
}

rage::phBound* rage::phBound::CreateOfType(phBoundType type)
{
	switch (type)
	{
	case PH_BOUND_BOX:	return new phBoundBox();
	default:
		AM_UNREACHABLE("phBound::CreateOfType() -> Type %u is not supported.", type);
	}
}
