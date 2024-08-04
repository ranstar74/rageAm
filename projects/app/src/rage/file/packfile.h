//
// File: packfile.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "device.h"
#include "rage/atl/string.h"
#include "rage/paging/resourceinfo.h"
#include "rage/atl/map.h"

namespace rage
{
	struct fiPackHeader
	{
		static constexpr u32 MAGIC = MAKEFOURCC('7', 'F', 'P', 'R');

		u32 Magic;
		u32 EntryCount;
		u32 NameHeapSize : 28; // Multiple of 16
		u32 NameShift    : 3;
		u32 XCompress    : 1; // XCompress (XBOX) instead of ZLib, not supported on PC though
		u32 Encryption;
	};

	struct fiPackEntry
	{
		static constexpr u64 IS_DIRECTORY_MASK = (1 << 23) - 1;
		static constexpr s32 FILE_OFFSET_SHIFT = 9; // Multiple of 512 bytes
		static constexpr u32 MAX_SIZE = 0xFFFFFF;

		u64 NameOffset     : 16;
		u64 CompressedSize : 24; // 0 if uncompressed, MAX_SIZE for resources that exceed max size limit (around 16MBs)
		u64 FileOffset     : 23;
		u64 IsResource     : 1;

		union
		{
			struct { u32 UncompressedSize; u32 Encryption; } File;
			struct { u32 StartIndex; u32 ChildCount; } Directory;
			struct { datResourceInfo Info; } Resource;
		};

		bool IsDirectory() const { return FileOffset == IS_DIRECTORY_MASK; }
		bool IsFile() const { return !IsDirectory() && !IsResource; }
		bool IsCompressed() const { return CompressedSize != 0; }

		u32 GetFileOffset() const { return static_cast<u32>(FileOffset) << FILE_OFFSET_SHIFT; }
		void SetFileOffset(u32 offset) { FileOffset = offset >> FILE_OFFSET_SHIFT; AM_ASSERTS(offset == GetFileOffset()); }

		// For resources with size larger than 16MB~ the actual size is obfuscated in resource header, see fiPackfile::GetResourceSize
		bool IsBigResource() const { return CompressedSize == MAX_SIZE; }
		u32  GetCompressedSize() const { return IsDirectory() ? 0 : IsCompressed() && !IsBigResource() ? CompressedSize : 0; } // Don't return MAX_FILE for big resources
		u32  GetUncompressedSize() const { return IsFile() ? File.UncompressedSize : 0; } // Uncompressed size is not defined for resources
	};

	class fiCollection : public fiDevice
	{
		bool m_IsStreaming = true;
	public:
		virtual void Shutdown() {}
		virtual fiHandle_t OpenBulkEntry(u32 index, u64& offset) const = 0;
		virtual const fiPackEntry& GetEntry(u32 index) const = 0;
		virtual u32 GetEntryPhysicalSortKey(u32 index, bool uncached) const = 0;
		virtual ConstString GetEntryName(u32 index) const = 0;
		virtual ConstString GetEntryFullName(u32 index, char* dest, int destSize) const = 0;
		virtual int GetEntryIndex(const char* name) const = 0;
		virtual u32 GetBasePhysicalSortKey() const = 0;
		virtual bool Prefetch(u32 index) const = 0;
		virtual bool IsPackfile() const { return false; }
		virtual bool IsStreaming() const { return m_IsStreaming; }
		virtual void SetStreaming(bool val) { m_IsStreaming = val; }
	};

	/**
	 * \brief Device that represents .RPF 7 archive.
	 * NOTE: File system in packfile is lower-case, all entries are sorted for faster search.
	 */
	class fiPackfile : public fiCollection
	{
		char*        m_NameHeap;	   // Heap where each file name is placed next to each other
		u16*         m_ParentIndices;
		fiPackEntry* m_Entries;
		u32          m_EntryCount;
		fiHandle_t   m_Handle = FI_INVALID_HANDLE;
		u64          m_FileTime;
		u64          m_ArchiveOffset;  // In parent archive (if nested)
		u64          m_ArchiveSize;
		fiDevice*    m_Device;
		int          m_RelativeOffset; // Length of mounting prefix (i.e. 'common:/)
		char         m_Name[32];
		atString     m_Fullname;
		bool         m_IsXCompressed;
		bool         m_IsByteSwapped;
		u16          m_SelfIndex = 0;  // Index in fiCollection::sm_Collections
		u32          m_SortKey;
		void*        m_DrmKey;		   // Only for PS3
		char*        m_MemoryBuffer;
		bool         m_IsRpf;
		bool         m_IsCached;
		bool         m_IsInstalled;
		bool         m_IsUserHeader;
		u32          m_AllocatedSize = 0; // Called 'm_CachedMetaDataSize' originally, we use it to store total size of allocated space instead
		u32          m_CachedDataSize; // Size of header + entries
		u32          m_EncryptionKey;
		u8           m_NameShift;
		bool         m_ForceOnOdd = false; // Optical device, only for consoles NOLINT(clang-diagnostic-unused-private-field)
		bool         m_KeepNameHeap = false;

	public:
		fiPackfile() = default;
		~fiPackfile() override;

		void Shutdown() override;
		bool Init(ConstString archivePath, fiPackfile* parent = nullptr, fiPackEntry* entry = nullptr);
		// Init with exception handler, to make sure custom RPFs don't cause crash
		// We allow providing custom nested parent because fiDevice::GetDevice is not only slow in native implementation, but also won't work for RPFs 'opened outside'
		bool InitSafe(ConstString archivePath, fiPackfile* parent = nullptr, fiPackEntry* entry = nullptr);
		bool ReInit(ConstString archivePath);
		void UnInit();

		// Gets device where this archive is lies, used for e.g. to tell if this is a nested archive
		fiDevice* GetParentDevice() const { return m_Device; }
		// Whether this archive is placed inside another one (nested)
		bool IsNestedArchive() const { return m_Device && m_Device->IsRpf(); }
		fiHandle_t GetHandle() const { return m_Handle; }
		// Assuming given entry is nested RPF, reads header and computes hash sum of nametable and entries section (without decryption)
		u32 ComputeSearchHashSumOfNestedArchive(const fiPackEntry& entry) const;

		u32 GetParentIndex(u32 entryIndex) const { return m_ParentIndices[entryIndex]; }

		// How much this packfile takes + name heap + entries + parent indices
		u32 GetAllocatedSize() const { return m_AllocatedSize; }

		// NOTE: This function (as well as Open/OpenBulk) use binary entry search,
		// instead it is recommended to use rageam::FileCache + OpenBulkEntry, which is magnitude faster
		virtual fiPackEntry* FindEntry(ConstString path) const;
		// Fastest way to open entry in archive, as long as entry was already found
		fiHandle_t   OpenBulkEntry(u32 index, u64& offset) const override;
		fiHandle_t   OpenBulkEntry(const fiPackEntry& entry, u64& offset) const;
		fiPackEntry* GetEntries() const { return m_Entries; }
		u32          GetEntryCount() const { return m_EntryCount; }
		fiPackEntry& GetEntry(u32 index) const override { return m_Entries[index]; }
		u32          GetEntryPhysicalSortKey(u32 index, bool uncached) const override { return 0; }
		int          GetEntryIndex(ConstString name) const override; // -1 if entry wasn't found
		int          GetEntryIndex(const fiPackEntry& entry) const;
		ConstString  GetEntryName(u32 index) const override;
		ConstString  GetEntryName(const fiPackEntry& entry) const { return GetEntryName(GetEntryIndex(entry)); }
		ConstString  GetEntryFullName(u32 index, char* dest, int destSize) const override; // Quite slow, file cache... caches full paths
		u64          GetEntrySize(const fiPackEntry& entry) const { return entry.GetUncompressedSize(); }
		u64          GetEntryCompressedSize(const fiPackEntry& entry); // Handles big (16MB+) resources 
		u32          GetResourceCompressedSize(const fiPackEntry& entry);
		u32          GetResourceInfo(const fiPackEntry& entry, datResourceInfo& info) const;
		fiPackEntry& GetRootEntry() const { return m_Entries[0]; }

		ConstString GetFullName() const { return m_Fullname; }
		ConstString GetDebugName() override { return m_Name; }
		u64 GetPackfileSize() const { return m_ArchiveSize; }
		u64 GetPackfileTime() const { return m_FileTime; }
		u32 GetBasePhysicalSortKey() const override { return m_SortKey; }

		virtual fiHandle_t OpenDirect(const char* filename, bool readOnly) const { return FI_INVALID_HANDLE; }
		fiHandle_t         Open(ConstString path, bool isReadOnly) override { return FI_INVALID_HANDLE; }
		fiHandle_t         OpenBulk(ConstString path, u64& offset) override { return FI_INVALID_HANDLE; }
		fiHandle_t         Create(ConstString path) override { return FI_INVALID_HANDLE; }

		u32  Read(fiHandle_t file, pVoid buffer, u32 size) override { return 0; }
		u32  ReadBulk(fiHandle_t file, u64 offset, pVoid buffer, u32 size) override;
		u32  Write(fiHandle_t file, pConstVoid buffer, u32 size) override { return 0; }
		u32  Seek(fiHandle_t file, s32 offset, eFiSeekWhence whence) override { return 0; }
		u64  Seek64(fiHandle_t file, s64 offset, eFiSeekWhence whence) override { return 0; }
		u32  Size(fiHandle_t file) override { return 0; }
		u64  Size64(fiHandle_t file) override { return 0; }
		bool Close(fiHandle_t file) override { return false; }
		bool CloseBulk(fiHandle_t file) override { return true; /* Bulk doesn't allocate any handle */ }
		u64  GetFileSize(ConstString path) override { return 0; }
		u64  GetFileTime(ConstString path) override { return m_FileTime; /* Modify time is not stored for entries... */ }
		bool SetFileTime(ConstString path, u64 time) override { return false; }
		u32  GetAttributes(ConstString path) override { return 0; }

		fiHandle_t FindFileBegin(ConstString path, fiFindData& findData) override { return 0; }
		bool       FindFileNext(fiHandle_t file, fiFindData& findData) override { return false; }
		bool       FindFileEnd(fiHandle_t file) override { return false; }

		u32 GetBulkOffset(ConstString path) override { return 0; }
		u32 GetPhysicalSortKey(ConstString path) override { return 0; }
		u32 GetPackfileIndex() override { return m_SelfIndex; }
		u32 GetEncryption() override { return m_EncryptionKey; }
		u64 GetRootDeviceId(ConstString fileName) override { return m_Device->GetRootDeviceId(m_Fullname); }

		bool IsRpf() const override { return m_IsRpf; }
		bool IsPackfile() const override { return true; }
		bool Prefetch(u32 index) const override { return false; }
	};
}
