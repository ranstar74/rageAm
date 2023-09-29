//
// File: pathutils.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/string/string.h"
#include "am/string/stringwrapper.h"

namespace rageam::file
{
	/**
	 * \brief Gets extension from given path; Empty string if path don't have extension.
	 * \note "GrandTheftAutoV/GTA5.exe" -> "exe"
	 * \returns View on input string.
	 */
	template<typename TChar>
	const TChar* GetExtension(const TChar* path)
	{
		s32 index = StringWrapper(path).LastIndexOf('.');
		if (index == -1)
			return String::Empty<TChar>();
		return path + index + 1;
	}

	/**
	 * \brief Gets file name including extension from given path.
	 * \note "GrandTheftAutoV/GTA5.exe" -> "GTA5.exe"
	 * \returns View on input string.
	 */
	template<typename TChar>
	const TChar* GetFileName(const TChar* path)
	{
		s32 index = StringWrapper(path).template LastIndexOf<'/', '\\'>();
		if (index == -1)
			return path;
		return path + index + 1;
	}

	/**
	 * \brief Gets file path without extension.
	 * \note "GrandTheftAutoV/GTA5.exe" -> "GrandTheftAutoV/GTA5"
	 * \remarks Extension cannot be removed without altering path, buffer is required.
	 */
	template<typename TChar>
	void GetFilePathWithoutExtension(TChar* destination, int destinationSize, const TChar* path)
	{
		String::Copy(destination, destinationSize, path);

		s32 index = StringWrapper(destination).LastIndexOf('.');
		if (index == -1)
			return; // Path has no extension

		destination[index] = '\0'; // Terminate string where extension starts
	}

	/**
	 * \brief Gets file name without extension from given path.
	 * \note "GrandTheftAutoV/GTA5.exe" -> "GTA5"
	 * \remarks Extension cannot be removed without altering path, buffer is required.
	 */
	template<typename TChar>
	void GetFileNameWithoutExtension(TChar* destination, int destinationSize, const TChar* path)
	{
		GetFilePathWithoutExtension(destination, destinationSize, GetFileName(path));
	}

	/**
	 * \brief Gets parent directory from given path.
	 * \note "src/am/file" -> "src/am"
	 * \remarks Extension cannot be removed without altering path, buffer is required.
	 */
	template<typename TChar>
	void GetParentDirectory(TChar* destination, int destinationSize, const TChar* path)
	{
		String::Copy(destination, destinationSize, path);

		s32 index = StringWrapper(destination).template LastIndexOf<'/', '\\'>();
		if (index == -1)
			return; // Path contains single directory

		destination[index] = '\0'; // Terminate string where current directory starts
	}
}
