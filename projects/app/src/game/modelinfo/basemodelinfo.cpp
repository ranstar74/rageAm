#include "basemodelinfo.h"

#include "game/drawable.h"
#include "rage/rmc/drawable.h"
#include "rage/rmc/lodgroup.h"
#include "game/shader/customeffect_base.h"
#include "am/integration/memory/address.h"

CBaseModelInfo::~CBaseModelInfo()
{
	if (IsStreamedArchetype())
	{
		// TODO: Streaming bs
	}

	AM_ASSERT(m_BuoyancyInfo == nullptr && m_2dEffects == nullptr, "~CBaseModelInfo -> Shutdown wasn't called.");
}

void CBaseModelInfo::Init()
{
	fwArchetype::Init();

	m_PtfxAssetSlot = -1;
	m_FxEntityFlags = 0;

	m_ShaderEffectType = nullptr;

	m_LodSkelBoneMap = nullptr;
	m_LodSkelBoneCount = 0;

	m_HasPreRenderEffects = false;
	m_NeverDummy = false;
	m_LeakObjectsIntentionally = false;
	m_HasDrawableProxyForWaterReflections = false;
}

void CBaseModelInfo::InitArchetypeFromDefinition(rage::strLocalIndex slot, rage::fwArchetypeDef* def, bool loadModels)
{
	fwArchetype::InitArchetypeFromDefinition(slot, def, loadModels);
	CBaseArchetypeDef* baseDef = dynamic_cast<CBaseArchetypeDef*>(def);
	ConvertLoadFlagsAndAttributes(def->Flags, baseDef->SpecialAttribute);
}

void CBaseModelInfo::SetPhysics(const rage::phArchetypePtr& physics)
{
	// This function is quite big and contains a lot of network code to reimplement
	static auto fn = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"89 44 24 7C 48 8B 84 24 38 02 00 00", "CBaseModelInfo::SetPhysics+0xA1D")
		.GetAt(-0xA1D)
#else
		"41 84 C6 74 7E", "CBaseModelInfo::SetPhysics+0x38")
		.GetAt(-0x38)
#endif
		.ToFunc<void(CBaseModelInfo*, const rage::phArchetypePtr&)>();
	fn(this, physics);
}

bool CBaseModelInfo::WillGenerateBuilding()
{
	// Not CBuilding
	if (m_Flags & FLAG_IS_TYPE_OBJECT)
		return false;

	if (m_Flags & FLAG_HAS_ANIM || GetClipDictionarySlot() != rage::INVALID_STR_INDEX)
		return false;

	return GetModelType() == MI_TYPE_BASE;
}

void CBaseModelInfo::Shutdown()
{
	static auto fn = gmAddress::Scan(
#ifdef APP_BUILD_2699_16_RELEASE_NO_OPT
		"48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 8B D9 E8 ?? ?? ?? ?? 48 8B 03", "CBaseModelInfo::Shutdown")
#else
		"74 1D 66 39 77 0A", "CBaseModelInfo::Shutdown+0x3B")
		.GetAt(-0x3B)
#endif
		.ToFunc<void(fwArchetype*)>();
	fn(this);
}

void CBaseModelInfo::ConvertLoadFlagsAndAttributes(u32 loadFlags, u32 attribute)
{
	using namespace rage;

	SetFlag(MODEL_DRAW_LAST, loadFlags & FLAG_DRAW_LAST);
	SetFlag(MODEL_IS_TYPE_OBJECT, loadFlags & FLAG_IS_TYPE_OBJECT);
	SetFlag(MODEL_DONT_CAST_SHADOWS, loadFlags & FLAG_DONT_CAST_SHADOWS);
	SetFlag(MODEL_SHADOW_PROXY, loadFlags & FLAG_SHADOW_ONLY);
	SetFlag(MODEL_IS_FIXED, loadFlags & FLAG_IS_FIXED);
	SetFlag(MODEL_IS_TREE, loadFlags & FLAG_IS_TREE);
	SetFlag(MODEL_USES_DOOR_PHYSICS, loadFlags & FLAG_DOOR_PHYSICS);
	SetFlag(MODEL_IS_FIXED_FOR_NAVIGATION, loadFlags & FLAG_IS_FIXED_FOR_NAVIGATION);
	SetFlag(MODEL_NOT_AVOIDED_BY_PEDS, loadFlags & FLAG_DONT_AVOID_BY_PEDS);
	SetFlag(MODEL_DOES_NOT_PROVIDE_AI_COVER, loadFlags & FLAG_DOES_NOT_PROVIDE_AI_COVER);
	SetFlag(MODEL_DOES_NOT_PROVIDE_PLAYER_COVER, loadFlags & FLAG_DOES_NOT_PROVIDE_PLAYER_COVER);
	SetFlag(MODEL_DONT_WRITE_ZBUFFER, loadFlags & FLAG_DONT_WRITE_ZBUFFER);
	SetFlag(MODEL_CLIMBABLE_BY_AI, loadFlags & FLAG_PROP_CLIMBABLE_BY_AI);
	SetFlag(MODEL_OVERRIDE_PHYSICS_BOUNDS, loadFlags & FLAG_OVERRIDE_PHYSICS_BOUNDS);
	SetFlag(MODEL_HAS_PRE_REFLECTED_WATER_PROXY, loadFlags & FLAG_HAS_PRE_REFLECTED_WATER_PROXY);
	SetAutoStartAnim(loadFlags & FLAG_AUTOSTART_ANIM);
	SetFlag(MODEL_HAS_ALPHA_SHADOW, loadFlags & FLAG_HAS_ALPHA_SHADOW);
	SetNeverDummy(loadFlags & FLAG_HAS_CLOTH);
	SetFlag(MODEL_USE_AMBIENT_SCALE, loadFlags & FLAG_USE_AMBIENT_SCALE);
	SetFlag(MODEL_HAS_ANIMATION, loadFlags & FLAG_HAS_ANIM);
	SetFlag(MODEL_HAS_UVANIMATION, loadFlags & FLAG_HAS_UVANIM);
	SetFlag(MODEL_HD_TEX_CAPABLE, !(loadFlags & FLAG_SUPPRESS_HD_TXDS));
	SetHasDrawableProxyForWaterReflections(loadFlags & FLAG_HAS_DRAWABLE_PROXY_FOR_WATER_REFLECTIONS);

	if (attribute == FLAG_IS_LADDER_DEPRECATED)
		attribute = MODEL_ATTRIBUTE_IS_LADDER;
	if (attribute == MODEL_ATTRIBUTE_IS_TREE_DEPRECATED)
		SetFlag(MODEL_IS_TREE, true);
	if (attribute == MODEL_ATTRIBUTE_IS_TRAFFIC_LIGHT)
		SetHasPreRenderEffects(true);

	SetAttribute(attribute);
}

void CBaseModelInfo::InitMasterDrawableData(u32 modelSlot)
{
	SetFlag(rage::MODEL_HAS_LOADED, true); // Required by GetDrawable to lookup asset store...
	rage::rmcDrawable* drawable = GetDrawable();
	SetFlag(rage::MODEL_HAS_LOADED, drawable != nullptr);
	if (!drawable)
		return;

	// Initialize bounds if they weren't set...
	if (m_BsRadius == 0.0f)
	{
		const rage::spdSphere& bs = drawable->GetBoundingSphere();
		const rage::spdAABB&   bb = drawable->GetBoundingBox();
		m_BoundingMin = bb.Min;
		m_BoundingMax = bb.Max;
		m_BsCentre = bs.GetCenter();
		m_BsRadius = bs.GetRadius().Get();
	}

	m_ShaderEffectType = CCustomShaderEffectBaseType::SetupMasterForModelInfo(this);

	// This shouldn't be necessary because it's already done in constructor, but just to make sure
	drawable->ComputeBucketMask();

	// TODO: Link RT (is necessary? used rarely...)

	if (GetLightArray()->Any())
		SetHasPreRenderEffects(true);
}

void CBaseModelInfo::DeleteMasterDrawableData()
{
	AM_ASSERT(GetRefCount() == 0, "CBaseModelInfo::DeleteMasterDrawableData() -> There are still %u refs on archetype.",
	          GetRefCount());

	if (m_ShaderEffectType)
	{
		m_ShaderEffectType = nullptr;
	}
	SetFlag(rage::MODEL_HAS_LOADED, false);
}

void CBaseModelInfo::InitWaterSamples()
{
	static auto fn = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"48 89 4C 24 08 55 48 81 EC 20 08", "CBaseModelInfo::InitWaterSamples")
#else
		"45 85 F6 78 72", "CBaseModelInfo::InitWaterSamples+0xB8")
		.GetAt(-0xB8)
#endif
		.ToFunc<void(CBaseModelInfo*)>();
	fn(this);
}

rage::atArray<CLightAttr>* CBaseModelInfo::GetLightArray() const
{
	// TODO: fragType

	gtaDrawable* drawable = static_cast<gtaDrawable*>(GetDrawable());
	if (drawable)
		return &drawable->GetLightArray();

	return nullptr;
}
