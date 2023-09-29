//
// File: local.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/file/device.h"

namespace rage
{
	/**
	 * \brief Platform device (In our case - Windows).
	 * \remarks Supports UTF8 encoded paths.
	 */
	class fiDeviceLocal : public fiDevice
	{
		static void Win32FindDataToFi(const WIN32_FIND_DATAW& from, fiFindData& to);

		// Native implementation doesn't support unicode characters but we do.
		// Converts UTF8 encoded string to Wide
		static ConstWString ConvertPathToWide(ConstString path);
	public:
		static fiDeviceLocal* GetInstance();

		fiHandle_t Open(ConstString path, bool isReadOnly = true) override;
		fiHandle_t OpenBulk(ConstString path, u64& offset) override;

		fiHandle_t CreateBulk(ConstString path) override;
		fiHandle_t Create(ConstString path) override;

		u32 Read(fiHandle_t file, pVoid buffer, u32 size) override;
		u32 ReadBulk(fiHandle_t file, u64 offset, pVoid buffer, u32 size) override;

		u32 WriteOverlapped(fiHandle_t file, u64 offset, pConstVoid buffer, u32 size) override;
		u32 Write(fiHandle_t file, pConstVoid buffer, u32 size) override;

		u64 Seek64(fiHandle_t file, s64 offset, eFiSeekWhence whence = SEEK_FILE_BEGIN) override;

		bool Close(fiHandle_t file) override;
		bool CloseBulk(fiHandle_t file) override { return Close(file); }

		bool Delete(ConstString path) override;
		bool Rename(ConstString oldPath, ConstString newPath) override;

		bool MakeDirectory(ConstString path) override;
		bool UnmakeDirectory(ConstString path) override;

		u64 GetFileSize(ConstString path) override;
		u64 GetFileTime(ConstString path) override;
		bool SetFileTime(ConstString path, u64 time) override;

		fiHandle_t FindFileBegin(ConstString path, fiFindData& findData) override;
		bool FindFileNext(fiHandle_t file, fiFindData& findData) override;
		bool FindFileEnd(fiHandle_t file) override;

		bool SetEndOfFile(fiHandle_t file) override;

		u32 GetAttributes(ConstString path) override;
		bool SetAttributes(ConstString path, u32 attributes) override;

		u64 GetRootDeviceId() override { return 2; /* Whatever this means */ }

		ConstString GetDebugName() override { return "Local"; }
	};
}
