//
// File: lightattr.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/integration/memory/address.h"
#include "rage/math/vec.h"
#include "am/system/asserts.h"
#include "rage/math/mtxv.h"

// see CLightAttr::TimeFlags

static constexpr u32 LIGHT_TIME_NIGHT_MASK = (1 << 12) - 1;
static constexpr u32 LIGHT_TIME_DAY_MASK = ((1 << 24) - 1) ^ LIGHT_TIME_NIGHT_MASK;
static constexpr u32 LIGHT_TIME_ALWAYS_MASK = LIGHT_TIME_NIGHT_MASK | LIGHT_TIME_DAY_MASK;

enum eLightType : u8
{
	LIGHT_TYPE_POINT = 1,
	LIGHT_TYPE_SPOT = 2,
	LIGHT_TYPE_CAPSULE = 4,
	LIGHT_TYPE_DIRECTIONAL = 8,
	LIGHT_TYPE_AO_VOLUME = 16,
};

enum LightFlags : u32
{
	// Some names are taken from alexguirre research. In game light flags are defined as #define LIGHTFLAG_
	// https://github.com/alexguirre/Spotlight/blob/master/Source/Core/Memory/GameStructs.cs#L292

	LIGHTFLAG_NONE                          = 0,
	LIGHTFLAG_INTERIOR_ONLY                 = 1 << 0,
	LIGHTFLAG_EXTERIOR_ONLY                 = 1 << 1,
	LIGHTFLAG_DONT_USE_IN_CUTSCENE          = 1 << 2,
	LIGHTFLAG_RENDER_UNDERGROUND            = 1 << 3, // LIGHTFLAG_VEHICLE | Draw light in tunnels
	LIGHTFLAG_IGNORE_ARTIFICIAL_LIGHT_STATE = 1 << 4, // LIGHTFLAG_FX | Light will ignore SET_ARTIFICIAL_LIGHTS_STATE(FALSE) and keep rendering
	LIGHTFLAG_TEXTURE_PROJECTION			= 1 << 5,
	LIGHTFLAG_CAST_SHADOWS				    = 1 << 6,
	LIGHTFLAG_CAST_STATIC_GEOM_SHADOWS	    = 1 << 7,
	LIGHTFLAG_CAST_DYNAMIC_GEOM_SHADOWS		= 1 << 8,
	LIGHTFLAG_IGNORE_TIME					= 1 << 9, // LIGHTFLAG_CALC_FROM_SUN | Don't use light intensity based on game time (brighter at night, dimmer at day)
	LIGHTFLAG_ENABLE_BUZZING                = 1 << 10,
	LIGHTFLAG_FORCE_BUZZING                 = 1 << 11, // Electrical hum/buzz sound (volume is pretty low)
	LIGHTFLAG_DRAW_VOLUME                   = 1 << 12, // Force enable volume ignoring timecycle
	LIGHTFLAG_NO_SPECULAR                   = 1 << 13, // Don't reflect light in surfaces (contribute specular)
	LIGHTFLAG_BOTH_INTERIOR_AND_EXTERIOR    = 1 << 14,
	LIGHTFLAG_CORONA_ONLY                   = 1 << 15,
	LIGHTFLAG_NOT_IN_REFLECTION				= 1 << 16,
	LIGHTFLAG_ONLY_IN_REFLECTION            = 1 << 17,
	LIGHTFLAG_USE_CULL_PLANE                = 1 << 18,
	LIGHTFLAG_USE_VOLUME_OUTER_COLOR        = 1 << 19,
	LIGHTFLAG_CAST_HIGHER_RES_SHADOWS       = 1 << 20,
	LIGHTFLAG_CAST_LOWRES_SHADOWS           = 1 << 21,
	LIGHTFLAG_FAR_LOD_LIGHT                 = 1 << 22,
	LIGHTFLAG_DONT_LIGHT_ALPHA              = 1 << 23, // Previously called LIGHTFLAG_IGNORE_GLASS
	LIGHTFLAG_CAST_SHADOWS_IF_POSSIBLE      = 1 << 24, // Unused
	LIGHTFLAG_CUTSCENE                      = 1 << 25,
	LIGHTFLAG_MOVING_LIGHT_SOURCE           = 1 << 26, // Runtime flag
	LIGHTFLAG_USE_VEHICLE_TWIN              = 1 << 27,
	LIGHTFLAG_FORCE_MEDIUM_LOD_LIGHT        = 1 << 28,
	LIGHTFLAG_CORONA_ONLY_LOD_LIGHT         = 1 << 29,
	LIGHTFLAG_DELAY_RENDER                  = 1 << 30, // Only volume will be rendered
	LIGHTFLAG_ALREADY_TESTED_FOR_OCCLUSION  = 1 << 31, // Set by ProcessLightOcclusionAsync

	LIGHTFLAG_USE_SHADOWS = LIGHTFLAG_CAST_SHADOWS | LIGHTFLAG_CAST_STATIC_GEOM_SHADOWS | LIGHTFLAG_CAST_DYNAMIC_GEOM_SHADOWS,
};

enum eLightFlashiness
{
	FL_CONSTANT = 0,				// on all the time
	FL_RANDOM,						// flickers randomly
	FL_RANDOM_OVERRIDE_IF_WET,		// flickers randomly (forced if road is wet)
	FL_ONCE_SECOND,					// on/off once every second
	FL_TWICE_SECOND,				// on/off twice every second
	FL_FIVE_SECOND,					// on/off five times every second
	FL_RANDOM_FLASHINESS,			// might flicker, might not
	FL_OFF,							// always off. really only used for traffic lights
	FL_UNUSED1,						// Not used
	FL_ALARM,						// Flashes on briefly
	FL_ON_WHEN_RAINING,				// Only render when it's raining.
	FL_CYCLE_1,						// Fades in and out in so that the three of them together always are on (but cycle through colours)
	FL_CYCLE_2,						// Fades in and out in so that the three of them together always are on (but cycle through colours)
	FL_CYCLE_3,						// Fades in and out in so that the three of them together always are on (but cycle through colours)
	FL_DISCO,						// In tune with the music
	FL_CANDLE,						// Just like a candle
	FL_PLANE,						// The little lights on planes/helis. They flash briefly
	FL_FIRE,						// A more hectic version of the candle
	FL_THRESHOLD,					// experimental
	FL_ELECTRIC,					// Change colour slightly
	FL_STROBE,						// Strobe light
	FL_COUNT
};

static constexpr ConstString LightFlashinessName[] =
{
	"Constant (None)",
	"Random",
	"Random Override If Wet",
	"Once Second",
	"Twice Second",
	"Five Second",
	"Random Flashiness",
	"Off",
	"Unused",
	"Alarm",
	"Only When Raining",
	"Cycle 1",
	"Cycle 2",
	"Cycle 3",
	"Disco",
	"Candle",
	"Plane",
	"Fire",
	"Threshold",
	"Electric",
	"Strobe",
};

static constexpr ConstString LightFlashinessDesc[] =
{
	"On all the time",
	"Flickers randomly",
	"Flickers randomly (force if road is wet)",
	"On/Off once every second",
	"On/Off twice every second",
	"On/Off five times every second",
	"Might flicker, might not",
	"Always off (Used only for traffic lights)",
	"",
	"Flashes on briefly",
	"Only render when it's raining",
	"Fades in and out in so that the three of them together always are on (but cycle through colours)",
	"Fades in and out in so that the three of them together always are on (but cycle through colours)",
	"Fades in and out in so that the three of them together always are on (but cycle through colours)",
	"In tune with the music",
	"Just like a candle",
	"The little lights on planes/helis, they flash briefly.",
	"A more hectic version of the candle",
	"Experemental",
	"Change color slightly",
	"Strobe light",
};

#define EXTRA_LIGHTFLAG_HIGH_PRIORITY   (1 << 2)
#define EXTRA_LIGHTFLAG_USE_AS_PED_ONLY	(1 << 7)

inline ConstString GetLightTypeName(eLightType type)
{
	switch (type)
	{
	case LIGHT_TYPE_POINT:		 return "Point";
	case LIGHT_TYPE_SPOT:		 return "Spot";
	case LIGHT_TYPE_CAPSULE:	 return "Capsule";
	case LIGHT_TYPE_DIRECTIONAL: return "Directional";
	case LIGHT_TYPE_AO_VOLUME:	 return "AO Volume";
	default:
		AM_UNREACHABLE("GetLightTypeName() -> Type %u is not defined.", type);
	}
}

// Game clamps cone angle to 1 - 89, can be patched here if needed 40 53 48 83 EC 40 0F 29 7C 24 30 F3 0F 10 3D
static constexpr float SPOT_LIGHT_MIN_CONE_ANGLE = 1.0f;
static constexpr float SPOT_LIGHT_MAX_CONE_ANGLE = 89.0f;

struct CLightAttr
{
	u64				Vftable;

	rage::Vector3	Position;
	u32				Pad0;
	u8				ColorR;
	u8				ColorG;
	u8				ColorB;
	u8				Flashiness;
	float			Intensity;
	u32				Flags;
	u16				BoneTag;
	eLightType		Type;
	u8				GroupId;
	// Flags for every hour in a day
	// 1 << 0 - 00:00 to 01:00
	// 1 << 23 - 23:00 - 00:00
	u32				TimeFlags;
	float			Falloff;
	float			FallofExponent;
	rage::Vector3	CullingPlaneNormal;
	float			CullingPlaneOffset;
	u8				ShadowBlur;
	u8				Padding1; // Stores EXTRA_LIGHTFLAG_HIGH_PRIORITY
	s16				Padding2;
	u32				Padding3; // Stores EXTRA_LIGHTFLAG_USE_AS_PED_ONLY
	float			VolumeIntensity;
	float			VolumeSizeScale;
	u8				VolumeOuterColorR;
	u8				VolumeOuterColorG;
	u8				VolumeOuterColorB;
	// Used to override light from map light extension, light hash must match in both
	u8				LightHash;
	float			VolumeOuterIntensity;
	float			CoronaSize;
	float			VolumeOuterExponent;
	u8				LightFadeDistance;
	u8				ShadowFadeDistance;
	u8				SpecularFadeDistance;
	u8				VolumetricFadeDistance;
	float			ShadowNearClip;
	float			CoronaIntensity;
	float			CoronaZBias;
	rage::Vector3	Direction;
	rage::Vector3	Tangent;				// Used for projecting textures
	float			ConeInnerAngle;
	float			ConeOuterAngle;
	rage::Vector3	Extent;					// X is capsule light height/length
	u32				ProjectedTexture;

	CLightAttr()
	{
#ifndef AM_STANDALONE
		// Invoke constructor to get VFT
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		static auto fn = gmAddress::Scan("EB 9B 48 8B 44 24 60 48 83 C4 58", "rage::atArray<CLightAttr>::ctor + 0xCA")
			.GetAt(-0xCA)
			.GetAt(0xB5)
			.GetCall()
			.ToFunc<void(CLightAttr*, pVoid)>();
		fn(this, nullptr);
#else
		static auto fn = gmAddress::Scan("E8 ?? ?? ?? ?? 0F B7 83 B8 00 00 00", "CLightAttr::ctor")
			.GetCall()
			.ToFunc<void(CLightAttr*)>();
		fn(this);
#endif
#endif
	}

	void SetSpotTarget(const rage::Vec3V& targetPoint)
	{
		rage::ScalarV dist = targetPoint.Length();
		rage::Vec3V dir = targetPoint.Normalized();
		rage::Vec3V tan = dir.Tangent().Cross(dir);

		Falloff = dist.Get();
		Direction = dir;
		Tangent = tan;
	}

	void SetCapsulePoints(const rage::Vec3V& p1, const rage::Vec3V& p2)
	{
		rage::Vec3V   to = p2 - p1;
		rage::ScalarV mag = to.Length();
		rage::Vec3V   dir = to / mag;
		rage::Vec3V   tan = dir.BiNormal();
		rage::Vec3V   center = (p1 + p2) * rage::S_HALF;

		Extent.X = mag.Get();
		Position = center;
		Direction = dir;
		Tangent = tan;
	}

	void SetMatrix(const rage::Mat44V& localMatrix)
	{
		Position = rage::Vec3V(localMatrix.Pos);
		Direction = -rage::Vec3V(localMatrix.Up);
		Tangent = rage::Vector3(localMatrix.Right);
	}

	rage::Mat44V GetMatrix() const
	{
		rage::Vec3V down = Direction;
		rage::Vec3V right = Tangent;

		rage::Mat34V local;
		local.Up = -down;
		local.Right = right;
		local.Front = right.Cross(down);
		local.Pos = Position;
		return local.To44();
	}
};
static_assert(sizeof(CLightAttr) == 0xA8);
