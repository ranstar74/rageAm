#include "composite.h"

#include "am/integration/memory/address.h"

void rage::phBoundComposite::AssertWithinArray(u16 index) const
{
	AM_ASSERT(index < m_MaxNumBounds,
		"phBoundComposite::AssertWithinArray() -> Index %u out of range (%u).", index, m_MaxNumBounds);
}

rage::phBoundComposite::phBoundComposite()
{
	m_Type = PH_BOUND_COMPOSITE;
	m_MaxNumBounds = 0;
	m_NumBounds = 0;

	m_Margin = 0;
}

// ReSharper disable once CppPossiblyUninitializedMember
// ReSharper disable CppObjectMemberMightNotBeInitialized
rage::phBoundComposite::phBoundComposite(const datResource& rsc) : phBound(rsc)
{
	m_Bounds.PlaceItems(rsc, m_MaxNumBounds);
}
// ReSharper restore CppObjectMemberMightNotBeInitialized

rage::phBoundComposite::~phBoundComposite()
{
	m_Bounds.DestroyItems(m_MaxNumBounds);

	// See Init function, without internal motion m_LastMatrices pointer
	// will refer to m_CurrentMatrices and we'll basically delete the same
	// pointer twice which will lead to crash
	if (!AllowsInternalMotion())
		m_LastMatrices.SuppressDelete();
}

rage::phBoundPtr& rage::phBoundComposite::GetBound(u16 index)
{
	AssertWithinArray(index);
	return m_Bounds[index];
}

void rage::phBoundComposite::SetBound(u16 index, const phBoundPtr& bound)
{
	AssertWithinArray(index);
	m_Bounds[index] = bound;
	m_AABBs[index] = bound->GetBoundingBox();
}

void rage::phBoundComposite::SetTypeFlags(u16 index, CollisionFlags flags)
{
	AssertWithinArray(index);
	m_TypeAndIncludeFlags[index].Type = flags;
	m_OwnedTypeAndIncludeFlags[index].Type = flags;
}

void rage::phBoundComposite::SetIncludeFlags(u16 index, CollisionFlags flags)
{
	AssertWithinArray(index);
	m_TypeAndIncludeFlags[index].Include = flags;
	m_OwnedTypeAndIncludeFlags[index].Include = flags;
}

rage::CollisionFlags rage::phBoundComposite::GetTypeFlags(u16 index)
{
	AssertWithinArray(index);
	return m_TypeAndIncludeFlags[index].Type;
}

rage::CollisionFlags rage::phBoundComposite::GetIncludeFlags(u16 index)
{
	AssertWithinArray(index);
	return m_TypeAndIncludeFlags[index].Include;
}

void rage::phBoundComposite::SetMatrix(u16 index, const Mat44V& mtx)
{
	AssertWithinArray(index);
	m_CurrentMatrices[index] = mtx;
	m_LastMatrices[index] = mtx;
	if (m_Bounds[index])
		m_AABBs[index] = m_Bounds[index]->GetBoundingBox().Transform(mtx);
}

const rage::Mat44V& rage::phBoundComposite::GetMatrix(u16 index)
{
	AssertWithinArray(index);
	return m_LastMatrices[index];
}

void rage::phBoundComposite::InitFromArray(const atArray<phBoundPtr>& bounds, bool allowInternalMotion)
{
	Init(bounds.GetSize(), allowInternalMotion);
	for (u16 i = 0; i < bounds.GetSize(); i++)
	{
		SetBound(i, bounds[i]);
	}
}

void rage::phBoundComposite::Init(u16 numBounds, bool allowInternalMotion)
{
	m_NumBounds = numBounds;
	m_MaxNumBounds = numBounds;

	// Don't use operator[] new here because for types with destructor c++
	// stores array size and rage system allocators don't support it
	m_Bounds.AllocItems(numBounds);
	m_AABBs.AllocItems(numBounds);

	m_OwnedTypeAndIncludeFlags.AllocItems(numBounds);
	m_TypeAndIncludeFlags.AllocItems(numBounds);

	m_CurrentMatrices.AllocItems(numBounds);
	for (u16 i = 0; i < m_MaxNumBounds; i++)
		m_CurrentMatrices[i] = Mat44V::Identity();

	// If no internal motion allowed, last matrices array pointer must be set to current matrices pointer
	if (!allowInternalMotion)
		m_LastMatrices = m_CurrentMatrices.Get(); // This weak reference additionally has to be handled in destructor
	else
		MakeDynamic();
}

void rage::phBoundComposite::MakeDynamic()
{
	m_LastMatrices.AllocItems(m_MaxNumBounds);
	for (u16 i = 0; i < m_MaxNumBounds; i++)
		m_LastMatrices[i] = Mat44V::Identity();
}

void rage::phBoundComposite::CalculateBoundingBox()
{
	spdAABB bb(S_MAX, S_MIN);

	for (u16 i = 0; i < m_MaxNumBounds; i++)
	{
		bb = bb.Merge(m_AABBs[i]);
	}

	m_BoundingBoxMin = bb.Min;
	m_BoundingBoxMax = bb.Max;

	spdSphere bs = bb.ToBoundingSphere();
	m_RadiusAroundCentroid = bs.GetRadius().Get();
	m_CentroidOffset = bs.GetCenter();
}

void rage::phBoundComposite::CalculateVolume()
{
	float totalVolume = 0;
	for (u16 i = 0; i < m_NumBounds; i++)
		totalVolume += m_Bounds[i]->GetVolume();
	m_VolumeDistribution.SetW(totalVolume);
}

rage::phMaterial* rage::phBoundComposite::GetMaterial(int partIndex) const
{
	return GetMaterialFromPartIndexAndComponent(partIndex, 0);
}

u64 rage::phBoundComposite::GetMaterialIdFromPartIndex(int partIndex, int boundIndex) const
{
	const phBoundPtr& bound = m_Bounds[boundIndex];
	if (bound)
		return bound->GetMaterialIdFromPartIndex(partIndex);
	return 0;
}

bool rage::phBoundComposite::CanBecomeActive() const
{
	for (u16 i = 0; i < m_NumBounds; i++)
	{
		const phBoundPtr& bound = m_Bounds[i];
		if (bound && !bound->CanBecomeActive())
			return false;
	}
	return true;
}

void rage::phBoundComposite::CalculateExtents()
{
	CalculateBoundingBox();
	CalculateVolume();
}

rage::phMaterial* rage::phBoundComposite::GetMaterialFromPartIndexAndComponent(int partIndex, int boundIndex) const
{
	const phBoundPtr& bound = m_Bounds[boundIndex];
	if (bound)
		return bound->GetMaterial(partIndex);
	return phMaterialMgr::GetInstance()->GetDefaultMaterial();
}

u64 rage::phBoundComposite::GetMaterialIdFromPartIndexAndComponent(int partIndex, int boundIndex) const
{
	const phBoundPtr& bound = m_Bounds[boundIndex];
	if (bound)
		return bound->GetMaterialIdFromPartIndex(partIndex);
	return 0;
}

u64 rage::phBoundComposite::GetMaterialIdFromPartIndexDefaultComponent(int partIndex) const
{
	return GetMaterialIdFromPartIndexAndComponent(partIndex, 0);
}
