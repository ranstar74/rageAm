#include "device.h"
#include "bbuffer.h"
#include "glob.h"

#include <Tracy.hpp>

#include "rage/framework/streaming/assettypes.h"

void rageam::file::PackfileCache::Init(u32 pathHash, const rage::fiPackfile* packfile)
{
	ZoneScoped;
	PathHash = pathHash;
	BuildMap(packfile);

	ZoneNameF("Full Name Heap");
	FullNameHeapSize = 0;
	EntryToFullNameOffset = std::unique_ptr<u16[]>(new u16[packfile->GetEntryCount()]);
	List<char> nameHeap; // This can be done simpler using bump allocator...
	nameHeap.Reserve(80 * 10); // There's average 80 entries in archive, every entry is average 10 characters long
	for (u32 i = 0; i < packfile->GetEntryCount(); i++)
	{
		char fullName[FI_MAX_PATH];
		packfile->GetEntryFullName(i, fullName, FI_MAX_PATH);
		u32 fullNameSize = ALIGN_16(strlen(fullName) + 1); // We append aligned names so we don't have to re-align later...
		u32 heapOffset = nameHeap.GetSize();
		u32 nameOffset = heapOffset >> NAME_SHIFT;
		nameHeap.GrowCapacity(heapOffset + fullNameSize);
		nameHeap.SetSize(heapOffset + fullNameSize);
		memcpy(nameHeap.GetItems() + heapOffset, fullName, fullNameSize);
		FullNameHeapSize += fullNameSize;
		EntryToFullNameOffset[i] = nameOffset;
	}
	FullNameHeap = std::unique_ptr<char[]>(nameHeap.MoveBuffer());
}

void rageam::file::PackfileCache::WriteToFile() const
{
	/*ZoneScoped;
	BinaryBuffer buffer;
	buffer.Write(FILE_VER);
	buffer.Write(WriteTime);
	buffer.Write(Map.Paths.GetNumUsedSlots());
	buffer.Write(Map.Names.GetNumUsedSlots());
	for (auto it = Map.Paths.begin(); it != Map.Paths.end(); ++it)
	{
		buffer.Write(it.GetKey());
		buffer.Write(it.GetValue());
	}
	for (auto it = Map.Names.begin(); it != Map.Names.end(); ++it)
	{
		const List<u32>& indices = it.GetValue();
		buffer.Write(it.GetKey());
		buffer.Write(indices.GetSize());
		for (u32 index : indices)
			buffer.Write(index);
	}
	buffer.ToFile(GetPath(), ENABLE_PACKFILE_CACHE_COMPRESSION);*/
}

bool rageam::file::PackfileCache::ReadFromFile()
{
	//ZoneScoped;
	//BinaryBuffer buffer;
	//if (!buffer.FromFile(GetPath()))
	//	return false;
	//if (buffer.Read<int>() != FILE_VER)
	//	return false;
	//if (buffer.Read<u64>() != WriteTime)
	//	return false;
	//u32 numPaths = buffer.Read<u32>();
	//u32 numNames = buffer.Read<u32>();
	//for (u32 i = 0; i < numPaths; i++)
	//	Map.Paths.InsertAt(buffer.Read<u32>(), buffer.Read<u32>());
	//for (u32 i = 0; i < numNames; i++)
	//{
	//	u32 key = buffer.Read<u32>();
	//	u32 numIndices = buffer.Read<u32>();
	//	Map.Names[key].Reserve(numIndices);
	//	for (u32 k = 0; k < numIndices; k++)
	//		Map.Names[key].Add(k);
	//}
	return true;
}

void rageam::file::PackfileCache::BuildMap(const rage::fiPackfile* packfile, u32 dirIndex, const Path& dirPath)
{
	rage::fiPackEntry& dirEntry = packfile->GetEntry(dirIndex);
	u32 start = dirEntry.Directory.StartIndex;
	u32 stop = start + dirEntry.Directory.ChildCount;
	for (u32 i = start; i < stop; i++)
	{
		ConstString entryName = packfile->GetEntryName(i);
		Path entryPath = dirPath / entryName;

		if (packfile->GetEntry(i).IsDirectory())
			BuildMap(packfile, i, entryPath);
	}
}

rageam::file::FileDevice::PackfileIndex rageam::file::FileDevice::EnsurePackfileIsCached(ConstWString path, rage::fiPackfile* parent, rage::fiPackEntry* entry)
{
	WPath normalizedPath = GetNormalizedPath(path);
	HashValue pathHash = Hash(normalizedPath);
	PackfileIndex* cachedIndex = m_PathToPackfile.TryGetAt(pathHash);
	if (cachedIndex)
		return *cachedIndex;

	PackfileUPtr device = std::make_unique<rage::fiPackfile>();
	if (!device->InitSafe(PATH_TO_UTF8(path), parent, entry))
	{
		AM_ERRF(L"FileCache::CachePackfile() -> Failed to initialize cache for '%ls'!", path);
		return -1;
	}

	Packfile packfile;
	packfile.WriteTime = device->GetPackfileTime();
	packfile.Cache.Init(pathHash, device.get());
	packfile.Device = std::move(device);
	PackfileIndex newIndex = m_Packfiles.GetSize();
	m_PathToPackfile.InsertAt(pathHash, newIndex);
	if (entry) // NULL if not nested
		m_EntryToPackfile.InsertAt(DataHash(entry, 8), newIndex);
	m_Packfiles.Emplace(std::move(packfile));
	return newIndex;
}

void rageam::file::FileDevice::ScanAndCacheNestedPackfilesRecurse(PackfileIndex rootPackfileIndex, const WPath& basePath, const WPath& relativePath)
{
	// What's good is that we don't have to iterate recursevly, hierarchy is laid out flat
	rage::fiPackfile* packfile = m_Packfiles[rootPackfileIndex].Device.get();
	for (u32 i = 0; i < packfile->GetEntryCount(); i++)
	{
		rage::fiPackEntry& entry = packfile->GetEntry(i);
		if (!entry.IsFile())
			continue;

		ConstString entryName = packfile->GetEntryName(i);
		if (!String::Equals(GetExtension(entryName), "rpf", true))
			continue;

		WPath currentRelativePath = relativePath / PATH_TO_WIDE(m_Packfiles[rootPackfileIndex].Cache.GetFullEntryPath(i));

		PackfileIndex packfileIndex;
		{
			ConstString rpfPath = PATH_TO_UTF8(currentRelativePath);
			ConstString rpfName = GetFileName(rpfPath);
			ZoneScoped;
			ZoneNameF(rpfName);
			ZoneColor(tracy::Color::Blue);
			ZoneTextF(rpfPath, 1);

			// Cache packfile
			WPath fullPath = basePath / currentRelativePath;
			packfileIndex = EnsurePackfileIsCached(fullPath, packfile, &entry);
		}

		// Failed to load
		if (packfileIndex < 0)
			continue;

		// Recursevly scan it for nested packfiles
		ScanAndCacheNestedPackfilesRecurse(packfileIndex, basePath, currentRelativePath);
	}
}

void rageam::file::FileDevice::ScanAndCachePackfilesRecurse(const WPath& basePath, const WPath& relativePath)
{
	Iterator iterator(basePath / relativePath / L"*.*");
	FindData findData;
	while (iterator.Next())
	{
		iterator.GetCurrent(findData);

		WPath currentRelativePath = relativePath / findData.Name;

		// A packfile, cache it and all nested packfiles
		if (String::Equals(GetExtension<wchar_t>(findData.Name), L"rpf", true))
		{
			ConstString rpfPath = PATH_TO_UTF8(currentRelativePath);
			ConstString rpfName = GetFileName(rpfPath);
			ZoneScoped;
			ZoneNameF(rpfName);
			ZoneColor(tracy::Color::Orange);
			ZoneTextF(rpfPath, 1);
			PackfileIndex packfileIndex = EnsurePackfileIsCached(findData.Path, nullptr, nullptr);
			if (packfileIndex >= 0) // Make sure it loaded
			{
				// Now that packfile is cached, we have to iterate nested archives, though this is much easier
				ScanAndCacheNestedPackfilesRecurse(packfileIndex, basePath, currentRelativePath);
			}
		}
		// A directory, scan recursevly
		else if (IsDirectory(findData.Path))
		{
			ScanAndCachePackfilesRecurse(basePath, currentRelativePath);
		}
	}
}

bool rageam::file::FileDevice::EnumeratePackfileRecurse(const Packfile& packfile, rage::fiPackEntry* entry, bool recurse, const PackfileEnumerateFn& findFn) const
{
	ZoneScoped;
	u32 start = entry->Directory.StartIndex;
	u32 end = start + entry->Directory.ChildCount;
	for (u32 i = start; i < end; i++)
	{
		entry = &packfile.Device->GetEntry(i);

		// Combine full entry name (including path to packfile in win file system)
		ImmutableString entryName = packfile.Device->GetEntryName(i);
		Path entryPath = packfile.Device->GetFullName();
		entryPath /= packfile.Cache.GetFullEntryPath(i);

		// Only files can be ... packfiles
		bool isPackfile = entry->IsFile() && entryName.EndsWith(".rpf");

		if (!findFn(packfile.Device.get(), *entry, entryPath, isPackfile))
			return false;

		if (recurse && isPackfile)
		{
			PackfileIndex* nestedPackfileIndex = m_EntryToPackfile.TryGetAt(DataHash(entry, 8));
			if (nestedPackfileIndex) // Might be NULL if failed loading
			{
				const Packfile& nestedPackfile = m_Packfiles[*nestedPackfileIndex];
				if (!EnumeratePackfileRecurse(nestedPackfile, &nestedPackfile.Device->GetRootEntry(), true, findFn))
					return false;
			}
		}
		else if (recurse && entry->IsDirectory())
		{
			if (!EnumeratePackfileRecurse(packfile, entry, recurse, findFn))
				return false;
		}
	}
	return true;
}

void rageam::file::FileDevice::EnumeratePackfile(ConstWString path, const PackfileEnumerateFn& findFn, bool recurse)
{
	ZoneScoped;
	PackfileIndex lazyIndex = LookupPackfileInCacheOrOpen(path);
	if (lazyIndex < 0)
		return;

	Packfile& packfile = m_Packfiles[lazyIndex];

	// Unicode is not supported in packfiles... instead of doing unicode conversion, cast all wide characters to ansi
	Path ansiPath;
	String::ToAnsi(ansiPath.GetBuffer(), MAX_PATH, path);

	// Get entry (directory, in our case) path relative to packfile
	Path entryPath = ansiPath.GetRelativePath(packfile.Device->GetFullName());

	rage::fiPackEntry* entry = packfile.Device->FindEntry(entryPath);
	if (entry && entry->IsDirectory())
		(void) EnumeratePackfileRecurse(packfile, entry, recurse, findFn);
}

rageam::file::FileDevice::PackfileIndex rageam::file::FileDevice::LookupPackfileInCacheOrOpen(ConstWString normalizedPath)
{
	ZoneScoped;
	HashValue pathHash = Hash(normalizedPath);
	PackfileIndex* packfileIndex = m_PathToPackfile.TryGetAt(pathHash);
	if (packfileIndex)
		return *packfileIndex;

	// We have to store parent for loading nested archives
	PackfileIndex currentPackfileIndex = -1;
	rage::fiPackEntry* currentPackEntry = nullptr;

	// Build path to every packfile in absolute path, and load them in order
	// for e.g. absolute path "Grand Theft Auto V/x64h.rpf/levels/gta5/props/roadside/v_rubbish.rpf"
	// Algorithm output:
	// "Grand Theft Auto V/x64h.rpf"
	// "levels/gta5/props/roadside/v_rubbish.rpf"
	WPath pathCopy = normalizedPath;
	MutableWString cursor = pathCopy.GetBuffer();
	s32 nextIndex;
	while ((nextIndex = cursor.IndexOf(L".rpf")) != -1)
	{
		// Move cursor right after packfile extension
		wchar_t* subPathEnd = cursor + nextIndex + 4;
		// Instead of copying buffer we artificially trim path string
		wchar_t  oldChar = *subPathEnd;
		*subPathEnd = 0;
		{
			ConstWString subPath = cursor;

			// If we are not in archive yet, first check if RPF file exists at all, before doing any loading
			if (currentPackfileIndex < 0 && !file::IsFileExists(cursor))
				return -1;

			// If were in archive, we need to find current packfile entry
			if (currentPackfileIndex >= 0)
			{
				currentPackEntry = m_Packfiles[currentPackfileIndex].Device->FindEntry(PATH_TO_UTF8(subPath));
				if (!currentPackEntry) // Path is invalid, entry doesn't exists...
					return -1;
			}

			// Navigate to next packfile in sub path
			rage::fiPackfile* currentDevice = currentPackfileIndex >= 0 ? m_Packfiles[currentPackfileIndex].Device.get() : nullptr;
			PackfileIndex newPackfileIndex = EnsurePackfileIsCached(cursor, currentDevice, currentPackEntry);
			if (newPackfileIndex < 0)
			{
				AM_ERRF(L"FileCache::GetDevice() -> Failed to load archive %ls", subPath);
				return -1;
			}
			currentPackfileIndex = newPackfileIndex;
		}
		*subPathEnd = oldChar;
		// Path ends here
		if (!*subPathEnd)
			break;
		// Move cursor right after ".rpf/"
		cursor = subPathEnd + 1;
	}
	return currentPackfileIndex;
}

#include <am/graphics/dx11.h>
#include <am/graphics/render.h>
void rageam::file::FileDevice::Update()
{
	if (!m_Initialized)
	{
		CreateDirectoryW(DataManager::GetPackCacheFolder(), nullptr);
		m_Initialized = true;

		// Benchmark testing code
		/*
		ScanDirectory(L"C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Grand Theft Auto V");
		rage::fiPackfile* packfile = GetPackfile(L"C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Grand Theft Auto V\\x64q.rpf\\levels\\gta5\\_hills\\country_01\\cs1_04.rpf");
		Timer timer = Timer::StartNew();
		int count = 0;
		int totalEntries = 0;
		constexpr int TOTAL_ITERATIONS = 1;
		constexpr double TOTAL_ITERATIONS_D = (double)TOTAL_ITERATIONS;
		for (int i = 0; i < TOTAL_ITERATIONS; i++)
		{
			totalEntries = 0;
			Search(L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Grand Theft Auto V\\common.rpf\\", L"*", [&](const FileSearchData& search)
				{
					//AM_TRACEF(search.Path);
					count++;
					totalEntries++;
				}, true, FileSearchInclude_All);
		}
		timer.Stop();
		double avgTime = ((double)timer.GetElapsedMilliseconds() / TOTAL_ITERATIONS_D);
		double entryPerSecond = (double)totalEntries * (1000.0 / avgTime);
		AM_TRACEF("%.02fms average, %u entries, %.02f entry/s", avgTime, totalEntries, entryPerSecond);
		*/
	}
}

void rageam::file::FileDevice::SearchInPackfileCache(ConstString pattern, const FileSearchFn& onFindFn, FileSearchInclude includeMask) const
{
	// Convert search string to lower case
	char processedPattern[MAX_PATH];
	strcpy_s(processedPattern, MAX_PATH, pattern);
	MutableString(processedPattern).ToLower();

	// We support the most simple glob, only '*' and '?'
	bool doGlobSearch = MutableString(processedPattern).IndexOf<'*', '?'>() != -1;

	// Iterate all entries in loaded packfiles
	for (const Packfile& lazy : m_Packfiles)
	{
		rage::fiPackfile* packfile = lazy.Device.get();
		u32 entryCount = packfile->GetEntryCount();
		for (u32 i = 1 /* Root dir is implicit, skip... */; i < entryCount; i++)
		{
			rage::fiPackEntry& entry = packfile->GetEntry(i);
			ImmutableString entryName = packfile->GetEntryName(i);

			// Match include type mask
			FileEntryType type;
			if (entry.IsResource) type = FileEntry_Resource;
			else if (entry.IsDirectory()) type = FileEntry_Directory;
			else if (entryName.EndsWith(".rpf")) type = FileEntry_Packfile;
			else type = FileEntry_File;
			if (!(includeMask & 1 << type))
				continue;

			// Compare name with search request
			if (doGlobSearch && !GlobMatch<char>(entryName, processedPattern))
				continue;
			if (!doGlobSearch && !entryName.Contains(processedPattern))
				continue;

			FileSearchData searchResult;
			searchResult.Packfile = packfile;
			searchResult.Entry = &entry;
			searchResult.Path = PATH_TO_WIDE(lazy.Cache.GetFullEntryPath(i));
			searchResult.Type = type;
			if (!onFindFn(searchResult))
				return;
		}
	}
}

void rageam::file::FileDevice::Search(ConstWString path, ConstWString pattern, const FileSearchFn& onFindFn, bool recurse, FileSearchInclude includeMask)
{
	// Convert search string to lower case
	wchar_t processedPattern[MAX_PATH];
	wcscpy_s(processedPattern, MAX_PATH, pattern);
	MutableWString(processedPattern).ToLower();

	// We support the most simple glob, only '*' and '?'
	bool doGlobSearch = ImmutableWString(processedPattern).IndexOf<L'*', L'?'>() != -1;

	Enumerate(path, [&](const FileSearchData& searchData)
	{
		// Match include type mask
		if (!(includeMask & 1 << searchData.Type))
			return true;

		ConstWString entryName = GetFileName<wchar_t>(searchData.Path);

		// Compare name with search request
		if (doGlobSearch && !GlobMatch(entryName, processedPattern))
			return true;
		if (!doGlobSearch && !ImmutableWString(entryName).Contains(processedPattern))
			return true;

		return onFindFn(searchData);
	}, recurse);
}

void rageam::file::FileDevice::Enumerate(ConstWString path, const FileSearchFn& onFindFn, bool recurse)
{
	WPath normalizedPath = path;
	normalizedPath.Normalize();

	// Put in lambda so it can be called from windows enumerate directory
	auto enumeratePackfile = [&] (ConstWString path_)
	{
		EnumeratePackfile(path_, [&](rage::fiPackfile* packfile, rage::fiPackEntry& entry, ConstString fullPath, bool isPackfile)
		{
			FileSearchData data;
			data.Packfile = packfile;
			data.Entry = &entry;
			data.Path = PATH_TO_WIDE(fullPath);
			if (isPackfile) 
				data.Type = FileEntry_Packfile;
			else if (entry.IsResource) 
				data.Type = FileEntry_Resource;
			else if (entry.IsDirectory()) 
				data.Type = FileEntry_Directory;
			else 
				data.Type = FileEntry_File;
			return onFindFn(data);
		}, recurse);
	};

	// If there's packfile in search path we don't need to search in os file system
	if (IsInPackfile(normalizedPath))
	{
		enumeratePackfile(normalizedPath);
		return;
	}

	EnumerateDirectory(normalizedPath, recurse, [&](const WIN32_FIND_DATAW& findData, ConstWString fullPath)
	{
		ConstString fileNameA = String::ToAnsiTemp(findData.cFileName);

		FileSearchData data;
		data.Packfile = nullptr;
		data.Entry = nullptr;
		data.Path = fullPath;
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			data.Type = FileEntry_Directory;
		else if (ImmutableString(fileNameA).EndsWith(".rpf"))
			data.Type = FileEntry_Packfile;
		else if (rage::IsResourceExtension(GetExtension(fileNameA)))
			data.Type = FileEntry_Resource;
		else
			data.Type = FileEntry_File;

		onFindFn(data);

		// We found packfile in os file system, let's enumerate it...
		if (recurse && data.Type == FileEntry_Packfile)
			enumeratePackfile(fullPath);
	});
}

bool rageam::file::FileDevice::IsFileExists(ConstWString path)
{
	if (!IsInPackfile(path))
		return file::IsFileExists(path); // Fallback to OS file system

	bool exists = false;
	WPath directory = path;
	directory = directory.GetParentDirectory();
	Path fileName = String::ToAnsiTemp(GetFileName(path));
	fileName.ToLower();

	Enumerate(directory, [&](const FileSearchData& searchData)
		{
			ConstString entryName = searchData.Packfile->GetEntryName(*searchData.Entry);
			if (!String::Equals(fileName, entryName))
				return true;
			exists = true;
			return false;
		}, false);

	return exists;
}

rage::fiPackfile* rageam::file::FileDevice::GetPackfile(ConstWString path)
{
	WPath normalizedPath = GetNormalizedPath(path);
	PackfileIndex packfileIndex = LookupPackfileInCacheOrOpen(normalizedPath);
	return packfileIndex < 0 ? nullptr : m_Packfiles[packfileIndex].Device.get();
}

rage::fiPackfile* rageam::file::FileDevice::GetPackfile(const rage::fiPackEntry* entry)
{
	AM_ASSERTS(entry);
	PackfileIndex* packfileIndex = m_EntryToPackfile.TryGetAt(DataHash(entry, 8));
	if (packfileIndex)
		return m_Packfiles[*packfileIndex].Device.get();
	return nullptr;
}
