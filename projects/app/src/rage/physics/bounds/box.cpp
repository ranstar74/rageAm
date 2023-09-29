#include "box.h"

rage::phBoundBox::phBoundBox(const Vec3V& size)
{
	m_Type = PH_BOUND_BOX;

	Vec3V halfSize = size * S_HALF;
	m_BoundingBoxMin = -halfSize;
	m_BoundingBoxMax = halfSize;
	phBoundBox::CalculateExtents();
}

rage::phBoundBox::phBoundBox(const Vec3V& min, const Vec3V& max)
{
	m_Type = PH_BOUND_BOX;

	m_BoundingBoxMin = min;
	m_BoundingBoxMax = max;
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
