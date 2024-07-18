//
// File: skeletondata.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "bonedata.h"
#include "rage/atl/map.h"
#include "rage/math/mtxv.h"
#include "rage/paging/base.h"
#include "rage/paging/ref.h"
#include "rage/paging/place.h"

namespace rage
{
	static constexpr u16 MAX_BONES = 128; // Size of bone array in shader

	class crProperties // TODO: Placeholder
	{
		atMap<u32> m_Properties;

	public:
		crProperties() = default;
		crProperties(const crProperties& other) = default;
		crProperties(const datResource& rsc) : m_Properties(rsc)
		{

		}
	};

	class crSkeletonData : public pgBase
	{
		atMap<s32>             m_TagToIndex;
		pgCArray<crBoneData[]> m_Bones;
		pgCArray<Mat44V>       m_DefaultTransformsInverted;
		pgCArray<Mat44V>       m_DefaultTransforms;
		pgCArray<s16>          m_ParentIndices;
		pgCArray<u16>          m_ChildParentIndices; // Pairs of (BoneIndex, BoneParentIndex)
		pgUPtr<crProperties>   m_Properties;
		u32                    m_Signature;
		u32                    m_SignatureNonChiral;
		u32                    m_SignatureComprehensive;
		s16                    m_RefCount;
		u16                    m_NumBones;
		u16                    m_NumChildParents;
		u16                    m_Padding;

		void ComputeChildParentIndices();
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

		s16 GetParentIndex(u16 boneIndex) const { return m_ParentIndices[boneIndex]; }

		Mat44V		  GetBoneWorldTransform(u16 index);
		const Mat44V& GetBoneLocalTransform(u16 index);
		const Mat44V& GetBoneLocalTransform(const crBoneData* bone) { return GetBoneLocalTransform(bone->GetIndex()); }
		Mat44V		  GetBoneWorldTransform(const crBoneData* bone) { return GetBoneWorldTransform(bone->GetIndex()); }

		// Performs linear search for bone with given name
		// Use GetBoneIndexFromTag & pre-computed bone tag if possible
		crBoneData* FindBone(ConstString name);
		crBoneData* GetFirstChildBone(u16 index);
		u16 GetFirstChildBoneIndex(u16 index);

		// Looks up bone index in map and returns whether bone exists
		bool GetBoneIndexFromTag(u16 tag, s32& outIndex) const;
		crBoneData* GetBoneFromTag(u16 tag);

		// Allocates skeleton for given number of bones
		// Once work with skeleton data is done, Finalize function must be called
		void Init(u16 numBones);

		// Must be called once all operations with bones are finished
		// Functions like GetBoneIndexFromTag won't work properly otherwise
		void Finalize();

		void DebugPrint() const;

		IMPLEMENT_REF_COUNTER16(crSkeletonData);
	};
}
