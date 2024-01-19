//
// File: customeffect_base.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "game/modelinfo/basemodelinfo.h"
#include "rage/framework/entity/customshader.h"

namespace rage
{
	class grmShaderGroup;
}

enum eCustomShaderEffectType
{
	CSE_BASE,
	CSE_PED,
	CSE_ANIMUV,
	CSE_VEHICLE,
	CSE_TINT,
	CSE_PROP,
	CSE_TREE,
	CSE_GRASS,
	CSE_CABLE,
	CSE_MIRROR,
	CSE_WEAPON,
	CSE_INTERIOR,

	CSE_COUNT
};

/**
 * \brief Similar to archetype, provides type template for instantiation.
 */
class CCustomShaderEffectBaseType : public rage::fwCustomShaderEffectBaseType
{
	int m_RefCount = 1;

public:
	CCustomShaderEffectBaseType(u8 type) { m_Type = type; }

	int  GetRefCount() const { return m_RefCount; }
	void AddRef() { m_RefCount++; }
	void RemoveRef() override { if (--m_RefCount == 0) delete this; }

	static CCustomShaderEffectBaseType* SetupMasterForModelInfo(CBaseModelInfo* modelInfo);
};
