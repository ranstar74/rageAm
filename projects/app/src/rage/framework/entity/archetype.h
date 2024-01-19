//
// File: archetype.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "dynamicarchetype.h"
#include "extensionlist.h"
#include "rage/dat/base.h"
#include "rage/paging/ref.h"
#include "rage/physics/archetype.h"
#include "rage/streaming/streamingdefs.h"

namespace rage
{
	struct fwDynamicArchetypeComponent;
	class rmcDrawable;
	class fwEntity;

	/**
	 * \brief Base class for all map type definitions.
	 */
	struct fwArchetypeDef : datBase
	{
		enum eAssetType
		{
			ASSET_TYPE_UNINITIALIZED,
			ASSET_TYPE_FRAGMENT,
			ASSET_TYPE_DRAWABLE,
			ASSET_TYPE_DRAWABLEDICTIONARY,
			ASSET_TYPE_ASSETLESS,
		};

		float           LodDist;
		u32             Flags;
		u32             SpecialAttribute;
		spdAABB         BoundingBox;
		Vec3V           BsCentre;
		float           BsRadius;
		float           HdTextureDist;
		atHashValue     Name;
		atHashValue     TextureDictionary;
		atHashValue     ClipDictionary;
		atHashValue     DrawableDictionary;
		atHashValue     PhysicsDictionary;
		eAssetType      AssetType;
		atHashValue     AssetName;
		fwExtensionDefs Extensions;
	};
	using fwArchetypeDefPtr = pgUPtr<fwArchetypeDef>;
	using fwArchetypeDefs = pgArray<fwArchetypeDefPtr>;

	// Flags are extended by derived classes (CBaseModelInfo...), don't give any name to enumeration
	enum
	{
		MODEL_HAS_LOADED    = 1 << 0,
		MODEL_HAS_ANIMATION = 1 << 1,
		MODEL_DRAW_LAST     = 1 << 2,
	};

	/**
	 * \brief Archetype holds various game object information, similar concept to prefabs.
	 */
	class fwArchetype
	{
	protected:
		enum DrawableType : u8
		{
			DT_UNINITIALIZED,
			DT_FRAGMENT,
			DT_DRAWABLE,
			DT_DRAWABLEDICTIONARY,
			DT_ASSETLESS,
		};

		fwExtensionList              m_ExtensionList;
		atHashString                 m_ModelName;
		u32                          m_BsPad;
		Vector3                      m_BsCentre;
		float                        m_BsRadius;
		Vector3                      m_BoundingMin;			// Native definition uses spdAABB and user variables for lod distances
		float                        m_LodDist;
		Vector3                      m_BoundingMax;
		float                        m_HdTextureDist;
		u32                          m_Flags      : 31;
		u32                          m_IsStreamed : 1;
		fwDynamicArchetypeComponent* m_DynamicComponent;
		DrawableType                 m_DrawableType;
		u8                           m_DwdIndex;			// Index of drawable in dictionary
		strLocalIndex                m_DrawableIndex;
		u16                          m_RefCount       : 15;
		u16                          m_IsModelMissing : 1;	// Unused in final
		u16                          m_ModelIndex;			// In archetype manager

		fwDynamicArchetypeComponent& CreateDynamicComponentIfMissing();
		void                         DestroyDynamicComponent();

	public:
		fwArchetype() = default;
		virtual ~fwArchetype();

		virtual void Init();
		// Slot from map type store, specify INVALID_STR_INDEX if archetype is not streamed
		// If loadModels is set, it will look up #dr/#dd/#ft in asset stores and update streaming index & asset type,
		// otherwise ASSET_TYPE_ASSETLESS will be used
		virtual void InitArchetypeFromDefinition(strLocalIndex mapTypesSlot, fwArchetypeDef* def, bool loadModels);

		virtual fwEntity* CreateEntity() { return nullptr; }
		virtual fwEntity* CreateEntityGrassBatch() { return nullptr; }
		virtual fwEntity* CreateEntityBatch() { return nullptr; }

		virtual bool CheckIsFixed() = 0;
		virtual bool WillGenerateBuilding() = 0;
		virtual bool IsInteriorPhysicsBindingSupressed() { return false; }

		virtual void Shutdown();

		virtual const u16* GetLodSkeletonMap() { return nullptr; }
		virtual u16        GetLodSkeletonBoneNum() { return 0; }

		virtual void AddHDRef(bool) {}
		virtual void RemoveHDRef(bool) {}

		virtual void SetPhysics(const phArchetypePtr& physics);

		ConstString GetModelName() const { return m_ModelName.TryGetCStr(); }

		strLocalIndex GetStreamingSlot() const { return strLocalIndex(m_ModelIndex); }
		bool          IsStreamed() const { return m_IsStreamed; }

		bool          SetDrawableOrFragmentFile(u32 nameHash, strLocalIndex txdSlot, bool isFragment);
		bool          SetDrawableDictionaryFile(u32 nameHash, strLocalIndex txdSlot);
		void          SetClipDictionary(u32 clipDictNameHash);
		strLocalIndex GetClipDictionarySlot() const;
		rmcDrawable*  GetDrawable() const;

		float GetLodDistance() const { return m_LodDist; }
		void  SetLodDistance(float d) { m_LodDist = d; }
		float GetHDTextureDistance() const { return m_HdTextureDist; }
		void  SetHDTextureDistance(float d) { m_HdTextureDist = d; }

		spdSphere GetBoundingSphere() const { return spdSphere(m_BsCentre, m_BsRadius); }
		spdAABB   GetBoundingBox() const { return spdAABB(m_BoundingMin, m_BoundingMax); }

		// Extends bounding box / sphere to this bound
		void UpdateBoundingVolumes(const phBound& bound);
		bool HasPhysics() const { return m_DynamicComponent && m_DynamicComponent->Physics; }

		// In archetype manager
		u16 GetModelIndex() const { return m_ModelIndex; }

		void AddRef() { m_RefCount++; }
		u16  Release() { return --m_RefCount; }
		u16  GetRefCount() const { return m_RefCount; }

		// MODEL_
		bool IsFlagSet(u32 flag) const { return m_Flags & flag; }
		void SetFlag(u32 flag, bool set) { m_Flags &= ~flag; if (set) m_Flags |= flag; }

		// --- Flag wrappers ---
		bool IsModelLoaded() const { return IsFlagSet(MODEL_HAS_LOADED); }
		bool HasAnimation() const { return IsFlagSet(MODEL_HAS_ANIMATION); }
		bool IsDrawLast() const { return IsFlagSet(MODEL_DRAW_LAST); }

		// --- Misc ---
		void SetAutoStartAnim(bool autoStartAnim);
	};
	using fwArchetypePtr = amComPtr<fwArchetype>;
}
