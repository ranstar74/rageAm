//
// File: bonedata.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "hash.h"
#include "common/types.h"
#include "rage/math/quatv.h"
#include "rage/math/vecv.h"
#include "rage/math/mtxv.h"
#include "rage/paging/resource.h"
#include "rage/atl/conststring.h"

namespace rage
{
	enum eBoneDof : u16
	{
		BD_ROT_X = 1 << 0,
		BD_ROT_Y = 1 << 1,
		BD_ROT_Z = 1 << 2,
		BD_LIMIT_ROT = 1 << 3,
		BD_TRANS_X = 1 << 4,
		BD_TRANS_Y = 1 << 5,
		BD_TRANS_Z = 1 << 6,
		BD_LIMIT_TRANS = 1 << 7,
		BD_SCALE_X = 1 << 8,
		BD_SCALE_Y = 1 << 9,
		BD_SCALE_Z = 1 << 10,
		BD_LIMIT_SCALE = 1 << 11,
	};

	static constexpr u16 BONE_DOF_ALL =
		BD_ROT_X | BD_ROT_Y | BD_ROT_Z |
		BD_TRANS_X | BD_TRANS_Y | BD_TRANS_Z |
		BD_SCALE_X | BD_SCALE_Y | BD_SCALE_Z;

	class crBoneData
	{
		QuatV			m_DefaultRotation;
		Vec3V			m_DefaultTranslation;
		Vec3V			m_DefaultScale;
		s16				m_Next;					// Index of next bone within parent child group
		s16				m_Parent;				// Index of parent bone
		atConstString	m_Name;
		eBoneDof		m_Dofs;
		u16				m_Index;
		u16				m_BoneId;
		u16				m_MirrorIndex;			// Always seem to match m_Index. On PEDs skeleton may be?

		void ComputeBoneTag()
		{
			// Root bones always have tag 0
			if (m_Index == 0)
			{
				m_BoneId = 0;
				return;
			}

			m_BoneId = crBoneHash(m_Name);
		}

	public:
		crBoneData()
		{
			m_Next = -1;
			m_Parent = -1;
			m_BoneId = 0;

			m_DefaultScale = S_ONE;

			SetIndex(0);
			SetDofs(BONE_DOF_ALL);
		}

		// ReSharper disable once CppPossiblyUninitializedMember
		crBoneData(const datResource& rsc)
		{

		}

		s32 GetParentIndex() const { return m_Parent; }
		s32 GetNextIndex() const { return m_Next; }
		void SetParentIndex(s32 index) { m_Parent = index; }
		void SetNextIndex(s32 index) { m_Next = index; }

		u16 GetIndex() const { return m_Index; }
		void SetIndex(u16 index)
		{
			m_Index = index;
			m_MirrorIndex = index; // For now...
		}

		ConstString GetName() const { return m_Name; }
		// Sets bone name and computes tag
		void SetName(ConstString name)
		{
			m_Name = name;
			ComputeBoneTag();
		}

		u16 GetBoneTag() const { return m_BoneId; }

		eBoneDof GetDofs() const { return m_Dofs; }
		void SetDofs(u16 value) { m_Dofs = eBoneDof(value); }

		// Unwanted parameters can be set to NULL
		void SetTransform(const Vec3V* trans, const Vec3V* scale, const QuatV* rot)
		{
			if (trans) m_DefaultTranslation = *trans;
			if (scale) m_DefaultScale = *scale;
			if (rot) m_DefaultRotation = *rot;
		}

		void SetTransform(const Mat44V& mtx)
		{
			mtx.Decompose(&m_DefaultTranslation, &m_DefaultScale, &m_DefaultRotation);
		}

		const Vec3V& GetScale() const { return m_DefaultScale; }
		const Vec3V& GetTranslation() const { return m_DefaultTranslation; }
		const QuatV& GetRotation() const { return m_DefaultRotation; }
	};
}
