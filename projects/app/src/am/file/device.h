//
// File: device.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "fileutils.h"
#include "iterator.h"
#include "am/types.h"
#include "am/system/datamgr.h"
#include "am/system/singleton.h"
#include "rage/file/packfile.h"

//
// This file contains high-level wrappers for fiDevice/fiPackfile, including caching and nice&fast search API.
//
// TODO: We need to check if packfiles were modified (just by looking at write time) before accesing them in lazy device
//

namespace rageam::file
{
	using PackfileUPtr = amUPtr<rage::fiPackfile>;

	struct PackfileCache
	{
		static constexpr ConstWString FILE_EXT = L"packcache";
		static constexpr int FILE_VER = 0;
		static constexpr int NAME_SHIFT = 4; // Every string is aligned to 16

		u32			   PathHash; // Normalized path hash to the archive
		u32			   FullNameHeapSize;
		amUPtr<char[]> FullNameHeap;
		amUPtr<u16[]>  EntryToFullNameOffset;

		WPath GetPath() const { return DataManager::GetPackCacheFolder() / String::FormatTemp(L"%X.%s", PathHash, FILE_EXT); }

		// Builds map for given packfile
		void Init(u32 pathHash, const rage::fiPackfile* packfile);

		void WriteToFile() const;
		bool ReadFromFile();

		ConstString GetFullEntryPath(u32 entryIndex) const { return FullNameHeap.get() + (EntryToFullNameOffset[entryIndex] << NAME_SHIFT); }

	private:
		void BuildMap(const rage::fiPackfile* packfile, u32 dirIndex = 0, const Path& dirPath = "");
	};

	enum FileEntryType
	{
		FileEntry_File,		 // Any binary file exlucindg resources and packfiles
		FileEntry_Packfile,  // RPFs
		FileEntry_Directory, // Folders
		FileEntry_Resource,  // Drawables / Fragments / Texture Dicts / *
	};

	enum FileEncryption
	{
		FileEncryption_None,
		FileEncryption_TFIT,
		FileEncryption_AES,
	};

	enum FileSearchInclude_
	{
		FileSearchInclude_None      = 0,
		FileSearchInclude_File      = 1 << 0,
		FileSearchInclude_Archive   = 1 << 1,
		FileSearchInclude_Directory = 1 << 2,
		FileSearchInclude_Resource  = 1 << 3,
		FileSearchInclude_All       = 0xF,
	};
	using FileSearchInclude = int;

	struct FileSearchData
	{
		rage::fiPackfile*  Packfile; // NULL if entry is not in packfile
		rage::fiPackEntry* Entry;
		WPath			   Path;
		FileEntryType	   Type;
	};
	using FileSearchFn = std::function<void(FileSearchData)>;

	/**
	 * \brief High-level wrapper for fiDevice (primarily for fiPackfile) with caching.
	 * NOTE: This class is not thread-safe! Use async functions for making requests from background threads.
	 */
	class FileDevice : public Singleton<FileDevice>
	{
		using PackfileEnumerateFn = std::function<void(rage::fiPackfile*, rage::fiPackEntry&, ConstString fullPath, bool isPackfile)>;
		using PackfileIndex = s32;

		struct Packfile
		{
			u64			  WriteTime; // Only for root packfiles
			PackfileIndex ParentIndex;
			PackfileUPtr  Device; // Never NULL
			PackfileCache Cache;
		};

		HashSet<PackfileIndex>      m_PathToPackfile;  // Key is normalized path
		HashSet<PackfileIndex>      m_EntryToPackfile; // Key is entry pointer
		List<Packfile>              m_Packfiles;
		bool                        m_Initialized = false;

		static WPath GetNormalizedPath(ConstWString path) { WPath p = path; p.Normalize(); return p; }

		// Create map to link entry name hashes (and their paths) to their indices, does nothing if packfile was already cached
		// Returns NULL only in case if archive was failed to init
		// Packfile and handle must be specified for nested archives, it will make loading faster
		PackfileIndex EnsurePackfileIsCached(ConstWString path, rage::fiPackfile* parent, rage::fiPackEntry* entry);
		// ScanAndCachePackfilesRecurse but for nested packfiles
		void ScanAndCacheNestedPackfilesRecurse(PackfileIndex rootPackfileIndex, const WPath& basePath, const WPath& relativePath = L"");
		// Recursevly iterates all packfiles in specified base directory
		void ScanAndCachePackfilesRecurse(const WPath& basePath, const WPath& relativePath = L"");
		// This is a cached up version to prevent unnecessary slow lookup of search directory in archive every call
		void EnumeratePackfileRecurse(const Packfile& packfile, rage::fiPackEntry* entry, bool recurse, const PackfileEnumerateFn& findFn) const;
		void EnumeratePackfile(ConstWString path, const PackfileEnumerateFn& findFn, bool recurse = true);

		PackfileIndex LookupPackfileInCacheOrOpen(ConstWString normalizedPath);

	public:
		void Update();

		// Removes all cached packfiles. Can be used to clean up memory or do a quick fix if cache system state is corrupted
		void FlushCache()
		{
			m_EntryToPackfile.Destroy();
			m_PathToPackfile.Destroy();
			m_Packfiles.Destroy();
		}

		// Scans and caches all RPFs in specified directory recursevly, including nested packfiles
		void ScanDirectory(ConstWString directory) { ScanAndCachePackfilesRecurse(directory); }

		// Magnitude faster than ::Search because of flat lookup in all loaded archives, no hierarchy iterating what so ever
		// NOTES:
		// - Call ScanDirectory before performing search
		// - Only works with .RPF files, no OS file system support, use regular search if that's needed
		// - Performs search in WHOLE cache, if you've called ScanDirectory before with different path, those archives will be included in results too
		void SearchInPackfileCache(ConstString pattern, const FileSearchFn& onFindFn, FileSearchInclude includeMask = FileSearchInclude_All) const;

		// Search algorithm that behaves the same way as search in any file explorer - performs search in specified directory with search pattern,
		// pattern might be in glob format ('*.ydr' / '*' / '*.y??'). If glob format is not specified, check that entry name string contains pattern is used instead
		void Search(ConstWString path, ConstWString pattern, const FileSearchFn& onFindFn, bool recurse, FileSearchInclude includeMask = FileSearchInclude_All);

		// Enumerates all entries in directory or archive specified in path
		// Call ScanDirectory to pre-cache all packfiles before searching for better performance
		void Enumerate(ConstWString path, const FileSearchFn& onFindFn, bool recurse);

		// Could be file or directory, including entries in packfiles
		bool IsFileExists(ConstWString path)
		{
			if (IsInPackfile(path))
				return GetPackfile(path);
			return file::IsFileExists(path); // Fallback to OS file system
		}

		// Tries to find already cached device or opens it
		// Note that if packfiles is nested, all parent packfiles will be open and cached too
		rage::fiPackfile* GetPackfile(ConstWString path);

		// Gets packfile from entry pointer. Entry must be a packfile, not a file located in it!
		rage::fiPackfile* GetPackfile(const rage::fiPackEntry* entry);

		// Tiny helper to tell whether there's packfile in path
		bool IsInPackfile(ConstWString path) const { return ImmutableWString(path).IndexOf(L".rpf") >= 0; }
	};
}
