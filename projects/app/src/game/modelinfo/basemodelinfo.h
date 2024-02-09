//
// File: basemodelinfo.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/framework/entity/archetype.h"

struct CLightAttr;
class CTimeInfo;
class CCustomShaderEffectBaseType;

enum ModelInfoType
{
	MI_TYPE_NONE,
	MI_TYPE_BASE,
	MI_TYPE_MLO,
	MI_TYPE_TIME,
	MI_TYPE_WEAPON,
	MI_TYPE_VEHICLE,
	MI_TYPE_PED,
	MI_TYPE_COMPOSITE,

	MI_TYPE_COUNT,
	MI_HIGHEST_FACTORY_ID = MI_TYPE_COUNT - 1
};

enum ÑBaseArchetypeDefLoadFlags
{
	FLAG_DRAW_LAST                                = 1 << 2,
	FLAG_PROP_CLIMBABLE_BY_AI                     = 1 << 3,
	FLAG_SUPPRESS_HD_TXDS                         = 1 << 4,
	FLAG_IS_FIXED                                 = 1 << 5,
	FLAG_DONT_WRITE_ZBUFFER                       = 1 << 6,
	FLAG_HAS_ANIM                                 = 1 << 9,
	FLAG_HAS_UVANIM                               = 1 << 10,
	FLAG_SHADOW_ONLY                              = 1 << 11,
	FLAG_DAMAGE_MODEL                             = 1 << 12,
	FLAG_DONT_CAST_SHADOWS                        = 1 << 13,
	FLAG_CAST_TEXTURE_SHADOWS                     = 1 << 14,
	FLAG_IS_TREE                                  = 1 << 16,
	FLAG_IS_TYPE_OBJECT                           = 1 << 17,
	FLAG_OVERRIDE_PHYSICS_BOUNDS                  = 1 << 18,
	FLAG_AUTOSTART_ANIM                           = 1 << 19,
	FLAG_HAS_PRE_REFLECTED_WATER_PROXY            = 1 << 20,
	FLAG_HAS_DRAWABLE_PROXY_FOR_WATER_REFLECTIONS = 1 << 21,
	FLAG_DOES_NOT_PROVIDE_AI_COVER                = 1 << 22,
	FLAG_DOES_NOT_PROVIDE_PLAYER_COVER            = 1 << 23,
	FLAG_IS_LADDER_DEPRECATED                     = 1 << 24,
	FLAG_HAS_CLOTH                                = 1 << 25,
	FLAG_DOOR_PHYSICS                             = 1 << 26,
	FLAG_IS_FIXED_FOR_NAVIGATION                  = 1 << 27,
	FLAG_DONT_AVOID_BY_PEDS                       = 1 << 28,
	FLAG_USE_AMBIENT_SCALE                        = 1 << 29,
	FLAG_IS_DEBUG                                 = 1 << 30,
	FLAG_HAS_ALPHA_SHADOW                         = 1 << 31
};

enum CBaseModelInfoFlags // Extend fwArchetypeFlags
{
	MODEL_DONT_WRITE_ZBUFFER            = 1 << 3,
	MODEL_IS_TYPE_OBJECT                = 1 << 4, // Creates a CObject
	MODEL_SHADOW_PROXY                  = 1 << 5,
	MODEL_LOD                           = 1 << 6,
	MODEL_SYNC_OBJ_IN_NET_GAME          = 1 << 7,
	MODEL_HD_TEX_CAPABLE                = 1 << 8, // Model uses a txd which can be upgraded to HD
	MODEL_CARRY_SCRIPTED_RT             = 1 << 9,
	MODEL_HAS_ALPHA_SHADOW              = 1 << 10,
	MODEL_DOES_NOT_PROVIDE_PLAYER_COVER = 1 << 11,
	MODEL_USE_AMBIENT_SCALE             = 1 << 12,
	MODEL_DONT_CAST_SHADOWS             = 1 << 13,
	MODEL_HAS_SPAWN_POINTS              = 1 << 14,
	MODEL_DOES_NOT_PROVIDE_AI_COVER     = 1 << 15,
	MODEL_IS_FIXED                      = 1 << 16, // Position is fixed in world
	MODEL_IS_TREE                       = 1 << 17,
	MODEL_CLIMBABLE_BY_AI               = 1 << 18,
	MODEL_USES_DOOR_PHYSICS             = 1 << 19,
	MODEL_IS_FIXED_FOR_NAVIGATION       = 1 << 20,
	MODEL_HAS_UVANIMATION               = 1 << 21,
	MODEL_NOT_AVOIDED_BY_PEDS           = 1 << 22,
	MODEL_OVERRIDE_PHYSICS_BOUNDS       = 1 << 24,
	MODEL_HAS_PRE_REFLECTED_WATER_PROXY = 1 << 25,

	// Bits 26 - 31 are used by special attributes
};

// Only one of them can be set
enum
{
	MODEL_ATTRIBUTE_NOTHING_SPECIAL = 0,
	MODEL_ATTRIBUTE_UNUSED1,
	MODEL_ATTRIBUTE_IS_LADDER,
	MODEL_ATTRIBUTE_IS_TRAFFIC_LIGHT,
	MODEL_ATTRIBUTE_IS_BENDABLE_PLANT,
	MODEL_ATTRIBUTE_IS_GARAGE_DOOR,
	MODEL_ATTRIBUTE_MLO_WATER_LEVEL,
	MODEL_ATTRIBUTE_IS_NORMAL_DOOR,
	MODEL_ATTRIBUTE_IS_SLIDING_DOOR,
	MODEL_ATTRIBUTE_IS_BARRIER_DOOR,
	MODEL_ATTRIBUTE_IS_SLIDING_DOOR_VERTICAL,
	MODEL_ATTRIBUTE_NOISY_BUSH,
	MODEL_ATTRIBUTE_IS_RAIL_CROSSING_DOOR,
	MODEL_ATTRIBUTE_NOISY_AND_DEFORMABLE_BUSH,
	MODEL_SINGLE_AXIS_ROTATION,
	MODEL_ATTRIBUTE_HAS_DYNAMIC_COVER_BOUND,
	MODEL_ATTRIBUTE_RUMBLE_ON_LIGHT_COLLISION_WITH_VEHICLE,
	MODEL_ATTRIBUTE_IS_RAIL_CROSSING_LIGHT,
	MODEL_ATTRIBUTE_UNUSED18,
	MODEL_ATTRIBUTE_UNUSED19,
	MODEL_ATTRIBUTE_UNUSED20,
	MODEL_ATTRIBUTE_UNUSED21,
	MODEL_ATTRIBUTE_UNUSED22,
	MODEL_ATTRIBUTE_UNUSED23,
	MODEL_ATTRIBUTE_UNUSED24,
	MODEL_ATTRIBUTE_UNUSED25,
	MODEL_ATTRIBUTE_UNUSED26,
	MODEL_ATTRIBUTE_IS_DEBUG,
	MODEL_ATTRIBUTE_RUBBISH,
	MODEL_ATTRIBUTE_RUBBISH_ONLY_ON_BIN_DAY,
	MODEL_ATTRIBUTE_CLOCK,
	MODEL_ATTRIBUTE_IS_TREE_DEPRECATED, // Turned into a flag (MODEL_IS_TREE)
	MODEL_ATTRIBUTE_IS_STREET_LIGHT,

	MODEL_ATTRIBUTE_SHIFT = 26,
	MODEL_ATTRIBUTE_MASK = 0x3F << MODEL_ATTRIBUTE_SHIFT,
};

struct CBaseArchetypeDef : rage::fwArchetypeDef {};
struct CTimeArchetypeDef : CBaseArchetypeDef
{
	u32 TimeFlags;
};

class CBaseModelInfo : public rage::fwArchetype
{
	CCustomShaderEffectBaseType* m_ShaderEffectType;

	pVoid m_BuoyancyInfo;
	pVoid m_AudioOcclusionsOverride;
	u32   m_ScriptID;
	u16*  m_LodSkelBoneMap;
	u16   m_LodSkelBoneCount;
	s16   m_PtfxAssetSlot;
	u8    m_FxEntityFlags;
	u8    m_Type                                : 5; // ModelInfoType
	u8    m_HasPreRenderEffects                 : 1;
	u8    m_HasDrawableProxyForWaterReflections : 1;
	u8    m_UnusedFlag                          : 1;
	u32   m_LeakObjectsIntentionally            : 1;
	u32   m_NeverDummy                          : 1;
	u32   m_UnusedObjectFlags                   : 30;
	pVoid m_2dEffects;

public:
	CBaseModelInfo() = default;
	~CBaseModelInfo() override;

	void Init() override;
	void InitArchetypeFromDefinition(rage::strLocalIndex slot, rage::fwArchetypeDef* def, bool loadModels) override;

	void SetPhysics(const rage::phArchetypePtr& physics) override;
	bool CheckIsFixed() override { return IsFlagSet(MODEL_IS_FIXED) || IsFlagSet(MODEL_IS_FIXED_FOR_NAVIGATION); }
	bool WillGenerateBuilding() override;

	void Shutdown() override;

	virtual void ConvertLoadFlagsAndAttributes(u32 loadFlags, u32 attribute);
	virtual void ShutdownExtra() {}

	virtual void InitMasterDrawableData(u32 modelSlot);
	virtual void DeleteMasterDrawableData();
	virtual void InitMasterFragData(u32 modelSlot) {} // TODO:
	virtual void DeleteMasterFragData() {} // TODO:

	virtual CTimeInfo*          GetTimeInfo() { return NULL; }
	virtual rage::strLocalIndex GetPropsFileIndex() const { return rage::INVALID_STR_INDEX; }
	virtual rage::atHashString  GetExpressionSet() { return rage::atHashString::Null(); } // ->fwMvExpressionSetId
	virtual void                InitWaterSamples();

	void		  SetIsStreamedArchetype(bool v) { m_IsStreamed = v; }
	bool          IsStreamedArchetype() const { return m_IsStreamed; }
	ModelInfoType GetModelType() const { return ModelInfoType(m_Type); }

	void SetAttribute(u32 attribute) { m_Flags &= ~MODEL_ATTRIBUTE_MASK; m_Flags |= attribute << MODEL_ATTRIBUTE_SHIFT; }
	u32	 GetAttribute() const { return m_Flags >> MODEL_ATTRIBUTE_SHIFT; }
	bool IsAttributeSet(u32 attribute) const { return GetAttribute() == attribute; }

	rage::atArray<CLightAttr>* GetLightArray() const;

	// --- Misc ---
	void SetNeverDummy(bool neverDummy) { m_NeverDummy = neverDummy; }
	bool GetNeverDummy() const { return m_NeverDummy; }
	void SetHasDrawableProxyForWaterReflections(bool hasProxy) { m_HasDrawableProxyForWaterReflections = hasProxy; }
	bool SetHasDrawableProxyForWaterReflections() const { return m_HasDrawableProxyForWaterReflections; }
	void SetHasPreRenderEffects(bool hasEffects) { m_HasPreRenderEffects = hasEffects; }
	bool SetHasPreRenderEffects() const { return m_HasPreRenderEffects; }
};
