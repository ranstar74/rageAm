#include "boundbase.h"

#include "boundbvh.h"
#include "boundcomposite.h"
#include "boundprimitives.h"

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

// ReSharper disable once CppPossiblyUninitializedMember
rage::phBound::phBound(const datResource& rsc)
{

}

rage::phBoundComposite* rage::phBound::AsComposite()
{
	AM_ASSERTS(m_Type == PH_BOUND_COMPOSITE);
	return reinterpret_cast<phBoundComposite*>(this);
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

rage::phMaterialMgr::Id rage::phBound::GetPrimitiveMaterialId() const
{
	return (m_MaterialID1 << 32) | m_MaterialID0;
}

void rage::phBound::SetPrimitiveMaterialId(phMaterialMgr::Id id)
{
	m_MaterialID0 = id;
	m_MaterialID1 = id >> 32;
}

rage::phOptimizedBvh* rage::phBound::GetBVH() const
{
	if (m_Type == PH_BOUND_BVH)
		return reinterpret_cast<const phBoundBVH*>(this)->GetBVH();
	if (m_Type == PH_BOUND_COMPOSITE)
		return reinterpret_cast<const phBoundComposite*>(this)->GetBVH();
	return nullptr;
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

void rage::phBound::Copy(const phBound* from)
{
	AM_ASSERTS(m_Type == from->m_Type);

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
		case PH_BOUND_BOX:		 return new (that) phBoundBox(rsc);
		case PH_BOUND_SPHERE:	 return new (that) phBoundSphere(rsc);
		case PH_BOUND_CYLINDER:	 return new (that) phBoundCylinder(rsc);
		case PH_BOUND_CAPSULE:	 return new (that) phBoundCapsule(rsc);
		case PH_BOUND_GEOMETRY:	 return new (that) phBoundGeometry(rsc);
		case PH_BOUND_COMPOSITE: return new (that) phBoundComposite(rsc);
		case PH_BOUND_BVH:		 return new (that) phBoundBVH(rsc);

		default:
			AM_UNREACHABLE("phBound::Place() -> Type %u is not supported.", that->m_Type);
	}
}

rage::phBound* rage::phBound::CreateOfType(phBoundType type)
{
	switch (type)
	{
	case PH_BOUND_BOX:		 return new phBoundBox();
	case PH_BOUND_SPHERE:	 return new phBoundSphere();
	case PH_BOUND_CYLINDER:	 return new phBoundCylinder();
	case PH_BOUND_CAPSULE:	 return new phBoundCapsule();
	case PH_BOUND_GEOMETRY:	 return new phBoundGeometry();
	case PH_BOUND_COMPOSITE: return new phBoundComposite();
	case PH_BOUND_BVH:		 return new phBoundBVH();

		default:
			AM_UNREACHABLE("phBound::CreateOfType() -> Type %u is not supported.", type);
	}
}
