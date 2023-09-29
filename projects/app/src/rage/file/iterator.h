//
// File: iterator.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "device.h"
#include "common/types.h"

namespace rage
{
	/**
	 * \brief Allows to search files using rage devices (Local, Packfile).
	 */
	class fiIterator
	{
		fiDevicePtr	m_Device = nullptr;
		fiFindData	m_FindData = {};
		fiHandle_t	m_FindHandle = FI_INVALID_HANDLE;
		bool		m_Initialized = false;
		fiPath		m_Path;
		char		m_SearchPattern[FI_MAX_NAME];
		fiPath		m_FilePath; // Full path to current name as fiFindData only provides the name of it

		bool Begin();
		bool FindNext();
	public:
		fiIterator(ConstString path, ConstString searchPattern = "*");
		~fiIterator();

		bool Next();

		const fiFindData& GetFindData() const { return m_FindData; }
		ConstString GetFilePath() const { return m_FilePath; }
		ConstString GetFileName() const { return m_FindData.FileName; }
		bool IsDirectory() const;
	};
}
