#pragma once

#include <Windows.h>
#include "rage/atl/string.h"

namespace rageam::file
{
	struct DriveInfo
	{
		static constexpr u32 MAX_NAME = 4; // 'C:/' + nul

		wchar_t	Name[MAX_NAME];
		WPath	VolumeLabel;
		bool	IsSystem;

		static rage::atArray<DriveInfo> GetDrives()
		{
			rage::atArray<DriveInfo> drives;

			DWORD driveStringsSize = GetLogicalDriveStringsW(0, NULL);
			wchar_t* driveStrings = new wchar_t[driveStringsSize];

			GetLogicalDriveStringsW(driveStringsSize, driveStrings);

			// Retrieve system drive name from AppData path
			wchar_t systemDrive[4];
			GetSystemDisk(systemDrive);

			// Drive strings is a continuous string array
			wchar_t* driveCursor = driveStrings;
			while (driveCursor[0] != '\0')
			{
				DriveInfo info;
				String::Copy(info.Name, MAX_NAME, driveCursor);

				// Retrieve volume name
				GetVolumeInformationW(
					info.Name, info.VolumeLabel.GetBuffer(), info.VolumeLabel.GetBufferSize(),
					NULL, NULL, NULL, NULL, NULL);

				info.IsSystem = String::Equals(systemDrive, info.Name);

				drives.Emplace(std::move(info));

				// Jump to next string
				driveCursor += String::Length(driveCursor) + 1;
			}

			delete[] driveStrings;

			return drives;
		}
	};
}
