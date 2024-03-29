#include "skeletondata.h"

void rage::crSkeletonData::ComputeChildParentIndices()
{
	// TODO: Is this correct way? We need to make sure that this algorithm gives the same output
	atArray<u16> childIndices;
	childIndices.Reserve(m_NumChildParents);

	for (u16 i = 0; i < m_NumBones; i++)
	{
		s32 parentIndex = m_Bones[i].GetParentIndex();
		if (parentIndex == -1)
			continue;

		childIndices.Add(i);
		childIndices.Add(u16(parentIndex));
	}

	m_NumChildParents = childIndices.GetSize();
	m_ChildParentIndices = childIndices.MoveBuffer();
}

void rage::crSkeletonData::ComputeParentIndices()
{
	for (u16 i = 0; i < m_NumBones; i++)
	{
		m_ParentIndices[i] = m_Bones[i].GetParentIndex();
	}
}

void rage::crSkeletonData::BuildBoneMap()
{
	m_TagToIndex.Clear();
	for (u16 i = 0; i < m_NumBones; i++)
	{
		m_TagToIndex.InsertAt(m_Bones[i].GetBoneTag(), i);
	}
}

void rage::crSkeletonData::ComputeTransforms()
{
	for (u16 i = 0; i < m_NumBones; i++)
	{
		crBoneData& bone = m_Bones[i];

		m_DefaultTransforms[i] = Mat44V::Transform(
			bone.GetScale(), bone.GetRotation(), bone.GetTranslation());
		m_DefaultTransformsInverted[i] = m_DefaultTransforms[i].Inverse();
	}
}

rage::crSkeletonData::crSkeletonData()
{
	m_RefCount = 1;
	m_Signature = 0;
	m_SignatureNonChiral = 0;
	m_SignatureComprehensive = 0;
	m_NumBones = 0;
	m_NumChildParents = 0;
	m_Padding = 0;
}

// ReSharper disable once CppPossiblyUninitializedMember
// ReSharper disable CppObjectMemberMightNotBeInitialized
rage::crSkeletonData::crSkeletonData(const datResource& rsc) : m_TagToIndex(rsc), m_Properties(rsc)
{
	// TODO: Since we've got crBoneData[], we can just use count from pointer itself
	m_Bones.PlaceItems(rsc, m_NumBones);
}

rage::crSkeletonData::crSkeletonData(const crSkeletonData& other) : pgBase(other)
{
	m_TagToIndex = other.m_TagToIndex;
	m_Signature = other.m_Signature;
	m_SignatureNonChiral = other.m_SignatureNonChiral;
	m_SignatureComprehensive = other.m_SignatureComprehensive;
	m_NumBones = other.m_NumBones;
	m_NumChildParents = other.m_NumChildParents;

	if (pgRscCompiler::GetCurrent())
		m_RefCount = other.m_RefCount;
	else
		m_RefCount = 1;

	m_Properties.Copy(other.m_Properties);
	m_Bones.Copy(other.m_Bones, other.m_NumBones);
	m_DefaultTransformsInverted.Copy(other.m_DefaultTransformsInverted, other.m_NumBones);
	m_DefaultTransforms.Copy(other.m_DefaultTransforms, other.m_NumBones);
	m_ParentIndices.Copy(other.m_ParentIndices, other.m_NumBones);
	m_ChildParentIndices.Copy(other.m_ChildParentIndices, other.m_NumChildParents);

	m_Padding = 0;
}
// ReSharper restore CppObjectMemberMightNotBeInitialized

rage::crBoneData* rage::crSkeletonData::FindBone(ConstString name)
{
	for (u16 i = 0; i < m_NumBones; i++)
	{
		crBoneData* bone = &m_Bones[i];
		if (String::Equals(bone->GetName(), name))
			return bone;
	}
	return nullptr;
}

rage::crBoneData* rage::crSkeletonData::GetBone(u16 index)
{
	if (index >= m_NumBones) // To handle -1 properly
		return nullptr;

	return &m_Bones[index];
}

rage::Mat44V rage::crSkeletonData::GetBoneWorldTransform(u16 index)
{
	crBoneData* bone = GetBone(index);

	Mat44V world = GetBoneLocalTransform(index);

	// Iterate parent hierarchy
	s32 parentIndex = bone->GetParentIndex();
	while (parentIndex != -1)
	{
		world *= GetBoneLocalTransform(parentIndex);
		parentIndex = GetBone(parentIndex)->GetParentIndex();
	}

	return world;
}

const rage::Mat44V& rage::crSkeletonData::GetBoneLocalTransform(u16 index)
{
	AM_ASSERT(index < m_NumBones, "crSkeletonData::GetBoneTransform() -> Index is out of bounds.");
	return m_DefaultTransforms[index];
}

rage::crBoneData* rage::crSkeletonData::GetFirstChildBone(u16 index)
{
	return GetBone(GetFirstChildBoneIndex(index));
}

u16 rage::crSkeletonData::GetFirstChildBoneIndex(u16 index)
{
	if (index >= m_NumBones) // To handle -1 properly
		return u16(-1);

	for (u16 i = 0; i < m_NumChildParents; i += 2)
	{
		u16 boneIndex = m_ChildParentIndices[i];
		u16 parentBoneIndex = m_ChildParentIndices[i + 1];

		if (parentBoneIndex == index)
			return boneIndex;
	}
	return u16(-1);
}

bool rage::crSkeletonData::GetBoneIndexFromTag(u16 tag, s32& outIndex) const
{
	// Root bone is always the first one with tag 0
	if (tag == 0)
	{
		outIndex = 0;
		return true;
	}

	s32* pIndex = m_TagToIndex.TryGetAt(tag);
	if (pIndex != nullptr)
	{
		outIndex = *pIndex;
		return true;
	}
	outIndex = -1;
	return false;
}

rage::crBoneData* rage::crSkeletonData::GetBoneFromTag(u16 tag)
{
	s32 index;
	GetBoneIndexFromTag(tag, index);
	if (index == -1)
		return nullptr;
	return GetBone(index);
}

void rage::crSkeletonData::Init(u16 numBones)
{
	AM_ASSERT(numBones != 0,
		"crSkeletonData::Init() -> At least one bone is required to initialize skeleton.");

	AM_ASSERT(numBones <= MAX_BONES,
		"crSkeletonData::Init() -> Too much bones %u, maximum allowed %u.", numBones, MAX_BONES);

	m_Bones.AllocItems(numBones);
	m_DefaultTransformsInverted.AllocItems(numBones);
	m_DefaultTransforms.AllocItems(numBones);
	m_ParentIndices.AllocItems(numBones);
	m_ChildParentIndices = nullptr;

	m_NumBones = numBones;
	m_NumChildParents = 0;

	m_TagToIndex.InitAndAllocate(m_NumBones, false);

	for (u16 i = 0; i < numBones; i++)
	{
		m_DefaultTransformsInverted[i] = Mat44V::Identity();
		m_DefaultTransforms[i] = Mat44V::Identity();
		m_ParentIndices[i] = -1;
	}
}

void rage::crSkeletonData::Finalize()
{
	ComputeParentIndices();
	ComputeChildParentIndices();
	BuildBoneMap();
	ComputeTransforms();
}

void rage::crSkeletonData::DebugPrint() const
{
	AM_TRACEF("-- crSkeletonData --");
	AM_TRACEF("- Bone Count: %u", m_NumBones);
	AM_TRACEF("- Signature: %u", m_Signature);
	AM_TRACEF("- Child Indices: %u", m_NumChildParents);
	for (u16 i = 0; i < m_NumChildParents; i++)
		AM_TRACEF("[%u] - %i", i, m_ChildParentIndices[i]);
	AM_TRACEF("- Parent Indices:");
	for (u16 i = 0; i < m_NumBones; i++)
		AM_TRACEF("[%u] - %i", i, m_ParentIndices[i]);
	AM_TRACEF("- Bones:");
	for (u16 i = 0; i < m_NumBones; i++)
	{
		const crBoneData& bone = m_Bones[i];
		AM_TRACEF("[%u] - Name: %s, Tag: %u, Parent: %i, Next: %i",
		          i, bone.GetName(), bone.GetBoneTag(), bone.GetParentIndex(), bone.GetNextIndex());
	}
}
