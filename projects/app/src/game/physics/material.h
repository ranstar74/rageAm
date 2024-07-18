//
// File: material.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "rage/grcore/color.h"
#include "rage/physics/material.h"
#include "am/system/enum.h"
#include "am/xml/serialize.h"

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

enum ePolyFlags
{
	POLYFLAG_NONE				               = 0,
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
inline ConstString ToString(ePolyFlags e);
inline bool FromString(ConstString str, ePolyFlags& e);
IMPLEMENT_FLAGS_TO_STRING(ePolyFlags, "POLYFLAG_");

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
	union
	{
		struct
		{
			u64 Id           : 8;  // >> 0  & 0xFF
			u64 ProceduralId : 8;  // >> 8  & 0xFF
			u64 RoomId       : 5;  // >> 16 & 0x1F
			u64 PedDensity   : 3;  // >> 21 & 0x7
			u64 PolyFlags    : 32; // >> 24 & 0xFFFF
			u64 Color        : 8;  // >> 40 & 0xFF
		};
		u64 Packed;
	};

	gtaMaterialId() { Packed = 0; }
	gtaMaterialId(u64 packed) { Packed = packed; }

	void Serialize(const XmlHandle& node) const
	{
		XML_SET_CHILD_VALUE_ATTR_IGNORE_DEF(node, ProceduralId);
		XML_SET_CHILD_VALUE_ATTR_IGNORE_DEF(node, RoomId);
		XML_SET_CHILD_VALUE_ATTR_IGNORE_DEF(node, PedDensity);
		XML_SET_CHILD_VALUE_FLAGS_IGNORE_DEF(node, PolyFlags, FlagsToString_ePolyFlags);
	}

	void Deserialize(const XmlHandle& node)
	{
		// We can't reference bitfields
		int proceduralId = 0;
		int roomId = 0;
		int pedDensity = 0;
		ConstString polyFlags = 0;
		node.GetChild("ProceduralId").GetValue(proceduralId);
		node.GetChild("RoomId").GetValue(roomId);
		node.GetChild("PedDensity").GetValue(pedDensity);
		node.GetChild("PolyFlags").GetValue(polyFlags);
		if (!String::IsNullOrEmpty(polyFlags)) 
			PolyFlags = StringToFlags_ePolyFlags(polyFlags);
		ProceduralId = proceduralId;
		RoomId = roomId;
		PedDensity = pedDensity;
	}

	operator u64() const { return Packed; }
	gtaMaterialId& operator=(u64 packed) { Packed = packed; return *this; }

	XML_DEFINE(gtaMaterialId);
};
static_assert(sizeof gtaMaterialId == 8);

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

// This is not part of the game, those colors are taken from CW/Sollumz
// Synced with materials.dat

struct gtaMaterialInfo
{
	ConstString    DisplayName;
	rage::ColorU32 Color;
};

struct gtaMaterialCategory
{
	ConstString        DisplayName;
	int                StartIndex;
	int                LastIndex;
	rage::atArray<int> SplitIndices = {}; // When we need to put separator in UI
};

static inline gtaMaterialCategory g_MaterialCategories[] =
{ // Taken directly from materials.dat
	{ "Default", 0, 0 },
	{ "Hard Terrain", 1, 17 },
	{ "Loose Terrain", 18, 45, { 25 /* ICE */, 31 /* GRAVEL_SMALL */ } },
	{ "Vegetation", 46, 54 },
	{ "Metals", 55, 68 },
	{ "Woods", 69, 80 },
	{ "Man-made", 81, 111 },
	{ "Glass", 112, 115 },
	{ "Vehicles", 116, 124, { 120 /* CAR_GLASS_WEAK */ }},
	{ "Liquids", 125, 128 },
	{ "Misc", 129, 130 },
	{ "Projtex", 131, 132 },
	{ "VFX Specific (Particles)", 133, 134 },
	{ "VFX Specific (Explosions)", 135, 136 },
	{ "Physics", 137, 149 },
	{ "Natural Motion", 150, 171 },
	{ "New", 172, 176 },
	{ "Temp", 177, 212, },
};

static inline gtaMaterialInfo g_MaterialInfo[] =
{
	{ "Default", rage::ColorU32(255, 0, 255, 255) },
	{ "Concrete", rage::ColorU32(145, 145, 145, 255) },
	{ "Concrete Pothole", rage::ColorU32(145, 145, 145, 255) },
	{ "Concrete Dusty", rage::ColorU32(145, 140, 130, 255) },
	{ "Tarmac", rage::ColorU32(90, 90, 90, 255) },
	{ "Tarmac Painted", rage::ColorU32(90, 90, 90, 255) },
	{ "Tarmac Pothole", rage::ColorU32(70, 70, 70, 255) },
	{ "Rumble Strip", rage::ColorU32(90, 90, 90, 255) },
	{ "Breeze Block", rage::ColorU32(145, 145, 145, 255) },
	{ "Rock", rage::ColorU32(185, 185, 185, 255) },
	{ "Rock Mossy", rage::ColorU32(185, 185, 185, 255) },
	{ "Stone", rage::ColorU32(185, 185, 185, 255) },
	{ "Cobblestone", rage::ColorU32(185, 185, 185, 255) },
	{ "Brick", rage::ColorU32(195, 95, 30, 255) },
	{ "Marble", rage::ColorU32(195, 155, 145, 255) },
	{ "Paving Slab", rage::ColorU32(200, 165, 130, 255) },
	{ "Sandstone Solid", rage::ColorU32(215, 195, 150, 255) },
	{ "Sandstone Brittle", rage::ColorU32(205, 180, 120, 255) },
	{ "Sand Loose", rage::ColorU32(235, 220, 190, 255) },
	{ "Sand Compact", rage::ColorU32(250, 240, 220, 255) },
	{ "Sand Wet", rage::ColorU32(190, 185, 165, 255) },
	{ "Sand Track", rage::ColorU32(250, 240, 220, 255) },
	{ "Sand Underwater", rage::ColorU32(135, 130, 120, 255) },
	{ "Sand Dry Deep", rage::ColorU32(110, 100, 85, 255) },
	{ "Sand Wet Deep", rage::ColorU32(110, 100, 85, 255) },
	{ "Ice", rage::ColorU32(200, 250, 255, 255) },
	{ "Ice Tarmac", rage::ColorU32(200, 250, 255, 255) },
	{ "Snow Loose", rage::ColorU32(255, 255, 255, 255) },
	{ "Snow Compact", rage::ColorU32(255, 255, 255, 255) },
	{ "Snow Deep", rage::ColorU32(255, 255, 255, 255) },
	{ "Snow Tarmac", rage::ColorU32(255, 255, 255, 255) },
	{ "Gravel Small", rage::ColorU32(255, 255, 255, 255) },
	{ "Gravel Large", rage::ColorU32(255, 255, 255, 255) },
	{ "Gravel Deep", rage::ColorU32(255, 255, 255, 255) },
	{ "Gravel Train Track", rage::ColorU32(145, 140, 130, 255) },
	{ "Dirt Track", rage::ColorU32(175, 160, 140, 255) },
	{ "Mud Hard", rage::ColorU32(175, 160, 140, 255) },
	{ "Mud Pothole", rage::ColorU32(105, 95, 75, 255) },
	{ "Mud Soft", rage::ColorU32(105, 95, 75, 255) },
	{ "Mud Underwater", rage::ColorU32(75, 65, 50, 255) },
	{ "Mud Deep", rage::ColorU32(105, 95, 75, 255) },
	{ "Marsh", rage::ColorU32(105, 95, 75, 255) },
	{ "Marsh Deep", rage::ColorU32(105, 95, 75, 255) },
	{ "Soil", rage::ColorU32(105, 95, 75, 255) },
	{ "Clay Hard", rage::ColorU32(160, 160, 160, 255) },
	{ "Clay Soft", rage::ColorU32(160, 160, 160, 255) },
	{ "Grass Long", rage::ColorU32(130, 205, 75, 255) },
	{ "Grass", rage::ColorU32(130, 205, 75, 255) },
	{ "Grass Short", rage::ColorU32(130, 205, 75, 255) },
	{ "Hay", rage::ColorU32(240, 205, 125, 255) },
	{ "Bushes", rage::ColorU32(85, 160, 30, 255) },
	{ "Twigs", rage::ColorU32(115, 100, 70, 255) },
	{ "Leaves", rage::ColorU32(70, 100, 50, 255) },
	{ "Woodchips", rage::ColorU32(115, 100, 70, 255) },
	{ "Tree Bark", rage::ColorU32(115, 100, 70, 255) },
	{ "Metal Solid Small", rage::ColorU32(155, 180, 190, 255) },
	{ "Metal Solid Medium", rage::ColorU32(155, 180, 190, 255) },
	{ "Metal Solid Large", rage::ColorU32(155, 180, 190, 255) },
	{ "Metal Hollow Small", rage::ColorU32(155, 180, 190, 255) },
	{ "Metal Hollow Medium", rage::ColorU32(155, 180, 190, 255) },
	{ "Metal Hollow Large", rage::ColorU32(155, 180, 190, 255) },
	{ "Metal Chainlink Small", rage::ColorU32(155, 180, 190, 255) },
	{ "Metal Chainlink Large", rage::ColorU32(155, 180, 190, 255) },
	{ "Metal Corrugated Iron", rage::ColorU32(155, 180, 190, 255) },
	{ "Metal Grille", rage::ColorU32(155, 180, 190, 255) },
	{ "Metal Railing", rage::ColorU32(155, 180, 190, 255) },
	{ "Metal Duct", rage::ColorU32(155, 180, 190, 255) },
	{ "Metal Garage Door", rage::ColorU32(155, 180, 190, 255) },
	{ "Metal Manhole", rage::ColorU32(155, 180, 190, 255) },
	{ "Wood Solid Small", rage::ColorU32(155, 130, 95, 255) },
	{ "Wood Solid Medium", rage::ColorU32(155, 130, 95, 255) },
	{ "Wood Solid Large", rage::ColorU32(155, 130, 95, 255) },
	{ "Wood Solid Polished", rage::ColorU32(155, 130, 95, 255) },
	{ "Wood Floor Dusty", rage::ColorU32(165, 145, 110, 255) },
	{ "Wood Hollow Small", rage::ColorU32(170, 150, 125, 255) },
	{ "Wood Hollow Medium", rage::ColorU32(170, 150, 125, 255) },
	{ "Wood Hollow Large", rage::ColorU32(170, 150, 125, 255) },
	{ "Wood Chipboard", rage::ColorU32(170, 150, 125, 255) },
	{ "Wood Old Creaky", rage::ColorU32(155, 130, 95, 255) },
	{ "Wood High Density", rage::ColorU32(155, 130, 95, 255) },
	{ "Wood Lattice", rage::ColorU32(155, 130, 95, 255) },
	{ "Ceramic", rage::ColorU32(220, 210, 195, 255) },
	{ "Roof Tile", rage::ColorU32(220, 210, 195, 255) },
	{ "Roof Felt", rage::ColorU32(165, 145, 110, 255) },
	{ "Fibreglass", rage::ColorU32(255, 250, 210, 255) },
	{ "Tarpaulin", rage::ColorU32(255, 250, 210, 255) },
	{ "Plastic", rage::ColorU32(255, 250, 210, 255) },
	{ "Plastic Hollow", rage::ColorU32(240, 230, 185, 255) },
	{ "Plastic High Density", rage::ColorU32(255, 250, 210, 255) },
	{ "Plastic Clear", rage::ColorU32(255, 250, 210, 255) },
	{ "Plastic Hollow Clear", rage::ColorU32(240, 230, 185, 255) },
	{ "Plastic High Density Clear", rage::ColorU32(255, 250, 210, 255) },
	{ "Fibreglass Hollow", rage::ColorU32(240, 230, 185, 255) },
	{ "Rubber", rage::ColorU32(70, 70, 70, 255) },
	{ "Rubber Hollow", rage::ColorU32(70, 70, 70, 255) },
	{ "Linoleum", rage::ColorU32(205, 150, 80, 255) },
	{ "Laminate", rage::ColorU32(170, 150, 125, 255) },
	{ "Carpet Solid", rage::ColorU32(250, 100, 100, 255) },
	{ "Carpet Solid Dusty", rage::ColorU32(255, 135, 135, 255) },
	{ "Carpet Floorboard", rage::ColorU32(250, 100, 100, 255) },
	{ "Cloth", rage::ColorU32(250, 100, 100, 255) },
	{ "Plaster Solid", rage::ColorU32(145, 145, 145, 255) },
	{ "Plaster Brittle", rage::ColorU32(225, 225, 225, 255) },
	{ "Cardboard Sheet", rage::ColorU32(120, 115, 95, 255) },
	{ "Cardboard Box", rage::ColorU32(120, 115, 95, 255) },
	{ "Paper", rage::ColorU32(230, 225, 220, 255) },
	{ "Foam", rage::ColorU32(230, 235, 240, 255) },
	{ "Feather Pillow", rage::ColorU32(230, 230, 230, 255) },
	{ "Polystyrene", rage::ColorU32(255, 250, 210, 255) },
	{ "Leather", rage::ColorU32(250, 100, 100, 255) },
	{ "Tvscreen", rage::ColorU32(115, 125, 125, 255) },
	{ "Slatted Blinds", rage::ColorU32(255, 250, 210, 255) },
	{ "Glass Shoot Through", rage::ColorU32(205, 240, 255, 255) },
	{ "Glass Bulletproof", rage::ColorU32(115, 125, 125, 255) },
	{ "Glass Opaque", rage::ColorU32(205, 240, 255, 255) },
	{ "Perspex", rage::ColorU32(205, 240, 255, 255) },
	{ "Car Metal", rage::ColorU32(255, 255, 255, 255) },
	{ "Car Plastic", rage::ColorU32(255, 255, 255, 255) },
	{ "Car Softtop", rage::ColorU32(250, 100, 100, 255) },
	{ "Car Softtop Clear", rage::ColorU32(250, 100, 100, 255) },
	{ "Car Glass Weak", rage::ColorU32(210, 245, 245, 255) },
	{ "Car Glass Medium", rage::ColorU32(210, 245, 245, 255) },
	{ "Car Glass Strong", rage::ColorU32(210, 245, 245, 255) },
	{ "Car Glass Bulletproof", rage::ColorU32(210, 245, 245, 255) },
	{ "Car Glass Opaque", rage::ColorU32(210, 245, 245, 255) },
	{ "Water", rage::ColorU32(55, 145, 230, 255) },
	{ "Blood", rage::ColorU32(205, 5, 5, 255) },
	{ "Oil", rage::ColorU32(80, 65, 65, 255) },
	{ "Petrol", rage::ColorU32(70, 100, 120, 255) },
	{ "Fresh Meat", rage::ColorU32(255, 55, 20, 255) },
	{ "Dried Meat", rage::ColorU32(185, 100, 85, 255) },
	{ "Emissive Glass", rage::ColorU32(205, 240, 255, 255) },
	{ "Emissive Plastic", rage::ColorU32(255, 250, 210, 255) },
	{ "Vfx Metal Electrified", rage::ColorU32(155, 180, 190, 255) },
	{ "Vfx Metal Water Tower", rage::ColorU32(155, 180, 190, 255) },
	{ "Vfx Metal Steam", rage::ColorU32(155, 180, 190, 255) },
	{ "Vfx Metal Flame", rage::ColorU32(155, 180, 190, 255) },
	{ "Phys No Friction", rage::ColorU32(0, 0, 0, 255) },
	{ "Phys Golf Ball", rage::ColorU32(0, 0, 0, 255) },
	{ "Phys Tennis Ball", rage::ColorU32(0, 0, 0, 255) },
	{ "Phys Caster", rage::ColorU32(0, 0, 0, 255) },
	{ "Phys Caster Rusty", rage::ColorU32(0, 0, 0, 255) },
	{ "Phys Car Void", rage::ColorU32(0, 0, 0, 255) },
	{ "Phys Ped Capsule", rage::ColorU32(0, 0, 0, 255) },
	{ "Phys Electric Fence", rage::ColorU32(0, 0, 0, 255) },
	{ "Phys Electric Metal", rage::ColorU32(0, 0, 0, 255) },
	{ "Phys Barbed Wire", rage::ColorU32(0, 0, 0, 255) },
	{ "Phys Pooltable Surface", rage::ColorU32(155, 130, 95, 255) },
	{ "Phys Pooltable Cushion", rage::ColorU32(155, 130, 95, 255) },
	{ "Phys Pooltable Ball", rage::ColorU32(255, 250, 210, 255) },
	{ "Buttocks", rage::ColorU32(0, 0, 0, 255) },
	{ "Thigh Left", rage::ColorU32(0, 0, 0, 255) },
	{ "Shin Left", rage::ColorU32(0, 0, 0, 255) },
	{ "Foot Left", rage::ColorU32(0, 0, 0, 255) },
	{ "Thigh Right", rage::ColorU32(0, 0, 0, 255) },
	{ "Shin Right", rage::ColorU32(0, 0, 0, 255) },
	{ "Foot Right", rage::ColorU32(0, 0, 0, 255) },
	{ "Spine0", rage::ColorU32(0, 0, 0, 255) },
	{ "Spine1", rage::ColorU32(0, 0, 0, 255) },
	{ "Spine2", rage::ColorU32(0, 0, 0, 255) },
	{ "Spine3", rage::ColorU32(0, 0, 0, 255) },
	{ "Clavicle Left", rage::ColorU32(0, 0, 0, 255) },
	{ "Upper Arm Left", rage::ColorU32(0, 0, 0, 255) },
	{ "Lower Arm Left", rage::ColorU32(0, 0, 0, 255) },
	{ "Hand Left", rage::ColorU32(0, 0, 0, 255) },
	{ "Clavicle Right", rage::ColorU32(0, 0, 0, 255) },
	{ "Upper Arm Right", rage::ColorU32(0, 0, 0, 255) },
	{ "Lower Arm Right", rage::ColorU32(0, 0, 0, 255) },
	{ "Hand Right", rage::ColorU32(0, 0, 0, 255) },
	{ "Neck", rage::ColorU32(0, 0, 0, 255) },
	{ "Head", rage::ColorU32(0, 0, 0, 255) },
	{ "Animal Default", rage::ColorU32(0, 0, 0, 255) },
	{ "Car Engine", rage::ColorU32(255, 255, 255, 255) },
	{ "Puddle", rage::ColorU32(55, 145, 230, 255) },
	{ "Concrete Pavement", rage::ColorU32(145, 145, 145, 255) },
	{ "Brick Pavement", rage::ColorU32(195, 95, 30, 255) },
	{ "Phys Dynamic Cover Bound", rage::ColorU32(0, 0, 0, 255) },
	{ "Vfx Wood Beer Barrel", rage::ColorU32(155, 130, 95, 255) },
	{ "Wood High Friction", rage::ColorU32(155, 130, 95, 255) },
	{ "Rock Noinst", rage::ColorU32(185, 185, 185, 255) },
	{ "Bushes Noinst", rage::ColorU32(85, 160, 30, 255) },
	{ "Metal Solid Road Surface", rage::ColorU32(155, 180, 190, 255) },
	{ "Stunt Ramp Surface", rage::ColorU32(155, 180, 190, 255) },
	{ "Temp 01", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 02", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 03", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 04", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 05", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 06", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 07", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 08", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 09", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 10", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 11", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 12", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 13", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 14", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 15", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 16", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 17", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 18", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 19", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 20", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 21", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 22", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 23", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 24", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 25", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 26", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 27", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 28", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 29", rage::ColorU32(255, 0, 255, 255) },
	{ "Temp 30", rage::ColorU32(255, 0, 255, 255) },
};

ConstString ToString(ePolyFlags e)
{
	switch (e)
	{
	case POLYFLAG_NONE: return "POLYFLAG_NONE";
	case POLYFLAG_STAIRS: return "POLYFLAG_STAIRS";
	case POLYFLAG_NOT_CLIMBABLE: return "POLYFLAG_NOT_CLIMBABLE";
	case POLYFLAG_SEE_THROUGH: return "POLYFLAG_SEE_THROUGH";
	case POLYFLAG_SHOOT_THROUGH: return "POLYFLAG_SHOOT_THROUGH";
	case POLYFLAG_NOT_COVER: return "POLYFLAG_NOT_COVER";
	case POLYFLAG_WALKABLE_PATH: return "POLYFLAG_WALKABLE_PATH";
	case POLYFLAG_NO_CAM_COLLISION: return "POLYFLAG_NO_CAM_COLLISION";
	case POLYFLAG_SHOOT_THROUGH_FX: return "POLYFLAG_SHOOT_THROUGH_FX";
	case POLYFLAG_NO_DECAL: return "POLYFLAG_NO_DECAL";
	case POLYFLAG_NO_NAVMESH: return "POLYFLAG_NO_NAVMESH";
	case POLYFLAG_NO_RAGDOLL: return "POLYFLAG_NO_RAGDOLL";
	case POLYFLAG_VEHICLE_WHEEL: return "POLYFLAG_VEHICLE_WHEEL";
	case POLYFLAG_NO_PTFX: return "POLYFLAG_NO_PTFX";
	case POLYFLAG_TOO_STEEP_FOR_PLAYER: return "POLYFLAG_TOO_STEEP_FOR_PLAYER";
	case POLYFLAG_NO_NETWORK_SPAWN: return "POLYFLAG_NO_NETWORK_SPAWN";
	case POLYFLAG_NO_CAM_COLLISION_ALLOW_CLIPPING: return "POLYFLAG_NO_CAM_COLLISION_ALLOW_CLIPPING";
	}
	AM_UNREACHABLE("Failed to convert ePolyFlags (%i)", e);
}

bool FromString(ConstString str, ePolyFlags& e)
{
	u32 key = rageam::Hash(str);
	switch (key)
	{
	case rageam::Hash("POLYFLAG_NONE"): e = POLYFLAG_NONE; return true;
	case rageam::Hash("POLYFLAG_STAIRS"): e = POLYFLAG_STAIRS; return true;
	case rageam::Hash("POLYFLAG_NOT_CLIMBABLE"): e = POLYFLAG_NOT_CLIMBABLE; return true;
	case rageam::Hash("POLYFLAG_SEE_THROUGH"): e = POLYFLAG_SEE_THROUGH; return true;
	case rageam::Hash("POLYFLAG_SHOOT_THROUGH"): e = POLYFLAG_SHOOT_THROUGH; return true;
	case rageam::Hash("POLYFLAG_NOT_COVER"): e = POLYFLAG_NOT_COVER; return true;
	case rageam::Hash("POLYFLAG_WALKABLE_PATH"): e = POLYFLAG_WALKABLE_PATH; return true;
	case rageam::Hash("POLYFLAG_NO_CAM_COLLISION"): e = POLYFLAG_NO_CAM_COLLISION; return true;
	case rageam::Hash("POLYFLAG_SHOOT_THROUGH_FX"): e = POLYFLAG_SHOOT_THROUGH_FX; return true;
	case rageam::Hash("POLYFLAG_NO_DECAL"): e = POLYFLAG_NO_DECAL; return true;
	case rageam::Hash("POLYFLAG_NO_NAVMESH"): e = POLYFLAG_NO_NAVMESH; return true;
	case rageam::Hash("POLYFLAG_NO_RAGDOLL"): e = POLYFLAG_NO_RAGDOLL; return true;
	case rageam::Hash("POLYFLAG_VEHICLE_WHEEL"): e = POLYFLAG_VEHICLE_WHEEL; return true;
	case rageam::Hash("POLYFLAG_NO_PTFX"): e = POLYFLAG_NO_PTFX; return true;
	case rageam::Hash("POLYFLAG_TOO_STEEP_FOR_PLAYER"): e = POLYFLAG_TOO_STEEP_FOR_PLAYER; return true;
	case rageam::Hash("POLYFLAG_NO_NETWORK_SPAWN"): e = POLYFLAG_NO_NETWORK_SPAWN; return true;
	case rageam::Hash("POLYFLAG_NO_CAM_COLLISION_ALLOW_CLIPPING"): e = POLYFLAG_NO_CAM_COLLISION_ALLOW_CLIPPING; return true;
	}
	return false;
}
