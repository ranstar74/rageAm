//
// File: compressor.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <d3d11.h>
#include <dxgiformat.h>

#include "nvtt.h"
#include "am/file/path.h"
#include "common/types.h"
#include "rage/atl/string.h"

namespace rageam::texture
{
	static constexpr u32 MAX_RESOLUTION = 1 << 14; // Max supported by DirectX 11
	static constexpr DXGI_FORMAT TEXTURE_FORMAT_AUTO = DXGI_FORMAT_UNKNOWN;

	enum CompressorQuality
	{
		COMPRESSOR_QUALITY_FASTEST,
		COMPRESSOR_QUALITY_NORMAL,
		COMPRESSOR_QUALITY_PRODUCTION,
		COMPRESSOR_QUALITY_HIGHEST,
	};

	enum MipFilter
	{
		MIP_FILTER_BOX,
		MIP_FILTER_TRIANGLE,
		MIP_FILTER_KRAISER,
	};

	enum ResizeFilter
	{
		RESIZE_FILTER_BOX,
		RESIZE_FILTER_TRIANGLE,
		RESIZE_FILTER_KRAISER,
		RESIZE_FILTER_MITCHELL,
	};

	struct CompressionOptions
	{
		ResizeFilter		ResizeFilter = RESIZE_FILTER_BOX;
		MipFilter			MipFilter = MIP_FILTER_BOX;
		CompressorQuality	Quality = COMPRESSOR_QUALITY_NORMAL;
		DXGI_FORMAT			Format = TEXTURE_FORMAT_AUTO;
		u32					MaxResolution = MAX_RESOLUTION;
		bool				MipMaps = true;
	};

	/**
	 * \brief Stores information on compressed image.
	 * Values will be different from original image properties depending on CompressionOptions.
	 */
	struct CompressedInfo
	{
		u32			Width;
		u32			Height;
		u32			Size;		// Size of compressed image in bytes
		u8			MipCount;	// Number of actually generated mip maps
		DXGI_FORMAT Format;		// Actual compressed format
	};

	/**
	 * \brief NVTT Powered texture compressor.
	 */
	class Compressor
	{
		nvtt::Surface	m_Image;
		rage::atString	m_Name;
		u32				m_Width = 0;
		u32				m_Height = 0;
		u32				m_PixelCount = 0;
		bool			m_HasAlpha = false;
		bool			m_SharpAlpha = false;
		bool			m_Loaded = false;
		bool			m_SupportBlockCompression = false;
		DXGI_FORMAT		m_BestFormat = TEXTURE_FORMAT_AUTO;

		void FindBestFormat();
	public:
		/**
		 * \brief Loads image from UTF16 path.
		 * \remarks Image file name has to be written in english, unicode characters are not supported.
		 */
		bool LoadImageFromFile(const file::WPath& path);
		bool LoadImageFromMemory(const char* data, u32 dataSize);
		bool LoadFromPixelData(int width, int height, const char* data, nvtt::InputFormat inputFormat);

		void SetDebugName(ConstString name) { m_Name = name; }

		bool IsLoaded() const { return m_Loaded; }

		u32 GetWidth() const { return m_Width; }
		u32 GetHeight() const { return m_Height; }
		bool GetHasAlpha() const { return m_HasAlpha; }
		bool GetSharpAlpha() const { return m_SharpAlpha; }
		bool SupportsBlockCompression() const { return m_SupportBlockCompression; }
		ConstString GetName() const { return m_Name.GetCStr(); }

		DXGI_FORMAT GetFormat(DXGI_FORMAT fmt) const;
		DXGI_FORMAT GetFormat(const CompressionOptions& options) const;
		void GetResolution(const CompressionOptions& options, u32& width, u32& height) const;
		u32 GetMipCount(const CompressionOptions& options) const;
		u32 GetAvailableMipCount() const;
		u32 ComputeSize(const CompressionOptions& options) const;
		bool Compress(const CompressionOptions& options, char** ppPixelData, CompressedInfo& outInfo) const;

		// TODO: Deprecated

		D3D11_TEXTURE2D_DESC GetDesc2D(const CompressionOptions& options) const;
		ID3D11ShaderResourceView* CompressAndGetShaderView(const CompressionOptions& options, CompressedInfo& outInfo) const;
	};
}
