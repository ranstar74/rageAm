//
// File: material.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "rage/physics/material.h"

enum
{
	MTLFLAG_NONE			= 0,
	MTLFLAG_SEE_THROUGH		= 1 << 0, // AI probes pass through this poly
	MTLFLAG_SHOT_THROUGH	= 1 << 1, // Weapon probes pass through this poly
	MTLFLAG_SHOT_THROUGH_FX = 1 << 2, // Weapon probes report a collision for this poly - even though the probe will pass through it (used for vfx/sfx)
	MTLFLAG_NO_DECAL		= 1 << 3, // No projected texture decal is applied
	MTLFLAG_POROUS			= 1 << 4, // Whether the material is porous (will soak liquids)
	MTLFLAG_HEATS_TYRE		= 1 << 5, // Whether the material causes the tyre to heat up when doing a burnout
};

enum
{
	POLYFLAG_NONE                            = 0,
	POLYFLAG_STAIRS                          = 1 << 0,
	POLYFLAG_NOT_CLIMBABLE                   = 1 << 1,
	POLYFLAG_SEE_THROUGH                     = 1 << 2,
	POLYFLAG_SHOOT_THROUGH                   = 1 << 3,
	POLYFLAG_NOT_COVER                       = 1 << 4,
	POLYFLAG_WALKABLE_PATH                   = 1 << 5,
	POLYFLAG_NO_CAM_COLLISION                = 1 << 6,
	POLYFLAG_SHOOT_THROUGH_FX                = 1 << 7,
	POLYFLAG_NO_DECAL                        = 1 << 8,
	POLYFLAG_NO_NAVMESH                      = 1 << 9,
	POLYFLAG_NO_RAGDOLL                      = 1 << 10,
	POLYFLAG_VEHICLE_WHEEL                   = 1 << 11, // Reserved for run-time only
	POLYFLAG_NO_PTFX                         = 1 << 12,
	POLYFLAG_TOO_STEEP_FOR_PLAYER            = 1 << 13,
	POLYFLAG_NO_NETWORK_SPAWN                = 1 << 14,
	POLYFLAG_NO_CAM_COLLISION_ALLOW_CLIPPING = 1 << 15,
};

enum VfxGroup
{
	VFXGROUP_UNDEFINED = -1,

	VFXGROUP_VOID = 0,
	VFXGROUP_GENERIC,

	VFXGROUP_CONCRETE,
	VFXGROUP_CONCRETE_DUSTY,
	VFXGROUP_TARMAC,
	VFXGROUP_TARMAC_BRITTLE,
	VFXGROUP_STONE,
	VFXGROUP_BRICK,
	VFXGROUP_MARBLE,
	VFXGROUP_PAVING,
	VFXGROUP_SANDSTONE,
	VFXGROUP_SANDSTONE_BRITTLE,

	VFXGROUP_SAND_LOOSE,
	VFXGROUP_SAND_COMPACT,
	VFXGROUP_SAND_WET,
	VFXGROUP_SAND_UNDERWATER,
	VFXGROUP_SAND_DEEP,
	VFXGROUP_SAND_WET_DEEP,
	VFXGROUP_ICE,
	VFXGROUP_SNOW_LOOSE,
	VFXGROUP_SNOW_COMPACT,
	VFXGROUP_GRAVEL,
	VFXGROUP_GRAVEL_DEEP,
	VFXGROUP_DIRT_DRY,
	VFXGROUP_MUD_SOFT,
	VFXGROUP_MUD_DEEP,
	VFXGROUP_MUD_UNDERWATER,
	VFXGROUP_CLAY,

	VFXGROUP_GRASS,
	VFXGROUP_GRASS_SHORT,
	VFXGROUP_HAY,
	VFXGROUP_BUSHES,
	VFXGROUP_TREE_BARK,
	VFXGROUP_LEAVES,

	VFXGROUP_METAL,

	VFXGROUP_WOOD,
	VFXGROUP_WOOD_DUSTY,
	VFXGROUP_WOOD_SPLINTER,

	VFXGROUP_CERAMIC,
	VFXGROUP_CARPET_FABRIC,
	VFXGROUP_CARPET_FABRIC_DUSTY,
	VFXGROUP_PLASTIC,
	VFXGROUP_PLASTIC_HOLLOW,
	VFXGROUP_RUBBER,
	VFXGROUP_LINOLEUM,
	VFXGROUP_PLASTER_BRITTLE,
	VFXGROUP_CARDBOARD,
	VFXGROUP_PAPER,
	VFXGROUP_FOAM,
	VFXGROUP_FEATHERS,
	VFXGROUP_TVSCREEN,

	VFXGROUP_GLASS,
	VFXGROUP_GLASS_BULLETPROOF,

	VFXGROUP_CAR_METAL,
	VFXGROUP_CAR_PLASTIC,
	VFXGROUP_CAR_GLASS,

	VFXGROUP_PUDDLE,

	VFXGROUP_LIQUID_WATER,
	VFXGROUP_LIQUID_BLOOD,
	VFXGROUP_LIQUID_OIL,
	VFXGROUP_LIQUID_PETROL,
	VFXGROUP_LIQUID_MUD,

	VFXGROUP_FRESH_MEAT,
	VFXGROUP_DRIED_MEAT,

	VFXGROUP_PED_HEAD,
	VFXGROUP_PED_TORSO,
	VFXGROUP_PED_LIMB,
	VFXGROUP_PED_FOOT,
	VFXGROUP_PED_CAPSULE,

	NUM_VFX_GROUPS,
};

enum VfxDisturbanceType
{
	VFX_DISTURBANCE_UNDEFINED = -1,

	VFX_DISTURBANCE_DEFAULT = 0,
	VFX_DISTURBANCE_SAND,
	VFX_DISTURBANCE_DIRT,
	VFX_DISTURBANCE_WATER,
	VFX_DISTURBANCE_FOLIAGE,

	NUM_VFX_DISTURBANCES,
};

/**
 * \brief GTA Physics material holds extra information in phMaterialMgr::Id bits.
 */
struct gtaMaterialId
{
	u64 Id;

	gtaMaterialId(u64 id) { Id = id; }

	u64 GetId()			  const { return Id & 0xFF; } // phMaterialMgr::Id
	int GetProceduralId() const { return static_cast<int>(Id >> 8 & 0xFF); }
	int GetRoomId()		  const { return static_cast<int>(Id >> 16 & 0x1F); }
	int GetPedDensity()   const { return static_cast<int>(Id >> 21 & 0x7); }
	int GetPolyFlags()	  const { return static_cast<int>(Id >> 24 & 0xFFFF); }
	int GetColor()		  const { return static_cast<int>(Id >> 40 & 0xFF); }

	operator u64() const { return Id; }
	gtaMaterialId& operator=(u64 id) { Id = id; return *this; }
};

struct phMaterialGta : rage::phMaterial
{
	VfxGroup           VfxGroup = VFXGROUP_VOID;
	VfxDisturbanceType VfxDisturbanceType = VFX_DISTURBANCE_DEFAULT;
	u32				   ReactWeaponType = 0;
	int                RumbleProfileIndex = -1;
	float              Density = 1.0f;
	float              TyreGrip = 1.0f;
	float              WetGripAdjust = -0.25f;
	float              TyreDrag = 0.0f;
	float              TopSpeedMult = 0.0f;
	float              Vibration = 0.0f;
	float              Softness = 0.0f;
	float              Noisiness = 0.0f;
	float              PenetrationResistance = 0.0f;
	u32                Flags = 0;

	phMaterialGta()
	{
		Type = 1; // GTA_MATERIAL
	}
};
