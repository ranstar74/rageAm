#include "bc.h"

#include "bc7enc.h"
#include "resize.h"
#include "rgbcx.h"
#include "am/system/enum.h"
#include "common/logger.h"
#include "helpers/align.h"
#include "rage/atl/datahash.h"
#include "rage/math/math.h"

rageam::graphics::BlockCompressor_bc7enc::BlockCompressor_bc7enc(const ImageCompressorOptions& options)
	: IBlockCompressor(options)
{
	if (options.Format == BlockFormat_None)
	{
		return;
	}

	if (options.Format == BlockFormat_BC7)
	{
		bc7enc_compress_block_init();
		return;
	}

	// BC1 - BC5
	// rgbcx::init(); // TODO: ...
}

void rageam::graphics::BlockCompressor_bc7enc::CompressBlock(pVoid in, pVoid out)
{

}

rageam::HashValue rageam::graphics::ImageCompressorCache::ComputeImageHash(
	const PixelData imageData, u32 imageDataSize, const ImageCompressorOptions& options) const
{
	u32 hash;
	hash = rage::atDataHash(imageData, imageDataSize);
	hash = rage::atDataHash(&options, sizeof ImageCompressorOptions, hash);
	return hash;
}

amPtr<rageam::graphics::Image_DDS> rageam::graphics::ImageCompressorCache::GetFromCache(
	const PixelData imageData, u32 imageDataSize, const ImageCompressorOptions& options) const
{
	u32 hash = ComputeImageHash(imageData, imageDataSize, options);

	// TODO: ...

	return nullptr;
}

void rageam::graphics::ImageCompressorCache::Cache(
	const PixelData imageData, u32 imageDataSize, const ImageCompressorOptions& options)
{
	// TODO: ...
}

pVoid rageam::graphics::ImageCompressor::GetImagePixelBlock(
	int width, int height, int blockX, int blockY, const PixelData pixelData, ImagePixelFormat pixelFmt) const
{
	u32 elementWidth = ImagePixelFormatToSize[pixelFmt];
	u32 lineStride = width * elementWidth;

	int pixelX = blockX * 4;
	int pixelY = blockY * 4;

	u32 byteOffset = lineStride * pixelY + elementWidth * pixelX;

	return pixelData->Bytes + byteOffset;
}

int rageam::graphics::ImageCompressor::ComputeMipCount(int width, int height, const ImageCompressorOptions& options)
{
	if (!options.GenerateMipMaps)
		return 1;

	return GetMaximumMipCountForResolution(width, height);
}

amPtr<rageam::graphics::Image_DDS> rageam::graphics::ImageCompressor::Compress(
	const ImagePtr& img, const ImageCompressorOptions& options)
{
	AM_ASSERT(img->CanBeCompressed(), "ImageCompressor::Compress() -> Given image can't be compressed!");

	ImageInfo imgInfo = img->GetInfo();

	// Attempt to retrieve image from cache
	amPtr<Image_DDS> compressedImage = m_Cache.GetFromCache(img->GetPixelData().Data, img->GetSlicePitch(), options);
	if (compressedImage)
		return compressedImage;

	// Image was not in cache, compress it. We compute compress time to cache only expensive images
	Timer timer = Timer::StartNew();

	// We support only one compressor implementation for now
	// NOTE: Compressor instance is meant to be created&destructed for every new image
	BlockCompressor_bc7enc encoder(options);

	// Make sure encoder supports requested format
	AM_ASSERT(options.Format & encoder.GetFeatures(),
		"ImageCompressor::Compress() -> Encoder '%s' does not support requested format '%s'!",
		encoder.GetDebugName(), Enum::GetName(options.Format));

	ImagePixelFormat compressedPixelFormat = BlockCompressorFormatToPixelFormat[imgInfo.PixelFormat];

	int mipCount = ComputeMipCount(imgInfo.Width, imgInfo.Height, options);

	// Allocate continuous block of memory for all mip maps
	u32 totalCompressedSize = 0;
	for (int i = 0; i < mipCount; i++)
	{
		int mipWidth = imgInfo.Width >> i;
		int mipHeight = imgInfo.Height >> i;
		totalCompressedSize += ComputeImageSlicePitch(mipWidth, mipHeight, compressedPixelFormat);
	}
	char* mipsPixelBlock = new char[totalCompressedSize];

	// For BC1-BC5 encoders
	u32 rgbcxQuality = static_cast<u32>(options.Quality * rgbcx::MAX_LEVEL);

	// We compress every mip layer by downsampling image by factor of 2 every time
	ImagePtr mipImg = img;
	ImageInfo mipInfo;
	for (int i = 0; i < mipCount; i++) // Compress mip slice
	{
		mipInfo = mipImg->GetInfo();

		int blocksX = mipInfo.Width / 4;
		int blocksY = mipInfo.Height / 4;

		for (int y = 0; y < blocksY; y++)
		{
			for (int x = 0; x < blocksX; x++)
			{
				// Get 4x4 block to compress from source mip image
				pVoid sourcePixels = GetImagePixelBlock(
					mipInfo.Width, mipInfo.Height, x, y, mipImg->GetPixelData().Data, mipInfo.PixelFormat);

				pVoid destPixels = 0; // TODO: ...

				// rgbcx::encode_bc3(rgbcxQuality, destPixels, static_cast<u8*>(sourcePixels), true, false);
			}
		}

		// Downsample next mip layer
		mipImg = mipImg->Resize(mipInfo.Width / 2, mipInfo.Height / 2);
	}

	// See if image compression took long enough to compress it
	timer.Stop();
	if (timer.GetElapsedMilliseconds() > 50)
	{
		m_Cache.Cache(img->GetPixelData().Data, img->GetSlicePitch(), options);
	}

	// Create DDS image from compressed pixel data
	/*compressedImage = std::make_shared<Image_DDS>(
		mipsPixelBlock, compressedPixelFormat, imgInfo.Width, imgInfo.Height, mipCount, true);
	return compressedImage;*/
	return nullptr;
}
