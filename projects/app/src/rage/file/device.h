//
// File: device.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/file/path.h"
#include "am/string/string.h"
#include "common/types.h"

// NOTE: 'Bias' is often used with meaning of 'Offset'; We use offset because it sounds simpler

// Device-specific file handle. HANDLE for fiDeviceLocal (win-api); fiCollection::Handle for fiPackfile;
typedef pVoid fiHandle_t;

#define FI_INVALID_HANDLE			(pVoid)(-1)
#define FI_INVALID_RESULT			(u32)(-1)
#define FI_INVALID_ATTRIBUTES		(u32)(-1)
#define FI_MAX_PATH					256
#define FI_MAX_NAME					64

// Exists because rage uses 256 as max path instead of 260 (in WinApi)
using fiPath = rageam::file::PathBase<char, FI_MAX_PATH>;

// Matches win-api MoveMethod (FILE_BEGIN, FILE_CURRENT, FILE_END)
enum eFiSeekWhence
{
	SEEK_FILE_BEGIN = 0,
	SEEK_FILE_CURRENT = 1,
	SEEK_FILE_END = 2,
};

enum eFiAttribute : u32
{
	FI_ATTRIBUTE_INVALID = static_cast<u32>(-1),
	FI_ATTRIBUTE_DIRECTORY = 16,
};

namespace rage
{
	class fiPackfile;
	struct datResourceInfo;

	struct fiFindData
	{
		char FileName[FI_MAX_PATH]; // In our implementation uses UTF8
		u64 FileSize;
		u64 LastWriteTime;
		u32 FileAttributes;
	};

	/**
	 * \brief I/O File system abstraction over different systems.
	 * \n Local - OS Level
	 * \n Collection - ZIP / RPF (Base class)
	 * \n Packfile - RPF
	 * \n Memory - RAM
	 */
	class fiDevice
	{
	public:
		fiDevice() = default;
		virtual ~fiDevice() = default;

		static fiDevice* GetDeviceImpl(ConstString path, bool isReadOnly = true);
		static void MakeMemoryFileName() { /* TODO */ }
		static bool FileExists(ConstString path);

		virtual fiHandle_t Open(ConstString path, bool isReadOnly = true) = 0;
		virtual fiHandle_t OpenBulk(ConstString path, u64& offset) = 0;

		virtual fiHandle_t OpenBulkDrm(const char* filename, u64& offset, const void* pDrmKey) { return OpenBulk(filename, offset); }

		virtual fiHandle_t CreateBulk(ConstString path) { return FI_INVALID_HANDLE; }
		virtual fiHandle_t Create(ConstString path) = 0;

		virtual u32 Read(fiHandle_t file, pVoid buffer, u32 size) = 0;
		virtual u32 ReadBulk(fiHandle_t file, u64 offset, pVoid buffer, u32 size) { return -1; }

		virtual u32 WriteBulk(fiHandle_t file, u64 offset, pConstVoid buffer, u32 size) { return -1; }
		virtual u32 Write(fiHandle_t file, pConstVoid buffer, u32 size) = 0;

		virtual u32 Seek(fiHandle_t file, s32 offset, eFiSeekWhence whence = SEEK_FILE_BEGIN)
		{
			return static_cast<u32>(Seek64(file, offset, whence));
		}
		virtual u64 Seek64(fiHandle_t file, s64 offset, eFiSeekWhence whence = SEEK_FILE_BEGIN) = 0;

		virtual bool Close(fiHandle_t file) = 0;
		virtual bool CloseBulk(fiHandle_t file) = 0;

		virtual u32 Size(fiHandle_t file)
		{
			u32 current = Seek(file, 0, SEEK_FILE_CURRENT);
			u32 size = Seek(file, 0, SEEK_FILE_END);
			Seek(file, static_cast<s32>(current), SEEK_FILE_BEGIN);
			return size;
		}

		virtual u64 Size64(fiHandle_t file)
		{
			u64 current = Seek64(file, 0, SEEK_FILE_CURRENT);
			u64 size = Seek64(file, 0, SEEK_FILE_END);
			Seek64(file, static_cast<s64>(current), SEEK_FILE_BEGIN);
			return size;
		}

		// Not implemented / present in Release.
		// Return value & type is wrong, native implementation probably using some enumeration.
		virtual bool Flush(fiHandle_t file) { return true; }

		virtual bool Delete(ConstString path) { return false; }
		virtual bool Rename(ConstString oldPath, ConstString newPath) { return false; }

		virtual bool MakeDirectory(ConstString path) { return false; }
		virtual bool UnmakeDirectory(ConstString path) { return false; }

		virtual void Sanitize(fiHandle_t handle) const { }

		virtual u64	GetFileSize(ConstString path) = 0;
		virtual u64 GetFileTime(ConstString path) = 0;
		virtual bool SetFileTime(ConstString path, u64 time) = 0;

		virtual fiHandle_t FindFileBegin(ConstString path, fiFindData& findData) = 0;
		virtual bool FindFileNext(fiHandle_t file, fiFindData& findData) = 0;
		virtual bool FindFileEnd(fiHandle_t file) = 0;

		virtual fiDevice* GetLowLevelDevice() { return this; }

		virtual void FixRelativeName(char* destination, int destinationSize, ConstString path)
		{
			String::Copy(destination, destinationSize, path);
		}

		virtual bool SetEndOfFile(fiHandle_t file) { return false; }
		virtual u32	 GetAttributes(ConstString path) { return 0; }
		virtual bool PrefetchDir() { return true; } // For remote device
		virtual bool SetAttributes(ConstString path, u32 attributes) { return false; }
		virtual u64  GetRootDeviceId(ConstString fileName) = 0;
		virtual bool SafeRead(fiHandle_t file, pVoid buffer, u32 size);
		virtual bool SafeWrite(fiHandle_t file, pConstVoid buffer, u32 size);
		virtual u32  GetResourceInfo(ConstString path, datResourceInfo& info);

		virtual u32 GetEncryption() { return 0; }
		virtual u32 GetBulkOffset(ConstString path) { return 0; }
		virtual u32 GetPhysicalSortKey(ConstString path) { return 0x40000000; /* HARDDRIVE_LSN */ }

		virtual bool IsRpf() const { return false; }
		virtual bool IsMaskingAnRpf() const { return false; } // Is this relative device holds packfile?
		virtual fiPackfile* GetRpfDevice() { return (fiPackfile*) this; } // Get relative device from relative device
		virtual bool IsCloud() const { return false; } // If fiDeviceCloud

		virtual u32 GetPackfileIndex() { return 0; }
		virtual ConstString GetDebugName() = 0;
	};

	using fiDevicePtr = fiDevice*;
}
