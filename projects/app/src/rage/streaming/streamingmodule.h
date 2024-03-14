//
// File: streamingmodule.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "streamingdefs.h"
#include "rage/atl/string.h"
#include "rage/atl/hashstring.h"
#include "rage/dat/base.h"
#include "rage/framework/streaming/assettypes.h"

namespace rage
{
	struct datResourceMap;
	struct datResourceInfo;

	enum eHierarchyModType
	{
		HMT_ENABLE,
		HMT_DISABLE,
		HMT_FLUSH,
	};

	class strStreamingModule : public datBase
	{
	protected:
		s32			m_StreamInfoIndex = -1;		// Start index (offset) in rage::strStreamingEngine::ms_info
		u32			m_StreamingModuleId = 0;	// Module index in g_streamingModules array
		u32			m_Size;
		atString	m_Name;
		bool		m_NeedTempMemory;
		bool		m_CanDefragment;
		bool		m_UsesExtraMemory;
		u32			m_AssetVersion;
		strAssetID	m_AssetTypeID;

	public:
		strStreamingModule(ConstString name, u32 assetVersion, strAssetID assetTypeID, u32 defaultSize, bool needTempMemory = false);
		~strStreamingModule() override = default;

#if APP_BUILD_2699_16_RELEASE
		virtual ConstString GetName(strLocalIndex index) { return nullptr; }
#endif

		// Creates a definition for object with given name
		// When a packfile is registered with the streaming system it goes through every file in the packfile, works 
		//  out which streaming module it belongs to and then calls Register() on that 
		//  streaming module with the filename minus the extension
		virtual void Register(strLocalIndex& index, ConstString name) = 0;
		// Finds slot of registered object (or INVALID_STR_INDEX if object is not in packfile...)
		virtual strLocalIndex FindSlot(ConstString name) = 0;
		// Unloads object from memory, if it was loaded
		virtual void Remove(strLocalIndex slot) {}
		// Unregisters object slot
		virtual void RemoveSlot(strLocalIndex slot) = 0;
		// Streams in object. For resources, object is resource file name
		virtual bool Load(strLocalIndex slot, pVoid object, int size) { return false; }
		virtual void PlaceResource(strLocalIndex slot, datResourceMap& map, datResourceInfo& info) {}
		virtual void SetResource(strLocalIndex slot, datResourceMap& map) {}

		virtual pVoid GetPtr(strLocalIndex slot) = 0;
		virtual pVoid GetDataPtr(strLocalIndex slot) { return GetPtr(slot); }

		// Defragmentation is not supported in PC version
		virtual pVoid Defragment(strLocalIndex slot, datResourceMap& map, bool& success) { success = false; return nullptr; }
		virtual void  DefragmentComplete(strLocalIndex slot) {}
		virtual void  DefragmentPreprocess(strLocalIndex slot) {}

		virtual pVoid GetResource(strLocalIndex slot) { return nullptr; }

		virtual void GetModelMapTypeIndex(strLocalIndex slot, strIndex& outSlot) {}
		virtual bool ModifyHierarchyStatus(strLocalIndex slot, eHierarchyModType modType) { return false; }

		virtual void AddRef(strLocalIndex slot) = 0;
		virtual void RemoveRef(strLocalIndex slot) = 0;
		virtual void ResetAllRefs(strLocalIndex slot) = 0;
		virtual s32  GetNumRefs(strLocalIndex slot) = 0;
		virtual void GetRefCountString(strLocalIndex slot, char* destination, u32 destinationSize) = 0;

		// Return the streaming indices of all the objects that are required to be resident in memory before this
		// object can be streamed in
		// For example - texture dictionary is dependency for drawables
		// Returns number of valid indices
		virtual int  GetDependencies(strLocalIndex slot, strIndex* indices, u32 indicesSize) = 0;
		virtual void PrintExtraInfo(strLocalIndex slot, char* extraInfo, int maxSize) = 0;
		// Some (non resource) modules require extra memory to load object in
		virtual void RequestExtraMemory(strLocalIndex slot, datResourceMap& map, int maxAllocs) = 0;
		virtual void ReceiveExtraMemory(strLocalIndex slot, datResourceMap& map) = 0;
		virtual u64  GetExtraVirtualMemory(strLocalIndex slot) = 0;
		virtual u64  GetExtraPhysicalMemory(strLocalIndex slot) = 0;
#if APP_BUILD_2699_16_RELEASE
		virtual void MarkDirty(strLocalIndex slot) {}
#endif
		virtual bool IsDefragmentCopyBlocked() { return false; }
		virtual bool RequiresTempMemory(strLocalIndex index) { return m_NeedTempMemory; }
		virtual bool CanPlaceAsynchronously() { return false; }
		virtual void PlaceAsynchronously() {}
		virtual u32  GetStreamerReadFlags() { return 0; }

		ConstString GetModuleName() const { return m_Name; }

		u32  GetStreamingModuleId() const { return m_StreamingModuleId; }
		u32  GetStreamInfoIndex() const { return m_StreamInfoIndex; }
		void SetStreamingModuleId(u32 id) { m_StreamingModuleId = id; }
		void SetStreamInfoIndex(s32 index) { m_StreamInfoIndex = index; }

		strIndex      GetGlobalIndex(strLocalIndex index) const { return strIndex(m_StreamInfoIndex + index.Get()); }
		strLocalIndex GetLocalIndex(strIndex index) const { return strLocalIndex(index.Get() - m_StreamInfoIndex); }
	};
}
