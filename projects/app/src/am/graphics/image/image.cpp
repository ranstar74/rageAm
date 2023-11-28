#include "image.h"

#include "bc.h"

#include "am/types.h"
#include "am/file/fileutils.h"
#include "am/file/pathutils.h"
#include "am/system/enum.h"
#include "am/graphics/render/engine.h"

#include <webp/decode.h>
#include <webp/encode.h>
#include <stb_image_write.h>
#include <stb_image_resize2.h>

pVoid rageam::graphics::ImageAlloc(u32 size)
{
	pVoid block = rage_malloc(size);
	// AM_DEBUGF("ImageAlloc() -> Allocated %p with size %u", block);
	return block;
}

pVoid rageam::graphics::ImageAllocTemp(u32 size)
{
	pVoid block = rage_malloc(size); // TODO: Actual temp allocator...
	// AM_DEBUGF("ImageAllocTemp() -> Allocated %p with size %u", block, size);
	return block;
}

void rageam::graphics::ImageFree(pVoid block)
{
	// AM_DEBUGF("ImageFree() -> Freeing %p", block);
	rage_free(block);
}

void rageam::graphics::ImageFreeTemp(pVoid block)
{
	// AM_DEBUGF("ImageFreeTemp() -> Freeing %p", block);
	rage_free(block); // TODO: Actual temp allocator...
}

bool rageam::graphics::IsCompressedPixelFormat(ImagePixelFormat fmt)
{
	static constexpr char map[] =
	{
		0,					// None
		0, 0, 0, 0,			// U32, U24, U16, U8
		1, 1, 1, 1, 1, 1	// BC1, BC2, BC3, BC4, BC5
	};
	return map[fmt];
}

bool rageam::graphics::IsAlphaPixelFormat(ImagePixelFormat fmt)
{
	static constexpr char map[] =
	{
		0,					// None
		1, 0, 1, 0,			// U32, U24, U16, U8
		1, 1, 1, 0, 0, 1	// BC1, BC2, BC3, BC4, BC5
	};
	return map[fmt];
}

DXGI_FORMAT rageam::graphics::ImagePixelFormatToDXGI(ImagePixelFormat fmt)
{
	switch (fmt)
	{
	case ImagePixelFormat_U32:	return DXGI_FORMAT_R8G8B8A8_UNORM;
	case ImagePixelFormat_U16:	return DXGI_FORMAT_R8G8_UNORM;
	case ImagePixelFormat_U8:	return DXGI_FORMAT_A8_UNORM;

	case ImagePixelFormat_BC1:	return DXGI_FORMAT_BC1_UNORM;
	case ImagePixelFormat_BC2:	return DXGI_FORMAT_BC2_UNORM;
	case ImagePixelFormat_BC3:	return DXGI_FORMAT_BC3_UNORM;
	case ImagePixelFormat_BC4:	return DXGI_FORMAT_BC4_UNORM;
	case ImagePixelFormat_BC5:	return DXGI_FORMAT_BC5_UNORM;
	case ImagePixelFormat_BC7:	return DXGI_FORMAT_BC7_UNORM;

	case ImagePixelFormat_None:
	case ImagePixelFormat_U24:	return DXGI_FORMAT_UNKNOWN;
	}
	AM_UNREACHABLE("ImagePixelFormatToDXGI() -> Format '%s' is not implemented.", Enum::GetName(fmt));
}

rageam::graphics::ImagePixelFormat rageam::graphics::ImagePixelFormatFromDXGI(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:	return ImagePixelFormat_U32;
	case DXGI_FORMAT_R8G8_UNORM:		return ImagePixelFormat_U16;
	case DXGI_FORMAT_A8_UNORM:			return ImagePixelFormat_U8;

	case DXGI_FORMAT_BC1_UNORM:			return ImagePixelFormat_BC1;
	case DXGI_FORMAT_BC2_UNORM:			return ImagePixelFormat_BC2;
	case DXGI_FORMAT_BC3_UNORM:			return ImagePixelFormat_BC3;
	case DXGI_FORMAT_BC4_UNORM:			return ImagePixelFormat_BC4;
	case DXGI_FORMAT_BC5_UNORM:			return ImagePixelFormat_BC5;
	case DXGI_FORMAT_BC7_UNORM:			return ImagePixelFormat_BC7;

	default:							return ImagePixelFormat_None;
	}
}

u32 rageam::graphics::ComputeImageSlicePitch(int width, int height, ImagePixelFormat pixelFormat)
{
	int totalBits = ImagePixelFormatBitsPerPixel[pixelFormat] * width * height;
	float totalBytes = ceil(static_cast<float>(totalBits) / 8.0f);
	return static_cast<u32>(totalBytes);
}

u32 rageam::graphics::ComputeImageRowPitch(int width, ImagePixelFormat pixelFormat)
{
	// For block-compressed formats row pitch is computed differently
	if (!IsCompressedPixelFormat(pixelFormat))
		return ComputeImageSlicePitch(width, 1, pixelFormat);

	int totalNumBlocks = ceil(static_cast<float>(width) / 4.0f);

	if (pixelFormat == ImagePixelFormat_BC1 || pixelFormat == ImagePixelFormat_BC4)
		return totalNumBlocks * 8;

	// BC2, BC3, BC5, BC7
	return totalNumBlocks * 16;
}

u32 rageam::graphics::ComputeImageSizeWithMips(int width, int height, int mipCount, ImagePixelFormat pixelFormat)
{
	u32 size = 0;
	for (int i = 0; i < mipCount; i++)
	{
		int w = width >> i;
		int h = height >> i;
		size += ComputeImageSlicePitch(w, h, pixelFormat);
	}
	return size;
}

bool rageam::graphics::IsResolutionValidForMipMapsAndCompression(int width, int height)
{
	if (!IsResolutionValidForMipMaps(width, height))
		return false;

	// Not enough pixels for block compression
	if (MIN(width, height) < 4)
		return false;

	return true;
}

bool rageam::graphics::IsResolutionValidForMipMaps(int width, int height)
{
	return IS_POWER_OF_TWO(width) && IS_POWER_OF_TWO(height);
}

int rageam::graphics::ComputeMaximumMipCountForResolution(int width, int height)
{
	if (!IsResolutionValidForMipMapsAndCompression(width, height))
		return 1;

	// -1 to make 4x4 be minimum possible mip level
	return BitScanR32(MIN(width, height)) - 1;
}

bool rageam::graphics::ScanPixelsForAlpha(pVoid data, int width, int height, ImagePixelFormat fmt)
{
	if (!IsAlphaPixelFormat(fmt))
		return false;

	AM_ASSERT(!IsCompressedPixelFormat(fmt), "ScanPixelsForAlpha() -> BC Formats are not implemented.");

	PixelData pixelData = static_cast<PixelData>(data);

	int totalPixels = width * height;
	for (int k = 0; k < totalPixels; k++)
	{
		u8 alphaValue = 0;
		switch (fmt)
		{
		case ImagePixelFormat_U32: alphaValue = pixelData->U32[k][3];	break;
		case ImagePixelFormat_U16: alphaValue = pixelData->U16[k][1];	break;

		default: break;
		}

		if (alphaValue != 255)
			return true;
	}
	return false;
}

bool rageam::graphics::WriteImageStb(ConstWString path, ImageKind kind, int w, int h, int c, pVoid data, int quality)
{
	if (kind == ImageKind_DDS || kind == ImageKind_PSD)
	{
		AM_UNREACHABLE("ImageMemory::Save() -> Formats DDS and PSD are not supported.");
	}

	if (kind == ImageKind_None)
		kind = ImageKind_PNG;

	file::U8Path uPath = file::PathConverter::WideToUtf8(path);

	switch (kind)
	{
	case ImageKind_PNG:		stbi_write_png(uPath, w, h, c, data, w * c);	break;
	case ImageKind_BMP:		stbi_write_bmp(uPath, w, h, c, data);			break;
	case ImageKind_JPEG:
	case ImageKind_JPG:		stbi_write_jpg(uPath, w, h, c, data, quality);	break;
	case ImageKind_TGA:		stbi_write_tga(uPath, w, h, c, data);			break;

	default: break;
	}
	return true;
}

bool rageam::graphics::WriteImageWebp(ConstWString path, int w, int h, pVoid data, bool hasAlpha, float quality)
{
	HANDLE hFile = file::CreateNew(path);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		AM_ERRF("WriteImageWebp() -> Failed to open file for writing.");
	}

	u8* blob = nullptr;
	size_t blobSize;

	u32 stride = ComputeImageRowPitch(w, hasAlpha ? ImagePixelFormat_U32 : ImagePixelFormat_U24);
	if (quality == 100.0f)
	{
		blobSize = hasAlpha ?
			WebPEncodeLosslessRGBA(static_cast<u8*>(data), w, h, static_cast<int>(stride), &blob) :
			WebPEncodeLosslessRGB(static_cast<u8*>(data), w, h, static_cast<int>(stride), &blob);
	}
	else
	{
		blobSize = hasAlpha ?
			WebPEncodeRGBA(static_cast<u8*>(data), w, h, static_cast<int>(stride), quality, &blob) :
			WebPEncodeRGB(static_cast<u8*>(data), w, h, static_cast<int>(stride), quality, &blob);
	}

	if (!blob)
	{
		CloseHandle(hFile);
		AM_ERRF("WriteImageWebp() -> Failed to encode pixels.");
		return false;
	}

	DWORD sizeWritten;
	if (!WriteFile(hFile, blob, blobSize, &sizeWritten, NULL))
	{
		AM_ERRF("WriteImageWebp() -> Failed to write bytes.");
	}
	CloseHandle(hFile);

	free(blob);

	return true;
}

void rageam::graphics::ResizeImagePixels(
	pVoid dst, pVoid src,
	ResizeFilter filter,
	ImagePixelFormat fmt,
	int xFrom, int yFrom,
	int xTo, int yTo,
	bool hasAlphaPixels)
{
	AM_ASSERT(!IsCompressedPixelFormat(fmt), "ResizeImagePixels() -> Compressed formats are not supported.");

	// Input is identical to output size, just copy source pixels
	if (xFrom == xTo && yFrom == yTo)
	{
		u32 byteWidth = ComputeImageSlicePitch(xFrom, yFrom, fmt);
		memcpy(dst, src, byteWidth);
		return;
	}

	stbir_filter irFilter;
	switch (filter)
	{
	case ResizeFilter_Box:			irFilter = STBIR_FILTER_BOX;			break;
	case ResizeFilter_Triangle:		irFilter = STBIR_FILTER_TRIANGLE;		break;
	case ResizeFilter_CubicBSpline: irFilter = STBIR_FILTER_CUBICBSPLINE;	break;
	case ResizeFilter_CatmullRom:	irFilter = STBIR_FILTER_CATMULLROM;		break;
	case ResizeFilter_Mitchell:		irFilter = STBIR_FILTER_MITCHELL;		break;

	default: AM_UNREACHABLE("ResizeImagePixels() -> Filter %s is not implemented.", Enum::GetName(filter));
	}

	stbir_pixel_layout irPixelLayout;
	switch (fmt)
	{
	case ImagePixelFormat_U32:	irPixelLayout = hasAlphaPixels ? STBIR_RGBA : STBIR_4CHANNEL;	break;
	case ImagePixelFormat_U24:	irPixelLayout = STBIR_RGB;										break;
	case ImagePixelFormat_U16:	irPixelLayout = hasAlphaPixels ? STBIR_RA : STBIR_2CHANNEL;		break;
	case ImagePixelFormat_U8:	irPixelLayout = STBIR_1CHANNEL;									break;

	default: AM_UNREACHABLE("ResizeImagePixels() -> Pixel format '%s' is not supported.", Enum::GetName(fmt));
	}

	stbir_resize(src, xFrom, yFrom, 0, dst, xTo, yTo, 0, irPixelLayout, STBIR_TYPE_UINT8, STBIR_EDGE_CLAMP, irFilter);
}

void rageam::graphics::ConvertImagePixels(pVoid dst, pVoid src, ImagePixelFormat fromFmt, ImagePixelFormat toFmt, int width, int height)
{
	AM_ASSERT(!IsCompressedPixelFormat(fromFmt) && !IsCompressedPixelFormat(toFmt), "ConvertImagePixels() -> BC Formats are not supported!");

	PixelData srcData = static_cast<PixelData>(src);
	PixelData dstData = static_cast<PixelData>(dst);

	int totalPixels = width * height;

	char* srcXmm = srcData->Bytes;
	char* dstXmm = dstData->Bytes;

	int remainderPixels = totalPixels;
	int pixelsXmm = 0; // Aligned to 8

	// RGBA -> RGB
	if (fromFmt == ImagePixelFormat_U32 && toFmt == ImagePixelFormat_U24)
	{
#ifdef AM_IMAGE_USE_AVX2_RESIZE
		if (totalPixels > 8) // We can't process less than 8 pixels with xmm
		{
			remainderPixels = totalPixels % 8;
			pixelsXmm = totalPixels - remainderPixels;

			// We convert every 8 RGBA pixels (256 bytes) to 8 RGB pixels (192 bytes)
			// xmm store function will still write all 256 bytes with 64 byte padding at the end
			if (remainderPixels < 2)
			{
				pixelsXmm -= 8;
				remainderPixels += 8;
			}

			// Load 8 pixels at the time
			__m256i rgbaToRgbShuffle = _mm256_set_epi8(
				// Remainder from 8 alpha channels
				0, 0, 0, 0,
				0, 0, 0, 0,

				30, 29, 28,
				26, 25, 24,
				22, 21, 20,
				18, 17, 16,
				14, 13, 12,
				10, 9, 8,
				6, 5, 4,
				2, 1, 0
			);

			int xmmGroupCount = pixelsXmm / 8;
			for (int i = 0; i < xmmGroupCount; i++)
			{
				__m256i eightPixelsRGBA = _mm256_loadu_si256(reinterpret_cast<__m256i*>(srcXmm));
				__m256i eightPixelsRGB = _mm256_shuffle_epi8(eightPixelsRGBA, rgbaToRgbShuffle);

				_mm256_storeu_si256(reinterpret_cast<__m256i*>(dstXmm), eightPixelsRGB);

				srcXmm += 32; // sizeof(RGBA) * 8
				dstXmm += 24; // sizeof(RGB) * 8
			}
		}
#endif

		// And then remainders without intrinsics
		for (int i = 0; i < remainderPixels; i++)
		{
			int k = pixelsXmm + i;
			dstData->U24[k][0] = srcData->U32[k][0];	// R
			dstData->U24[k][1] = srcData->U32[k][1];	// G
			dstData->U24[k][2] = srcData->U32[k][2];	// B
		}

		return;
	}

	// RGB - RGBA
	if (fromFmt == ImagePixelFormat_U24 && toFmt == ImagePixelFormat_U32)
	{
#ifdef AM_IMAGE_USE_AVX2_RESIZE
		// We load 8 RGB pixels and shuffle them with white alpha to RGBA
		if (totalPixels > 8)
		{
			remainderPixels = totalPixels % 8;
			pixelsXmm = totalPixels - remainderPixels;

			// We use OR operand to set RGBA alpha to 255
			__m256i rgbaAlphaMask = _mm256_set_epi8(
				-1, 0, 0, 0,
				-1, 0, 0, 0,
				-1, 0, 0, 0,
				-1, 0, 0, 0,
				-1, 0, 0, 0,
				-1, 0, 0, 0,
				-1, 0, 0, 0,
				-1, 0, 0, 0
			);

			__m256i rgbToRgbaShuffle = _mm256_set_epi8(
				// Alpha value is doesn't matter here
				0, 23, 22, 21,
				0, 20, 19, 18,
				0, 17, 16, 15,
				0, 14, 13, 12,
				0, 11, 10, 9,
				0, 8, 7, 6,
				0, 5, 4, 3,
				0, 2, 1, 0
			);

			int xmmGroupCount = pixelsXmm / 8;
			for (int i = 0; i < xmmGroupCount; i++)
			{
				__m256i eightPixelsRGB = _mm256_loadu_si256(reinterpret_cast<__m256i*>(srcXmm));
				__m256i eightPixelsRGBA = _mm256_shuffle_epi8(eightPixelsRGB, rgbToRgbaShuffle);
				eightPixelsRGBA = _mm256_or_epi32(eightPixelsRGBA, rgbaAlphaMask);

				_mm256_storeu_si256(reinterpret_cast<__m256i*>(dstXmm), eightPixelsRGBA);

				srcXmm += 24; // sizeof(RGB) * 8
				dstXmm += 32; // sizeof(RGBA) * 8
			}
		}

#endif
		// And then remainders without intrinsics
		for (int i = 0; i < remainderPixels; i++)
		{
			int k = pixelsXmm + i;
			dstData->U32[k][0] = srcData->U24[k][0];	// R
			dstData->U32[k][1] = srcData->U24[k][1];	// G
			dstData->U32[k][2] = srcData->U24[k][2];	// B
			dstData->U32[k][3] = 255;					// A
		}

		return;
	}

	AM_UNREACHABLE("ConvertImagePixels() -> Conversion from '%s' to '%s' is not implemented.",
		Enum::GetName(fromFmt), Enum::GetName(toFmt));
}

void rageam::graphics::PixelDataOwner::AddRef() const
{
	if (!m_RefCount) return;
	int& refCount = *m_RefCount;
	++refCount;
}

int rageam::graphics::PixelDataOwner::DecRef() const
{
	if (!m_RefCount) return 0;
	int& refCount = *m_RefCount;
	--refCount;
	return refCount;
}

rageam::graphics::PixelDataOwner::PixelDataOwner()
{
	m_RefCount = nullptr;
	m_Data = nullptr;
	m_Owned = false;
}

rageam::graphics::PixelDataOwner::PixelDataOwner(const PixelDataOwner& other)
{
	m_Data = other.m_Data;
	m_Owned = other.m_Owned;
	m_RefCount = other.m_RefCount;
	DeleteFn = other.DeleteFn;
	AddRef();
}

rageam::graphics::PixelDataOwner::PixelDataOwner(PixelDataOwner&& other) noexcept : PixelDataOwner()
{
	std::swap(m_Data, other.m_Data);
	std::swap(m_Owned, other.m_Owned);
	std::swap(m_RefCount, other.m_RefCount);
	std::swap(DeleteFn, other.DeleteFn);
}

void rageam::graphics::PixelDataOwner::Release()
{
	if (!m_Owned)
	{
		m_Data = nullptr;
		return;
	}

	// Ref counted, we delete pixel data when no object references it anymore
	if (DecRef() == 0)
	{
		if (DeleteFn)	DeleteFn(m_Data);
		else			ImageFree(m_Data);

		m_Data = nullptr;
		delete m_RefCount;
		m_RefCount = nullptr;
	}
}

rageam::graphics::PixelDataOwner rageam::graphics::PixelDataOwner::CreateUnowned(pVoid data)
{
	PixelDataOwner owner;
	owner.m_Data = static_cast<char*>(data);
	owner.m_Owned = false;
	owner.m_RefCount = nullptr; // Not ref counted
	return owner;
}

rageam::graphics::PixelDataOwner rageam::graphics::PixelDataOwner::CreateOwned(pVoid data)
{
	PixelDataOwner owner;
	owner.m_Data = static_cast<char*>(data);
	owner.m_Owned = true;
	owner.m_RefCount = new int();
	owner.AddRef();
	return owner;
}

rageam::graphics::PixelDataOwner rageam::graphics::PixelDataOwner::AllocateWithSize(u32 size)
{
	pVoid block = ImageAlloc(size);
	return CreateOwned(block);
}

rageam::graphics::PixelDataOwner rageam::graphics::PixelDataOwner::AllocateForImage(int width, int height, ImagePixelFormat fmt, int mipCount)
{
	u32 totalSize = ComputeImageSizeWithMips(width, height, mipCount, fmt);
	return AllocateWithSize(totalSize);
}

rageam::graphics::PixelDataOwner& rageam::graphics::PixelDataOwner::operator=(const PixelDataOwner& other)
{
	Release();
	m_Data = other.m_Data;
	m_Owned = other.m_Owned;
	m_RefCount = other.m_RefCount;
	DeleteFn = other.DeleteFn;
	AddRef();
	return *this;
}

rageam::graphics::PixelDataOwner& rageam::graphics::PixelDataOwner::operator=(PixelDataOwner&& other) noexcept
{
	std::swap(m_Data, other.m_Data);
	std::swap(m_Owned, other.m_Owned);
	std::swap(m_RefCount, other.m_RefCount);
	std::swap(DeleteFn, other.DeleteFn);
	return *this;
}

void rageam::graphics::IImage::PostLoadCompute()
{
	ImageInfo info = GetInfo();
	m_Slice = ComputeImageSlicePitch(info.Width, info.Height, info.PixelFormat);
	m_Stride = ComputeImageRowPitch(info.Width, info.PixelFormat);
	m_SizeWithMips = ComputeImageSizeWithMips(info.Width, info.Height, info.MipCount, info.PixelFormat);
}

void rageam::graphics::IImage::ComputeHasAlpha(const bool* forceHasAlpha)
{
	if (forceHasAlpha)
	{
		m_HasAlpha = *forceHasAlpha;
		return;
	}

	// We scan only first mip map because others are derived from it,
	// if first mip map has no alpha then no other mip map will
	ImageInfo info = GetInfo();
	PixelData pixelData = GetPixelData().Data();
	m_HasAlpha = ScanPixelsForAlpha(pixelData, info.Width, info.Height, info.PixelFormat);
}

rageam::graphics::PixelDataOwner rageam::graphics::IImage::CreatePixelDataWithMips(bool generateMips, ResizeFilter mipFilter, int& outMipCount) const
{
	ImageInfo info = GetInfo();

	AM_ASSERT(!IsCompressedPixelFormat(info.PixelFormat),
		"IImage::CreatePixelDataWithMips() -> Format is compressed, this should never happen!");

	outMipCount = 1;

	// No mip maps needed, just return original pixel data
	if (!generateMips)
		return GetPixelData();

	if (!IsResolutionValidForMipMaps(info.Width, info.Height))
	{
		AM_WARNINGF(
			L"IImage::CreatePixelDataWithMips() -> Image '%ls' resolution is not power of two, mip maps will not be generated.",
			GetDebugName());

		// Mips cannot be generated, return original pixel data
		return GetPixelData();
	}

	int maxMipMaps = ComputeMaximumMipCountForResolution(info.Width, info.Height);

	PixelDataOwner bufferData = PixelDataOwner::AllocateForImage(info.Width, info.Height, info.PixelFormat, maxMipMaps);

	char* buffer = bufferData.Data()->Bytes;
	char* sourcePixelData = GetPixelData().Data()->Bytes;

	// Mip maps must be packed continuously
	u32 pixelDataOffset = 0;
	for (int i = 0; i < maxMipMaps; i++)
	{
		// Compute size of current mip map pixel data in bytes
		int mipWidth = info.Width >> i;
		int mipHeight = info.Height >> i;
		u32 slicePitch = ComputeImageSlicePitch(mipWidth, mipHeight, info.PixelFormat);

		char* mipPixelData = buffer + pixelDataOffset;

		// First mip map is original image
		if (i == 0)
		{
			memcpy(mipPixelData, sourcePixelData, slicePitch);
		}
		else
		{
			ResizeImagePixels(mipPixelData, sourcePixelData,
				mipFilter, info.PixelFormat, info.Width, info.Height, mipWidth, mipHeight, HasAlpha());
		}

		// Shift pointer to next mip map pixel data
		pixelDataOffset += slicePitch;
	}

	outMipCount = maxMipMaps;

	return bufferData;
}

bool rageam::graphics::IImage::CanBeCompressed() const
{
	ImageInfo info = GetInfo();
	return IsResolutionValidForMipMapsAndCompression(info.Width, info.Height);
}

bool rageam::graphics::IImage::CanGenerateMips() const
{
	ImageInfo info = GetInfo();
	return IsResolutionValidForMipMaps(info.Width, info.Height);
}

amPtr<rageam::graphics::IImage> rageam::graphics::IImage::Resize(int newWidth, int newHeight, ResizeFilter resizeFilter) const
{
	ImageInfo info = GetInfo();

	PixelDataOwner resizedData = PixelDataOwner::AllocateForImage(newWidth, newHeight, info.PixelFormat);

	char* dst = resizedData.Data()->Bytes;
	char* src = GetPixelData().Data()->Bytes;

	ResizeImagePixels(dst, src, resizeFilter, info.PixelFormat, info.Width, info.Height, newWidth, newHeight, HasAlpha());

	return ImageFactory::CreateFromMemory(resizedData, info.PixelFormat, newWidth, newHeight, false);
}

amPtr<rageam::graphics::IImage> rageam::graphics::IImage::PadToPowerOfTwo(Vec2S& outUvExtent) const
{
	ImageInfo info = GetInfo();

	AM_ASSERT(!IsCompressedPixelFormat(info.PixelFormat), "Image::PadToPowerOfTwo() -> Compressed pixel formats are not supported.");

	// Create new canvas with size aligned to next power of two, if dimension(s) are not aligned already
	int newWidth = ALIGN_POWER_OF_TWO_32(info.Width);
	int newHeight = ALIGN_POWER_OF_TWO_32(info.Height);
	u32 newSize = ComputeImageSlicePitch(newWidth, newHeight, info.PixelFormat);
	u32 newStride = ComputeImageRowPitch(newWidth, info.PixelFormat);
	u32 oldStride = GetStride();

	PixelDataOwner bufferOwner = PixelDataOwner::AllocateWithSize(newSize);

	char* buffer = bufferOwner.Data()->Bytes;
	char* sourcePixels = GetPixelData().Data()->Bytes;

	// We fill everything with white pixels
	memset(buffer, 0xFF, newSize);

	// Then simply overlay initial image on top of extended canvas
	// Case when we have to pad X is a little bit more complex because we'll have to leave empty are at the end of each line
	// In case of padding only Y we can just use mem copy, because padded area is placed right after
	bool needToPadX = !IS_POWER_OF_TWO(info.Width);
	if (needToPadX)
	{
		// For every row...
		for (int y = 0; y < info.Height; y++)
		{
			// We copy whole source line and leave padding at the end
			char* destRow = buffer + static_cast<size_t>(newStride * y);
			char* srcRow = sourcePixels + static_cast<size_t>(oldStride * y);
			memcpy(destRow, srcRow, oldStride);
		}
	}
	else
	{
		memcpy(buffer, sourcePixels, GetSlicePitch());
	}

	outUvExtent.X = static_cast<float>(info.Width) / static_cast<float>(newWidth);
	outUvExtent.Y = static_cast<float>(info.Height) / static_cast<float>(newHeight);

	return ImageFactory::CreateFromMemory(bufferOwner, info.PixelFormat, newWidth, newHeight);
}

bool rageam::graphics::IImage::CreateDX11Resource(
	const ShaderResourceOptions& options, amComPtr<ID3D11ShaderResourceView>& view, amComPtr<ID3D11Texture2D>* tex) const
{
	ImageInfo info = GetInfo();

	// Even though that DDS can be saved with non-power-of two resolution, DX11 doesn't accept such resolutions
	if (IsCompressedPixelFormat(info.PixelFormat) && !IsResolutionValidForMipMapsAndCompression(info.Width, info.Height))
	{
		AM_ERRF("IImage::CreateDXResource() -> Can't create DX11 texture with compressed pixel format '%s' and non power-of-two resolution (%ux%u).",
			Enum::GetName(info.PixelFormat), info.Width, info.Height);
		return false;
	}

	// U24 don't have matching DXGI format so we have to create temporary image with U32 (RGBA) format
	if (info.PixelFormat == ImagePixelFormat_U24)
	{
		PixelDataOwner rgbaPixels = PixelDataOwner::AllocateForImage(info.Width, info.Height, ImagePixelFormat_U32);
		ConvertImagePixels(rgbaPixels.Data(), GetPixelData().Data(), ImagePixelFormat_U24, ImagePixelFormat_U32, info.Width, info.Height);
		ImageMemory tempImage(rgbaPixels, ImagePixelFormat_U32, info.Width, info.Height, false);
		tempImage.SetDebugName(GetDebugName());
		return tempImage.CreateDX11Resource(options, view, tex);
	}

	// Special case for .Ico
	if (options.BaseMipIndex != -1)
	{
		AM_ASSERT(options.BaseMipIndex < info.MipCount, "IImage::CreateDX11Resource() -> Given mip index is out of bounds!");
		PixelDataOwner mipPixels = GetPixelData(options.BaseMipIndex);
		ImageMemory tempImage(mipPixels, info.PixelFormat, info.Width, info.Height);
		tempImage.SetDebugName(GetDebugName());
		return tempImage.CreateDX11Resource(options, view, tex);
	}

	int mipCount;
	PixelDataOwner texPixelData = CreatePixelDataWithMips(options.CreateMips, options.MipFilter, mipCount);

	HRESULT code;
	ID3D11Device* device = render::GetDevice();

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Format = ImagePixelFormatToDXGI(info.PixelFormat);
	texDesc.ArraySize = 1;
	texDesc.MipLevels = mipCount;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.Width = info.Width;
	texDesc.Height = info.Height;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;

	// Fill pixel data
	D3D11_SUBRESOURCE_DATA texData[MAX_MIP_MAPS] = {};
	// Mip maps are packed continuously, we have to add slice size to offset to get to next mip
	u32 pixelDataOffset = 0;
	for (int i = 0; i < mipCount; i++)
	{
		// Compute size of current mip map pixel data in bytes
		int mipWidth = info.Width >> i;
		int mipHeight = info.Height >> i;
		u32 slicePitch = ComputeImageSlicePitch(mipWidth, mipHeight, info.PixelFormat);
		u32 stride = ComputeImageRowPitch(mipWidth, info.PixelFormat);;

		texData[i].pSysMem = texPixelData.Data()->Bytes + pixelDataOffset;
		texData[i].SysMemPitch = stride;
		texData[i].SysMemSlicePitch = slicePitch;

		// Shift pointer to next mip map pixel data
		pixelDataOffset += slicePitch;
	}

	ID3D11Texture2D* pTex;
	code = device->CreateTexture2D(&texDesc, texData, &pTex);
	if (code != S_OK)
	{
		AM_ERRF(L"IImage::CreateDX11Resource() -> Failed to create Texture2D for '%ls', error code: %u", GetDebugName(), code);
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = texDesc.MipLevels;
	viewDesc.Texture2D.MostDetailedMip = 0;

	ID3D11ShaderResourceView* pView;
	code = device->CreateShaderResourceView(pTex, &viewDesc, &pView);

	// Texture is not needed anymore and out param is NULL, we can release it now
	if (!tex) pTex->Release();

	if (code != S_OK)
	{
		AM_ERRF(L"IImage::CreateDX11Resource() -> Failed to create ShaderResourceView for '%ls', error code: %u", GetDebugName(), code);
		return false;
	}

	if (tex) *tex = pTex;
	view = pView;

	return true;
}

rageam::graphics::ImageMemory::ImageMemory(const PixelDataOwner& pixelData, ImagePixelFormat fmt, int width, int height, bool copyPixels)
{
	m_Width = width;
	m_Height = height;
	m_PixelFormat = fmt;

	PostLoadCompute();

	if (copyPixels)
	{
		u32 sliceWidth = GetSlicePitch();
		m_PixelData = PixelDataOwner::AllocateWithSize(sliceWidth);
		memcpy(m_PixelData.Data(), pixelData.Data(), sliceWidth);
	}
	else
	{
		m_PixelData = pixelData;
	}

	ComputeHasAlpha();
}

rageam::graphics::ImageInfo rageam::graphics::ImageMemory::GetInfo() const
{
	ImageInfo info = {};
	info.Width = m_Width;
	info.Height = m_Height;
	info.PixelFormat = m_PixelFormat;
	return info;
}

rageam::graphics::PixelDataOwner rageam::graphics::ImageMemory::GetPixelData(int mipIndex) const
{
	return PixelDataOwner::CreateUnowned(m_PixelData.Data());
}

rageam::graphics::Image_Stb::~Image_Stb()
{
	stbi_image_free(m_PixelData);
	m_PixelData = nullptr;
}

bool rageam::graphics::Image_Stb::LoadPixelData(ConstWString path)
{
	FILE* fs = file::OpenFileStream(path, L"rb");
	if (!fs)
	{
		AM_ERRF(L"Image_Stb::LoadPixelData() -> Can't open image file '%ls'", path);
		return false;
	}

	m_PixelData = stbi_load_from_file(fs, &m_Width, &m_Height, &m_NumChannels, 0);
	if (!m_PixelData)
	{
		ConstString error = stbi_failure_reason();
		AM_ERRF("Image_Stb::LoadPixelData() -> Failed with error: '%s'", error);
	}

	file::CloseFileStream(fs);

	PostLoadCompute();
	ComputeHasAlpha();

	// Pixel data will be null if image is corrupted / failed to read
	return m_PixelData != nullptr;
}

bool rageam::graphics::Image_Stb::LoadMetaData(ConstWString path)
{
	FILE* fs = file::OpenFileStream(path, L"rb");
	if (!fs)
	{
		AM_ERRF(L"Image_Stb::LoadMetaData() -> Can't open image file '%ls'", path);
		return false;
	}

	stbi_info_from_file(fs, &m_Width, &m_Height, &m_NumChannels);
	file::CloseFileStream(fs);

	PostLoadCompute();

	return true;
}

rageam::graphics::ImageInfo rageam::graphics::Image_Stb::GetInfo() const
{
	ImageInfo info = {};

	info.Width = m_Width;
	info.Height = m_Height;

	switch (m_NumChannels)
	{
	case 1: info.PixelFormat = ImagePixelFormat_U8;		break;
	case 2: info.PixelFormat = ImagePixelFormat_U16;	break;
	case 3: info.PixelFormat = ImagePixelFormat_U24;	break;
	case 4: info.PixelFormat = ImagePixelFormat_U32;	break;
	}

	return info;
}

rageam::graphics::PixelDataOwner rageam::graphics::Image_Stb::GetPixelData(int mipIndex) const
{
	return PixelDataOwner::CreateUnowned(m_PixelData);
}

bool rageam::graphics::Image_Webp::LoadMetaData(ConstWString path)
{
	char header[32] = {};

	FILE* fs = file::OpenFileStream(path, L"rb");
	if (!fs)
	{
		AM_ERRF("Image_Webp::LoadMetaData() -> Failed to open file");
		return false;
	}

	file::ReadFileSteam(header, sizeof header, sizeof header, fs);
	file::CloseFileStream(fs);

	u8* headerData = reinterpret_cast<u8*>(header);
	if (WebPGetInfo(headerData, sizeof header, &m_Width, &m_Height) == 0)
	{
		AM_ERRF("Image_Webp::LoadMetaData() -> File is corrupted");
		return false;
	}

	return true;
}

bool rageam::graphics::Image_Webp::LoadPixelData(ConstWString path)
{
	file::FileBytes blob;
	if (!ReadAllBytes(path, blob))
	{
		AM_ERRF("Image_Webp::LoadPixelData() -> Failed to read file");
		return false;
	}

	u8* blobData = reinterpret_cast<u8*>(blob.Data.get());
	if (WebPGetInfo(blobData, blob.Size, &m_Width, &m_Height) == 0)
	{
		AM_ERRF("Image_Webp::LoadPixelData() -> File is corrupted");
		return false;
	}

	u32 slicePitch = ComputeImageSlicePitch(m_Width, m_Height, WEBP_PIXEL_FORMAT);
	u32 stride = ComputeImageRowPitch(m_Width, WEBP_PIXEL_FORMAT);

	PixelDataOwner bufferOwner = PixelDataOwner::AllocateWithSize(slicePitch);
	char* buffer = bufferOwner.Data()->Bytes;

	if (!WebPDecodeRGBAInto(blobData, blob.Size, reinterpret_cast<u8*>(buffer), slicePitch, static_cast<int>(stride)))
	{
		delete[] buffer;
		AM_ERRF("Image_Webp::LoadPixelData() -> Failed to decode pixel data");
		return false;
	}

	m_PixelData = std::move(bufferOwner);

	PostLoadCompute();
	ComputeHasAlpha();

	return true;
}

rageam::graphics::ImageInfo rageam::graphics::Image_Webp::GetInfo() const
{
	ImageInfo info = {};
	info.PixelFormat = WEBP_PIXEL_FORMAT;
	info.Width = m_Width;
	info.Height = m_Height;
	return info;
}

void rageam::graphics::Image_DDS::SetFromMetadata(const DirectX::TexMetadata& meta)
{
	m_Width = static_cast<int>(meta.width);
	m_Height = static_cast<int>(meta.height);
	m_MipCount = static_cast<int>(meta.mipLevels);
	m_PixelFormat = ImagePixelFormatFromDXGI(meta.format);
}

rageam::graphics::Image_DDS::Image_DDS(
	const PixelDataOwner& pixelData, ImagePixelFormat fmt, int width, int height, int mipCount, bool copyPixels)
{
	m_Width = width;
	m_Height = height;
	m_MipCount = mipCount;
	m_PixelFormat = fmt;

	PostLoadCompute();

	if (copyPixels)
	{
		u32 totalSize = GetSizeWithMips();
		m_PixelData = PixelDataOwner::AllocateWithSize(totalSize);
		memcpy(m_PixelData.Data(), pixelData.Data(), totalSize);
	}
	else
	{
		m_PixelData = pixelData;
	}

	ComputeHasAlpha();
}

bool rageam::graphics::Image_DDS::LoadMetaData(ConstWString path)
{
	DirectX::TexMetadata meta;
	HRESULT code = GetMetadataFromDDSFile(path, DirectX::DDS_FLAGS_NONE, meta);

	if (code != S_OK)
	{
		AM_ERRF("Image_DDS::LoadMetaData() -> Failed with code %u", code);
		return false;
	}

	SetFromMetadata(meta);

	return true;
}

bool rageam::graphics::Image_DDS::LoadPixelData(ConstWString path)
{
	DirectX::TexMetadata meta;
	HRESULT code = LoadFromDDSFile(path, DirectX::DDS_FLAGS_NONE, &meta, m_ScratchImage);

	if (code != S_OK)
	{
		AM_ERRF("Image_DDS::LoadPixelData() -> Failed with code %u", code);
		return false;
	}

	SetFromMetadata(meta);

	PostLoadCompute();
	// Alpha test is too expensive to compute on block-compressed pixels, we need it mostly only
	// on image resizing (to ignore alpha channel) so we can safely ignore it
	bool hasAlpha = false;
	ComputeHasAlpha(&hasAlpha);

	m_LoadedFromFile = true;

	return true;
}

rageam::graphics::ImageInfo rageam::graphics::Image_DDS::GetInfo() const
{
	ImageInfo info = {};
	info.Width = m_Width;
	info.Height = m_Height;
	info.PixelFormat = m_PixelFormat;
	info.MipCount = m_MipCount;
	return info;
}

rageam::graphics::PixelDataOwner rageam::graphics::Image_DDS::GetPixelData(int mipIndex) const
{
	PixelDataOwner pixelData;

	if (m_LoadedFromFile)
		pixelData = PixelDataOwner::CreateUnowned(m_ScratchImage.GetPixels());
	else
		pixelData = m_PixelData;

	// Skip mip maps pixel data
	if (mipIndex != 0)
	{
		u32 skipBytes = ComputeImageSizeWithMips(m_Width, m_Height, mipIndex, m_PixelFormat);
		char* mipData = pixelData.Data()->Bytes + skipBytes;
		return PixelDataOwner::CreateUnowned(mipData);
	}

	return pixelData;
}

rageam::graphics::PixelDataOwner rageam::graphics::Image_DDS::CreatePixelDataWithMips(bool generateMips, ResizeFilter mipFilter, int& outMipCount) const
{
	outMipCount = m_MipCount;
	return GetPixelData();
}

bool rageam::graphics::ImageFactory::VerifyImageSize(const ImagePtr& img)
{
	ImageInfo info = img->GetInfo();
	if (info.Width > MAX_IMAGE_RESOLUTION || info.Height > MAX_IMAGE_RESOLUTION)
	{
		AM_ERRF(
			L"ImageFactory::VerifyImageSize() -> Loaded image '%ls' has resolution greater than the maximum supported one! "
			"Image is not supported and most likely corrupted.", img->GetDebugName());
		return false;
	}
	return true;
}

rageam::graphics::ImagePtr rageam::graphics::ImageFactory::TryLoadFromPath(ConstWString path, bool onlyMeta)
{
	amPtr<IImageFile> image;

	ImageKind kind = ImageKind_None;

	ConstWString ext = file::GetExtension(path);
	switch (Hash(ext))
	{
		// Using goto here to assign type and perform instantiation at the same time
	case Hash("jpeg"):	kind = ImageKind_JPEG;	goto stb;
	case Hash("jpg"):	kind = ImageKind_JPG;	goto stb;
	case Hash("png"):	kind = ImageKind_PNG;	goto stb;
	case Hash("tga"):	kind = ImageKind_TGA;	goto stb;
	case Hash("bmp"):	kind = ImageKind_BMP;	goto stb;
	case Hash("psd"):	kind = ImageKind_PSD;	goto stb;
	stb:
		image = std::make_shared<Image_Stb>();
		break;

	case Hash("webp"):
		kind = ImageKind_WEBP;
		image = std::make_shared<Image_Webp>();
		break;

	case Hash("dds"):
		kind = ImageKind_DDS;
		image = std::make_shared<Image_DDS>();
		break;

	default:
		image = nullptr;
		AM_ERRF(L"ImageFactory::LoadFromPath() -> Image type '%ls' is not supported!", ext);
		break;
	}

	// Format is not supported
	if (!image)
		return nullptr;

	// Corrupted image file
	if (!onlyMeta && !image->LoadPixelData(path))
		return nullptr;

	// Corrupted image file
	if (onlyMeta && !image->LoadMetaData(path))
		return nullptr;

	image->m_Kind = kind;
	image->SetDebugName(file::GetFileName(path));

	if (!VerifyImageSize(image))
		return nullptr;

	return image;
}

rageam::graphics::ImageKind rageam::graphics::ImageFactory::GetImageKindFromPath(ConstWString path)
{
	if (String::IsNullOrEmpty(path))
		return ImageKind_None;

	ConstWString ext = file::GetExtension(path);

	// file::GetExtension will fail if only extension like 'png' is provided
	if (String::IsNullOrEmpty(ext))
		ext = path;

	HashValue extHash = Hash(ext);

	switch (extHash)
	{
	case Hash("webp"): return ImageKind_WEBP;
	case Hash("jpeg"): return ImageKind_JPEG;
	case Hash("jpg"): return ImageKind_JPG;
	case Hash("png"): return ImageKind_PNG;
	case Hash("tga"): return ImageKind_TGA;
	case Hash("bmp"): return ImageKind_BMP;
	case Hash("psd"): return ImageKind_PSD;
	case Hash("dds"): return ImageKind_DDS;
	case Hash("ico"): return ImageKind_ICO;

	default: return ImageKind_None;
	}
}

rageam::graphics::ImagePtr rageam::graphics::ImageFactory::LoadFromPath(ConstWString path, bool onlyMeta)
{
	ImagePtr image = TryLoadFromPath(path, onlyMeta);
	if (!image)
	{
		AM_ERRF(L"ImageFactory::LoadFromPath() -> An error occured during loading image from '%ls'", path);
		return nullptr;
	}
	return image;
}

rageam::graphics::ImagePtr rageam::graphics::ImageFactory::CreateFromMemory(
	const PixelDataOwner& pixelData, ImagePixelFormat fmt, int width, int height, bool copyPixels)
{
	return std::make_shared<ImageMemory>(pixelData, fmt, width, height, copyPixels);
}

bool rageam::graphics::ImageFactory::SaveImage(const ImagePtr& img, ConstWString path, ImageKind toKind, float quality)
{
	// Save kind is not specified, first attempt to retrieve it from path or fallback to original image kind
	if (toKind == ImageKind_None)
	{
		ImageKind pathKind = GetImageKindFromPath(path);
		if (pathKind != ImageKind_None)
			toKind = pathKind;
		else
			toKind = img->GetKind();
	}

	if (!(WriteableImageKinds & (1 << toKind)))
	{
		AM_ERRF("ImageFactory::SaveImage() -> Format %s is not supported for writing.",
			Enum::GetName(toKind));
		return false;
	}

	if (!img->HasPixelData())
	{
		AM_ERRF("ImageFactory::SaveImage() -> Image has no pixel data.");
		return false;
	}

	ImageInfo imgInfo = img->GetInfo();

	// Saving compressed format to uncompressed, need to decode
	if (IsCompressedPixelFormat(imgInfo.PixelFormat) && toKind != ImageKind_DDS)
	{
		AM_UNREACHABLE("BC Decoding is not implemented!"); // TODO:
	}

	int width = imgInfo.Width;
	int height = imgInfo.Height;
	char* pixels = img->GetPixelData().Data()->Bytes;

	// Replace extension in given path to actual extension of saving format
	file::WPath savePath = path;
	savePath = savePath.GetFilePathWithoutExtension();
	savePath += L".";
	savePath += ImageKindToFileExtensionW[toKind];

	// To prevent accidental use
	path = nullptr;

	switch (toKind)
	{
	case ImageKind_WEBP:
	{
		if (imgInfo.PixelFormat != ImagePixelFormat_U32 && imgInfo.PixelFormat != ImagePixelFormat_U24)
		{
			AM_ERRF("ImageFactory::SaveImage() -> Webp requires RGB or RGBA pixel format, format %s is not supported.",
				Enum::GetName(imgInfo.PixelFormat));
			return false;
		}

		bool alphaFormat = imgInfo.PixelFormat == ImagePixelFormat_U32;
		return WriteImageWebp(savePath, width, height, pixels, alphaFormat, quality * 100.0f);
	}

	case ImageKind_JPEG:
	case ImageKind_JPG:
	case ImageKind_PNG:
	case ImageKind_TGA:
	case ImageKind_BMP:
	{
		static int s_ChannelCounts[] = { 0, 4, 3, 2, 1 }; // For pixel formats from ImagePixelFormat_None to ImagePixelFormat_U8
		int channelCount = s_ChannelCounts[imgInfo.PixelFormat];
		return WriteImageStb(savePath, toKind, width, height, channelCount, pixels, static_cast<int>(quality * 100));
	}

	case ImageKind_DDS:
		AM_UNREACHABLE("DDS writing is not implemented");

	default: AM_UNREACHABLE("Format '%s' is not implemented for writing.", Enum::GetName(toKind));
	}
}
