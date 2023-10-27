//
// File: archetype.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "extension.h"
#include "common/types.h"
#include "rage/atl/hashstring.h"
#include "rage/dat/base.h"
#include "rage/paging/ref.h"
#include "rage/physics/archetype.h"
#include "rage/spd/aabb.h"
#include "rage/streaming/streaming.h"

namespace rage
{
	class rmcDrawable;

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

	struct fwArchetypeDef : datBase
	{
		enum eAssetType
		{
			ASSET_TYPE_UNINITIALIZED = 0,
			ASSET_TYPE_FRAGMENT = 1,
			ASSET_TYPE_DRAWABLE = 2,
			ASSET_TYPE_DRAWABLEDICTIONARY = 3,
			ASSET_TYPE_ASSETLESS = 4,
		};

		float			LodDist;
		u32				Flags;
		u32				SpecialAttribute;
		spdAABB			BoundingBox;
		Vec3V			BsCentre;
		float			BsRadius;
		float			HdTextureDist;
		atHashValue		Name;
		atHashValue		TextureDictionary;
		atHashValue		ClipDictionary;
		atHashValue		DrawableDictionary;
		atHashValue		PhysicsDictionary;
		eAssetType		AssetType;
		atHashValue		AssetName;
		fwExtensionDefs	Extensions;
	};
	using fwArchetypeDefPtr = pgUPtr<fwArchetypeDef>;
	using fwArchetypeDefs = pgArray<fwArchetypeDefPtr>;

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

	struct fwDynamicArchetypeComponent
	{
		static constexpr u16 PHYSICS_ENABLED_MASK = 1 << 15;

		__m128				Unknown0 = S_FLT_MAX;
		u32					Unknown10 = 0;
		pgPtr<phArchetype>	Physics;
		u16					ClipDictionarySlot = INVALID_STR_INDEX;
		u16					Unknown22 = -1;
		u16					Unknown24 = -1;
		u16					Unknown26 = 0;
		u8					Unknown28 = -1;
		u8					Unknown29 = -1;
		u16					Flags = 0x3FFF;

		fwDynamicArchetypeComponent()
		{
			ClipDictionarySlot = INVALID_STR_INDEX;

		}

		bool IsPhysicsEnabled() const { return Flags & PHYSICS_ENABLED_MASK; }
		void SetPhysicsEnabled(bool on)
		{
			Flags &= ~PHYSICS_ENABLED_MASK;
			if (on) Flags |= PHYSICS_ENABLED_MASK;
		}
	};

	class fwArchetype
	{
	protected:
		using DynamicCompPtr = amUniquePtr<fwDynamicArchetypeComponent>;

		// Most significant bit is masked out
		static constexpr u16 REF_COUNT_MASK = 0x7FFF;

		// Same as rage::fwArchetypeDef::eAssetType but U8 element type
		enum eAssetType : u8
		{
			ASSET_TYPE_UNINITIALIZED = 0,
			ASSET_TYPE_FRAGMENT = 1,
			ASSET_TYPE_DRAWABLE = 2,
			ASSET_TYPE_DRAWABLEDICTIONARY = 3,
			ASSET_TYPE_ASSETLESS = 4,
		};

		u64					m_Unknown8;
		u32					m_Unknown10;
		u32					m_Unknown14;
		u32					m_NameHash;
		u32					m_BsPad;
		Vector3				m_BsCentre;
		float				m_BsRadius;
		Vector3				m_BoundingMin;
		float				m_LodDist;
		Vector3				m_BoundingMax;
		float				m_HdTextureDist;
		u32					m_Flags;
		DynamicCompPtr		m_DynamicComponent;
		eAssetType			m_AssetType;
		u8					m_DwdIndex;
		strLocalIndex		m_AssetIndex;
		u16					m_RefCount;
		u16					m_ModelIndex;
		u64					m_ShaderEffect;
		u64					m_Unknown78;

		auto CreateDynamicComponentIfMissing() -> fwDynamicArchetypeComponent*;
		void DestroyDynamicComponent();
		void UpdateBoundingVolumes(phBound* bound);
		void TryCreateCustomShaderEffect();

	public:
		fwArchetype() = default;
		virtual ~fwArchetype();

		virtual void Init();
		// Slot from map type store, specify INVALID_STR_INDEX if archetype is not streamed
		// If loadModels is set, it will look up ydr/ydd/yft in asset stores and update streaming index & asset type,
		// otherwise ASSET_TYPE_ASSETLESS will be used
		virtual void InitArchetypeFromDefinition(strLocalIndex slot, fwArchetypeDef* def, bool loadModels);

		virtual pVoid CreateEntity()
		{
			// TODO: Is this function even used? Not overloaded on any of existing archetype classes
			return nullptr;
		}

		virtual pVoid CreateEntityGrassBatch() { return nullptr; }
		virtual pVoid CreateEntityBatch() { return nullptr; }

		virtual bool IsAffectedByPersistence()
		{
			return (m_Flags & ATF_STATIC) || (m_Flags & 0x100000);
		}

		virtual bool Func0() = 0;
		virtual int Func1() { return 0; }

		virtual void Shutdown();

		// Not sure about those 2
		virtual pVoid GetLodSkeletonMap() = 0;
		virtual u16 GetLodSkeletonBoneNum() = 0;

		virtual int Func2() { return 0; }
		virtual int Func3() { return 0; }

		virtual void SetPhysics(phArchetype* physics);

		// If skipDrawable is set, file will be only looked up in fragment store
		// Otherwise both drawable&fragments store will be checked
		bool SetDrawableOrFragmentFile(u32 nameHash, strLocalIndex txdSlot, bool skipDrawable);
		bool SetDrawableDictionaryFile(u32 nameHash, strLocalIndex txdSlot);
		void SetClipDictionary(u32 clipDictNameHash);
		auto GetDrawable() const->rmcDrawable*;

		// Index in CModelInfo pool
		u16 GetModelIndex() const { return m_ModelIndex; }

		u16 GetRefCount() const { return m_RefCount & REF_COUNT_MASK; }
		void AddRef()
		{
			u16 refCount = GetRefCount() + 1;
			m_RefCount &= ~REF_COUNT_MASK;
			m_RefCount |= refCount & REF_COUNT_MASK;
		}
		u16 Release()
		{
			u16 refCount = GetRefCount() - 1;
			m_RefCount &= ~REF_COUNT_MASK;
			m_RefCount |= refCount & REF_COUNT_MASK;
			return refCount;
		}
	};
	using fwArchetypePtr = amComPtr<fwArchetype>;
}
