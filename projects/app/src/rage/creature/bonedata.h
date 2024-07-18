//
// File: bonedata.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
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

#include "am/system/enum.h"

namespace rage
{
	enum eBoneDof : u16
	{
		DOF_NONE                 = 0,
		DOF_ROTATE_X             = 1 << 0,
		DOF_ROTATE_Y             = 1 << 1,
		DOF_ROTATE_Z             = 1 << 2,
		DOF_HAS_ROTATE_LIMITS    = 1 << 3,
		DOF_TRANSLATE_X          = 1 << 4,
		DOF_TRANSLATE_Y          = 1 << 5,
		DOF_TRANSLATE_Z          = 1 << 6,
		DOF_HAS_TRANSLATE_LIMITS = 1 << 7,
		DOF_SCALE_X              = 1 << 8,
		DOF_SCALE_Y              = 1 << 9,
		DOF_SCALE_Z              = 1 << 10,
		DOF_HAS_SCALE_LIMITS     = 1 << 11,
		DOF_HAS_CHILD            = 1 << 12,
		DOF_IS_SKINNED           = 1 << 13,
		DOF_ROTATION             = 0x7,
		DOF_TRANSLATION          = 0x70,
		DOF_SCALE                = 0x700,
		DOF_ALL                  = 0x777,
	};
	inline ConstString ToString(eBoneDof e);
	inline bool FromString(ConstString str, eBoneDof& e);
	IMPLEMENT_FLAGS_TO_STRING(eBoneDof, "DOF_");

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
			SetDofs(DOF_ALL);
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

	ConstString ToString(eBoneDof e)
	{
		switch (e)
		{
		case DOF_NONE: return "DOF_NONE";
		case DOF_ROTATE_X: return "DOF_ROTATE_X";
		case DOF_ROTATE_Y: return "DOF_ROTATE_Y";
		case DOF_ROTATE_Z: return "DOF_ROTATE_Z";
		case DOF_HAS_ROTATE_LIMITS: return "DOF_HAS_ROTATE_LIMITS";
		case DOF_TRANSLATE_X: return "DOF_TRANSLATE_X";
		case DOF_TRANSLATE_Y: return "DOF_TRANSLATE_Y";
		case DOF_TRANSLATE_Z: return "DOF_TRANSLATE_Z";
		case DOF_HAS_TRANSLATE_LIMITS: return "DOF_HAS_TRANSLATE_LIMITS";
		case DOF_SCALE_X: return "DOF_SCALE_X";
		case DOF_SCALE_Y: return "DOF_SCALE_Y";
		case DOF_SCALE_Z: return "DOF_SCALE_Z";
		case DOF_HAS_SCALE_LIMITS: return "DOF_HAS_SCALE_LIMITS";
		case DOF_HAS_CHILD: return "DOF_HAS_CHILD";
		case DOF_IS_SKINNED: return "DOF_IS_SKINNED";
		case DOF_ROTATION: return "DOF_ROTATION";
		case DOF_TRANSLATION: return "DOF_TRANSLATION";
		case DOF_SCALE: return "DOF_SCALE";
		case DOF_ALL: return "DOF_ALL";
		}
		AM_UNREACHABLE("Failed to convert eBoneDof (%i)", e);
	}

	bool FromString(ConstString str, eBoneDof& e)
	{
		u32 key = atStringHash(str);
		switch (key)
		{
		case atStringHash("DOF_NONE"): e = DOF_NONE; return true;
		case atStringHash("DOF_ROTATE_X"): e = DOF_ROTATE_X; return true;
		case atStringHash("DOF_ROTATE_Y"): e = DOF_ROTATE_Y; return true;
		case atStringHash("DOF_ROTATE_Z"): e = DOF_ROTATE_Z; return true;
		case atStringHash("DOF_HAS_ROTATE_LIMITS"): e = DOF_HAS_ROTATE_LIMITS; return true;
		case atStringHash("DOF_TRANSLATE_X"): e = DOF_TRANSLATE_X; return true;
		case atStringHash("DOF_TRANSLATE_Y"): e = DOF_TRANSLATE_Y; return true;
		case atStringHash("DOF_TRANSLATE_Z"): e = DOF_TRANSLATE_Z; return true;
		case atStringHash("DOF_HAS_TRANSLATE_LIMITS"): e = DOF_HAS_TRANSLATE_LIMITS; return true;
		case atStringHash("DOF_SCALE_X"): e = DOF_SCALE_X; return true;
		case atStringHash("DOF_SCALE_Y"): e = DOF_SCALE_Y; return true;
		case atStringHash("DOF_SCALE_Z"): e = DOF_SCALE_Z; return true;
		case atStringHash("DOF_HAS_SCALE_LIMITS"): e = DOF_HAS_SCALE_LIMITS; return true;
		case atStringHash("DOF_HAS_CHILD"): e = DOF_HAS_CHILD; return true;
		case atStringHash("DOF_IS_SKINNED"): e = DOF_IS_SKINNED; return true;
		case atStringHash("DOF_ROTATION"): e = DOF_ROTATION; return true;
		case atStringHash("DOF_TRANSLATION"): e = DOF_TRANSLATION; return true;
		case atStringHash("DOF_SCALE"): e = DOF_SCALE; return true;
		case atStringHash("DOF_ALL"): e = DOF_ALL; return true;
		}
		return false;
	}
}
