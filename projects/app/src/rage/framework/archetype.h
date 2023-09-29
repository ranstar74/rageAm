#pragma once

#include "extension.h"
#include "common/types.h"
#include "rage/atl/hashstring.h"
#include "rage/dat/base.h"
#include "rage/paging/ref.h"
#include "rage/spd/aabb.h"
#include "rage/streaming/streaming.h"

namespace rage
{
	enum eArchetypeDefFlags : u32
	{
		ADF_STATIC = 1 << 5,
		ADF_NO_ALPHA_SORTING = 1 << 6,
		ADF_INSTANCED = 1 << 7,
		ADF_BONE_ANIMS = 1 << 9,
		ADF_UV_ANIMS = 1 << 10,
		ADF_SHADOW_PROXY = 1 << 11,
		ADF_NO_SHADOWS = 1 << 13,
		ADF_NO_BACKFACE_CULLING = 1 << 16,
		ADF_DYNAMIC = 1 << 17,
		ADF_DYNAMIC_ANIMS = 1 << 19,
		ADF_USE_DOOR_ATTRIBUTES = 1 << 26,
		ADF_DISABLE_VTX_COL_RED = 1 << 28,
		ADF_DISABLE_VTX_COL_GREEN = 1 << 29,
		ADF_DISABLE_VTX_COL_BLUE = 1 << 30,
		ADF_DISABLE_VTX_COL_ALPHA = 1 << 31,
	};

	class fwArchetypeDef : public datBase
	{
	public: // TODO: ...
		enum eAssetType
		{
			ASSET_TYPE_UNINITIALIZED = 0,
			ASSET_TYPE_FRAGMENT = 1,
			ASSET_TYPE_DRAWABLE = 2,
			ASSET_TYPE_DRAWABLEDICTIONARY = 3,
			ASSET_TYPE_ASSETLESS = 4,
		};

		float			m_LodDist;
		u32				m_Flags;
		u32				m_SpecialAttribute;
		spdAABB			m_BoundingBox;
		Vec3V			m_BsCentre;
		float			m_BsRadius;
		float			m_HdTextureDist;
		atHashValue		m_Name;
		atHashValue		m_TextureDictionary;
		atHashValue		m_ClipDictionary;
		atHashValue		m_DrawableDictionary;
		atHashValue		m_PhysicsDictionary;
		eAssetType		m_AssetType;
		atHashValue		m_AssetName;
		fwExtensionDefs	m_Extensions;

	public:
		fwArchetypeDef() = default;

		// ReSharper disable once CppPossiblyUninitializedMember
		fwArchetypeDef(const datResource& rsc)
		{

		}
	};
	using fwArchetypeDefPtr = pgUPtr<fwArchetypeDef>;
	using fwArchetypeDefs = pgArray<fwArchetypeDefPtr>;

	static_assert(sizeof fwArchetypeDef == 0x90); // TEMP
	static_assert(offsetof(fwArchetypeDef, m_Name) == 0x58); // TEMP

	enum eArchetypeFlags : u32
	{
		ATF_BONE_ANIMS = 1 << 1,
		ATF_NO_ALPHA_SORTING = 1 << 3,
		ATF_DYNAMIC = 1 << 4,
		ATF_SHADOW_PROXY = 1 << 5,
		ATF_DISABLE_VTX_COL_GREEN = 1 << 12,
		ATF_NO_SHADOWS = 1 << 13,
		ATF_STATIC = 1 << 16,
		ATF_NO_BACKFACE_CULLING = 1 << 17,
		ATF_USE_DOOR_ATTRIBUTES = 1 << 19,
		ATF_UV_ANIMS = 1 << 21,
		ATF_DISABLE_VTX_COL_RED = 1 << 22,
	};

	class fwArchetype
	{
	public:
		u64				m_Unknown8;
		u32				m_Unknown10;
		u32				m_Unknown14;
		u32				m_NameHash;
		u32				m_Pad;
		Vector3			m_BsCentre;
		float			m_BsRadius;
		Vector3			m_BoundingMin;
		float			m_LodDist;
		Vector3			m_BoundingMax;
		float			m_HdTextureDist;
		u32				m_Flags;
		u64				m_DynamicComponent;
		u8				m_AssetType;
		u8				m_DwdIndex;
		strLocalIndex	m_AssetIndex;
		u16				m_RefCount;
		u16				m_ModelIndex;
		u64				m_ShaderEffect;
		u64				m_Unknown78;

	public:
		fwArchetype() = default;
		virtual ~fwArchetype() = default;
	};
	static_assert(sizeof(fwArchetype) == 0x80); // TEMP
}
