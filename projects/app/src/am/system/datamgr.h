//
// File: datamgr.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <shlobj_core.h>
#include <Windows.h>

#include "am/file/path.h"
#include "helpers/win32.h"

namespace rageam
{
	/**
	 * \brief Manages 'rageAm/data' directory path and subfolders.
	 */
	class DataManager
	{
	public:
		// data/logs
		static const file::WPath& GetLogsFolder()
		{
			static file::WPath logsFolder = GetDataFolder() / L"logs";
			return logsFolder;
		}

		// data/icons
		static const file::WPath& GetIconsFolder()
		{
			static file::WPath logsFolder = GetDataFolder() / L"icons";
			return logsFolder;
		}

		// data/fonts
		static const file::WPath& GetFontsFolder()
		{
			static file::WPath logsFolder = GetDataFolder() / L"fonts";
			return logsFolder;
		}

		// data/font_icons
		static const file::WPath& GetFontIconsFolder()
		{
			static file::WPath logsFolder = GetDataFolder() / L"font_icons";
			return logsFolder;
		}

		// data/shaders
		static const file::WPath& GetShadersFolder()
		{
			static file::WPath shadersFolder = GetDataFolder() / L"shaders";
			return shadersFolder;
		}

		// data/profiles
		static const file::WPath& GetProfilesFolder()
		{
			static file::WPath profilerFolder = GetDataFolder() / L"profiles";
			return profilerFolder;
		}

		// Gets path to RageAm data folder, this folder contains logs, icons, fonts
		static const file::WPath& GetDataFolder()
		{
			static file::WPath dataFolder;
			static bool initialized = false;
			if (!initialized)
			{
				// Default directory is used when app is launched from 'bin/' directory
				if (GetFileAttributesW(AM_DEFAULT_DATA_DIR) != INVALID_FILE_ATTRIBUTES)
				{
					dataFolder = AM_DEFAULT_DATA_DIR;
				}
				else // When app is published, we store data folder near exe/dll
				{
					GetModuleFileNameW(GetCurrentModule(), dataFolder.GetBuffer(), MAX_PATH);
					dataFolder = dataFolder.GetParentDirectory(); // Remove rageAm.exe
					dataFolder /= AM_DATA_DIR;
				}

				initialized = true;
			}
			return dataFolder;
		}

		// Folder for storing user data
		static const file::WPath& GetAppData()
		{
			static file::WPath appData;
			static bool init = false;
			if (!init)
			{
				wchar_t* path;
				AM_ASSERT(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, 0, &path)),
					"DataManager::GetAppData() -> Failed to get path.");
				appData = path;
				appData /= L"rageAm";
				CreateDirectoryW(appData, NULL); // Make sure dir is created
				CoTaskMemFree(path);
				init = true;
			}
			return appData;
		}
	};
}
