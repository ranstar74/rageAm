//
// File: bc.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "image.h"
#include "am/types.h"
#include "am/file/path.h"
#include "common/types.h"

namespace rageam::graphics
{
	// Block compression methods and common use:
	//
	// Color / Specular			DXT1, BC7
	// Color maps w opacity		DXT1, DXT3, DXT5, BC7
	// Normal					DXT1, BC5
	//
	// DXT3 better fits for images that have very sharp opacity (cutout), while for DXT5 opacity has to be slowly faded

	enum BlockFormat // We keep it as separate enumeration from pixel formats to prevent using non-compressed formats in compressor
	{
		BlockFormat_None,
		BlockFormat_BC1,
		BlockFormat_BC2,
		BlockFormat_BC3,
		BlockFormat_BC4,
		BlockFormat_BC5,
		BlockFormat_BC7,
	};

	static constexpr ImagePixelFormat BlockCompressorFormatToPixelFormat[] =
	{
		ImagePixelFormat_None,
		ImagePixelFormat_BC1,
		ImagePixelFormat_BC2,
		ImagePixelFormat_BC3,
		ImagePixelFormat_BC4,
		ImagePixelFormat_BC5,
		ImagePixelFormat_BC7,
	};

	struct ImageCompressorOptions
	{
		BlockFormat Format = BlockFormat_BC7;
		float		Quality = 0.65f;			// 0 - Worst, fastest; 1 - Best, slowest
		int			MaxResolution = 0;			// Down-scales base image resolution to specified one, 0 to use original
		bool		GenerateMipMaps = true;		// Creates mip maps up to 4x4
		bool		CutoutAlpha = false;		// All pixels with alpha less than threshold are made transparent, otherwise fully opaque
		int			CutoutAlphaThreshold = 90;
	};

	/**
	 * \brief Image block compressor (encoder).
	 */
	class IBlockCompressor
	{
		const ImageCompressorOptions& m_Options;
	public:
		IBlockCompressor(const ImageCompressorOptions& options) : m_Options(options) {}
		virtual ~IBlockCompressor() = default;

		/**
		 * \brief Formats supported by particular compressor implementation.
		 */
		enum Features_
		{
			Features_BC1 = 1 << 0, // DXT1
			Features_BC2 = 1 << 1, // DXT3
			Features_BC3 = 1 << 3, // DXT5
			Features_BC4 = 1 << 4, // ATI1
			Features_BC5 = 1 << 5, // ATI2
			Features_BC7 = 1 << 6, // At this point they finally realized that giving weird names to formats is not a good idea
		};
		using Features = int;

		virtual ConstString GetDebugName() = 0;
		virtual Features GetFeatures() = 0;
		virtual void CompressBlock(pVoid in, pVoid out) = 0;
	};

	class BlockCompressor_bc7enc : public IBlockCompressor
	{
	public:
		BlockCompressor_bc7enc(const ImageCompressorOptions& options);
		void CompressBlock(pVoid in, pVoid out) override;

		ConstString GetDebugName() override { return "bc7enc"; }

		Features GetFeatures() override
		{
			// No BC2
			return Features_BC1 | Features_BC3 | Features_BC4 | Features_BC5 | Features_BC7;
		}
	};

	/**
	 * \brief Two level cache - in memory and in file system.
	 */
	class ImageCompressorCache
	{
		// TODO: We should let user to set size and directory

		static constexpr u32 MEMORY_STORE_BUDGET = 256u * 1024u * 1024u; // 256MB
		static constexpr u32 FILESYSTEM_STORE_BUDGET = 2048u * 1024u * 1024u; // 2GB

		file::WPath m_CacheDirectory;

		// Computes unique hash based on pixel data and compression options
		HashValue ComputeImageHash(const PixelData imageData, u32 imageDataSize, const ImageCompressorOptions& options) const;

	public:
		ImageCompressorCache()
		{
			
		}

		amPtr<Image_DDS> GetFromCache(const PixelData imageData, u32 imageDataSize, const ImageCompressorOptions& options) const;
		void Cache(const PixelData imageData, u32 imageDataSize, const ImageCompressorOptions& options);
	};

	/**
	 * \brief DDS Image encoder with internal caching.
	 */
	class ImageCompressor
	{
		ImageCompressorCache m_Cache;

		// Retrieves starting position of 4x4 block in given image
		pVoid GetImagePixelBlock(
			int width, int height, 
			int blockX, int blockY,
			const PixelData pixelData, ImagePixelFormat pixelFmt) const;

		int ComputeMipCount(int width, int height, const ImageCompressorOptions& options);

	public:
		// Compresses given image with given options and returns newly created image(s)
		amPtr<Image_DDS> Compress(const ImagePtr& img, const ImageCompressorOptions& options);
	};
}
