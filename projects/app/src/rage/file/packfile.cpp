#include "packfile.h"
#include "am/system/errordisplay.h"
#include "am/system/exception/handler.h"
#include "am/system/exception/stacktrace.h"
#include "am/crypto/cipher.h"
#include "rage/atl/datahash.h"
#include "rage/atl/fixedarray.h"

#include <Tracy.hpp>

rage::fiPackfile::~fiPackfile()
{
	m_KeepNameHeap = false; // Make sure that Shutdown cleans up name heap...
	fiPackfile::Shutdown();
}

void rage::fiPackfile::Shutdown()
{
	UnInit();
	if (m_Device && m_Handle != FI_INVALID_HANDLE)
		m_Device->CloseBulk(m_Handle);
	m_Device = nullptr;
	m_Handle = FI_INVALID_HANDLE;
}

bool rage::fiPackfile::Init(ConstString archivePath, fiPackfile* parent, fiPackEntry* entry)
{
	ZoneScoped;
	AM_ASSERT(m_Handle == FI_INVALID_HANDLE, "fiPackfile::Init() -> Called without Shutdown!");

	// There are RPFs with name exceeding max 32 character length, for e.g. 'STREAMED_VEHICLES_GRANULAR_NPC.rpf'
	// We can't use memcpy_s because it will throw exception, copy name manually
	ConstString fileName = rageam::file::GetFileName(archivePath);
	for (size_t i = 0; i < std::size(m_Name); i++)
	{
		m_Name[i] = fileName[i];
		if (!fileName[i]) 
			break;
	}
	m_Fullname = archivePath; // Game resolves actual absolute path, but it's not necessary

	// For RPFs opened outside game system
	if (parent)
	{
		m_Device = parent;
		m_Handle = parent->OpenBulkEntry(*entry, m_ArchiveOffset);
		m_ArchiveSize = parent->GetEntrySize(*entry);
	}
	else
	{
		m_Device = GetDeviceImpl(archivePath);
		if (!m_Device)
		{
			AM_ERRF("fiPackfile::Init() -> Unable to get device for %s", archivePath);
			return false;
		}

		m_Handle = m_Device->OpenBulkDrm(archivePath, m_ArchiveOffset, m_DrmKey);
		if (m_Handle == FI_INVALID_HANDLE)
		{
			AM_ERRF("fiPackfile::Init() -> Unable to open file for '%s'", archivePath);
			return false;
		}
		m_ArchiveSize = m_Device->GetFileSize(archivePath);
	}

	// Game supports reading packfile from memory buffer, we don't need this
	m_MemoryBuffer = nullptr;

	m_FileTime = m_Device->GetFileTime(archivePath);
	m_RelativeOffset = 0; // We don't support mounting yet, just set it to 0
	m_SortKey = m_Device->GetPhysicalSortKey(archivePath);
	m_IsInstalled = true; // In source: cacheMode == CACHE_NONE
	m_IsCached = false; // Not really sure how caching works in any case...
	m_IsRpf = false;

	// Most likely empty/corrupted file... not an archive
	if (m_ArchiveSize < 16)
	{
		AM_ERRF("fiPackfile::Init() -> Too small RPF file (%llu bytes).", m_ArchiveSize);
		m_Device->CloseBulk(m_Handle);
		m_Handle = FI_INVALID_HANDLE;
		return false;
	}

	return ReInit(archivePath);
}

static int PackInitExceptionFilter(const EXCEPTION_POINTERS* exInfo)
{
	AM_ERRF("fiPackfile::Init() -> Unhandled exception!");

	static char stacktraceBuffer[rageam::STACKTRACE_BUFFER_SIZE];
	char header[36];
	rageam::ExceptionHandler::Context context(exInfo);
	rageam::StackTracer::CaptureAndWriteTo(context, stacktraceBuffer, rageam::STACKTRACE_BUFFER_SIZE);
	sprintf_s(header, 36, "%s %x", context.ExceptionName, context.ExceptionCode);
	AM_TRACE(header);
	AM_TRACE(stacktraceBuffer);

	return EXCEPTION_EXECUTE_HANDLER;
}

bool rage::fiPackfile::InitSafe(ConstString archivePath, fiPackfile* parent, fiPackEntry* entry)
{
	__try { return Init(archivePath, parent, entry); }
	__except (PackInitExceptionFilter(GetExceptionInformation())) { return false; }
}

bool rage::fiPackfile::ReInit(ConstString archivePath)
{
	ZoneScoped;

	// In source there's option to pass custom header, we don't need it
	m_IsUserHeader = false;
	m_IsByteSwapped = false; // Non-PC

	// Read header
	fiPackHeader header;
	if (!(m_Device->IsRpf() && m_Device->IsMaskingAnRpf()))
		m_Device->Seek(m_Handle, 0, SEEK_FILE_BEGIN);
	m_Device->ReadBulk(m_Handle, m_ArchiveOffset, &header, 16);

	// Validate magic
	if (header.Magic != fiPackHeader::MAGIC)
	{
		AM_ERRF("fiPackfile::ReInit() -> Invalid header '%X'! File is not a packfile.", header.Magic);
		m_Device->CloseBulk(m_Handle);
		m_Handle = FI_INVALID_HANDLE;
		return false;
	}

	m_IsXCompressed = header.XCompress;
	m_NameShift = header.NameShift;
	m_EntryCount = header.EntryCount;
	m_EncryptionKey = header.Encryption;
	m_IsRpf = true;

	// Allocate and read entries
	u32 entriesSize;
	{
		ZoneScopedN("Read Entries");
		entriesSize = sizeof(fiPackEntry) * m_EntryCount;
		m_Entries = new fiPackEntry[m_EntryCount];
		m_Device->ReadBulk(m_Handle, m_ArchiveOffset + sizeof fiPackHeader, m_Entries, entriesSize);
		m_CachedDataSize = entriesSize + sizeof fiPackHeader;
	}

	// Create name heap + parent indices, packed in one allocation for 'simplicity'
	u32 nameHeapAndIndicesSize;
	{
		ZoneScopedN("Read NameHeap");
		nameHeapAndIndicesSize = header.NameHeapSize + header.EntryCount * sizeof(u16);
		m_NameHeap = new char[nameHeapAndIndicesSize];
		m_ParentIndices = reinterpret_cast<u16*>(m_NameHeap + header.NameHeapSize);
		m_Device->ReadBulk(m_Handle, m_ArchiveOffset + sizeof(fiPackHeader) + entriesSize, m_NameHeap, nameHeapAndIndicesSize);
	}

	// Decrypt entries and name heap
	bool decrypted = true;
	if (header.Encryption == CIPHER_KEY_ID_TFIT) // TransformIT (TFIT); aka NG Encryption
	{
		// TFIT Key is picked based on archive name and size
		u32 selector = 0;
		selector += atStringHash(rageam::file::GetFileName(archivePath));
		selector += static_cast<int>(m_ArchiveSize);
		selector %= CIPHER_TFIT_NUM_KEYS;

		// Since key selected is based on archive name, renaming file will corrupt archive
		// OpenIV/Swage brute force valid key, we are going to do the same
		// - Copy first 16 bytes (AES operates on 16 bit blocks) from entries and nametable to test decryption on it
		{
			ZoneScopedN("Validate selector");
			auto validateSelector = [&] (int selectorToValidate)
			{
				fiPackEntry rootEntry = m_Entries[0];
				char entryName[16];
				memcpy(entryName, m_NameHeap, 16);
				if (!CIPHER.Decrypt(header.Encryption, selectorToValidate, &rootEntry, 16)) return false;
				if (!CIPHER.Decrypt(header.Encryption, selectorToValidate, entryName, 16)) return false;
				// Sanity check - first name heap character is always null terminator for valid RPF, because that's the name of implicit root directory
				// If decryption fails (most likely - due to renamed file causing invalid TFIT selector), name heap will contain garbage
				return entryName[0] == '\0' && rootEntry.IsDirectory();
			};
			// Brute-force right key...
			if (!validateSelector((int) selector))
			{
				AM_ERRF("fiPackfile::ReInit() -> Default key is not valid for archive! Most likely it was renamed, trying to brute-force... %s", archivePath);
				int validSelector = -1;
				for (int i = 0; i < CIPHER_TFIT_NUM_KEYS; i++)
				{
					if (i != (int) selector && validateSelector(i))
					{
						validSelector = i;
						break;
					}
				}
				if (validSelector == -1)
				{
					AM_ERRF("fiPackfile::ReInit() -> Failed to brute-force key! Archive is corrupt.");
					Shutdown();
					return false;
				}
				AM_WARNINGF("fiPackfile::ReInit() -> Managed to find correct key for decrypting archive, but game woudln't be able to read it. Please re-encrypt archive.");
				selector = u32(validSelector);
			}
		}

		ZoneScopedN("Decrypt TFIT");
		if (!CIPHER.Decrypt(header.Encryption, selector, m_Entries, entriesSize)) decrypted = false;
		if (!CIPHER.Decrypt(header.Encryption, selector, m_NameHeap, header.NameHeapSize)) decrypted = false;
	}
	else if (header.Encryption == CIPHER_KEY_ID_AES)
	{
		ZoneScopedN("Decrypt AES");
		if (!CIPHER.Decrypt(header.Encryption, 0, m_Entries, entriesSize)) decrypted = false;
		if (!CIPHER.Decrypt(header.Encryption, 0, m_NameHeap, header.NameHeapSize)) decrypted = false;
	}
	else if (header.Encryption == CIPHER_KEY_ID_NONE || header.Encryption == CIPHER_KEY_ID_OPEN)
	{
		// Nothing to decrypt...
	}
	else
	{
		AM_ERRF("fiPackfile::ReInit() -> Unknown encryption ID '%X': %s", header.Encryption, archivePath);
		Shutdown();
		return false;
	}

	if (!decrypted)
	{
		AM_ERRF("fiPackfile::ReInit() -> Failed to decrypt archive %s", archivePath);
		Shutdown();
		return false;
	}

	// Generate parent indices - iterate through every directory recurse and set current directory for each file
	std::function<void(int)> fillParentIndices;
	fillParentIndices = [&](u16 dirIndex)
		{
			u32 start = m_Entries[dirIndex].Directory.StartIndex;
			u32 stop = start + m_Entries[dirIndex].Directory.ChildCount;
			for (u32 i = start; i < stop; i++)
			{
				m_ParentIndices[i] = dirIndex;
				if (m_Entries[i].IsDirectory())
					fillParentIndices(static_cast<u16>(i));
			}
		};
	{
		ZoneScopedN("Parent Indices");
		fillParentIndices(0 /* First entry is always directory */);
		m_ParentIndices[0] = 0;
	}

	// Update encryption key for all entries, '1' seems to be old way to indicate encryption
	for (u32 i = 0; i < m_EntryCount; i++)
	{
		fiPackEntry& entry = m_Entries[i];
		if (entry.IsFile() && entry.File.Encryption == 1)
			entry.File.Encryption = header.Encryption;
	}

	// Trim way too long strings - TODO
	{
		
	}

	m_AllocatedSize = sizeof(fiPackfile) + nameHeapAndIndicesSize + entriesSize;

	return true;
}

void rage::fiPackfile::UnInit()
{
	if (m_MemoryBuffer)
	{
		delete[] m_MemoryBuffer;
		m_MemoryBuffer = nullptr;
	}

	if (!m_KeepNameHeap)
	{
		delete[] m_NameHeap;
		m_NameHeap = nullptr;
	}

	if (!m_IsUserHeader)
		delete[] m_Entries;
	m_Entries = nullptr;
	m_AllocatedSize = sizeof fiPackfile;
}

u32 rage::fiPackfile::ComputeSearchHashSumOfNestedArchive(const fiPackEntry& entry) const
{
	ZoneScoped;
	fiPackHeader header;
	if (!(m_Device->IsRpf() && m_Device->IsMaskingAnRpf()))
		m_Device->Seek(m_Handle, 0, SEEK_FILE_BEGIN);
	m_Device->ReadBulk(m_Handle, m_ArchiveOffset, &header, 16);

	u32 totalSize = header.NameHeapSize + header.EntryCount * sizeof fiPackEntry;
	auto buffer = std::unique_ptr<char[]>(new char[totalSize]);
	m_Device->ReadBulk(m_Handle, sizeof fiPackHeader, buffer.get(), totalSize);

	return atDataHash(buffer.get(), totalSize);
}

rage::fiPackEntry* rage::fiPackfile::FindEntry(ConstString path) const
{
	fiPackEntry* entry = &m_Entries[0]; // Root
	if (Char::IsPathSeparator(*path)) // Skip leading separator
		++path;
	if (!*path) // Search path was '/', return root
		return entry;
	while(true)
	{
		// Skip all path separators
		while (Char::IsPathSeparator(*path))
			++path;

		// Get token
		char  token[FI_MAX_PATH];
		char* cursor = token;
		while (*path && !Char::IsPathSeparator(*path))
		{
			*cursor = Char::ToLower(*path); cursor++;
			++path;
		}
		*cursor = '\0';

		// Search for token in current directory using binary search
		AM_ASSERTS(entry->IsDirectory());
		u32 begin = entry->Directory.StartIndex;
		u32 end   = begin + entry->Directory.ChildCount - 1;
		entry = nullptr;
		while (begin <= end)
		{
			u32 mid = (begin + end) >> 1;
			int result = strcmp(token, m_NameHeap + (m_Entries[mid].NameOffset << m_NameShift));
			if (result < 0) end = mid - 1;
			else if (result > 0) begin = mid + 1;
			else 
			{
				entry = &m_Entries[mid];
				break;
			}
		}

		// Nothing found
		if (!entry)
			return nullptr;
		// There's still entry to find, but we are not in directory? Invalid search request
		if (!entry->IsDirectory() && *path)
			return nullptr;
		// Search is done, return whatever we found, if any
		if (!*path)
			return entry;
	}
}

fiHandle_t rage::fiPackfile::OpenBulkEntry(u32 index, u64& offset) const
{
	offset = m_Entries[index].GetFileOffset();
	return m_Handle;
}

fiHandle_t rage::fiPackfile::OpenBulkEntry(const fiPackEntry& entry, u64& offset) const
{
	offset = entry.GetFileOffset();
	return m_Handle;
}

int rage::fiPackfile::GetEntryIndex(ConstString name) const
{
	fiPackEntry* entry = FindEntry(name);
	if (entry)
		return GetEntryIndex(*entry);
	return -1;
}

int rage::fiPackfile::GetEntryIndex(const fiPackEntry& entry) const
{
	int index = static_cast<int>(std::distance(m_Entries, const_cast<fiPackEntry*>(&entry)));
	AM_ASSERT(index >= 0 && index < (int) m_EntryCount, 
		"fiPackfile::GetEntryIndex() -> Entry index (%i) is invalid! It is placed outside archive.", index);
	return index;
}

ConstString rage::fiPackfile::GetEntryName(u32 index) const
{
	AM_ASSERTS(m_NameHeap);
	return m_NameHeap + (m_Entries[index].NameOffset << m_NameShift);
}

ConstString rage::fiPackfile::GetEntryFullName(u32 index, char* dest, int destSize) const
{
	// Iterate all parent indices and write them to stack, then read in reverse order
	rageam::file::Path    path;
	atFixedArray<u16, 64> parentIndices; // Maximum depth - 64, only corrupted packfiles will have this much
	u32                   currentEntryIndex = index;
	while (currentEntryIndex != 0 /* Index 0 means we've reached root */)
	{
		parentIndices.Add(currentEntryIndex);
		currentEntryIndex = GetParentIndex(currentEntryIndex);
	}
	for (int k = static_cast<int>(parentIndices.GetSize()) - 1; k >= 0; k--)
		path /= GetEntryName(parentIndices[k]);
	String::Copy(dest, destSize, path);
	return dest;
}

u64 rage::fiPackfile::GetEntryCompressedSize(const fiPackEntry& entry)
{
	if (entry.IsResource && entry.IsBigResource())
		return GetResourceCompressedSize(entry);
	return entry.GetCompressedSize();
}

u32 rage::fiPackfile::GetResourceCompressedSize(const fiPackEntry& entry)
{
	AM_ASSERTS(entry.IsResource);
	if (!entry.IsBigResource())
		return entry.CompressedSize;

	// See IsBigResource comment, actual size is hidden in header
	u8 buffer[16];
	u64 offset;
	fiHandle_t handle = OpenBulkEntry(entry, offset);
	ReadBulk(handle, offset, buffer, 16);
	CloseBulk(handle);
	return u32(buffer[2] << 24 | buffer[5] << 16 | buffer[14] << 8 | buffer[7]);
}

u32 rage::fiPackfile::GetResourceInfo(const fiPackEntry& entry, datResourceInfo& info)
{
	AM_ASSERTS(entry.IsResource);
	info = {};
	u64 offset;
	u32 version = 0;
	fiHandle_t file = OpenBulkEntry(entry, offset);
	datResourceHeader header;
	if (ReadBulk(file, offset, &header, sizeof datResourceHeader) != FI_INVALID_RESULT)
	{
		version = header.Version;
		info = header.Info;
	}
	CloseBulk(file);
	return version;
}

u32 rage::fiPackfile::ReadBulk(fiHandle_t file, u64 offset, pVoid buffer, u32 size)
{
	offset += m_ArchiveOffset;

	u64 totalSize = m_ArchiveSize;
	if (m_Device->IsRpf() || m_Device->IsMaskingAnRpf())
	{
		fiPackfile* parent = m_Device->GetRpfDevice();
		totalSize += parent->GetPackfileSize();
	}

	if (offset + size > totalSize)
		size = static_cast<int>(totalSize) - offset;

	return m_Device->ReadBulk(file, offset, buffer, size);
}

