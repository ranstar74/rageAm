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

enum eLightFlags : u32
{
	// https://github.com/alexguirre/Spotlight/blob/master/Source/Core/Memory/GameStructs.cs#L292

	LF_NONE = 0,
	LF_RENDER_UNDERGROUND = 1 << 3,							// Draw light in tunnels
	LF_IGNORE_ARTIFICIAL_LIGHT_STATE = 1 << 4,				// Light will ignore SET_ARTIFICIAL_LIGHTS_STATE(FALSE) and keep rendering
	LF_ENABLE_SHADOWS = 0x40 | 0x80 | 0x100 | 0x4000000,
	LF_IGNORE_TIME = 1 << 9,								// Don't use light intensity based on game time (brighter at night, dimmer at day)
	LF_ELECTRICAL_HUM = 1 << 11,							// Electrical hum/buzz sound (volume is pretty low)
	LF_ENABLE_VOLUME = 1 << 12,								// Force enable volume ignoring timecycle
	LF_NO_REFLECTION = 1 << 13,								// Don't reflect light in surfaces (contribute specular)
	LF_ENABLE_CULLING_PLANE = 1 << 18,
	LF_ENABLE_VOLUME_OUTER_COLOR = 1 << 19,
	LF_IGNORE_GLASS = 1 << 23,								// Light won't affect glass
	LF_DISABLE_LIGHT = 1 << 30,								// Only volume will be rendered
};

inline ConstString GetLightTypeName(eLightType type)
{
	switch (type)
	{
	case LIGHT_TYPE_POINT:	return "Point";
	case LIGHT_TYPE_SPOT:	return "Spot";
	case LIGHT_TYPE_CAPSULE: return "Capsule";
	case LIGHT_TYPE_DIRECTIONAL: return "Directional";
	case LIGHT_TYPE_AO_VOLUME: return "AO Volume";
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
	u32				Pad1;
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
	rage::Vector3	Tangent;
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
