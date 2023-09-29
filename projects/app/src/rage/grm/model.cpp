#include "model.h"

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
	m_RenderMask = 0xFF;
	m_GeometryCount = 0;

	SetTesselatedGeometryCount(0);
	SetIsSkinned(false);
}

rage::grmModel::grmModel(const grmModel& other) : m_Geometries(other.m_Geometries)
{
	m_AABBs = grmAABBGroup::Copy(other.m_AABBs, other.m_GeometryCount);

	pgRscCompiler* compiler = pgRscCompiler::GetCurrent();

	if (compiler)
	{
		m_AABBs.AddCompilerRef();
		m_GeometryToMaterial.Snapshot(other.m_GeometryToMaterial);
	}
	else
	{
		m_GeometryToMaterial.Copy(other.m_GeometryToMaterial);
	}

	m_BoneCount = other.m_BoneCount;
	m_Flags = other.m_Flags;
	m_Type = other.m_Type;
	m_BoneIndex = other.m_BoneIndex;
	m_RenderMask = other.m_RenderMask;
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

void rage::grmModel::SortForTessellation(const pgUPtr<grmShaderGroup>& shaderGroup)
{
	//struct SortedGeometry
	//{
	//	pgUPtr<grmGeometryQB>	Geometry;
	//	spdAABB					BoundingBox;
	//	u16						MaterialIndex;
	//	bool					IsTesselated;
	//};

	//atArray<SortedGeometry> temp;
	//temp.Reserve(m_GeometryCount);

	//u16 tesselatedCount = 0;
	//u16 regularCount = 0;
	//u16 geometryIndex = 0;

	//// We copy all existing geometry data to temporary array and then
	//// copy regular geometries first, 
	//for (u16 i = 0; i < m_GeometryCount; i++)
	//{
	//	auto& geometry = m_Geometries[i];
	//	spdAABB& bb = GetGeometryBoundingBox(i);
	//	u16	materialIndex = GetMaterialIndex(i);
	//	bool isTesselated = shaderGroup->GetMaterials()[i]->IsTessellated();

	//	if (isTesselated)	tesselatedCount++;
	//	else				regularCount++;

	//	temp.Construct(std::move(geometry), bb, materialIndex, isTesselated);
	//}

	//auto SetGeometry = [this](u16 index, SortedGeometry& modelGeometry)
	//{
	//	m_Geometries[index] = std::move(modelGeometry.Geometry);
	//	m_GeometryToMaterial.Get()[index] = modelGeometry.MaterialIndex;
	//	GetGeometryBBs()[index] = modelGeometry.BoundingBox;
	//};

	//auto CopyGeometries = [&, this](bool tesselated)
	//{
	//	for (u16 i = 0; i < m_GeometryCount; i++)
	//	{
	//		SortedGeometry& modelGeometry = temp[i];
	//		if (modelGeometry.IsTesselated != tesselated) continue;
	//		SetGeometry(geometryIndex++, modelGeometry);
	//	}
	//};

	//// Place all non-tessellated geometries first,
	//// then loop again and place remaining tessellated ones
	//CopyGeometries(false);
	//CopyGeometries(true);

	//// Now we also can update number of tessellated geometries
	//SetTesselatedGeometryCount(tesselatedCount);
}

void rage::grmModel::DrawPortion(int a2, u32 startGeometryIndex, u32 totalGeometries, const grmShaderGroup* shaderGroup, grcDrawBucketMask mask) const
{
	static auto fn = gmAddress::Scan("48 89 5C 24 08 44 89 4C 24 20 89 54 24 10 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 30")
		.To<void(*)(const grmModel*, int, u32, u32, const grmShaderGroup*, grcDrawBucketMask)>();
	fn(this, a2, startGeometryIndex, totalGeometries, shaderGroup, mask);
}

void rage::grmModel::DrawUntessellatedPortion(const grmShaderGroup* shaderGroup, grcDrawBucketMask mask) const
{
	u32 nonTessellatedGeometriesCount = m_GeometryCount - GetTesselatedGeometryCount();
	if (nonTessellatedGeometriesCount == 0)
		return;
	DrawPortion(0, 0 /* Non tessellated models placed first */, nonTessellatedGeometriesCount, shaderGroup, mask);
}

void rage::grmModel::DrawTesellatedPortion(const grmShaderGroup* shaderGroup, grcDrawBucketMask mask, bool a5) const
{
	u32 tessellatedGeometriesCount = GetTesselatedGeometryCount();
	u32 tessellatedGeometriesStartIdx = m_GeometryCount - tessellatedGeometriesCount; // Tessellated geometries placed after non-tessellated ones
	if (tessellatedGeometriesCount == 0)
		return;
	DrawPortion(a5 ? 2 : 0, tessellatedGeometriesStartIdx, tessellatedGeometriesCount, shaderGroup, mask);
}
