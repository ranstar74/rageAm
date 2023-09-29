//
// File: imageformat.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "rage/crypto/joaat.h"

namespace rageam::texture
{
	// Keep it synced with eImageFormat
#define IMAGE_FORMAT_STRING \
	"*.png;"		\
	"*.jpg"			\
	"*.jpeg"		\
	"*.dds"			\

	/**
	 * \brief Enumeration of supported image file formats by compressor.
	 */
	enum eImageFormat
	{
		IMAGE_FORMAT_PNG,
		IMAGE_FORMAT_JPG,
		IMAGE_FORMAT_JPEG,
		IMAGE_FORMAT_DDS,
	};

	/**
	 * \brief Gets whether image format is supported by compressor.
	 */
	inline bool IsImageFormat(ConstString extension)
	{
		u32 extensionHash = rage::joaat(extension);

		switch (extensionHash)
		{
		case rage::joaat("png"):	return true;
		case rage::joaat("jpg"):	return true;
		case rage::joaat("jpeg"):	return true;
		case rage::joaat("dds"):	return true;
		}
		return false;
	}

	inline bool IsImageFormat(ConstWString extension)
	{
		constexpr int maxSize = 5; // for largest extension with 4 chars such as .jpeg + nul
		char buffer[maxSize];
		String::ToAnsi(buffer, maxSize, extension);
		return IsImageFormat(buffer);
	}

	inline bool IsDDS(ConstWString path)
	{
		return String::Equals(file::GetExtension(path), L"dds", true);
	}

	/**
	 * \brief Gets pattern string in format '*.png;*.dds' with all supported image file extensions.
	 */
	inline ConstString GetImageFormatSearchMask()
	{
		return IMAGE_FORMAT_STRING;
	}

	/**
	 * \brief Gets wide pattern string in format '*.png;*.dds' with all supported image file extensions.
	 */
	inline ConstWString GetImageFormatSearchMaskW()
	{
		return L"" IMAGE_FORMAT_STRING;
	}
}
