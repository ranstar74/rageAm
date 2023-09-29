#pragma once

#include "assettypes.h"
#include "streaming.h"
#include "rage/atl/string.h"
#include "rage/atl/hashstring.h"
#include "common/types.h"

namespace rage
{
	struct datResourceMap;
	struct datResourceInfo;

	class strStreamingModule
	{
	protected:
		s32 m_StreamInfoIndex = -1;		// Start index in rage::strStreamingEngine::ms_info
		u32 m_StreamingModuleId = 0;	// Module index in g_streamingModules array

		u32 m_Size;
		atString m_Name;
		bool m_NeedTempMemory; // TODO: Figure out what is this for

		u32 m_AssetVersion;
		strAssetID m_AssetTypeID;

	public:
		strStreamingModule(ConstString name, u32 assetVersion, strAssetID assetTypeID, u32 defaultSize, bool needTempMemory = false)
		{
			m_Name = name;
			m_AssetVersion = assetVersion;
			m_AssetTypeID = assetTypeID;
			m_Size = defaultSize;
			m_NeedTempMemory = needTempMemory;
		}

		virtual ~strStreamingModule() = default;

		u32  GetStreamingModuleId() const { return m_StreamingModuleId; }
		u32  GetStreamInfoIndex() const { return m_StreamInfoIndex; }
		void SetStreamingModuleId(u32 id) { m_StreamingModuleId = id; }
		void SetStreamInfoIndex(s32 index) { m_StreamInfoIndex = index; }

		strIndex GetGlobalIndex(strLocalIndex index) const { return m_StreamInfoIndex + index; }
		strLocalIndex GetLocalIndex(strIndex index) const { return index - m_StreamInfoIndex; }

		virtual void Function0(strLocalIndex& index, ConstString name) = 0;
		virtual void GetSlotIndexByName(strLocalIndex& outSlot, ConstString name) = 0;
		virtual void Remove(strLocalIndex slot) = 0;
		virtual void RemoveSlot(strLocalIndex slot) = 0;
		virtual void Load(strLocalIndex slot, ConstString name, s32 unknown)
		{
			// TODO: ...
		}
		virtual void PlaceResource(strLocalIndex slot, datResourceMap& map, datResourceInfo& info) {}
		virtual void SetResource(strLocalIndex slot, datResourceMap& map) {}
		virtual pVoid GetPtr(strLocalIndex slot) = 0;
		virtual pVoid GetDataPtr(strLocalIndex slot) { return GetPtr(slot); }

		virtual pVoid Defragment(strLocalIndex slot, datResourceMap& map, bool& success) { success = false; return nullptr; }
		virtual void DefragmentComplete(strLocalIndex slot) {}
		virtual void DefragmentPreprocess(strLocalIndex slot) {}

		virtual pVoid GetResource(strLocalIndex slot) { return nullptr; }

	private:
		virtual void GetModelMapTypeIndex() {}
		virtual void ModifyHierarchyStatus() {}
	public:

		virtual void AddRef(strLocalIndex slot) = 0;
		virtual void RemoveRef(strLocalIndex slot) = 0;
		virtual void ResetAllRefs(strLocalIndex slot) = 0;
		virtual s32 GetNumRefs(strLocalIndex slot) = 0;
		virtual void GetRefCountString(strLocalIndex slot, char* destination, u32 destinationSize) = 0;

		virtual void GetDependencies(strLocalIndex slot, strIndex* pDeps, u32 depsSize) = 0;

	private:
		virtual void PrintExtraInfo() = 0;
		virtual void RequestExtraMemory() = 0;
		virtual void ReceiveExtraMemory() = 0;
		virtual void GetExtraVirtualMemory() = 0;
		virtual void GetExtraPhysicalMemory() = 0;
		virtual void IsDefragmentCopyBlocked() = 0;
		virtual void RequiresTempMemory() = 0;
	public:

		virtual bool CanPlaceAsynchronously() = 0;
		virtual void PlaceAsynchronously() = 0;

		virtual void GetStreamerReadFlags() = 0;
		virtual void GetSize() = 0;

		virtual u32 GetNumUsedSlots() = 0;

		virtual void Shutdown() = 0;

		virtual void RemoveRefWithoutDelete(strLocalIndex slot) = 0;

		virtual bool LoadFile(strLocalIndex slot, ConstString path) = 0;

		virtual void Set(strLocalIndex slot, pVoid object) = 0;

		virtual void AddSlotAt(strLocalIndex& outSlot, strLocalIndex slot, const atHashValue& name) = 0;
		virtual void AddSlot(strLocalIndex& outSlot, const atHashValue& name) = 0;
	};
}
