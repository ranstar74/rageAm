#include "lodgroup.h"

u32 rage::rmcLod::ComputeBucketMask(const grmShaderGroup* shaderGroup) const
{
	u32 bucketMask = 0;
	for (auto& model : m_Models)
	{
		if (model) bucketMask |= model->ComputeBucketMask(shaderGroup);
	}
	return bucketMask;
}

rage::rmcLodGroup::rmcLodGroup() : m_LodThreshold{ 30, 60, 120, 500 }
{

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
	{
		m_Lods[lod] = new rmcLod();
		m_LodRenderMasks[lod].SetDrawBucket(0);
	}

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

void rage::rmcLodGroup::ComputeBucketMask(const grmShaderGroup* shaderGroup)
{
	for (int i = 0; i < LOD_COUNT; i++)
	{
		if (m_Lods[i].IsNull())
		{
			m_LodRenderMasks[i] = 0;
		}
		else
		{
			// NOTE:
			// This is an ugly hack to force game to always call draw function for every draw bucket
			// This was added because game stores drawable draw bucket on spawn and if we change it -
			// change doesn't reflect to entity. So until we find where it's stored we do this
			// The are no issues in this way except of 'micro performance penalty'
			m_LodRenderMasks[i] = 0xFFFF; // m_Lods[i]->ComputeBucketMask(shaderGroup);
		}
	}
}

void rage::rmcLodGroup::SortForTessellation(const grmShaderGroup* shaderGroup) const
{
	for (int i = 0; i < LOD_COUNT; i++)
	{
		if (m_Lods[i].IsNull())
			continue;

		for (auto& model : m_Lods[i]->GetModels())
		{
			if (model) model->SortForTessellation(shaderGroup);
		}
	}
}

u32 rage::rmcLodGroup::GetBucketMask(int lod) const
{
	// In case if we don't have such lod we search for higher lod mask
	for (int i = lod; i >= 0; i--)
	{
		// Search first non-zero mask
		u32 mask = m_LodRenderMasks[i];
		if (mask != 0)
			return mask & 0xFFFEFFFF; // Mask 16th bit out, why?
	}
	return 0;
}
