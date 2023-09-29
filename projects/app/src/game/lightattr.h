#pragma once

#include "am/integration/memory/address.h"
#include "rage/math/vecv.h"

struct CLightAttr
{
	u64 Vftable;
	rage::Vec3V Position;
	u8 ColorR;
	u8 ColorG;
	u8 ColorB;
	u8 Flashiness;
	float Intensity;
	u32 Flags;
	u16 BoneTag;
	u8 Type;
	u8 GroupId;
	u32 TimeFlags;
	float Fallof;
	float FallofExponent;
	rage::Vector3 CullingPlaneNormal;
	float CullingPlaneOffset;
	u8 ShadowBlur;
	u32 Pad48;
	float VolumeIntensity;
	float VolumeSizeScale;
	u8 VolumeOuterColorR;
	u8 VolumeOuterColorG;
	u8 VolumeOuterColorB;
	u8 LightHash;
	float VolumeOuterIntensity;
	float CoronaSize;
	float VolumeOuterExponent;
	u8 LightFadeDistance;
	u8 ShadowFadeDistance;
	u8 SpecularFadeDistance;
	u8 VolumetricFadeDistance;
	float ShadowNearClip;
	float CoronaIntensity;
	float CoronaZBias;
	rage::Vector3 Direction;
	rage::Vector3 Tangent;
	float ConeInnerAngle;
	float ConeOuterAngle;
	rage::Vector3 Extent;
	u32 ProjectedTexture;

	void FixupVft() // Temp hack
	{
#ifndef AM_STANDALONE
		// Invoke constructor
		static gmAddress addr = gmAddress::Scan("E8 ?? ?? ?? ?? 0F B7 83 B8 00 00 00").GetCall();
		addr.To<void(*)(CLightAttr*)>()(this);
#endif
	}
};
