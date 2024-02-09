#include "bc.h"

#include "am/system/enum.h"
#include "am/file/fileutils.h"
#include "am/xml/doc.h"
#include "am/xml/serialize.h"
#include "am/system/worker.h"
#include "rage/math/math.h"
#include "imagecache.h"

#include <rgbcx.h>
#include <icbc.h>
#include <bc7decomp.h>
#include <easy/profiler.h>

rageam::BackgroundWorker* rageam::graphics::ImageCompressor::sm_RegionWorker = nullptr;

rageam::graphics::BlockFormat rageam::graphics::ImagePixelFormatToBlockFormat(ImagePixelFormat fmt)
{
	switch (fmt)
	{
	case ImagePixelFormat_BC1: return BlockFormat_BC1;
	case ImagePixelFormat_BC2: return BlockFormat_BC2;
	case ImagePixelFormat_BC3: return BlockFormat_BC3;
	case ImagePixelFormat_BC4: return BlockFormat_BC4;
	case ImagePixelFormat_BC5: return BlockFormat_BC5;
	case ImagePixelFormat_BC7: return BlockFormat_BC7;

	default: return BlockFormat_None;
	}
}

rageam::graphics::PixelDataOwner rageam::graphics::ImageDecodeBCToRGBA(const PixelDataOwner& pixels, int width, int height, ImagePixelFormat format)
{
	AM_ASSERT(ImageIsCompressedFormat(format) || format == ImagePixelFormat_U32, "ImageDecodeBCToRGBA() -> Pixel format must be BC or RGBA!");

	// Already RGBA
	if (format == ImagePixelFormat_U32)
		return pixels;

	char* mipBytes = pixels.Data()->Bytes;
	int mipWidth = width;
	int mipHeight = height;
	int blocksX = mipWidth / 4;
	int blocksY = mipHeight / 4;
	u32 blockWidth = BlockFormatToBlockSize[ImagePixelFormatToBlockFormat(format)];
	u32 encodedRowPitch = blocksX * blockWidth;
	u32 decodedRowPitch = mipWidth * IMAGE_RGBA_PITCH;

	PixelDataOwner decodedDataOwner = PixelDataOwner::AllocateForImage(mipWidth, mipHeight, ImagePixelFormat_U32);
	char* decodedPixels = decodedDataOwner.Data()->Bytes;

	for (int blockY = 0; blockY < blocksY; blockY++)
	{
		int y = blockY * 4;
		for (int blockX = 0; blockX < blocksX; blockX++)
		{
			int x = blockX * 4;

			// Decode 4x4 pixels block
			char decodedBlock[IMAGE_BC_BLOCK_SLICE_PITCH];
			char* encodedBlock = mipBytes + static_cast<size_t>(blockY * encodedRowPitch + blockX * blockWidth);
			ImageCompressor::DecompressBlock(encodedBlock, decodedBlock, format);

			// Copy it line by line to the decoded pixel array
			char* dstRow = decodedPixels + static_cast<size_t>(decodedRowPitch) * y + IMAGE_RGBA_PITCH * x;
			for (int i = 0; i < 4; i++)
			{
				char* srcRow = decodedBlock + i * IMAGE_BC_BLOCK_ROW_PITCH;

				memcpy(dstRow, srcRow, IMAGE_RGBA_PITCH * 4);

				dstRow += decodedRowPitch;
			}
		}
	}

	return decodedDataOwner;
}

void rageam::graphics::ImageCompressor::CompressBlocks(const EncoderState& encoderState, int numBlocks, char* dstBlocks, char* srcBlocks)
{
	for (int i = 0; i < numBlocks; i++)
	{
		u8* srcBlockU8 = reinterpret_cast<u8*>(srcBlocks);

		switch (encoderState.EncoderImpl)
		{
		case BlockCompressorImpl::bc7enc_rdo:
		{
			const EncoderData_bc7enc_rdo& rdoData = encoderState.EncodeInfo.EncoderData_bc7enc_rdo;

			auto rgbxLevel = rdoData.RgbxLevel;
			auto useHq = rdoData.RgbxHq345;

			switch (encoderState.DstPixelFormat)
			{
			case ImagePixelFormat_BC1:
				rgbcx::encode_bc1(rgbxLevel, dstBlocks, srcBlockU8, true, false);
				break;

			case ImagePixelFormat_BC3:
				if (useHq)	rgbcx::encode_bc3_hq(rgbxLevel, dstBlocks, srcBlockU8);
				else		rgbcx::encode_bc3(rgbxLevel, dstBlocks, srcBlockU8);
				break;

			case ImagePixelFormat_BC4:
				if (useHq)	rgbcx::encode_bc4_hq(dstBlocks, srcBlockU8);
				else		rgbcx::encode_bc4(dstBlocks, srcBlockU8);
				break;

			case ImagePixelFormat_BC5:
				if (useHq)	rgbcx::encode_bc5_hq(dstBlocks, srcBlockU8);
				else		rgbcx::encode_bc5(dstBlocks, srcBlockU8);
				break;

			default: break;
			}
			break;
		}
		case BlockCompressorImpl::icbc:
		{
			icbc::Quality icbcQuality = icbc::Quality(encoderState.EncodeInfo.EncoderData_icbc.Quality);
#define ICBC_U8_TO_FLOAT_UNORM(value) ((float)(value) / 255.0f)
			// For ICBC we have to convert 4x4 pixel block from U32 to FLOAT4
			alignas(32) float srcBlockFloat4[16 * 4];
			for (int y_ = 0; y_ < 4; y_++)
			{
				for (int x_ = 0; x_ < 4; x_++)
				{
					srcBlockFloat4[y_ * 16 + x_ * 4 + 0] = ICBC_U8_TO_FLOAT_UNORM(srcBlockU8[y_ * 16 + x_ * 4 + 0]); // R
					srcBlockFloat4[y_ * 16 + x_ * 4 + 1] = ICBC_U8_TO_FLOAT_UNORM(srcBlockU8[y_ * 16 + x_ * 4 + 1]); // G
					srcBlockFloat4[y_ * 16 + x_ * 4 + 2] = ICBC_U8_TO_FLOAT_UNORM(srcBlockU8[y_ * 16 + x_ * 4 + 2]); // B
					srcBlockFloat4[y_ * 16 + x_ * 4 + 3] = ICBC_U8_TO_FLOAT_UNORM(srcBlockU8[y_ * 16 + x_ * 4 + 3]); // A
				}
			}
#undef ICBC_U8_TO_FLOAT_UNORM

			alignas(32) static constexpr float inputWeights[16] =
			{
				1, 1, 1, 1,
				1, 1, 1, 1,
				1, 1, 1, 1,
				1, 1, 1, 1,
			};

			alignas(32) static constexpr float colorWeights[3] = { 3, 4, 2 };

			switch (encoderState.DstPixelFormat)
			{
			case ImagePixelFormat_BC1:
				compress_bc1(icbcQuality, srcBlockFloat4, inputWeights, colorWeights, true, true, dstBlocks);
				break;

			default: break;
			}
			break;
		}
		default: break;
		}

		srcBlocks += IMAGE_BC_BLOCK_SLICE_PITCH;
		dstBlocks += encoderState.DstPixelPitch;
	}
}

void rageam::graphics::ImageCompressor::CompressMipRegion(const EncoderState& encoderState, const Region& region)
{
	EASY_FUNCTION("");

	char* srcPixels = region.SrcPixels;
	char* dstPixels = region.DstPixels;

	int blockCountX = encoderState.BlockCountX;
	int regionCount = encoderState.RegionBlocksCount;

	const CompressedImageInfo& encodeInfo = encoderState.EncodeInfo;

	// According to bc7enc_rdo comments, 64 blocks at a time is ideal for efficient SIMD processing
	// One block is 4x4 pixels, 64 blocks is 256x4 pixels
	static constexpr int BLOCK_GROUP_SIZE = 64;

	alignas(32) char srcBlockGroupBuffer[IMAGE_BC_BLOCK_SLICE_PITCH * BLOCK_GROUP_SIZE];

	for (int blockY = 0; blockY < regionCount; blockY++)
	{
		char* srcBlockRowPixels = srcPixels;

		// We compress image by rows of 64 block groups
		for (int blockX = 0; blockX < blockCountX; blockX += BLOCK_GROUP_SIZE)
		{
			if (encoderState.Token && encoderState.Token->Canceled)
				return;

			char* dstBlockPixels = srcBlockGroupBuffer;

			// Then we have to fill pixel row in 64 block group
			int numBlocks = MIN(blockCountX - blockX, BLOCK_GROUP_SIZE);

			// For each block in group
			for (int block = 0; block < numBlocks; block++)
			{
				char* srcBlockPixels = srcBlockRowPixels;

				// For each row of block group
				for (int blockPixelY = 0; blockPixelY < 4; blockPixelY++)
				{
					// Copy 4 pixels to destination block row
					memcpy(dstBlockPixels, srcBlockPixels, IMAGE_BC_BLOCK_ROW_PITCH);

					// Apply coverage + cutout alpha on those 4 pixels that we just copied
					if (encodeInfo.CutoutAlpha || encodeInfo.AlphaTestCoverage)
					{
						// It is faster to do this inline than calling ImageCutoutAlpha
						ColorU32* pixels = reinterpret_cast<ColorU32*>(dstBlockPixels);
						for (int i = 0; i < 4; i++)
						{
							if (encodeInfo.CutoutAlpha)
							{
								if (pixels[i].A < static_cast<u8>(encodeInfo.CutoutAlphaThreshold))
									pixels[i].A = 0;
								else
									pixels[i].A = 255;
							}

							if (encodeInfo.AlphaTestCoverage)
							{
								float scaledAlpha = static_cast<float>(pixels[i].A) * encoderState.AlphaCoverageScale;
								if (scaledAlpha > 255.0f) scaledAlpha = 255.0f;
								pixels[i].A = static_cast<u8>(scaledAlpha);
							}
						}
					}

					// Shift to next row
					srcBlockPixels += encoderState.SrcRowPitch;
					dstBlockPixels += IMAGE_BC_BLOCK_ROW_PITCH;
				}

				// Shift to beginning of next block on this line
				srcBlockRowPixels += IMAGE_BC_BLOCK_ROW_PITCH;
			}

			// Finally, compress block group
			if (encoderState.EncoderImpl == BlockCompressorImpl::bc7enc_rdo &&
				encoderState.DstPixelFormat == ImagePixelFormat_BC7)
			{
				bc7e_compress_blocks(numBlocks, reinterpret_cast<u64*>(dstPixels), reinterpret_cast<u32*>(srcBlockGroupBuffer),
					&encoderState.bc7enc_rdo_params);
			}
			else
			{
				CompressBlocks(encoderState, numBlocks, dstPixels, srcBlockGroupBuffer);
			}

			dstPixels += static_cast<size_t>(numBlocks * encoderState.DstPixelPitch);
		}

		// We compressed all 4x4 pixel blocks, move to next block row 4 lines below
		srcPixels += static_cast<size_t>(4 * encoderState.SrcRowPitch);

		if (encoderState.Token)
		{
			std::unique_lock lock(encoderState.Token->Mutex);
			encoderState.Token->ProcessedBlockLines++;
		}
	}
}

void rageam::graphics::ImageCompressor::CompressMip(EncoderState& imgEncData)
{
	EASY_FUNCTION();

	ImageInfo imgInfo = imgEncData.Image->GetInfo();

	char* srcPixels = imgEncData.Image->GetPixelDataBytes();
	char* dstPixels = imgEncData.DstPixels;

	int blockCountY = imgEncData.BlockCountY;

	// Small image, process on main thread
	if (blockCountY < IMAGE_BC_MULTITHREAD_MIN_REGION_SIZE)
	{
		imgEncData.RegionBlocksCount = blockCountY;

		Region& regEncData = imgEncData.Regions[0];
		regEncData.SrcPixels = srcPixels;
		regEncData.DstPixels = dstPixels;
		CompressMipRegion(imgEncData, regEncData);
		return;
	}

	// Compute thread count
	int regionCount = blockCountY / IMAGE_BC_MULTITHREAD_MIN_REGION_SIZE;
	regionCount = MIN(regionCount, IMAGE_BC_MULTITHREAD_MAX_REGIONS);
	int regionBlocksCount = blockCountY / regionCount; // We have to recompute size after clamping count
	int regionPixelRows = imgInfo.Height / regionCount;

	imgEncData.RegionBlocksCount = regionBlocksCount;

	// Create background task for every region
	amPtr<BackgroundTask> regionTasks[IMAGE_BC_MULTITHREAD_MAX_REGIONS];

	u32 srcRegionSlicePitch = imgEncData.SrcRowPitch * regionPixelRows;
	u32 dstRegionSlicePitch = imgEncData.DstRowPitch * regionBlocksCount;

	BackgroundWorker::Push(sm_RegionWorker);

	for (int i = 0; i < regionCount; i++)
	{
		Region& regEncData = imgEncData.Regions[i];
		regEncData.SrcPixels = srcPixels;
		regEncData.DstPixels = dstPixels;

		regionTasks[i] = BackgroundWorker::Run([&]
			{
				CompressMipRegion(imgEncData, regEncData);
				return true;
			});

		srcPixels += srcRegionSlicePitch;
		dstPixels += dstRegionSlicePitch;
	}

	// Wait for parallel tasks to finish the job
	for (int k = 0; k < regionCount; k++)
	{
		regionTasks[k]->Wait();
	}

	BackgroundWorker::Pop();
}

rageam::graphics::CompressedImageInfo rageam::graphics::ImageCompressor::GetInfoAndHash(
	const ImageInfo& imgInfo,
	const ImageCompressorOptions& options,
	u32& outHash,
	const u32* pixelHashOverride,
	ImagePixelData pixelData,
	u32 pixelDataSize)
{
	EASY_FUNCTION();

	AM_ASSERT(options.Format == BlockFormat_None || ImageIsResolutionValidForMipMapsAndCompression(imgInfo.Width, imgInfo.Height),
		"ImageCompressor::GetInfoAndHash() -> Given image can't be compressed!");

	AM_ASSERT(options.MaxResolution == 0 || IS_POWER_OF_TWO(options.MaxResolution),
		"ImageCompressor::GetInfoAndHash() -> Max resolution is not power of two (%u)!", options.MaxResolution);

	// Compression is not currently implemented for other pixel formats...
	if (!options.AllowRecompress && ImageIsCompressedFormat(imgInfo.PixelFormat))
	{
		AM_UNREACHABLE("ImageCompressor::GetInfoAndHash() -> Pixel format '%s' is not implemented.",
			Enum::GetName(imgInfo.PixelFormat));
	}

	// Resize image to max resolution if it doesn't fit specified size constraint
	// Note that we are doing nothing at this point because we may retrieve this image from cache
	int baseWidth = imgInfo.Width;
	int baseHeight = imgInfo.Height;
	if (options.MaxResolution != 0 &&
		(imgInfo.Width > options.MaxResolution || imgInfo.Height > options.MaxResolution))
	{
		ImageScaleResolution(
			imgInfo.Width, imgInfo.Height, options.MaxResolution, options.MaxResolution, baseWidth, baseHeight,
			ResolutionScalingMode_Fit);
	}

	// Compute mip count
	int mipCount = 1;
	if (options.GenerateMipMaps)
	{
		mipCount = ImageComputeMaxMipCount(baseWidth, baseHeight);
	}

	CompressedImageInfo encodeInfo = {};
	encodeInfo.MipFilter = options.MipFilter;
	encodeInfo.CutoutAlpha = options.CutoutAlpha;
	encodeInfo.CutoutAlphaThreshold = options.CutoutAlpha ? options.CutoutAlphaThreshold : 0;
	encodeInfo.AlphaTestCoverage = options.AlphaTestCoverage;
	encodeInfo.AlphaTestThreshold = options.AlphaTestCoverage ? options.AlphaTestThreshold : 0;
	encodeInfo.IsSourceCompressed = ImageIsCompressedFormat(imgInfo.PixelFormat);
	encodeInfo.Brightness = options.Brightness;
	encodeInfo.Contrast = options.Contrast;

	// Threshold 0 causes weird artifacts (because whole image turned opaque), clamp to 1
	if (encodeInfo.CutoutAlphaThreshold == 0)
		encodeInfo.CutoutAlphaThreshold = 1;

	ImageInfo& encodeImageInfo = encodeInfo.ImageInfo;
	encodeImageInfo.Width = baseWidth;
	encodeImageInfo.Height = baseHeight;
	encodeImageInfo.MipCount = mipCount;
	encodeImageInfo.PixelFormat = BlockFormatToPixelFormat[options.Format];

	// Choose block compressor from options
	BlockCompressorImpl encoderImpl = options.CompressorImpl;
	if (encoderImpl == BlockCompressorImpl::None)
		encoderImpl = BlockFormatToDefaultCompressorImpl[options.Format];
	encodeInfo.EncoderImpl = encoderImpl;

	// Compute options for bc7enc_rdo
	if (encoderImpl == BlockCompressorImpl::bc7enc_rdo)
	{
		if (options.Format == BlockFormat_BC7)
		{
			encodeInfo.EncoderData_bc7enc_rdo.Bc7Quality = static_cast<int>(options.Quality * 6);
		}
		if (options.Format == BlockFormat_BC1 || options.Format == BlockFormat_BC3)
		{
			encodeInfo.EncoderData_bc7enc_rdo.RgbxLevel =
				static_cast<u32>(options.Quality * rgbcx::MAX_LEVEL);
		}
		if (options.Format == BlockFormat_BC3 || options.Format == BlockFormat_BC4 || options.Format == BlockFormat_BC5)
		{
			encodeInfo.EncoderData_bc7enc_rdo.RgbxHq345 = options.Quality > 0.6f;
		}
	}

	// Compute options for icbc
	if (encoderImpl == BlockCompressorImpl::icbc)
	{
		if (options.Format == BlockFormat_BC1)
		{
			// There are 9 levels of icbc::Quality
			encodeInfo.EncoderData_icbc.Quality = static_cast<int>(options.Quality * 9);
		}
	}

	// Compute image cache hash
	u32 imgHash;
	if (pixelHashOverride)
	{
		imgHash = DataHash(&encodeInfo, sizeof CompressedImageInfo);
		imgHash = DataHash(pixelHashOverride, sizeof u32, imgHash);
	}
	else
	{
		AM_ASSERT(pixelData && pixelDataSize != 0, "ImageCompressor::GetInfoAndHash() -> Neither pixelHashOverride nor pixelData were given");

		ImageCache* cache = ImageCache::GetInstance();
		imgHash = cache->ComputeImageHash(pixelData, pixelDataSize, encodeInfo);
	}

	outHash = imgHash;

	return encodeInfo;
}

rageam::graphics::ImagePtr rageam::graphics::ImageCompressor::Compress(
	const ImagePtr& img, const ImageCompressorOptions& options, const u32* pixelHashOverride, CompressedImageInfo* outCompInfo, ImageCompressorToken* token)
{
	EASY_FUNCTION();

	if (token) token->Reset();

	u32 cacheHash;
	CompressedImageInfo encodeInfo = GetInfoAndHash(
		img->GetInfo(), options, cacheHash, pixelHashOverride, img->GetPixelData().Data(), img->ComputeSlicePitch());

	if (options.PadToPowerOfTwo)
	{
		encodeInfo.UV2 = img->ComputePadExtent();
	}

	if (outCompInfo) *outCompInfo = encodeInfo;

	ImageInfo& encodedImageInfo = encodeInfo.ImageInfo;
	ImageInfo imageInfo = img->GetInfo();

	// Attempt to retrieve image from cache
	ImageCache* cache = ImageCache::GetInstance();
	amPtr<Image> compressedImage = cache->GetFromCache(cacheHash, &encodeInfo.UV2);
	if (compressedImage)
		return compressedImage;

	// Previously we needed only metadata to locate image in cache, now we need pixel data too to compress it
	if (!img->EnsurePixelDataLoaded())
	{
		AM_ERRF("ImageCompressor::Compress() -> Failed to load image pixel data!");
		return nullptr;
	}

	// Image was not in cache, compress it. We compute compress time to cache only expensive images
	Timer timer = Timer::StartNew();

	// Image can be converted to RGBA + rescaled, we hold separate pointer
	ImagePtr preparedImage = img;

	// Input image must be in RGBA for faster encoding, converting pixel format is extremely fast
	if (imageInfo.PixelFormat != ImagePixelFormat_U32)
	{
		if (ImageIsCompressedFormat(imageInfo.PixelFormat) && options.AllowRecompress)
			preparedImage = Decompress(img);
		else
			preparedImage = img->ConvertPixelFormat(ImagePixelFormat_U32);
		imageInfo.PixelFormat = ImagePixelFormat_U32;
	}

	// Pad to power of two if we want to generate mip maps...
	if (options.PadToPowerOfTwo)
	{
		Vec2S uv2;
		preparedImage = preparedImage->PadToPowerOfTwo(uv2);
		imageInfo = preparedImage->GetInfo();
		u32 unusedHash; // We must ignore this hash because we use one from non-padded image
		encodeInfo = GetInfoAndHash(
			imageInfo, options, unusedHash, pixelHashOverride, preparedImage->GetPixelData().Data(), preparedImage->ComputeSlicePitch());

		if (outCompInfo)
		{
			outCompInfo->UV2 = uv2;
		}
		else
		{
			AM_WARNINGF("ImageCompressor::Compress() -> Padded to power of two but outCompInfo is NULL.");
		}
	}

	// Now that we know image is not in cache, we can resize it (size depends on options.MaxResolution)
	int mipCount = encodedImageInfo.MipCount;
	int compWidth = encodeInfo.ImageInfo.Width;
	int compHeight = encodeInfo.ImageInfo.Height;
	bool needResizeImage = compWidth != imageInfo.Width || compHeight != imageInfo.Height;
	preparedImage = needResizeImage ? preparedImage->Resize(compWidth, compHeight) : preparedImage;

	// Skip encoders initialization for RGBA
	EncoderState encoderState = {};
	encoderState.Token = token;
	if (options.Format != BlockFormat_None)
	{
		encoderState.SrcPixelPitch = ImagePixelFormatBitsPerPixel[imageInfo.PixelFormat] / 8;
		encoderState.EncodeInfo = encodeInfo;
		encoderState.ImageInfo = imageInfo;
		encoderState.EncoderImpl = encodeInfo.EncoderImpl;

		if (encodeInfo.EncoderImpl == BlockCompressorImpl::None)
		{
			if (outCompInfo) *outCompInfo = {};
			AM_ERRF("ImageCompressor::Compress() -> Encoder was not resolved to any implementation, returning NULL.");
			return nullptr;
		}

		// Initialize block encoders
		static bool s_CompressorsInitialized = false;
		if (!s_CompressorsInitialized)
		{
			ispc::bc7e_compress_block_init();
			rgbcx::init();
			icbc::init();
			s_CompressorsInitialized = true;
		}

		if (options.Format == BlockFormat_BC1 || options.Format == BlockFormat_BC4)
			encoderState.DstPixelPitch = IMAGE_BC_1_4_BLOCK_SIZE;
		else
			encoderState.DstPixelPitch = IMAGE_BC_2_3_5_7_BLOCK_SIZE;

		// Choose encoder quality for bc7enc_rdo
		if (encodeInfo.EncoderImpl == BlockCompressorImpl::bc7enc_rdo && options.Format == BlockFormat_BC7)
		{
			switch (encodeInfo.EncoderData_bc7enc_rdo.Bc7Quality)
			{
			case 0: bc7e_compress_block_params_init_ultrafast(&encoderState.bc7enc_rdo_params, false);	break;
			case 1: bc7e_compress_block_params_init_veryfast(&encoderState.bc7enc_rdo_params, false);	break;
			case 2: bc7e_compress_block_params_init_fast(&encoderState.bc7enc_rdo_params, false);		break;
			case 3: bc7e_compress_block_params_init_basic(&encoderState.bc7enc_rdo_params, false);		break;
			case 4: bc7e_compress_block_params_init_slow(&encoderState.bc7enc_rdo_params, false);		break;
			case 5: bc7e_compress_block_params_init_veryslow(&encoderState.bc7enc_rdo_params, false);	break;
			case 6: bc7e_compress_block_params_init_slowest(&encoderState.bc7enc_rdo_params, false);	break;

			default: AM_UNREACHABLE("ImageCompressor::Compress() -> Invalid BC7 quality '%i' for bc7enc_rdo",
				encodeInfo.EncoderData_bc7enc_rdo.Bc7Quality);
			}
		}
	}

	// Allocate continuous block of memory for all mip maps
	u32 encodedDataSize = ImageComputeTotalSizeWithMips(compWidth, compHeight, mipCount, encodedImageInfo.PixelFormat);
	PixelDataOwner encodedDataOwner = PixelDataOwner::AllocateWithSize(encodedDataSize);
	pChar encodedPixels = encodedDataOwner.Data()->Bytes;

	// Compute total amount of rows for progress report
	if (encoderState.Token)
	{
		std::unique_lock lock(encoderState.Token->Mutex);
		for (int i = 0; i < mipCount; i++)
		{
			encoderState.Token->TotalBlockLines += (imageInfo.Height >> i) / 4;
		}
	}

	// We compress every mip layer by downsampling image by factor of 2 every time
	ImageInfo mipInfo;
	for (int i = 0; i < mipCount; i++)
	{
		// NOTE: We keep mipImage as separate copy so post-processing doesn't stack up...
		ImagePtr  mipImage = preparedImage;
		mipInfo = mipImage->GetInfo();

		u32 encodedMipSlicePitch = ImageComputeSlicePitch(mipInfo.Width, mipInfo.Height, encodedImageInfo.PixelFormat);

		// Compute scaling factor to preserve alpha coverage
		if (encodeInfo.AlphaTestCoverage)
		{
			float alphaCoverage = ImageAlphaTestCoverageRGBA(
				mipImage->GetPixelDataBytes(), mipInfo.Width, mipInfo.Height, encodeInfo.AlphaTestThreshold);

			// Compute alpha coverage if we're on the first mip (as reference) and then
			if (i == 0)
			{
				encoderState.DesiredAlphaCoverage = alphaCoverage;
				encoderState.AlphaCoverageScale = 1.0f;
			}
			else
			{
				encoderState.AlphaCoverageScale = ImageAlphaTestFindBestScaleRGBA(
					mipImage->GetPixelDataBytes(), mipInfo.Width, mipInfo.Height, encodeInfo.AlphaTestThreshold,
					encoderState.DesiredAlphaCoverage);
			}
		}

		// Post-processing, we do it per mip because it's very fast
		// If we do processing on the first mip, it will invalidate caching for Image::Resize...
		if (encodeInfo.Brightness != 0 || encodeInfo.Contrast != 0)
		{
			mipImage = mipImage->AdjustBrightnessAndContrast(encodeInfo.Brightness, encodeInfo.Contrast);
		}

		if (options.Format != BlockFormat_None)
		{
			encoderState.Image = mipImage;
			encoderState.DstPixels = encodedPixels;
			encoderState.DstPixelFormat = encodeInfo.ImageInfo.PixelFormat;
			encoderState.SrcRowPitch = ImageComputeRowPitch(mipInfo.Width, mipInfo.PixelFormat);
			encoderState.DstRowPitch = ImageComputeRowPitch(mipInfo.Width, encodedImageInfo.PixelFormat);
			encoderState.BlockCountX = mipInfo.Width / 4;
			encoderState.BlockCountY = mipInfo.Height / 4;
			CompressMip(encoderState);
		}
		else
		{
			// For RGBA we just need to copy pixels
			memcpy(encodedPixels, mipImage->GetPixelDataBytes(), encodedMipSlicePitch);

			// For BC we process this in compress block function
			if (encodeInfo.CutoutAlpha)
				ImageCutoutAlphaRGBA(encodedPixels, mipInfo.Width, mipInfo.Height, encodeInfo.CutoutAlphaThreshold);
			// First mip doesn't require alpha scaling
			if (i != 0 && encodeInfo.AlphaTestCoverage)
				ImageScaleAlphaRGBA(encodedPixels, mipInfo.Width, mipInfo.Height, encoderState.AlphaCoverageScale);
		}

		if (token && token->Canceled)
			return nullptr;

		// Move to next compressed mip map pixel data
		encodedPixels += encodedMipSlicePitch;

		// Downsample to next mip map
		preparedImage = preparedImage->Resize(mipInfo.Width / 2, mipInfo.Height / 2, options.MipFilter);
	}

	// Create DDS image from compressed pixel data
	ImagePtr compImage = std::make_shared<Image>(encodedDataOwner, encodedImageInfo);

	// See if image compression took long enough to compress it
	timer.Stop();
	if (cache->ShouldStore(timer.GetElapsedMilliseconds()))
	{
		cache->Cache(compImage, cacheHash, encodedDataSize, ImageCacheEntryFlags_StoreInFileSystem, outCompInfo->UV2);
	}

	return compImage;
}

rageam::graphics::ImagePtr rageam::graphics::ImageCompressor::Decompress(const ImagePtr& img, int mipIndex)
{
	ImageInfo info = img->GetInfo();

	int mipWidth = info.Width >> mipIndex;
	int mipHeight = info.Height >> mipIndex;
	PixelDataOwner decodedPixels = ImageDecodeBCToRGBA(img->GetPixelData(mipIndex), mipWidth, mipHeight, info.PixelFormat);

	return ImageFactory::Create(decodedPixels, ImagePixelFormat_U32, mipWidth, mipHeight);
}

void rageam::graphics::ImageCompressor::DecompressBlock(const char* inBlock, char* outPixels, ImagePixelFormat fmt)
{

	switch (fmt)
	{
	case ImagePixelFormat_BC1: rgbcx::unpack_bc1(inBlock, outPixels);		return;
	case ImagePixelFormat_BC3: rgbcx::unpack_bc3(inBlock, outPixels);		return;
	case ImagePixelFormat_BC4:
		memcpy(outPixels, &IMAGE_RGBA_ALPHA_MASK, IMAGE_BC_BLOCK_SLICE_PITCH); // unpack_bc4 only sets R component
		rgbcx::unpack_bc4(inBlock, (u8*)outPixels);	return;
	case ImagePixelFormat_BC5:
		memset(outPixels, 0, IMAGE_BC_BLOCK_SLICE_PITCH); // unpack_bc4 only sets R and G components
		rgbcx::unpack_bc5(inBlock, outPixels);
		// For some reason decoder messes up alpha channel (sets to random garbage) so we have to set it manually
		// out pixels is 64 byte array, we can use 4 128bit lanes to speed it up
		for (int i = 0; i < 4; i++)
		{
			char* lane = &outPixels[i * IMAGE_BC_BLOCK_ROW_PITCH];
			__m128i fourPixels;
			fourPixels = _mm_loadu_epi8(lane);
			fourPixels = _mm_or_epi32(fourPixels, IMAGE_RGBA_ALPHA_MASK);
			_mm_storeu_epi8(lane, fourPixels);
		}
		return;

	case ImagePixelFormat_BC7:
		bc7decomp_ref::unpack_bc7(inBlock, (bc7decomp::color_rgba*)outPixels);
		return;

	default: AM_UNREACHABLE("DecompressBlock() -> Unsupported format '%s'", Enum::GetName(fmt));
	}
}

void rageam::graphics::ImageCompressor::InitClass()
{
	sm_RegionWorker = new BackgroundWorker("Img BC", IMAGE_BC_MULTITHREAD_MAX_REGIONS);
}

void rageam::graphics::ImageCompressor::ShutdownClass()
{
	delete sm_RegionWorker;
	sm_RegionWorker = nullptr;
}
