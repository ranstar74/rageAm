#pragma once

#include "rage/framework/archetype.h"

class CBaseModelInfo;

class CModelInfo
{
public:
	static void RegisterModelInfo(rage::fwArchetype* archetype, rage::strLocalIndex slot);
};

class CBaseArchetypeDef : public rage::fwArchetypeDef
{
public:
	CBaseArchetypeDef() = default;
};

class CBaseModelInfo : public rage::fwArchetype
{
	u64 m_Unknown80;
	u64 m_Unknown88;
	u64 m_Unknown90;
	u16 m_Unknown98;
	u16 m_Unknown9A;
	u8	m_Unknown9C;
	u8	m_TypeAndExtraFlags;
	u32 m_FlagsA0;
	u64 m_UnknownA8;

public:
	CBaseModelInfo() = default;
	~CBaseModelInfo() override;

	void Init() override;
	void InitArchetypeFromDefinition(rage::strLocalIndex slot, rage::fwArchetypeDef* def, bool loadModels) override;
	bool Func0() override;
	void Shutdown() override;
	pVoid GetLodSkeletonMap() override { return (pVoid)m_Unknown90; }
	u16 GetLodSkeletonBoneNum() override { return m_Unknown98; }
	void SetPhysics(const rage::pgPtr<rage::phArchetype>& physics) override;

	virtual void SetFlags(u32 flags, u32 attributes);
	virtual int Func4() { return 0; }

	virtual void RefreshDrawable(int);
	virtual void ReleaseDrawable();
	virtual void RefreshFragType(int);
	virtual void ReleaseFragType();

	virtual int Func5() { return 0; }
	virtual u32 Func6() { return u32(-1); }
	virtual int Func7() { return 0; } // Is it always set to 0?
	virtual void Func8();
};
