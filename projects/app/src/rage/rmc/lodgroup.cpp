#include "lodgroup.h"

rage::rmcLodGroup::rmcLodGroup() : m_LodThreshold{ 30, 60, 120, 500 }
{
	m_LodRenderMasks[0].SetDrawBucket(0);
	/*for (int i = 0; i < LOD_COUNT; i++)
		m_Lods[i] = new rmcLod();*/
}

// ReSharper disable once CppPossiblyUninitializedMember
rage::rmcLodGroup::rmcLodGroup(const datResource& rsc)
{

}

rage::rmcLodGroup::rmcLodGroup(const rmcLodGroup& other)
{
	Copy(other);
}

void rage::rmcLodGroup::Copy(const rmcLodGroup& from)
{
	m_BoundingSphere = from.m_BoundingSphere;
	m_BoundingBox = from.m_BoundingBox;

	for (int i = 0; i < LOD_COUNT; i++)
	{
		if (pgRscCompiler::GetCurrent())
			m_Lods[i].Snapshot(from.m_Lods[i]);
		else
			m_Lods[i].Copy(from.m_Lods[i]);

		m_LodThreshold[i] = from.m_LodThreshold[i];
		m_LodRenderMasks[i] = from.m_LodRenderMasks[i];
	}
}

rage::rmcLod* rage::rmcLodGroup::GetLod(int lod)
{
	// Allocate lod on demand
	if (m_Lods[lod] == nullptr)
		m_Lods[lod] = new rmcLod();

	return m_Lods[lod].Get();
}

int rage::rmcLodGroup::GetLodCount() const
{
	for (int i = 0; i < LOD_COUNT; i++)
	{
		if (i + 1 == LOD_COUNT)
			return LOD_COUNT;

		if (m_Lods[i + 1].IsNull())
			return i + 1;
	}
	return 0;
}

void rage::rmcLodGroup::CalculateExtents()
{
	spdAABB bb = { S_MAX, S_MIN };

	// Combine AABB of all lod models
	for (int i = 0; i < LOD_COUNT; i++)
	{
		if (m_Lods[i] == nullptr)
			continue;

		for (const pgUPtr<grmModel>& model : m_Lods[i]->GetModels())
		{
			bb = bb.Merge(model->GetCombinedBoundingBox());
		}
	}

	m_BoundingBox = bb;
	m_BoundingSphere = bb.ToBoundingSphere();
}
