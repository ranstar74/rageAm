#include "model.h"
#include "shadergroup.h"
#include "rage/grcore/effect.h"

rage::spdAABB* rage::grmAABBGroup::GetAABBs(u16 bbCount)
{
	return HasCombinedBB(bbCount) ? Combined.AABBs : Single.AABBs;
}

rage::spdAABB& rage::grmAABBGroup::GetCombinedAABB(u16 bbCount)
{
	return HasCombinedBB(bbCount) ? Combined.AABB : Single.AABBs[0];
}

void rage::grmAABBGroup::ComputeCombinedBB(u16 bbCount)
{
	if (!HasCombinedBB(bbCount))
		return;

	Combined.AABB.ComputeFrom(Combined.AABBs, bbCount);
}

u16 rage::grmAABBGroup::GetIncludingCombined(u16 bbCount)
{
	return HasCombinedBB(bbCount) ? bbCount + 1 : bbCount;
}

bool rage::grmAABBGroup::HasCombinedBB(u16 bbCount)
{
	return bbCount > 1;
}

void rage::grmAABBGroup::Add(pgUPtr<grmAABBGroup>& group, u16 bbCount, const spdAABB& bbToAdd)
{
	u16 newBBCount = bbCount + 1;
	pgUPtr<grmAABBGroup> newGroup = Allocate(newBBCount);

	spdAABB* oldAABBs = group->GetAABBs(bbCount);
	spdAABB* newAABBs = newGroup->GetAABBs(newBBCount);

	// Copy old BBs to new array
	for (u16 i = 0; i < bbCount; i++)
		newAABBs[i] = oldAABBs[i];

	// Insert new one at the end
	newAABBs[bbCount] = bbToAdd;

	group = std::move(newGroup);
}

void rage::grmAABBGroup::Remove(const pgUPtr<grmAABBGroup>& group, u16 bbCount, u16 index)
{
	spdAABB* bbs = group->GetAABBs(bbCount);

	// Shift existing BB's
	for (u16 i = index; i < bbCount - 1; i++)
	{
		bbs[i] = bbs[i + 1];
	}
}

rage::pgUPtr<rage::grmAABBGroup> rage::grmAABBGroup::Allocate(u16 bbCount)
{
	if (bbCount == 0)
		return nullptr;

	u32 totalBBs = GetIncludingCombined(bbCount);
	u32 allocSize = totalBBs * sizeof spdAABB;

	pgSnapshotAllocator* virtualAllocator = pgRscCompiler::GetVirtualAllocator();
	pVoid block = virtualAllocator ? virtualAllocator->Allocate(allocSize) : rage_malloc(allocSize);

	return pgUPtr(static_cast<grmAABBGroup*>(block));
}

rage::pgUPtr<rage::grmAABBGroup> rage::grmAABBGroup::Copy(const pgUPtr<grmAABBGroup>& other, u16 bbCount)
{
	pgUPtr<grmAABBGroup> copy = Allocate(bbCount);

	if (HasCombinedBB(bbCount))
	{
		copy->Combined.AABB = other->Combined.AABB;
		for (u16 i = 0; i < bbCount; i++)
			copy->Combined.AABBs[i] = other->Combined.AABBs[i];
	}
	else
	{
		copy->Single.AABBs[0] = other->Single.AABBs[0];
	}

	return copy;
}

rage::grmModel::grmModel()
{
	m_BoneCount = 0;
	m_Flags = 0;
	m_Type = 0;
	m_BoneIndex = 0;
	m_GeometryCount = 0;

	SetTesselatedGeometryCount(0);
	SetIsSkinned(false);
	SetSubDrawBucketMask(
		1 << RB_MODEL_DEFAULT | 
		1 << RB_MODEL_SHADOW |
		1 << RB_MODEL_REFLECTION_PARABOLOID |
		1 << RB_MODEL_REFLECTION_MIRROR |
		1 << RB_MODEL_REFLECTION_WATER);
}

rage::grmModel::grmModel(const grmModel& other) : m_Geometries(other.m_Geometries)
{
	m_AABBs = grmAABBGroup::Copy(other.m_AABBs, other.m_GeometryCount);

	if (pgRscCompiler::GetCurrent())
		m_AABBs.AddCompilerRef();

	m_GeometryToMaterial.Copy(other.m_GeometryToMaterial, other.m_GeometryCount);
	m_BoneCount = other.m_BoneCount;
	m_Flags = other.m_Flags;
	m_Type = other.m_Type;
	m_BoneIndex = other.m_BoneIndex;
	m_ModelMask = other.m_ModelMask;
	m_GeometryCount = other.m_GeometryCount;
	m_TesselatedCountAndIsSkinned = other.m_TesselatedCountAndIsSkinned;

	m_Unknown30 = other.m_Unknown30;
}

const rage::spdAABB& rage::grmModel::GetCombinedBoundingBox() const
{
	return m_AABBs->GetCombinedAABB(m_GeometryCount);
}

const rage::spdAABB& rage::grmModel::GetGeometryBoundingBox(u16 geometryIndex) const
{
	AM_ASSERT(geometryIndex < m_GeometryCount,
		"grmModel::GetGeometryBoundingBox() -> Index %u is out of range.", geometryIndex);

	spdAABB* bbs = m_AABBs->GetAABBs(m_GeometryCount);
	return bbs[geometryIndex];
}

u16 rage::grmModel::GetMaterialIndex(u16 geometryIndex) const
{
	AM_ASSERT(geometryIndex < m_GeometryCount,
		"grmModel::GetMaterialIndex() -> Index %u is out of range.", geometryIndex);

	u16* indices = m_GeometryToMaterial.Get();
	return indices[geometryIndex];
}

void  rage::grmModel::SetMaterialIndex(u16 geometryIndex, u16 materialIndex) const
{
	AM_ASSERT(geometryIndex < m_GeometryCount,
		"grmModel::GetMaterialIndex() -> Index %u is out of range.", geometryIndex);

	u16* indices = m_GeometryToMaterial.Get();
	indices[geometryIndex] = materialIndex;
}

void rage::grmModel::SetIsSkinned(bool skinned, u8 numBones)
{
	m_TesselatedCountAndIsSkinned &= ~1;
	m_Flags &= ~1;
	m_BoneCount = 0;

	if (skinned)
	{
		m_TesselatedCountAndIsSkinned |= 1;
		m_Flags |= 1; // See description in header

		m_BoneCount = numBones;
	}
}

void rage::grmModel::UpdateTessellationDrawBucket()
{
	grcDrawMask mask = GetSubDrawBucketMask();
	mask.SetTessellated(GetTesselatedGeometryCount() > 0);
	SetSubDrawBucketMask(mask);
}

void rage::grmModel::SetSubDrawBucketFlags(u32 flags, bool on)
{
	AM_ASSERT((flags & RB_BASE_BUCKETS_MASK) == 0, "grmModel::SetSubDrawBucketFlags() -> Only model buckets are allowed!");
	u32 mask = GetSubDrawBucketMask();
	mask &= ~flags;
	if (on) mask |= flags;
	SetSubDrawBucketMask(mask);
}

void rage::grmModel::ComputeAABB() const
{
	m_AABBs->ComputeCombinedBB(m_GeometryCount);
}

void rage::grmModel::TransformAABB(const Mat44V& mtx) const
{
	spdAABB* bbs = m_AABBs->GetAABBs(m_GeometryCount);
	for (u16 i = 0; i < m_GeometryCount; i++)
	{
		bbs[i] = bbs[i].Transform(mtx);
	}
	ComputeAABB();
}

rage::pgUPtr<rage::grmGeometryQB>& rage::grmModel::AddGeometry(pgUPtr<grmGeometryQB>& geometry, const spdAABB& bb)
{
	grmAABBGroup::Add(m_AABBs, m_GeometryCount, bb);

	// Reallocate material indices
	u16* oldMaterials = m_GeometryToMaterial.Get();
	u16* newMaterials = new u16[m_GeometryCount + 1];
	memcpy(newMaterials, oldMaterials, m_GeometryCount * sizeof u16);
	newMaterials[m_GeometryCount] = 0; // Default new geometry to material at index 0
	m_GeometryToMaterial = newMaterials;

	m_GeometryCount++;
	return m_Geometries.Emplace(std::move(geometry));
}

void rage::grmModel::RemoveGeometry(u16 index)
{
	grmAABBGroup::Remove(m_AABBs, m_GeometryCount, index);

	// Shift material indices
	u16* materials = m_GeometryToMaterial.Get();
	for (u16 i = 0; i < m_GeometryCount - 1; i++)
		materials[i] = materials[i + 1];

	m_Geometries.RemoveAt(index);
	m_GeometryCount -= 1;
}

void rage::grmModel::SortForTessellation(const grmShaderGroup* shaderGroup)
{
	// Copy all geometries (including their bb's, material indices) to temporary buffer
	// then first copy back non-tessellated geometries, afterwards - tessellated ones

	struct SortedGeometry
	{
		pgUPtr<grmGeometryQB>	Geometry;
		spdAABB					BoundingBox;
		u16						MaterialIndex;
		bool					IsTesselated;
	};

	if (m_GeometryCount == 0)
	{
		SetTesselatedGeometryCount(0);
		return;
	}

	// No need to sort single geometry
	if (m_GeometryCount == 1)
	{
		u16	materialIndex = GetMaterialIndex(0);
		bool isTesselated = shaderGroup->GetShader(materialIndex)->IsTessellated();
		SetTesselatedGeometryCount(isTesselated ? 1 : 0);
		return;
	}

	atArray<SortedGeometry> temp;
	temp.Reserve(m_GeometryCount);

	u16 tesselatedCount = 0;
	u16 regularCount = 0;
	u16 geometryIndex = 0;

	// We copy all existing geometry data to temporary array and then
	// copy regular geometries first, 
	for (u16 i = 0; i < m_GeometryCount; i++)
	{
		auto& geometry = m_Geometries[i];
		auto& bb = GetGeometryBoundingBox(i);
		u16	materialIndex = GetMaterialIndex(i);
		bool isTesselated = shaderGroup->GetShader(materialIndex)->IsTessellated();

		if (isTesselated)	tesselatedCount++;
		else				regularCount++;

		temp.Construct(geometry.Get(), bb, materialIndex, isTesselated);
	}

	auto copyGeometries = [&](bool tesselated)
		{
			for (u16 i = 0; i < m_GeometryCount; i++)
			{
				SortedGeometry& modelGeometry = temp[i];
				if (modelGeometry.IsTesselated == tesselated)
				{
					m_Geometries[geometryIndex].Set(modelGeometry.Geometry.Get());
					m_GeometryToMaterial[geometryIndex] = modelGeometry.MaterialIndex;
					m_AABBs->GetAABBs(m_GeometryCount)[geometryIndex] = modelGeometry.BoundingBox;

					modelGeometry.Geometry.Set(nullptr);

					geometryIndex++;
				}
			}
		};

	// Place all non-tessellated geometries first,
	// then loop again and place remaining tessellated ones
	copyGeometries(false);
	copyGeometries(true);

	// Now we also can update number of tessellated geometries
	SetTesselatedGeometryCount(tesselatedCount);
}

u32 rage::grmModel::ComputeBucketMask(const grmShaderGroup* shaderGroup) const
{
	u32 modelMask = 0;
	for (u16 i = 0; i < m_GeometryCount; i++)
	{
		u16 materialIndex = GetMaterialIndex(i);
		modelMask |= shaderGroup->GetShader(materialIndex)->GetDrawBucketMask();
	}

	modelMask |= (u32)m_ModelMask << 8;
	return modelMask;
}

void rage::grmModel::DrawPortion(int a2, u32 startGeometryIndex, u32 totalGeometries, const grmShaderGroup* shaderGroup, grcDrawMask mask) const
{
	static auto fn = gmAddress::Scan("48 89 5C 24 08 44 89 4C 24 20 89 54 24 10 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 30")
		.To<void(*)(const grmModel*, int, u32, u32, const grmShaderGroup*, grcDrawMask)>();
	fn(this, a2, startGeometryIndex, totalGeometries, shaderGroup, mask);
}

void rage::grmModel::DrawUntessellatedPortion(const grmShaderGroup* shaderGroup, grcDrawMask mask) const
{
	u32 nonTessellatedGeometriesCount = m_GeometryCount - GetTesselatedGeometryCount();
	if (nonTessellatedGeometriesCount == 0)
		return;
	DrawPortion(0, 0 /* Non tessellated models placed first */, nonTessellatedGeometriesCount, shaderGroup, mask);
}

void rage::grmModel::DrawTesellatedPortion(const grmShaderGroup* shaderGroup, grcDrawMask mask, bool a5) const
{
	u32 tessellatedGeometriesCount = GetTesselatedGeometryCount();
	u32 tessellatedGeometriesStartIdx = m_GeometryCount - tessellatedGeometriesCount; // Tessellated geometries placed after non-tessellated ones
	if (tessellatedGeometriesCount == 0)
		return;
	DrawPortion(a5 ? 2 : 0, tessellatedGeometriesStartIdx, tessellatedGeometriesCount, shaderGroup, mask);
}
