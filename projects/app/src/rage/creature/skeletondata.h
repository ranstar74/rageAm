//
// File: skeletondata.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "bonedata.h"
#include "rage/atl/set.h"
#include "rage/math/mtxv.h"
#include "rage/paging/base.h"
#include "rage/paging/ref.h"
#include "rage/paging/place.h"

namespace rage
{
	static constexpr u16 MAX_BONES = 128; // Size of bone array in shader

	class crProperties // TODO: Placeholder
	{
		atSet<u32> m_Properties;

	public:
		crProperties() = default;
		crProperties(const crProperties& other) = default;
		crProperties(const datResource& rsc) : m_Properties(rsc)
		{

		}
	};

	class crSkeletonData : public pgBase
	{
		atSet<s32>				m_TagToIndex;
		pgCArray<crBoneData>	m_Bones;
		pgCArray<Mat44V>		m_DefaultTransformsInverted;
		pgCArray<Mat44V>		m_DefaultTransforms;
		pgCArray<s16>			m_ParentIndices;
		pgCArray<s16>			m_ChildIndices; // Pairs of (BoneIndex, BoneParentIndex)
		crProperties			m_Properties;
		u32						m_Signature;
		s16						m_RefCount;
		u16						m_NumBones;
		u16						m_NumChildIndices;

		void ComputeChildIndices(); // TODO: This need's to be tested on more complex skeletons
		void ComputeParentIndices();
		void BuildBoneMap();
		// Computes transformation matrices & inverses from bones
		void ComputeTransforms();

	public:
		crSkeletonData();
		crSkeletonData(const datResource& rsc);
		crSkeletonData(const crSkeletonData& other);

		u16 GetBoneCount() const { return m_NumBones; }
		crBoneData* GetBone(u16 index);

		const Mat44V& GetBoneTransform(u16 index);

		// Performs linear search for bone with given name
		// Use GetBoneIndexFromTag & pre-computed bone tag if possible
		crBoneData* FindBone(ConstString name);
		crBoneData* GetFirstChildBone(u16 index);
		u16 GetFirstChildBoneIndex(u16 index);

		// Looks up bone index in map and returns whether bone exists
		bool GetBoneIndexFromTag(u16 tag, s32& outIndex) const;

		// Allocates skeleton for given number of bones
		// Once work with skeleton data is done, Finalize function must be called
		void Init(u16 numBones);

		// Must be called once all operations with bones are finished
		// Functions like GetBoneIndexFromTag won't work properly otherwise
		void Finalize();

		void DebugPrint() const
		{
			AM_TRACEF("-- crSkeletonData --");
			AM_TRACEF("- Bone Count: %u", m_NumBones);
			AM_TRACEF("- Signature: %u", m_Signature);
			AM_TRACEF("- Child Indices: %u", m_NumChildIndices);
			for (u16 i = 0; i < m_NumChildIndices; i++)
				AM_TRACEF("[%u] - %i", i, m_ChildIndices[i]);
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

		IMPLEMENT_REF_COUNTER16(crSkeletonData);
	};
}
