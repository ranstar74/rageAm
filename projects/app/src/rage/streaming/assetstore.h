#pragma once

#include "assettypes.h"
#include "configmanager.h"
#include "streaming.h"
#include "streamingmodule.h"
#include "rage/atl/hashstring.h"
#include "rage/atl/map.h"
#include "rage/framework/pool.h"
#include "rage/paging/builder/builder.h"

namespace rage
{
	enum eFwDefFlags : u16
	{
		FW_DEF_FLAG_UNK0 = 1 << 0, // TODO: Could streamed / has parent / parent loaded / dependency loaded
		FW_DEF_FLAG_ON_DEFRAG = 1 << 1,
	};

	/**
	 * \brief Holds information on streamed asset.
	 */
	template<typename T>
	struct fwAssetDef
	{
		T* m_pObject = nullptr;

		s32 m_RefCount = 0;
		u32 m_NameHash = 0;
		s32 m_Unknown10 = 0;

		FlagSet<eFwDefFlags> m_Flags;

		void DestroyObject()
		{
			if (!m_pObject)
				return;

			m_pObject->~T();
			m_pObject = nullptr;
		}
	};

	template<typename TAsset, typename TDef>
	class fwAssetStore : public strStreamingModule
	{
	protected:
		fwPool<TDef> m_Defs;
		atString m_AssetExtension;
		atMap<u32, strLocalIndex> m_NameHashToIndex;
		bool m_Initialized = false;
	public:
		fwAssetStore(ConstString name, u32 assetVersion, strAssetID assetTypeID, u32 defaultSize)
			: strStreamingModule(name, assetVersion, assetTypeID, defaultSize, false)
		{
			m_AssetExtension = GetPlatformAssetExtensionByID(assetTypeID);
		}

		// Can be used for iterating through every asset
		auto& GetAssetPool() { return m_Defs; }
		// Direct access to store entry
		TDef* GetDef(strLocalIndex slot) { return m_Defs.GetSlot(slot); }

		bool SlotExists(strLocalIndex slot) const { return m_Defs.GetSlot(slot) != nullptr; }

		void FinalizeSize()
		{
			if (m_Initialized)
				return;

			u32 size = fwConfigManager::GetInstance()->GetSizeOfPool(atHashString(m_Name), m_Size);
			m_NameHashToIndex.InitAndAllocate(size);
			size = m_NameHashToIndex.GetSize(); // Use map size here because it's rounded to next prime number
			m_Defs.InitAndAllocate(size);

			m_Size = size;

			m_Initialized = true;
		}

		strLocalIndex FindSlotFromHashKey(u32 key)
		{
			strLocalIndex* pIndex = m_NameHashToIndex.TryGet(key);
			if (!pIndex || !m_Defs.GetSlot(*pIndex))
				return INVALID_STR_INDEX;

			return *pIndex;
		}

		void Function0(strLocalIndex& slot, ConstString name) override
		{
			slot = FindSlotFromHashKey(joaat(name));
			if (slot == INVALID_STR_INDEX)
			{
				// AddSlot()
			}
			else
			{
				TDef* def = m_Defs.GetSlot(slot);
				if (def->m_Flags.IsSet(FW_DEF_FLAG_UNK0))
				{
					// TODO: Streaming stuff...
					def->m_Flags.Set(FW_DEF_FLAG_UNK0, false);
					def->m_pObject = nullptr;
				}
			}
		}

		void GetSlotIndexByName(strLocalIndex& outIndex, ConstString name) override
		{
			outIndex = FindSlotFromHashKey(atHashString(name));
		}

		void Remove(strLocalIndex slot) override
		{
			TDef* def = m_Defs.GetSlot(slot);
			if (def->m_Flags.IsSet(FW_DEF_FLAG_UNK0))
			{
				def->m_pObject = nullptr;
			}
			else
			{
				def->DestroyObject();
			}
		}

		void RemoveSlot(strLocalIndex slot) override
		{
			Remove(slot);

			TDef* def = m_Defs.GetSlot(slot);
			m_NameHashToIndex.Remove(def->m_NameHash);
			m_Defs.Delete(def);
		}

		void Load(strLocalIndex slot, ConstString name, s32 unknown) override
		{
			TDef* def = m_Defs.GetSlot(slot);
			if (!def->m_Flags.IsSet(FW_DEF_FLAG_UNK0))
			{
				strStreamingModule::Load(slot, name, unknown);
			}
		}

		void PlaceResource(strLocalIndex slot, datResourceMap& map, datResourceInfo& info) override
		{
			datResource rsc(map);

			// TODO: Fails with phBound
			//TDef* def = m_Defs.GetSlot(slot);
			//def->m_pObject = new (rsc.Map->MainChunk) TAsset(rsc);

			pgRscBuilder::Cleanup(map);
		}

		void SetResource(strLocalIndex slot, datResourceMap& map) override
		{
			// TODO
		}

		pVoid GetPtr(strLocalIndex slot) override
		{
			TDef* def = m_Defs.GetSlot(slot);
			if (!AM_VERIFY(def != nullptr, "fwAssetStore::GetPtr() -> Slot at index %i is not in image (RPF)!", slot))
				return nullptr;

			return def->m_pObject;
		}

		pVoid Defragment(strLocalIndex slot, datResourceMap& map, bool& success) override
		{
			success = false;

			TDef* def = m_Defs.GetSlot(slot);
			if (!AM_VERIFY(def != nullptr, "fwAssetStore::DefragmentComplete() -> Slot at index %i is not in image (RPF)!", slot))
				return nullptr;

			// TODO: Fails with phBound...
			//datResource rsc(map, "<defrag>");
			//def->m_pObject = new (rsc.Map->MainChunk) TAsset(rsc);

			success = true;
			return def->m_pObject;
		}

		void DefragmentComplete(strLocalIndex slot) override
		{
			TDef* def = m_Defs.GetSlot(slot);
			if (!AM_VERIFY(def != nullptr, "fwAssetStore::DefragmentComplete() -> Slot at index %i is not in image (RPF)!", slot))
				return;

			def->m_Flags.Set(FW_DEF_FLAG_ON_DEFRAG, false);
		}

		void DefragmentPreprocess(strLocalIndex slot) override
		{
			TDef* def = m_Defs.GetSlot(slot);
			if (!AM_VERIFY(def != nullptr, "fwAssetStore::DefragmentComplete() -> Slot at index %i is not in image (RPF)!", slot))
				return;

			def->m_Flags.Set(FW_DEF_FLAG_ON_DEFRAG, true);
		}

		pVoid GetResource(strLocalIndex slot) override
		{
			TDef* def = m_Defs.GetSlot(slot);
			if (!AM_VERIFY(def != nullptr, "fwAssetStore::GetPtr() -> Slot at index %i is not in image (RPF)!", slot))
				return nullptr;

			return def->m_pObject;
		}
		
		//void Set(strLocalIndex slot, pVoid object) override
		//{
		//	TDef* def = m_Defs.GetSlot(slot);
		//	if (!AM_VERIFY(def != nullptr, "fwAssetStore::Set() -> Slot at index %i is not in image (RPF)!", slot))
		//		return;

		//	def->m_pObject = object;
		//}
	};

	template<typename TAsset, typename TDef>
	class fwRscAssetStore : public fwAssetStore<TAsset, TDef>
	{
	public:
		fwRscAssetStore(ConstString name, u32 resourceVersion, strAssetID resourceTypeID, u32 defaultSize) :
			fwAssetStore<TAsset, TDef>(name, resourceVersion, resourceTypeID, defaultSize) {}

		bool LoadFile(strLocalIndex index, ConstString path) override
		{
			TDef* pDef = this->m_Defs.GetSlot(index);
			if (!pDef)
			{
				AM_ERRF("fwRscAssetStore::LoadFile(index: %u, path: %s) -> Resource is not in the image!",
					index, path);
				return false;
			}

			pgRscBuilder::Load(&pDef->m_pObject, path, this->m_AssetExtension.GetCStr(), this->m_AssetVersion);

			if (!pDef->m_pObject)
			{
				AM_ERRF("fwRscAssetStore::LoadFile(index: %u, path: %s) -> Failed to load resource from file!",
					index, path);
			}
			return pDef->m_pObject != nullptr;
		}
	};
}
