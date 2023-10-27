#include "modelinfo.h"
#include "am/integration/memory/address.h"
#include "rage/rmc/lodgroup.h"

void CModelInfo::RegisterModelInfo(rage::fwArchetype* archetype, rage::strLocalIndex slot)
{
	static auto fn = gmAddress::Scan("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 83 3D ?? ?? ?? ?? ?? 8B FA")
		.ToFunc<void(rage::fwArchetype*, rage::strLocalIndex)>();
	fn(archetype, slot);
}

CBaseModelInfo::~CBaseModelInfo()
{
	// TODO: Streaming related code
}

void CBaseModelInfo::Init()
{
	fwArchetype::Init();

	m_TypeAndExtraFlags &= 0x9F;
	m_FlagsA0 &= 0xFFFFFFFC;
	m_ShaderEffect = 0;
	m_Unknown9A = -1;
	m_Unknown9C = 0;
	m_Unknown90 = 0;
	m_Unknown98 = 0;
}

void CBaseModelInfo::InitArchetypeFromDefinition(rage::strLocalIndex slot, rage::fwArchetypeDef* def, bool loadModels)
{
	fwArchetype::InitArchetypeFromDefinition(slot, def, loadModels);

	u32 specialAttribute = def->SpecialAttribute;
	if ((specialAttribute & 0x3F) >= 0x20)
		specialAttribute &= 0x1F;

	SetFlags(def->Flags, specialAttribute);
}

bool CBaseModelInfo::Func0()
{
	if (m_Flags & rage::ATF_DYNAMIC)
		return false;

	if (m_Flags & rage::ATF_BONE_ANIMS)
		return false;

	if (m_DynamicComponent && m_DynamicComponent->ClipDictionarySlot != rage::INVALID_STR_INDEX)
		return false;

	if ((m_TypeAndExtraFlags & 0x1F) != 1)
		return false;

	return true;
}

void CBaseModelInfo::Shutdown()
{
	static auto fn = gmAddress::Scan("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 8B D9 E8 ?? ?? ?? ?? 48 8B 03")
		.ToFunc<void(fwArchetype*)>();
	fn(this);
}

void CBaseModelInfo::SetPhysics(rage::phArchetype* physics)
{
	static auto fn = gmAddress::Scan("48 89 5C 24 08 48 89 6C 24 18 56 57 41 56 48 83 EC 30 48 8B F2")
		.ToFunc<void(fwArchetype*, rage::phArchetype*)>();
	fn(this, physics);
}

void CBaseModelInfo::SetFlags(u32 flags, u32 attributes)
{
	static auto fn = gmAddress::Scan("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 41 8B E8 8B")
		.ToFunc<void(fwArchetype*, u32, u32)>();
	fn(this, flags, attributes);
}

void CBaseModelInfo::RefreshDrawable(int value)
{
	//m_Flags |= 0x1;
	//rage::rmcDrawable* drawable = GetDrawable();
	//if (!drawable)
	//	m_Flags &= ~0x1;

	//drawable->ComputeBucketMask();

	//// Update bounds ::UpdateBoundingVolumes?
	//if(m_BsRadius == 0.0f)
	//{
	//	const rage::rmcLodGroup& lodGroup = drawable->GetLodGroup();
	//	const rage::spdAABB& bb = lodGroup.GetBoundingBox();
	//	const rage::spdSphere& bs = lodGroup.GetBoundingSphere();

	//	m_BoundingMin = bb.Min;
	//	m_BoundingMax = bb.Max;
	//	m_BsCentre = bs.GetCenter();
	//	m_BsRadius = bs.GetRadius().Get();
	//}

	static auto fn = gmAddress::Scan("48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 40 83")
		.ToFunc<void(fwArchetype*, int)>();
	fn(this, value);
}

void CBaseModelInfo::ReleaseDrawable()
{
	static auto fn = gmAddress::Scan("48 89 5C 24 08 57 48 83 EC 20 48 8B 59 70 48 8B F9 48 85 DB 74 25")
		.ToFunc<void(fwArchetype*)>();
	fn(this);
}

void CBaseModelInfo::RefreshFragType(int value)
{
	static auto fn = gmAddress::Scan("48 8B C4 48 89 58 10 48 89 68 18 48 89 70 20 57 41 54 41 55 41 56 41 57 48 83 EC 30 41")
		.ToFunc<void(fwArchetype*, int)>();
	fn(this, value);

}

void CBaseModelInfo::ReleaseFragType()
{
	static auto fn = gmAddress::Scan("48 89 5C 24 08 57 48 83 EC 20 48 8B 59 70 48 8B F9 48 85 DB 74 21 48 8B 1B E8 ?? ?? ?? ?? 48 8B 4F 70 48 8B D0 FF 53 ?? 48 8B 4F 70 48 8B 01 FF 50 ?? 48 83 67 70 00 80")
		.ToFunc<void(fwArchetype*)>();
	fn(this);
}

void CBaseModelInfo::Func8()
{
	static auto fn = gmAddress::Scan("40 55 53 56 57 41 54 41 55 41 56 41 57 48 81 EC 18")
		.ToFunc<void(fwArchetype*)>();
	fn(this);
}
