#include "boundprimitives.h"

#include "rage/physics/math.h"

void rage::phBoundPrimitive::SetMaterial(phMaterialMgr::Id materialId, int partIndex)
{
	SetPrimitiveMaterialId(materialId);
}

u64 rage::phBoundPrimitive::GetMaterialID() const
{
	return GetPrimitiveMaterialId();
}

rage::phBoundBox::phBoundBox(const spdAABB& aabb)
{
	m_Type = PH_BOUND_BOX;

	// Box AABB must always be centered on centroid offset!
	m_CentroidOffset = aabb.Center();
	m_BoundingBoxMin = aabb.Min;
	m_BoundingBoxMax = aabb.Max;

	phBoundBox::CalculateExtents();
}

void rage::phBoundBox::CalculateExtents()
{
	Vector3 diagonal = m_BoundingBoxMax - m_BoundingBoxMin;

	// Radius of a sphere that inscribes a bounding box is distance to bb corner from bb center
	m_RadiusAroundCentroid = diagonal.Length() * 0.5f;

	// Volume distribution
	Vector3 dSq = diagonal * diagonal;
	Vector3 volumeDistribution(
		(dSq.Y + dSq.Z) / 12.0f,
		(dSq.X + dSq.Z) / 12.0f,
		(dSq.X + dSq.Y) / 12.0f);
	m_VolumeDistribution.SetXYZ(volumeDistribution);

	// Volume
	m_VolumeDistribution.SetW(diagonal.X * diagonal.Y * diagonal.Z);

	// We have to clamp margin for very small boxes
	float margin = diagonal.Min();
	margin /= 8.0f;
	if (m_Margin > margin)
		m_Margin = margin;
}

rage::phBoundSphere::phBoundSphere(const Vec3V& center, float radius)
{
	m_Type = PH_BOUND_SPHERE;
	m_CentroidOffset = center;
	SetRadius(radius);
}

void rage::phBoundSphere::CalculateExtents()
{
	float radius = m_RadiusAroundCentroid;

	m_BoundingBoxMin = m_CentroidOffset - radius;
	m_BoundingBoxMax = m_CentroidOffset + radius;

	m_VolumeDistribution.SetXYZ(CalculateSphereAngularInertia(1.0f, radius));
	m_VolumeDistribution.SetW(CalculateSphereVolume(radius));
}

void rage::phBoundSphere::SetRadius(float r)
{
	m_RadiusAroundCentroid = r;
	m_Margin = r;

	phBoundSphere::CalculateExtents();
}

void rage::phBoundCylinder::CalculateExtents(float radius, float halfHeight)
{
	float height = halfHeight * 2.0f;
	Vec3V center = m_CentroidOffset;

	// Y Axis up
	Vec3V halfExtent = Vec3V(radius, halfHeight, radius);
	m_BoundingBoxMin = center - halfExtent;
	m_BoundingBoxMax = center + halfExtent;

	m_RadiusAroundCentroid = sqrtf(radius * radius + halfHeight * halfHeight);

	m_VolumeDistribution.SetXYZ(CalculateCylinderAngularInertia(1.0f, radius, height));
	m_VolumeDistribution.SetW(CalculateCylinderVolume(radius, height));
}

rage::phBoundCylinder::phBoundCylinder(const Vec3V& center, float radius, float halfHeight)
{
	m_Type = PH_BOUND_CYLINDER;
	m_Margin = Min(PH_DEFAULT_MARGIN, Min(radius, halfHeight) * 0.25f);
	m_CentroidOffset = center;
	CalculateExtents(radius, halfHeight);
}

void rage::phBoundCylinder::CalculateExtents()
{
	float halfHeight = GetHalfHeight();
	float radius = GetRadius();
	CalculateExtents(radius, halfHeight);
}

rage::phBoundCapsule::phBoundCapsule(const Vec3V& center, float radius, float halfHeight)
{
	m_Type = PH_BOUND_CAPSULE;
	m_Margin = radius;
	m_CapsuleHalfHeight = halfHeight;
	m_RadiusAroundCentroid = halfHeight + radius;
	m_CentroidOffset = center;
	phBoundCapsule::CalculateExtents();
}

// ReSharper disable once CppPossiblyUninitializedMember
rage::phBoundCapsule::phBoundCapsule(const datResource& rsc): phBoundPrimitive(rsc)
{
	
}

void rage::phBoundCapsule::CalculateExtents()
{
	float radius = m_Margin;
	float height = m_CapsuleHalfHeight * 2.0f;
	Vec3V center = m_CentroidOffset;

	// Y Axis up
	Vec3V halfExtent(radius, m_RadiusAroundCentroid, radius);
	m_BoundingBoxMin = center - halfExtent;
	m_BoundingBoxMax = center + halfExtent;

	m_VolumeDistribution.SetXYZ(CalculateCapsuleAngularInertia(1.0f, radius, height));
	m_VolumeDistribution.SetW(CalculateCapsuleVolume(radius, height));
}

float rage::phBoundCapsule::GetHalfLenght() const
{
	return m_RadiusAroundCentroid - m_Margin;
}
