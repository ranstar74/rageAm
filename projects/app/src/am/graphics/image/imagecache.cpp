#include "imagecache.h"

#include "am/file/fileutils.h"
#include "am/system/datamgr.h"
#include "am/xml/doc.h"
#include "am/file/iterator.h"
#include "am/xml/iterator.h"
#include "helpers/format.h"
#include "bc.h"

rageam::HashValue rageam::graphics::ImageCache::ComputeImageHash(ImagePixelData imageData, u32 imageDataSize, const CompressedImageInfo& compInfo) const
{
	u32 hash;
	hash = rage::atDataHash(imageData, imageDataSize);
	hash = rage::atDataHash(&compInfo, sizeof CompressedImageInfo, hash);
	return hash;
}

rageam::HashValue rageam::graphics::ImageCache::ComputeImageHash(ImagePixelData imageData, u32 imageDataSize) const
{
	return rage::atDataHash(imageData, imageDataSize);
}

void rageam::graphics::ImageCache::SaveSettings(const file::WPath& path, const Settings& settings) const
{
	XmlDoc xDoc("ImageCache");
	XmlHandle xRoot = xDoc.Root();
	xRoot.SetAttribute("Version", SETTINGS_VERSION);
	xRoot.AddComment("If not specified, the default data folder is used instead");
	xRoot.AddChild("CacheDirectory", String::ToUtf8Temp(settings.CacheDirectory));
	xRoot.AddComment("How much RAM cached images can take up (in MB)");
	xRoot.AddChild("MemoryStoreBudget").SetTheValueAttribute(BytesToMb(settings.MemoryStoreBudget));
	xRoot.AddComment("After image is unloaded from RAM it goes to FS, oldest images are removed from cache when FS budget is reached (in MB)");
	xRoot.AddChild("FileSystemStoreBudget").SetTheValueAttribute(BytesToMb(settings.FileSystemStoreBudget));
	xRoot.AddComment("Processing time in milliseconds required to add an image to the cache. This allows small images to be skipped.");
	xRoot.AddChild("TimeToCacheThreshold").SetTheValueAttribute(settings.TimeToCacheThreshold);
	xRoot.AddComment("Maximum number of cached DX11 textures");
	xRoot.AddChild("MaxDX11Views").SetTheValueAttribute(settings.MaxDX11Views);

	try
	{
		xDoc.SaveToFile(path);
	}
	catch (const XmlException& ex)
	{
		AM_UNREACHABLE("ImageCache::SaveSettings() -> Failed to save settings xml file...");
	}
}

void rageam::graphics::ImageCache::LoadSettings()
{
	file::WPath settingsPath = DataManager::GetAppData() / L"ImageCache.xml";

	// Settings file does not exist yet, create one with default settings
	if (!IsFileExists(settingsPath))
	{
		AM_DEBUGF(L"ImageCache::LoadSettings() -> Running first time, creating config at path '%ls'", settingsPath.GetCStr());

		Settings defaultSettings = {};
		defaultSettings.MemoryStoreBudget = DEFAULT_MEMORY_STORE_BUDGET;
		defaultSettings.FileSystemStoreBudget = DEFAULT_FILESYSTEM_STORE_BUDGET;
		defaultSettings.TimeToCacheThreshold = DEFAULT_TIME_TO_CACHE_THRESHOLD;
		defaultSettings.MaxDX11Views = DEFAULT_DX11_VIEWS_MAX;
		m_Settings = defaultSettings;

		SaveSettings(settingsPath, defaultSettings);
		return;
	}

	try
	{
		XmlDoc xDoc;
		xDoc.LoadFromFile(settingsPath);
		XmlHandle xRoot = xDoc.Root();

		u32 version;
		xRoot.GetAttribute("Version", version);
		switch (version)
		{
		case 0:
		{
			ConstString cacheDirectory = xRoot.GetChild("CacheDirectory").GetText(false);

			Settings settings = {};
			settings.CacheDirectory = String::Utf8ToWideTemp(cacheDirectory);
			xRoot.GetChild("MemoryStoreBudget").GetTheValueAttribute(settings.MemoryStoreBudget);
			xRoot.GetChild("FileSystemStoreBudget").GetTheValueAttribute(settings.FileSystemStoreBudget);
			xRoot.GetChild("TimeToCacheThreshold").GetTheValueAttribute(settings.TimeToCacheThreshold);
			xRoot.GetChild("MaxDX11Views").GetTheValueAttribute(settings.MaxDX11Views);
			settings.MemoryStoreBudget = MbToBytes(settings.MemoryStoreBudget);
			settings.FileSystemStoreBudget = MbToBytes(settings.FileSystemStoreBudget);
			m_Settings = settings;
			break;
		}

		default: AM_UNREACHABLE("ImageCache::LoadSettings() -> Version '%u' is not supported, is file valid?", version);
		}
	}
	catch (const XmlException& ex)
	{
		AM_UNREACHABLE("ImageCache::LoadSettings() -> Failed to load settings xml file: '%s' at line %u", ex.what(), ex.GetLine());
	}
}

bool rageam::graphics::ImageCache::GetInfoFromCachedImagePath(const file::WPath& path, u32& hash, u32& imageSize) const
{
	file::WPath fileName = path.GetFileNameWithoutExtension();
	hash = 0;
	imageSize = 0;
	return swscanf_s(fileName.GetCStr(), L"%u_%u", &hash, &imageSize) > 0;
}

rageam::file::WPath rageam::graphics::ImageCache::GetCachedImagePath(u32 hash, u32 imageSize, ImageFileKind kind) const
{
	file::WPath path = m_CacheDirectory / String::FormatTemp(L"%u_%u", hash, imageSize);
	if (kind != ImageKind_None)
	{
		path += L".";
		path += ImageKindToFileExtensionW[kind];
	}
	return path;
}

void rageam::graphics::ImageCache::MoveImageToFileSystem(CacheEntry& entry, u32 hash) const
{
	// We don't need image kind here because file extension will be set by save function
	file::WPath cachedImagePath = GetCachedImagePath(hash, entry.ImageSize, ImageKind_DDS);

	// We don't actually remove images from file system cache when loading to ram,
	// writing file is more expensive than deleting it when it is surely not needed anymore
	if (!IsFileExists(cachedImagePath) && !ImageFactory::SaveImage(entry.Image, cachedImagePath))
	{
		AM_ERRF(L"ImageCache::Cache() -> Failed to store image in file system '%ls'", cachedImagePath.GetCStr());
	}

	// This does not guarantee that image will be unloaded from RAM, it still may be referenced somewhere
	entry.Image = nullptr;
}

void rageam::graphics::ImageCache::CleanUpOldEntriesToFitBudget()
{
	// Move oldest image from ram to file system (or just unload if fs cache is not needed)
	while (m_SizeRam > m_Settings.MemoryStoreBudget)
	{
		u32 hash = m_NewToOldEntriesRAM.Last();
		m_NewToOldEntriesRAM.RemoveLast();

		CacheEntry& entry = m_Entries.GetAt(hash);

		m_SizeRam -= entry.ImageSize;

		if (entry.StoreInFileSystem)
		{
			MoveImageToFileSystem(entry, hash);
			m_SizeFs += entry.ImageSize;
			m_NewToOldEntriesFS.Insert(0, hash);
			IMAGE_CACHE_LOG("ImageCache::Cache() -> Image (hash: %x; size: %x or %s) was unloaded from memory to file",
				hash, entry.ImageSize, FormatBytesTemp(entry.ImageSize));
		}
		else
		{
			IMAGE_CACHE_LOG("ImageCache::Cache() -> Image (hash: %x; size: %x or %s) was marked as memory only, removing completely",
				hash, entry.ImageSize, FormatBytesTemp(entry.ImageSize));
			m_Entries.RemoveAt(hash);
		}
	}

	// Note that we don't actually remove the file, see comment in MoveImageToFileSystem
	while (m_SizeFs > m_Settings.FileSystemStoreBudget)
	{
		u32 hash = m_NewToOldEntriesFS.Last();
		m_NewToOldEntriesFS.RemoveLast();

		CacheEntry& entry = m_Entries.GetAt(hash);
		m_SizeFs -= entry.ImageSize;

		file::WPath oldImagePath = GetCachedImagePath(hash, entry.ImageSize, entry.ImageKind);
		if (!DeleteFileW(oldImagePath))
		{
			AM_WARNINGF("ImageCache::Cache() -> Image (hash: %x; size: %x or %s) failed to remove from file system",
				hash, entry.ImageSize, FormatBytesTemp(entry.ImageSize));
		}
		else
		{
			IMAGE_CACHE_LOG("ImageCache::Cache() -> Image (hash: %x; size: %x or %s) was removed from file system",
				hash, entry.ImageSize, FormatBytesTemp(entry.ImageSize));
		}

		m_Entries.RemoveAt(hash);
	}
}

rageam::graphics::ImageCache::ImageCache()
{
	LoadSettings();

	if (!String::IsNullOrEmpty(m_Settings.CacheDirectory))
		m_CacheDirectory = m_Settings.CacheDirectory;
	else
		m_CacheDirectory = DataManager::GetDataFolder() / DEFAULT_CACHE_DIRECTORY_NAME;

	AM_ASSERT(CreateDirectoryW(m_CacheDirectory, NULL) != ERROR_PATH_NOT_FOUND,
		L"ImageCompressorCache::LoadSettings() -> Failed to create cache directory '%ls'", m_CacheDirectory.GetCStr());

	file::WPath listPath = m_CacheDirectory / CACHE_LIST_NAME;
	// First load, no cache list is stored
	if (!IsFileExists(listPath))
		return;

	// Load images from file system
	try
	{
		XmlDoc xCacheList;
		xCacheList.LoadFromFile(listPath);
		XmlHandle xRoot = xCacheList.Root();

		u32 totalSize;
		xRoot.GetAttribute("TotalSize", totalSize);

		for (const XmlHandle& item : XmlIterator(xRoot, "Item"))
		{
			ConstString fileName = item.GetText();
			file::WPath filePath = m_CacheDirectory / file::PathConverter::Utf8ToWide(fileName);

			u32 hash, imageSize;
			ImageFileKind imageKind = ImageFactory::GetImageKindFromPath(filePath);

			if (imageKind == ImageKind_None || !GetInfoFromCachedImagePath(filePath, hash, imageSize))
			{
				AM_ERRF(L"Cache -> File name is invalid '%ls'", filePath.GetCStr());
				continue;
			}

			CacheEntry entry = {};
			entry.ImageKind = imageKind;
			entry.ImageSize = imageSize;
			entry.StoreInFileSystem = true;

			m_Entries.EmplaceAt(hash, std::move(entry));
			m_NewToOldEntriesFS.Add(hash);

		}

		m_SizeFs = totalSize;

		// Budget was lower since last load images don't fit anymore...
		if (m_SizeFs > m_Settings.FileSystemStoreBudget)
		{
			CleanUpOldEntriesToFitBudget();
		}
	}
	catch (const XmlException& ex)
	{
		ex.Print();
	}
}

rageam::graphics::ImageCache::~ImageCache()
{
	// Sanity check if size is accounted correctly and state is not corrupted
#ifdef DEBUG
	u64 sizeRam = 0;
	u64 sizeFs = 0;
	for (CacheEntry& entry : m_Entries)
	{
		if (entry.Image)
			sizeRam += entry.ImageSize;
		else
			sizeFs += entry.ImageSize;
	}

	AM_ASSERT(sizeRam == m_SizeRam, "ImageCache~ -> Ram size doesn't match (expected: %llu, actual: %llu)", sizeRam, m_SizeRam);
	AM_ASSERT(sizeFs == m_SizeFs, "ImageCache~ -> Fs size doesn't match (expected: %llu, actual: %llu)", sizeFs, m_SizeFs);
#endif

	// Store all images in file system
	for (u32 hash : m_NewToOldEntriesRAM)
	{
		CacheEntry& entry = m_Entries.GetAt(hash);

		if (entry.StoreInFileSystem)
		{
			MoveImageToFileSystem(entry, hash);
			m_SizeFs += entry.ImageSize;
			m_SizeRam -= entry.ImageSize;
			m_NewToOldEntriesFS.Insert(0, hash);
		}

		entry.Image = nullptr;
	}

	// We keep list to easily search for cached files and preserve new/old order
	XmlDoc xCacheList("CacheList");
	XmlHandle xRoot = xCacheList.Root();
	xRoot.SetAttribute("Version", CACHE_LIST_VERSION);
	xRoot.SetAttribute("TotalSize", m_SizeFs); // To see if budget lowered and we need clean up on loading
	for (u32 hash : m_NewToOldEntriesFS)
	{
		CacheEntry& entry = m_Entries.GetAt(hash);
		file::WPath entryPath = GetCachedImagePath(hash, entry.ImageSize, entry.ImageKind);
		entryPath = entryPath.GetFileName();
		xRoot.AddChild("Item", file::PathConverter::WideToUtf8(entryPath));
	}
	try
	{
		file::WPath listPath = m_CacheDirectory / CACHE_LIST_NAME;
		xCacheList.SaveToFile(listPath);
	}
	catch (const XmlException& ex)
	{
		ex.Print();
	}
}

rageam::graphics::ImagePtr rageam::graphics::ImageCache::GetFromCache(u32 hash, Vec2S* outUV2)
{
	std::unique_lock lock(m_Mutex);

	if (outUV2) *outUV2 = { 1.0f, 1.0f };

	CacheEntry* entry = m_Entries.TryGetAt(hash);
	if (!entry)
		return nullptr;

	// Image was unloaded to file system before, load it now
	if (!entry->Image)
	{
		file::WPath cachedImagePath = GetCachedImagePath(hash, entry->ImageSize, entry->ImageKind);

		// Load without using cache here or it will cause recursion...
		entry->Image = ImageFactory::LoadFromPath(cachedImagePath, false, false);

		// Remove hash entry from file system history
		for (u32 i = 0; i < m_NewToOldEntriesFS.GetSize(); i++)
		{
			if (m_NewToOldEntriesFS[i] == hash)
			{
				m_NewToOldEntriesFS.RemoveAt(i);
				break;
			}
		}
		m_SizeFs -= entry->ImageSize;

		// Add image back to ram if it was loaded successfully
		if (entry->Image)
		{
			m_SizeRam += entry->ImageSize;
			m_NewToOldEntriesRAM.Insert(0, hash);
		}
		else
		{
			AM_ERRF("ImageCache::GetFromCache() -> Failed to reload image from file system...");
			m_Entries.RemoveAt(hash);
			return nullptr;
		}

		// Image was loaded from file system to memory, revalidate budget
		CleanUpOldEntriesToFitBudget();
	}
	// Image was already loaded, we have to update it's position in history list
	else if (m_NewToOldEntriesFS.Any() && m_NewToOldEntriesFS.First() != hash)
	{
		for (u32 i = 0; i < m_NewToOldEntriesFS.GetSize(); i++)
		{
			if (m_NewToOldEntriesFS[i] == hash)
			{
				m_NewToOldEntriesFS.RemoveAt(i);
				m_NewToOldEntriesFS.Insert(0, hash);
				break;
			}
		}
	}

	if (outUV2) *outUV2 = entry->ImagePaddingUV2;
	return entry->Image;
}

bool rageam::graphics::ImageCache::GetFromCacheDX11(
	u32 hash, amComPtr<ID3D11ShaderResourceView>& outView, Vec2S* outUV2, amComPtr<ID3D11Texture2D>* tex)
{
	std::unique_lock lock(m_Mutex);

	CacheEntryDX11* entry = m_EntriesDX11.TryGetAt(hash);
	if (entry)
	{
		outView = entry->View;
		if (tex) *tex = entry->Tex;
		if (outUV2) *outUV2 = entry->PaddingUV2;
		return true;
	}
	return false;
}

void rageam::graphics::ImageCache::Cache(const ImagePtr& image, u32 hash, u32 imageSize, bool storeInFileSystem, Vec2S uv2)
{
	std::unique_lock lock(m_Mutex);

	IMAGE_CACHE_LOG("ImageCache::Cache() -> Adding to cache, hash: %x; image size: %x or %s; fs store: %i",
		hash, imageSize, FormatBytesTemp(imageSize), storeInFileSystem);

	if (imageSize > m_Settings.MemoryStoreBudget)
	{
		AM_WARNINGF("ImageCache::Cache() -> Single image size is larger than whole FS budget... image will not be added");
		return;
	}

	CacheEntry entry;
	entry.Image = image;
	entry.ImageSize = imageSize;
	entry.ImageKind = ImageIsCompressedFormat(image->GetPixelFormat()) ? ImageKind_DDS : ImageKind_PNG;
	entry.StoreInFileSystem = storeInFileSystem;
	entry.ImagePaddingUV2 = uv2;
	m_Entries.EmplaceAt(hash, std::move(entry));

	m_SizeRam += imageSize;
	m_NewToOldEntriesRAM.Remove(hash);
	m_NewToOldEntriesRAM.Insert(0, hash);

	CleanUpOldEntriesToFitBudget();
}

void rageam::graphics::ImageCache::CacheDX11(u32 hash, const amComPtr<ID3D11ShaderResourceView>& view, const amComPtr<ID3D11Texture2D>& tex, Vec2S uv2)
{
	std::unique_lock lock(m_Mutex);

	IMAGE_CACHE_LOG("ImageCache::CacheDX11() -> Adding to cache, hash: %x", hash);

	CacheEntryDX11 entry;
	entry.View = view;
	entry.Tex = tex;
	entry.PaddingUV2 = uv2;

	m_NewToOldEntriesDX11.Remove(hash);
	m_NewToOldEntriesDX11.Insert(0, hash);
	m_EntriesDX11.EmplaceAt(hash, std::move(entry));

	if (m_NewToOldEntriesDX11.GetSize() > m_Settings.MaxDX11Views)
	{
		u32 oldestHash = m_NewToOldEntriesDX11.Last();
		m_NewToOldEntriesDX11.RemoveLast();
		m_EntriesDX11.RemoveAt(oldestHash);
	}
}

void rageam::graphics::ImageCache::Clear()
{
	std::unique_lock lock(m_Mutex);

	for (u32 fsHash : m_NewToOldEntriesFS)
	{
		CacheEntry& fsEntry = m_Entries.GetAt(fsHash);
		file::WPath fsPath = GetCachedImagePath(fsHash, fsEntry.ImageSize, fsEntry.ImageKind);
		DeleteFileW(fsPath);
	}

	m_EntriesDX11.Destruct();
	m_Entries.Destruct();
	m_NewToOldEntriesRAM.Destroy();
	m_NewToOldEntriesFS.Destroy();
	m_SizeRam = 0;
	m_SizeFs = 0;
}

rageam::graphics::ImageCacheState rageam::graphics::ImageCache::GetState()
{
	std::unique_lock lock(m_Mutex);

	ImageCacheState state;
	state.SizeRamUsed = m_SizeRam;
	state.SizeRamBudget = m_Settings.MemoryStoreBudget;
	state.SizeFsUsed = m_SizeFs;
	state.SizeFsBudget = m_Settings.FileSystemStoreBudget;
	state.ImageCountRam = m_NewToOldEntriesRAM.GetSize();
	state.ImageCountFs = m_NewToOldEntriesFS.GetSize();
	state.DX11ViewCount = m_NewToOldEntriesDX11.GetSize();
	return state;
}
