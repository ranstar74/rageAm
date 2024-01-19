//
// File: assetstore.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "assettypes.h"
#include "rage/atl/hashstring.h"
#include "rage/framework/nameregistrar.h"
#include "rage/framework/pool.h"
#include "rage/framework/system/config.h"
#include "rage/streaming/streamingmodule.h"

namespace rage
{
	template<typename T>
	struct fwAssetDef
	{
		T*           Object;
		int          RefCount;
		atHashString Name;
	};

	class fwAssetStoreBase : public strStreamingModule
	{
	public:
		fwAssetStoreBase(ConstString name, u32 assetVersion, strAssetID assetTypeID, u32 defaultSize) :
			strStreamingModule(name, assetVersion, assetTypeID, defaultSize, false) {}

		virtual int GetSize() const { return 0; }
		virtual int GetNumUsedSlots() const { return 0; }
	};

	template<typename T, typename Def>
	class fwAssetStore : public fwAssetStoreBase
	{
	protected:
		fwPool<Def>                         m_Defs;
		atString                            m_AssetExtension;
		fwNameRegistrar<u32, strLocalIndex> m_NameHashToIndex;
		bool                                m_SizeFinalized = false;

	public:
		fwAssetStore(ConstString name, u32 assetVersion, strAssetID assetTypeID, u32 defaultSize)
			: fwAssetStoreBase(name, assetVersion, assetTypeID, defaultSize)
		{
			m_AssetExtension = GetPlatformAssetExtensionByID(assetTypeID);
		}
		~fwAssetStore() override { fwAssetStore::Shutdown(); }

		// Gets object with templated type, use instead of GetPtr
		T* Get(strLocalIndex slot)
		{
			Def* def = GetSlot(slot);
			AM_ASSERTS(def);
			return def->Object;
		}
		// Gets object definition from registered index
		Def* GetSlot(strLocalIndex slot)
		{
			if (slot == INVALID_STR_INDEX)
				return nullptr;
			return m_Defs.GetSlot(slot.Get());
		}
		// Gets whether slot is registered and can be accessed
		bool IsValidSlot(strLocalIndex slot) const
		{
			if (!slot.IsValid()) return false;
			return m_Defs.GetSlot(slot.Get()) != nullptr;
		}
		// Gets size of internal object definition pool, can be used for iterating over all objects
		int GetSize() const override { return m_Defs.GetSize(); }

		// Allocates fixed size pool for object definitions
		void FinalizeSize();
		// Unloads and unregisters all objects
		virtual void Shutdown() {}

		void         RemoveRef(strLocalIndex slot) override { RemoveRefWithoutDelete(slot); }
		void         AddRef(strLocalIndex slot) override;
		void         ResetAllRefs(strLocalIndex slot) override;
		s32          GetNumRefs(strLocalIndex slot) override;
		virtual void RemoveRefWithoutDelete(strLocalIndex slot);

		virtual bool LoadFile(strLocalIndex slot, ConstString path) { return false; }
		virtual void Set(strLocalIndex slot, pVoid object) {}

		virtual strLocalIndex AddSlot(const atHashString& name) { return strLocalIndex(); }
		virtual strLocalIndex AddSlot(strLocalIndex slot, const atHashString& name) { return strLocalIndex(); }

		strLocalIndex FindSlotFromHashKey(u32 key) const
		{
			strLocalIndex* pSlot = m_NameHashToIndex.TryGet(key);
			if (!pSlot || !IsValidSlot(*pSlot))
				return strLocalIndex();
			return *pSlot;
		}
	};

	template <typename T, typename TDef>
	void fwAssetStore<T, TDef>::FinalizeSize()
	{
		if (m_SizeFinalized)
			return;

		u32 size = fwConfigManager::GetInstance()->GetSizeOfPool(atStringHash(m_Name), m_Size);
		m_NameHashToIndex.InitAndAllocate(size);
		size = m_NameHashToIndex.GetSize(); // Use map size here because it's rounded to next prime number
		m_Defs.InitAndAllocate(size);
		m_Size = size;
		m_SizeFinalized = true;
	}

	template<typename T, typename Def>
	void fwAssetStore<T, Def>::AddRef(strLocalIndex slot)
	{
		Def* def = GetSlot(slot);
		AM_ASSERTS(def && def->Object);
		++def->RefCount;
	}

	template<typename T, typename Def>
	void fwAssetStore<T, Def>::ResetAllRefs(strLocalIndex slot)
	{
		Def* def = GetSlot(slot);
		AM_ASSERTS(def && def->Object && def->RefCount > 0);
		def->RefCount = 0;
	}

	template<typename T, typename Def>
	s32 fwAssetStore<T, Def>::GetNumRefs(strLocalIndex slot)
	{
		Def* def = GetSlot(slot);
		AM_ASSERTS(def);
		return def->RefCount;
	}

	template<typename T, typename Def>
	void fwAssetStore<T, Def>::RemoveRefWithoutDelete(strLocalIndex slot)
	{
		Def* def = GetSlot(slot);
		AM_ASSERTS(def && def->Object && def->RefCount > 0);
		--def->RefCount;
	}
}
