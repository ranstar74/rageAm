#pragma once

#include "am/integration/memory/address.h"
#include "rage/math/vec.h"
#include "am/system/asserts.h"

enum eLightType : u8
{
	LIGHT_POINT = 1,
	LIGHT_SPOT = 2,
	LIGHT_CAPSULE = 4,
};

inline ConstString GetLightTypeName(eLightType type)
{
	switch (type)
	{
	case LIGHT_POINT:	return "Point";
	case LIGHT_SPOT:	return "Spot";
	case LIGHT_CAPSULE: return "Capsule";
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
	rage::Vector3	Extent;
	u32				ProjectedTexture;

	void FixupVft() // Temp hack
	{
#ifndef AM_STANDALONE
		// Invoke constructor
		static gmAddress addr = gmAddress::Scan("E8 ?? ?? ?? ?? 0F B7 83 B8 00 00 00").GetCall();
		addr.To<void(*)(CLightAttr*)>()(this);
#endif
	}
};
static_assert(sizeof(CLightAttr) == 0xA8);
