//
// File: bc.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "image.h"

#ifdef AM_IMAGE_USE_AVX2
#include <bc7e_ispc_avx2.h>
#else
#include <bc7e_ispc_sse2.h>
#endif

namespace rageam
{
	class BackgroundWorker;
}

namespace rageam::graphics
{
	// Used terms:
	// BC - Block compression
	// Encoded / Enc - same as compressed
	// Region / Reg - part of image defined by pixel row range

	// https://en.wikipedia.org/wiki/S3_Texture_Compression

	// Block compression methods and common use:
	//
	// Color / Specular			BC1, BC7
	// Color maps w opacity		BC1, BC2, BC3, BC7
	// Grayscale				BC4
	// Normal					BC1, BC5

#define IMAGE_BC_USE_MULTITHREADING

#ifdef IMAGE_BC_USE_MULTITHREADING // Must be multiple of 4!
	static constexpr int IMAGE_BC_MULTITHREAD_MAX_REGIONS = 16;		// Max threads
	static constexpr int IMAGE_BC_MULTITHREAD_MIN_REGION_SIZE = 16;	// Num of Y blocks per one region
#else
	static constexpr int IMAGE_BC_MULTITHREAD_MAX_REGIONS = 1;
	static constexpr int IMAGE_BC_MULTITHREAD_MIN_REGION_SIZE = IMAGE_MAX_RESOLUTION / 4;
#endif

	enum BlockFormat // We keep it as separate enumeration from pixel formats to prevent using non-compressed formats in compressor
	{
		BlockFormat_None,	// Raw RGBA32, not recommended, BC7 offers almost the same quality
		BlockFormat_BC1,	// DXT1
		BlockFormat_BC2,	// DXT3
		BlockFormat_BC3,	// DXT5
		BlockFormat_BC4,	// ATI1
		BlockFormat_BC5,	// ATI2
		BlockFormat_BC7,	// At this point they finally realized that giving weird names to formats is not a good idea
	};

	static constexpr ImagePixelFormat BlockFormatToPixelFormat[] =
	{
		ImagePixelFormat_U32,
		ImagePixelFormat_BC1,
		ImagePixelFormat_BC2,
		ImagePixelFormat_BC3,
		ImagePixelFormat_BC4,
		ImagePixelFormat_BC5,
		ImagePixelFormat_BC7,
	};
	BlockFormat ImagePixelFormatToBlockFormat(ImagePixelFormat fmt);

	// 4x4 block of RGBA pixels
	static constexpr size_t IMAGE_BC_BLOCK_SLICE_PITCH = IMAGE_RGBA_PITCH * 4 * 4;
	static constexpr size_t IMAGE_BC_BLOCK_ROW_PITCH = IMAGE_RGBA_PITCH * 4;

	// All convert a 4×4 block of pixels to a 64-bit or 128-bit quantity,
	// resulting in compression ratios of 6:1 with 24-bit RGB input data or 4:1 with 32-bit RGBA input data
	static constexpr u32 IMAGE_BC_1_4_BLOCK_SIZE = 8;		// BC1 and BC4 compress 4x4 pixel block to 64 bits
	static constexpr u32 IMAGE_BC_2_3_5_7_BLOCK_SIZE = 16;	// BC2, BC3, BC5 and BC7 compress 4x4 pixel block to 128 bits

	enum class BlockCompressorImpl
	{
		None,			// RGBA / To choose implementation automatically
		bc7enc_rdo,		// BC1, BC3, BC4, BC5, BC7
		icbc,			// BC1
	};

	static constexpr BlockCompressorImpl BlockFormatToDefaultCompressorImpl[] =
	{
		BlockCompressorImpl::None,			// RGBA
		BlockCompressorImpl::bc7enc_rdo,	// BC1
		BlockCompressorImpl::None,			// BC2
		BlockCompressorImpl::bc7enc_rdo,	// BC3
		BlockCompressorImpl::bc7enc_rdo,	// BC4
		BlockCompressorImpl::bc7enc_rdo,	// BC5
		BlockCompressorImpl::bc7enc_rdo,	// BC7
	};

	// Size of encoded 4x4 pixel block in bytes
	static constexpr u32 BlockFormatToBlockSize[] =
	{
		4,	// RGBA, for consistency
		8,	// BC1
		16,	// BC2
		16, // BC3
		8,	// BC4
		16,	// BC5
		16,	// BC7
	};

	// All except BC2
	static constexpr int IMAGE_BC7ENC_RDO_FORMATS =
		1 << BlockFormat_BC1 | 1 << BlockFormat_BC3 | 1 << BlockFormat_BC4 | 1 << BlockFormat_BC5 | 1 << BlockFormat_BC7;

	// Only BC1
	static constexpr int IMAGE_ICBC_FORMATS = 1 << BlockFormat_BC1;

	PixelDataOwner ImageDecodeBCToRGBA(const PixelDataOwner& pixels, int width, int height, ImagePixelFormat format);

	struct ImageCompressorOptions
	{
		BlockCompressorImpl CompressorImpl = BlockCompressorImpl::None;
		BlockFormat			Format = BlockFormat_BC7;
		ResizeFilter		MipFilter = ResizeFilter_Box;
		float				Quality = 0.65f;			// 0 - Worst, fastest; 1 - Best, slowest
		int					MaxResolution = 0;			// Down-scales base image resolution to specified one, 0 to use original
		bool				GenerateMipMaps = true;		// Creates mip maps up to 4x4
		bool				CutoutAlpha = false;		// All pixels with alpha less than threshold are made transparent, otherwise fully opaque
		int					CutoutAlphaThreshold = 127;
		bool				AlphaTestCoverage = false;	// Makes sure that % of alpha is preserved on mip maps
		int					AlphaTestThreshold = 170;
		int					Brightness = 0;
		int					Contrast = 0;
		bool				PadToPowerOfTwo = false;
		bool				AllowRecompress = false; 	// For users that want to re-compress .dds for their own reasons

		bool operator==(const ImageCompressorOptions&) const = default;
	};

	struct EncoderData_bc7enc_rdo
	{
		u32	 RgbxLevel; // Quality for RGBX BC1 and BC3. 0 - worst, fastest; 18 - best, slowest
		bool RgbxHq345; // High quality encoding for RGBX BC3, BC4, BC5
		// 0 - ultra-fast
		// 1 - very-fast
		// 2 - fast
		// 3 - basic
		// 4 - slow
		// 5 - very-slow
		// 6 - slowest
		int	Bc7Quality;
	};

	struct EncoderData_icbc
	{
		int Quality;
	};

	// Information about compressed image
	struct CompressedImageInfo
	{
		ImageInfo				ImageInfo;
		ResizeFilter			MipFilter;
		bool					CutoutAlpha;
		int						CutoutAlphaThreshold;
		bool					AlphaTestCoverage;
		int						AlphaTestThreshold;
		int						Brightness;
		int						Contrast;
		BlockCompressorImpl		EncoderImpl;
		EncoderData_bc7enc_rdo	EncoderData_bc7enc_rdo;
		EncoderData_icbc		EncoderData_icbc;
		Vec2S					UV2 = { 1.0f, 1.0f };
		// if ImageCompressorOptions::AllowRecompress was set to true, this flag
		// indicates if source DDS image was recompressed
		bool					IsSourceCompressed;
	};

	struct ImageCompressorToken
	{
		// Mutex is not required for reading and for canceling

		// If set to true, compressor will attempt to stop processing as soon as possible
		bool		Canceled;
		int			ProcessedBlockLines;
		int			TotalBlockLines;
		std::mutex	Mutex;

		void Reset()
		{
			Canceled = false;
			ProcessedBlockLines = 0;
			TotalBlockLines = 0;
		}
	};
	using ImageCompressorTokenPtr = ImageCompressorToken*;

	/**
	 * \brief BC Image encoder with internal caching.
	 */
	class ImageCompressor
	{
		struct Region
		{
			pChar SrcPixels;
			pChar DstPixels;
		};

		// Note: the way this struct is designed, only one mip allowed to be compressed at the time!
		// No multithreaded mip compression is possible
		struct EncoderState
		{
			ImageCompressorTokenPtr				Token;
			ImagePtr							Image;
			ImageInfo							ImageInfo;
			CompressedImageInfo					EncodeInfo;
			// Pointer to compressed pixel data buffer
			pChar								DstPixels;
			ImagePixelFormat					DstPixelFormat;
			u32									SrcPixelPitch;
			u32									DstPixelPitch;
			u32									SrcRowPitch;
			u32									DstRowPitch;
			BlockCompressorImpl					EncoderImpl;
			// Storage for all regions shared by parallel threads
			Region								Regions[IMAGE_BC_MULTITHREAD_MAX_REGIONS];
			int									BlockCountX;
			int									BlockCountY;
			int									RegionBlocksCount;
			float								DesiredAlphaCoverage;
			float								AlphaCoverageScale;
			ispc::bc7e_compress_block_params	bc7enc_rdo_params;
		};

		static void CompressBlocks(const EncoderState& encoderState, int numBlocks, char* dstBlocks, char* srcBlocks);
		static void CompressMipRegion(const EncoderState& encoderState, const Region& region);
		static void CompressMip(EncoderState& imgEncData);

		// Does not perform actual compression but only computes metadata of (potential) compressed image
		// NOTE: Either pixelHashOverride or pixelData must be provided!
		static CompressedImageInfo GetInfoAndHash(
			const ImageInfo& imgInfo,
			const ImageCompressorOptions& options,
			u32& outHash,
			const u32* pixelHashOverride = nullptr,
			ImagePixelData pixelData = nullptr,
			u32 pixelDataSize = 0);

		// We have separate from system worker because:
		// - Need more threads
		// - Scenario when all system worker threads are used would cause deadlock
		static BackgroundWorker* sm_RegionWorker;

	public:
		// Compresses given image with given options and returns newly created image
		// pixelHashOverride is used to compute the final hash sum of the image, iterating over pixel data is a bit expensive,
		// so something else (such as hash sum of image modify time + path) can be provided instead, as long as it is unique to the image
		static ImagePtr Compress(
			const ImagePtr& img,
			const ImageCompressorOptions& options,
			const u32* pixelHashOverride = nullptr,
			CompressedImageInfo* outCompInfo = nullptr,
			ImageCompressorToken* token = nullptr);

		// Decodes BC pixels to RGBA
		// If given image format is already RGBA32, a reference to original pixel data will be returned
		static ImagePtr Decompress(const ImagePtr& img, int mipIndex = 0);

		// Decodes single block of given format and outputs 4x4 RGBA pixels
		static void DecompressBlock(const char* inBlock, char* outPixels, ImagePixelFormat fmt);

		static void InitClass();
		static void ShutdownClass();
	};
}
