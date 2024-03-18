#include "image.h"

#include "am/graphics/render.h"
#include "am/file/fileutils.h"
#include "am/file/pathutils.h"
#include "am/system/enum.h"
#include "helpers/dx11.h"
#include "imagecache.h"
#include "bc.h"

#include <webp/decode.h>
#include <webp/encode.h>
#include <stb_image_write.h>
#include <stb_image_resize2.h>
#include <stb_image.h>
#include <ddraw.h> // DDS
#include <lunasvg.h>
#include <easy/profiler.h>

pVoid rageam::graphics::ImageAlloc(u32 size)
{
	return malloc(size);
}

pVoid rageam::graphics::ImageAllocTemp(u32 size)
{
	return ImageAlloc(size);
}

pVoid rageam::graphics::ImageReAlloc(pVoid block, u32 newSize)
{
	return realloc(block, newSize);
}

pVoid rageam::graphics::ImageReAllocTemp(pVoid block, u32 newSize)
{
	return ImageReAlloc(block, newSize);
}

void rageam::graphics::ImageFree(pVoid block)
{
	free(block);
}

void rageam::graphics::ImageFreeTemp(pVoid block)
{
	ImageFree(block);
}

void rageam::graphics::ImageScaleResolution(int wIn, int hIn, int wTo, int hTo, int& wOut, int& hOut, ResolutionScalingMode mode)
{
	// Prevent division by zero
	if (wIn == 0) wIn = 1;
	if (hIn == 0) hIn = 1;

	int maxIn = std::max(wIn, hIn);
	int minIn = std::min(wIn, hIn);
	int minTo = std::min(wTo, hTo);
	int maxTo = std::max(wTo, hTo);

	float ratio;
	switch (mode)
	{
	case ResolutionScalingMode_Stretch:
		wOut = wTo;
		hOut = hTo;
		return;
	case ResolutionScalingMode_Fill:
		ratio = static_cast<float>(maxTo) / static_cast<float>(minIn);
		wOut = static_cast<int>(static_cast<float>(wIn) * ratio);
		hOut = static_cast<int>(static_cast<float>(hIn) * ratio);
		return;
	case ResolutionScalingMode_Fit:
		ratio = static_cast<float>(minTo) / static_cast<float>(maxIn);
		wOut = static_cast<int>(static_cast<float>(wIn) * ratio);
		hOut = static_cast<int>(static_cast<float>(hIn) * ratio);
		return;
	}
	AM_UNREACHABLE("ImageScaleResolution() -> Mode %i is not supported.", mode);
}

void rageam::graphics::ImageFitInRect(int wIn, int hIn, int rectSize, int& wOut, int& hOut)
{
	if (rectSize == 0)
	{
		wOut = wIn;
		hOut = hIn;
		return;
	}

	ImageScaleResolution(wIn, hIn, rectSize, rectSize, wOut, hOut, ResolutionScalingMode_Fit);
}

int rageam::graphics::ImageFindBestResolutionMatch(int imageCount, int width, int height,
	const std::function<void(int i, int& w, int& h)>& getter)
{
	if (imageCount == 0) return -1;
	if (imageCount == 1) return 0;

	int bestFitIndex = -1;
	int bestFitResolutionDelta = INT_MAX;
	for (int i = 0; i < imageCount; i++)
	{
		int iw, ih;
		getter(i, iw, ih);

		int delta = MIN(abs(iw - width), abs(ih - height));

		// Size of image must satisfy preferred and be larger than previous most large image,
		// otherwise picked image will depend on order in file
		if (delta < bestFitResolutionDelta)
		{
			bestFitIndex = i;
			bestFitResolutionDelta = delta;
		}
	}
	return bestFitIndex;
}

bool rageam::graphics::ImageIsCompressedFormat(ImagePixelFormat fmt)
{
	static constexpr char map[] =
	{
		0,					// None
		0, 0, 0, 0,	0,		// U32, U24, U16, U8, A8
		1, 1, 1, 1, 1, 1	// BC1, BC2, BC3, BC4, BC5, BC7
	};
	static_assert(Enum::GetCount<ImagePixelFormat>() == sizeof(map) / sizeof(char));
	return map[fmt];
}

bool rageam::graphics::ImageIsAlphaFormat(ImagePixelFormat fmt)
{
	static constexpr char map[] =
	{
		0,					// None
		1, 0, 1, 0,	1,		// U32, U24, U16, U8, A8
		1, 1, 1, 0, 0, 1	// BC1, BC2, BC3, BC4, BC5, BC7
	};
	static_assert(Enum::GetCount<ImagePixelFormat>() == sizeof(map) / sizeof(char));
	return map[fmt];
}

DXGI_FORMAT rageam::graphics::ImagePixelFormatToDXGI(ImagePixelFormat fmt)
{
	switch (fmt)
	{
	case ImagePixelFormat_U32:	return DXGI_FORMAT_R8G8B8A8_UNORM;
	case ImagePixelFormat_U16:	return DXGI_FORMAT_R8G8_UNORM;
	case ImagePixelFormat_U8:	return DXGI_FORMAT_R8_UNORM;
	case ImagePixelFormat_A8:	return DXGI_FORMAT_A8_UNORM;

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
	case DXGI_FORMAT_R8_UNORM:			return ImagePixelFormat_U8;
	case DXGI_FORMAT_A8_UNORM:			return ImagePixelFormat_A8;

	case DXGI_FORMAT_BC1_UNORM:			return ImagePixelFormat_BC1;
	case DXGI_FORMAT_BC2_UNORM:			return ImagePixelFormat_BC2;
	case DXGI_FORMAT_BC3_UNORM:			return ImagePixelFormat_BC3;
	case DXGI_FORMAT_BC4_UNORM:			return ImagePixelFormat_BC4;
	case DXGI_FORMAT_BC5_UNORM:			return ImagePixelFormat_BC5;
	case DXGI_FORMAT_BC7_UNORM:			return ImagePixelFormat_BC7;

	default:							return ImagePixelFormat_None;
	}
}

u32 rageam::graphics::ImageComputeSlicePitch(int width, int height, ImagePixelFormat pixelFormat)
{
	if (!ImageIsCompressedFormat(pixelFormat))
	{
		u32 bytesPerPixel = ImagePixelFormatBitsPerPixel[pixelFormat] / 8;
		return bytesPerPixel * width * height;
	}

	int totalNumBlocksX = width / 4;
	int totalNumBlocksY = height / 4;

	if (pixelFormat == ImagePixelFormat_BC1 || pixelFormat == ImagePixelFormat_BC4)
		return totalNumBlocksY * totalNumBlocksX * 8;

	// BC2, BC3, BC5, BC7
	return totalNumBlocksY * totalNumBlocksX * 16;
}

u32 rageam::graphics::ImageComputeRowPitch(int width, ImagePixelFormat pixelFormat)
{
	// For block-compressed formats row pitch is computed differently
	if (!ImageIsCompressedFormat(pixelFormat))
	{
		u32 bytesPerPixel = ImagePixelFormatBitsPerPixel[pixelFormat] / 8;
		return bytesPerPixel * width;
	}

	int totalNumBlocks = width / 4;

	if (pixelFormat == ImagePixelFormat_BC1 || pixelFormat == ImagePixelFormat_BC4)
		return totalNumBlocks * 8;

	// BC2, BC3, BC5, BC7
	return totalNumBlocks * 16;
}

u32 rageam::graphics::ImageComputeTotalSizeWithMips(int width, int height, int mipCount, ImagePixelFormat pixelFormat)
{
	u32 size = 0;
	for (int i = 0; i < mipCount; i++)
	{
		int w = width >> i;
		int h = height >> i;
		size += ImageComputeSlicePitch(w, h, pixelFormat);
	}
	return size;
}

bool rageam::graphics::ImageIsResolutionValidForMipMapsAndCompression(int width, int height)
{
	if (!ImageIsResolutionValidForMipMaps(width, height))
		return false;

	// Not enough pixels for block compression
	if (MIN(width, height) < 4)
		return false;

	return true;
}

bool rageam::graphics::ImageIsResolutionValidForMipMaps(int width, int height)
{
	return IS_POWER_OF_TWO(width) && IS_POWER_OF_TWO(height);
}

int rageam::graphics::ImageComputeMaxMipCount(int width, int height)
{
	if (!ImageIsResolutionValidForMipMapsAndCompression(width, height))
		return 1;

	// -1 to make 4x4 be minimum possible mip level
	return BitScanR32(MIN(width, height)) - 1;
}

bool rageam::graphics::ImageScanAlpha(pVoid data, int width, int height, ImagePixelFormat fmt)
{
	if (!ImageIsAlphaFormat(fmt))
		return false;

	if (ImageIsCompressedFormat(fmt))
		return false;

	ImagePixelData pixelData = static_cast<ImagePixelData>(data);

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

bool rageam::graphics::ImageWriteStb(ConstWString path, ImageFileKind kind, int w, int h, int c, pVoid data, int quality)
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

bool rageam::graphics::ImageWriteWebp(ConstWString path, int w, int h, pVoid data, bool hasAlpha, float quality)
{
	HANDLE hFile = file::CreateNew(path);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		AM_ERRF("WriteImageWebp() -> Failed to open file for writing.");
	}

	u8* blob = nullptr;
	size_t blobSize;

	u32 stride = ImageComputeRowPitch(w, hasAlpha ? ImagePixelFormat_U32 : ImagePixelFormat_U24);
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

// https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide
struct DDS_PIXELFORMAT
{
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwFourCC;
	DWORD dwRGBBitCount;
	DWORD dwRBitMask;
	DWORD dwGBitMask;
	DWORD dwBBitMask;
	DWORD dwABitMask;
};
struct DDS_HEADER
{
	DWORD           dwSize;
	DWORD           dwFlags;
	DWORD           dwHeight;
	DWORD           dwWidth;
	DWORD           dwPitchOrLinearSize;
	DWORD           dwDepth;
	DWORD           dwMipMapCount;
	DWORD           dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	DWORD           dwCaps;
	DWORD           dwCaps2;
	DWORD           dwCaps3;
	DWORD           dwCaps4;
	DWORD           dwReserved2;
};
struct DDS_HEADER_DXT10
{
	DXGI_FORMAT              dxgiFormat;
	D3D10_RESOURCE_DIMENSION resourceDimension;
	UINT                     miscFlag;
	UINT                     arraySize;
	UINT                     miscFlags2;
};

bool rageam::graphics::ImageWriteDDS(ConstWString path, int w, int h, int mips, ImagePixelFormat fmt, pVoid data)
{
	file::FSHandle fs = file::OpenFileStream(path, L"wb");
	if (!fs)
	{
		AM_ERRF("WriteImageDDS() -> Failed to open file for writing");
		return false;
	}

	if (!file::WriteFileSteam("DDS ", 4, fs.Get()))
	{
		AM_ERRF("WriteImageDDS() -> Failed to write magic number");
		return false;
	}

	DDS_HEADER header = {};
	header.dwSize = sizeof DDS_HEADER;
	header.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT | DDSD_LINEARSIZE;
	header.dwWidth = w;
	header.dwHeight = h;
	header.dwMipMapCount = mips;
	header.dwCaps = DDSCAPS_TEXTURE;
	header.dwPitchOrLinearSize = ImageComputeRowPitch(w, fmt);

	header.ddspf.dwSize = sizeof DDS_PIXELFORMAT;
	header.ddspf.dwFlags = DDPF_FOURCC;
	if (fmt == ImagePixelFormat_BC1 ||
		fmt == ImagePixelFormat_BC2 ||
		fmt == ImagePixelFormat_BC3 ||
		fmt == ImagePixelFormat_BC4 ||
		fmt == ImagePixelFormat_BC5)
	{
		header.ddspf.dwFourCC = ImagePixelFormatToFourCC[fmt];

		if (!file::WriteFileSteam(&header, sizeof DDS_HEADER, fs.Get()))
		{
			AM_ERRF("WriteImageDDS() -> Failed to write header");
			return false;
		}
	}
	else // We have to use extended header for formats that don't have four cc
	{
		header.ddspf.dwFourCC = FOURCC('D', 'X', '1', '0');

		DDS_HEADER_DXT10 header10 = {};
		header10.dxgiFormat = ImagePixelFormatToDXGI(fmt);
		header10.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
		header10.arraySize = 1;

		if (!file::WriteFileSteam(&header, sizeof DDS_HEADER, fs.Get()))
		{
			AM_ERRF("WriteImageDDS() -> Failed to write header");
			return false;
		}

		if (!file::WriteFileSteam(&header10, sizeof DDS_HEADER_DXT10, fs.Get()))
		{
			AM_ERRF("WriteImageDDS() -> Failed to write DX10 header");
			return false;
		}
	}

	u32 dataSize = ImageComputeTotalSizeWithMips(w, h, mips, fmt);
	if (!file::WriteFileSteam(data, dataSize, fs.Get()))
	{
		AM_ERRF("WriteImageDDS() -> Failed to write pixel data");
		return false;
	}

	return true;
}

bool rageam::graphics::ImageReadStb(ConstWString path, int& w, int& h, ImagePixelFormat& fmt, bool onlyMeta, PixelDataOwner* pixels)
{
	FILE* fs = file::OpenFileStream(path, L"rb");
	if (!fs)
	{
		AM_ERRF(L"ImageReadStb() -> Can't open image file '%ls'", path);
		return false;
	}

	int channelCount;
	bool success;

	if (onlyMeta)
	{
		success = stbi_info_from_file(fs, &w, &h, &channelCount) == 1;
	}
	else
	{
		u8* pixelData = stbi_load_from_file(fs, &w, &h, &channelCount, 0);
		if (!pixelData)
		{
			ConstString error = stbi_failure_reason();
			AM_ERRF("ImageReadStb() -> Failed with error: '%s'", error);
			success = false;
		}
		else
		{
			PixelDataOwner stbiPixelOwner = PixelDataOwner::CreateOwned(pixelData);
			stbiPixelOwner.DeleteFn = stbi_image_free;
			*pixels = std::move(stbiPixelOwner);
			success = true;
		}
	}

	file::CloseFileStream(fs);

	switch (channelCount)
	{
	case 1: fmt = ImagePixelFormat_U8;	break;
	case 2: fmt = ImagePixelFormat_U16;	break;
	case 3: fmt = ImagePixelFormat_U24;	break;
	case 4: fmt = ImagePixelFormat_U32;	break;
	}

	return success;
}

bool rageam::graphics::ImageReadWebp(ConstWString path, int& w, int& h, ImagePixelFormat& fmt, bool onlyMeta, PixelDataOwner* pixels)
{
	fmt = ImagePixelFormat_U32;

	if (onlyMeta)
	{
		FILE* fs = file::OpenFileStream(path, L"rb");
		if (!fs)
		{
			AM_ERRF("ImageReadWebp() -> Failed to open file");
			return false;
		}

		char header[32] = {};

		file::ReadFileSteam(header, sizeof header, sizeof header, fs);
		file::CloseFileStream(fs);

		u8* headerData = reinterpret_cast<u8*>(header);
		if (WebPGetInfo(headerData, sizeof header, &w, &h) == 0)
		{
			AM_ERRF("ImageReadWebp() -> File is corrupted");
			return false;
		}

		return true;
	}

	file::FileBytes blob;
	if (!ReadAllBytes(path, blob))
	{
		AM_ERRF("ImageReadWebp() -> Failed to read file");
		return false;
	}

	u8* blobData = reinterpret_cast<u8*>(blob.Data.get());
	if (WebPGetInfo(blobData, blob.Size, &w, &h) == 0)
	{
		AM_ERRF("ImageReadWebp() -> File is corrupted");
		return false;
	}

	u32 slicePitch = ImageComputeSlicePitch(w, h, ImagePixelFormat_U32);
	u32 rowPitch = ImageComputeRowPitch(w, ImagePixelFormat_U32);

	PixelDataOwner pixelDataOwner = PixelDataOwner::AllocateWithSize(slicePitch);
	char* pixelData = pixelDataOwner.Data()->Bytes;

	if (!WebPDecodeRGBAInto(blobData, blob.Size, reinterpret_cast<u8*>(pixelData), slicePitch, static_cast<int>(rowPitch)))
	{
		AM_ERRF("ImageReadWebp() -> Failed to decode pixel data");
		return false;
	}

	*pixels = pixelDataOwner;

	return true;
}

// https://github.com/microsoft/Windows-universal-samples/blob/main/Samples/Simple3DGameDX/cpp/Common/DDSTextureLoader.cpp
#define DDS_ISBITMASK(r, g, b, a) (ddpf.dwRBitMask == r && ddpf.dwGBitMask == g && ddpf.dwBBitMask == b && ddpf.dwABitMask == a)
static DXGI_FORMAT GetDDSDXGIFormat(const DDS_PIXELFORMAT& ddpf)
{
	if (ddpf.dwFlags & DDPF_RGB)
	{
		// Note that sRGB formats are written using the "DX10" extended header

		switch (ddpf.dwRGBBitCount)
		{
		case 32:
			if (DDS_ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
			{
				return DXGI_FORMAT_R8G8B8A8_UNORM;
			}

			if (DDS_ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
			{
				return DXGI_FORMAT_B8G8R8A8_UNORM;
			}

			if (DDS_ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
			{
				return DXGI_FORMAT_B8G8R8X8_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000) aka D3DFMT_X8B8G8R8

			// Note that many common DDS reader/writers (including D3DX) swap the
			// the RED/BLUE masks for 10:10:10:2 formats. We assumme
			// below that the 'backwards' header mask is being used since it is most
			// likely written by D3DX. The more robust solution is to use the 'DX10'
			// header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

			// For 'correct' writers, this should be 0x000003ff, 0x000ffc00, 0x3ff00000 for RGB data
			if (DDS_ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
			{
				return DXGI_FORMAT_R10G10B10A2_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000) aka D3DFMT_A2R10G10B10

			if (DDS_ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
			{
				return DXGI_FORMAT_R16G16_UNORM;
			}

			if (DDS_ISBITMASK(0xffffffff, 0x00000000, 0x00000000, 0x00000000))
			{
				// Only 32-bit color channel format in D3D9 was R32F
				return DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114
			}
			break;

		case 24:
			// No 24bpp DXGI formats aka D3DFMT_R8G8B8
			break;

		case 16:
			if (DDS_ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x8000))
			{
				return DXGI_FORMAT_B5G5R5A1_UNORM;
			}
			if (DDS_ISBITMASK(0xf800, 0x07e0, 0x001f, 0x0000))
			{
				return DXGI_FORMAT_B5G6R5_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x0000) aka D3DFMT_X1R5G5B5
			if (DDS_ISBITMASK(0x0f00, 0x00f0, 0x000f, 0xf000))
			{
				return DXGI_FORMAT_B4G4R4A4_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x0f00, 0x00f0, 0x000f, 0x0000) aka D3DFMT_X4R4G4B4

			// No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
			break;
		}
	}
	else if (ddpf.dwFlags & DDPF_LUMINANCE)
	{
		if (8 == ddpf.dwRGBBitCount)
		{
			if (DDS_ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x00000000))
			{
				return DXGI_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension
			}

			// No DXGI format maps to ISBITMASK(0x0f, 0x00, 0x00, 0xf0) aka D3DFMT_A4L4
		}

		if (16 == ddpf.dwRGBBitCount)
		{
			if (DDS_ISBITMASK(0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
			{
				return DXGI_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension
			}
			if (DDS_ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
			{
				return DXGI_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
			}
		}
	}
	else if (ddpf.dwFlags & DDPF_ALPHA)
	{
		if (8 == ddpf.dwRGBBitCount)
		{
			return DXGI_FORMAT_A8_UNORM;
		}
	}
	else if (ddpf.dwFlags & DDPF_FOURCC)
	{
		if (MAKEFOURCC('D', 'X', 'T', '1') == ddpf.dwFourCC)
		{
			return DXGI_FORMAT_BC1_UNORM;
		}
		if (MAKEFOURCC('D', 'X', 'T', '3') == ddpf.dwFourCC)
		{
			return DXGI_FORMAT_BC2_UNORM;
		}
		if (MAKEFOURCC('D', 'X', 'T', '5') == ddpf.dwFourCC)
		{
			return DXGI_FORMAT_BC3_UNORM;
		}

		// While pre-mulitplied alpha isn't directly supported by the DXGI formats,
		// they are basically the same as these BC formats so they can be mapped
		if (MAKEFOURCC('D', 'X', 'T', '2') == ddpf.dwFourCC)
		{
			return DXGI_FORMAT_BC2_UNORM;
		}
		if (MAKEFOURCC('D', 'X', 'T', '4') == ddpf.dwFourCC)
		{
			return DXGI_FORMAT_BC3_UNORM;
		}

		if (MAKEFOURCC('A', 'T', 'I', '1') == ddpf.dwFourCC)
		{
			return DXGI_FORMAT_BC4_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '4', 'U') == ddpf.dwFourCC)
		{
			return DXGI_FORMAT_BC4_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '4', 'S') == ddpf.dwFourCC)
		{
			return DXGI_FORMAT_BC4_SNORM;
		}

		if (MAKEFOURCC('A', 'T', 'I', '2') == ddpf.dwFourCC)
		{
			return DXGI_FORMAT_BC5_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '5', 'U') == ddpf.dwFourCC)
		{
			return DXGI_FORMAT_BC5_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '5', 'S') == ddpf.dwFourCC)
		{
			return DXGI_FORMAT_BC5_SNORM;
		}

		// BC6H and BC7 are written using the "DX10" extended header

		if (MAKEFOURCC('R', 'G', 'B', 'G') == ddpf.dwFourCC)
		{
			return DXGI_FORMAT_R8G8_B8G8_UNORM;
		}
		if (MAKEFOURCC('G', 'R', 'G', 'B') == ddpf.dwFourCC)
		{
			return DXGI_FORMAT_G8R8_G8B8_UNORM;
		}

		// Check for D3DFORMAT enums being set here
		switch (ddpf.dwFourCC)
		{
		case 36: // D3DFMT_A16B16G16R16
			return DXGI_FORMAT_R16G16B16A16_UNORM;

		case 110: // D3DFMT_Q16W16V16U16
			return DXGI_FORMAT_R16G16B16A16_SNORM;

		case 111: // D3DFMT_R16F
			return DXGI_FORMAT_R16_FLOAT;

		case 112: // D3DFMT_G16R16F
			return DXGI_FORMAT_R16G16_FLOAT;

		case 113: // D3DFMT_A16B16G16R16F
			return DXGI_FORMAT_R16G16B16A16_FLOAT;

		case 114: // D3DFMT_R32F
			return DXGI_FORMAT_R32_FLOAT;

		case 115: // D3DFMT_G32R32F
			return DXGI_FORMAT_R32G32_FLOAT;

		case 116: // D3DFMT_A32B32G32R32F
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		}
	}

	return DXGI_FORMAT_UNKNOWN;
}
#undef DDS_ISBITMASK

bool rageam::graphics::ImageReadDDS(ConstWString path, int& w, int& h, int& mips, ImagePixelFormat& fmt, bool onlyMeta, PixelDataOwner* pixels)
{
	w = 0;
	h = 0;
	mips = 0;
	fmt = ImagePixelFormat_None;

	file::FSHandle fs = file::OpenFileStream(path, L"rb");
	if (!fs)
	{
		AM_ERRF("ReadImageDDS() -> Failed to open image file.");
		return false;
	}

	int magic = 0;
	file::ReadFileSteam(&magic, 4, 4, fs.Get());

	if (magic != FOURCC('D', 'D', 'S', ' '))
	{
		AM_ERRF("ReadImageDDS() -> File header is not valid (magic: %u)", magic);
		return false;
	}

	DDS_HEADER header = {};
	if (!file::ReadFileSteam(&header, sizeof DDS_HEADER, sizeof DDS_HEADER, fs.Get()))
	{
		AM_ERRF("ReadImageDDS() -> Failed to read header.");
		return false;
	}

	if (header.dwSize != sizeof(DDS_HEADER))
	{
		AM_ERRF("ReadImageDDS() -> Header size mismatch, file is corrupted.");
		return false;
	}

	// Sanity check
	if (header.dwWidth > IMAGE_MAX_RESOLUTION || header.dwHeight > IMAGE_MAX_RESOLUTION)
	{
		AM_ERRF("ReadImageDDS() -> Resolution is too high (%ix%i), image is most likely corrupted.", header.dwWidth, header.dwHeight);
		return false;
	}

	w = static_cast<int>(header.dwWidth);
	h = static_cast<int>(header.dwHeight);
	mips = (header.dwFlags & DDSD_MIPMAPCOUNT) ? static_cast<int>(header.dwMipMapCount) : 1;

	DXGI_FORMAT dxgiFormat;

	// Extended header
	if ((header.ddspf.dwFlags & DDPF_FOURCC) && header.ddspf.dwFourCC == FOURCC('D', 'X', '1', '0'))
	{
		DDS_HEADER_DXT10 header10 = {};
		if (!file::ReadFileSteam(&header10, sizeof DDS_HEADER_DXT10, sizeof DDS_HEADER_DXT10, fs.Get()))
		{
			AM_ERRF("ReadImageDDS() -> Failed to read extended DX10 header.");
			return false;
		}

		if (header10.resourceDimension != D3D10_RESOURCE_DIMENSION_TEXTURE2D || header10.arraySize != 1)
		{
			AM_ERRF("ReadImageDDS() -> Given texture is not 2D and not supported.");
			return false;
		}

		dxgiFormat = header10.dxgiFormat;
	}
	else
	{
		dxgiFormat = GetDDSDXGIFormat(header.ddspf);
		if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
		{
			AM_ERRF("ReadImageDDS() -> Unable to determine pixel format.");
			return false;
		}
	}

	fmt = ImagePixelFormatFromDXGI(dxgiFormat);
	if (fmt == ImagePixelFormat_None)
	{
		AM_ERRF("ReadImageDDS() -> Can't convert DXGI format '%s'.", Enum::GetName(dxgiFormat));
		return false;
	}

	// No pixel data is required
	if (onlyMeta)
		return true;

	u32 totalSize = ImageComputeTotalSizeWithMips(w, h, mips, fmt);
	PixelDataOwner pixelDataOwner = PixelDataOwner::AllocateWithSize(totalSize);

	if (file::ReadFileSteam(pixelDataOwner.Data()->Bytes, totalSize, totalSize, fs.Get()) != totalSize)
	{
		AM_ERRF("ReadImageDDS() -> Failed to read pixel data, file is corrupted.");
		return false;
	}

	*pixels = pixelDataOwner;

	return true;
}

bool rageam::graphics::ImageRead(ConstWString path, int& w, int& h, int& mips, ImagePixelFormat& fmt, ImageFileKind* outKind, bool onlyMeta, PixelDataOwner* outPixels)
{
	EASY_FUNCTION();

	// Mips are used only on DDS
	mips = 1;

	ImageFileKind kind = ImageKind_None;
	bool success = false;

	ConstWString ext = file::GetExtension(path);
	switch (Hash(ext))
	{
		// Using goto here to assign type and perform loading at the same time
	case Hash("jpeg"):	kind = ImageKind_JPEG;	goto stb;
	case Hash("jpg"):	kind = ImageKind_JPG;	goto stb;
	case Hash("png"):	kind = ImageKind_PNG;	goto stb;
	case Hash("tga"):	kind = ImageKind_TGA;	goto stb;
	case Hash("bmp"):	kind = ImageKind_BMP;	goto stb;
	case Hash("psd"):	kind = ImageKind_PSD;	goto stb;
	stb:
		success = ImageReadStb(path, w, h, fmt, onlyMeta, outPixels);
		break;

	case Hash("webp"):
		kind = ImageKind_WEBP;
		success = ImageReadWebp(path, w, h, fmt, onlyMeta, outPixels);
		break;

	case Hash("dds"):
		kind = ImageKind_DDS;
		success = ImageReadDDS(path, w, h, mips, fmt, onlyMeta, outPixels);
		break;

	default:
		AM_ERRF(L"ImageFactory::LoadFromPath() -> Image type '%ls' is not supported!", ext);
		break;
	}

	if (outKind) *outKind = kind;

	return success;
}

bool rageam::graphics::ImageWrite(ConstWString path, int w, int h, int mips, ImagePixelFormat fmt, const PixelDataOwner& pixelData, ImageFileKind kind, float quality)
{
	EASY_FUNCTION();

	char* pixels = pixelData.Data()->Bytes;

	switch (kind)
	{
	case ImageKind_WEBP:
	{
		if (fmt != ImagePixelFormat_U32)
		{
			AM_ERRF("ImageWrite() -> Only RGBA is supported for WEBP");
			return false;
		}
		return ImageWriteWebp(path, w, h, pixels, true, quality * 100.0f);
	}

	case ImageKind_JPEG:
	case ImageKind_JPG:
	case ImageKind_PNG:
	case ImageKind_TGA:
	case ImageKind_BMP:
	{
		static int s_ChannelCounts[] = { 0, 4, 3, 2, 1 }; // For pixel formats from ImagePixelFormat_None to ImagePixelFormat_U8
		int channelCount = s_ChannelCounts[fmt];
		return ImageWriteStb(path, kind, w, h, channelCount, pixels, static_cast<int>(quality * 100));
	}

	case ImageKind_DDS:
		return ImageWriteDDS(path, w, h, mips, fmt, pixels);

	default: AM_UNREACHABLE("Format '%s' is not implemented for writing.", Enum::GetName(kind));
	}
}

void rageam::graphics::ImageResize(
	pVoid dst, pVoid src,
	ResizeFilter filter,
	ImagePixelFormat fmt,
	int xFrom, int yFrom,
	int xTo, int yTo,
	bool hasAlphaPixels)
{
	EASY_FUNCTION();

	AM_ASSERT(!ImageIsCompressedFormat(fmt), "ResizeImagePixels() -> Compressed formats are not supported.");

	// Input is identical to output size, just copy source pixels
	if (xFrom == xTo && yFrom == yTo)
	{
		u32 slicePitch = ImageComputeSlicePitch(xFrom, yFrom, fmt);
		memcpy(dst, src, slicePitch);
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
	case ResizeFilter_Point:		irFilter = STBIR_FILTER_POINT_SAMPLE;	break;

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

char* rageam::graphics::ImageGetPixel(char* pixelData, int x, int y, int width, ImagePixelFormat fmt)
{
	AM_ASSERT(!ImageIsCompressedFormat(fmt), "ImageGetPixel() -> Compressed format '%s' is not supported.", Enum::GetName(fmt));
	u32 pixelPitch = ImagePixelFormatBitsPerPixel[fmt] / 8;
	u32 rowPitch = width * pixelPitch;
	return pixelData + static_cast<size_t>(rowPitch * y + x * pixelPitch);
}

rageam::graphics::ColorU32 rageam::graphics::ImageGetPixelColor(char* pixelData, int x, int y, int width, ImagePixelFormat fmt)
{
	if (!ImageIsCompressedFormat(fmt))
	{
		u8* pixels = reinterpret_cast<u8*>(ImageGetPixel(pixelData, x, y, width, fmt));
		switch (fmt)
		{
		case ImagePixelFormat_U32:	return *reinterpret_cast<u32*>(pixels);
		case ImagePixelFormat_U24:	return ColorU32(pixels[0], pixels[1], pixels[2]);
		case ImagePixelFormat_U16:	return ColorU32(pixels[0], pixels[0], pixels[0], pixels[1]);
		case ImagePixelFormat_U8:	return ColorU32(pixels[0], pixels[0], pixels[0]);
		case ImagePixelFormat_A8:	return ColorU32(pixels[0], pixels[0], pixels[0], pixels[0]);

		default: return 0;
		}
	}

	// For compressed formats we have to decode whole 4x4 block and lookup our pixel from there
	int blockX = x / 4;
	int blockY = y / 4;
	int subX = x % 4;
	int subY = y % 4;

	int totalBlocksX = width / 4;

	u32 blockWidth = BlockFormatToBlockSize[ImagePixelFormatToBlockFormat(fmt)];
	u32 rowPitch = totalBlocksX * blockWidth;

	char* encodedBlock = pixelData + static_cast<size_t>(blockY * rowPitch + blockX * blockWidth);

	ColorU32 decodedPixels[16];
	ImageCompressor::DecompressBlock(encodedBlock, reinterpret_cast<char*>(decodedPixels), fmt);

	return decodedPixels[subY * 4 + subX];
}

void rageam::graphics::ImageConvertPixelFormat(pVoid dst, pVoid src, ImagePixelFormat fromFmt, ImagePixelFormat toFmt, int width, int height)
{
	EASY_FUNCTION();

	AM_ASSERT(!ImageIsCompressedFormat(fromFmt) && !ImageIsCompressedFormat(toFmt), "ConvertImagePixels() -> BC Formats are not supported!");

	ImagePixelData srcData = static_cast<ImagePixelData>(src);
	ImagePixelData dstData = static_cast<ImagePixelData>(dst);

	int totalPixels = width * height;

	char* srcXmm = srcData->Bytes;
	char* dstXmm = dstData->Bytes;

	int remainderPixels = totalPixels;
	int pixelsXmm = 0;

	// RGBA -> RGB
	if (fromFmt == ImagePixelFormat_U32 && toFmt == ImagePixelFormat_U24)
	{
#ifdef AM_IMAGE_USE_AVX2
		if (totalPixels > 8) // We can't process less than 8 pixels with xmm
		{
			remainderPixels = totalPixels % 8;
			pixelsXmm = totalPixels - remainderPixels;

			// We convert every 8 RGBA pixels (256 bits) to 8 RGB pixels (192 bits)
			// xmm store function will still write all 256 bits with 64 bit padding at the end
			if (remainderPixels < 2)
			{
				pixelsXmm -= 8;
				remainderPixels += 8;
			}

			// Load 8 pixels at a time
			int xmmGroupCount = pixelsXmm / 8;
			for (int i = 0; i < xmmGroupCount; i++)
			{
				__m256i eightPixelsRGBA = _mm256_loadu_si256(reinterpret_cast<__m256i*>(srcXmm));
				__m256i eightPixelsRGB = _mm256_shuffle_epi8(eightPixelsRGBA, IMAGE_RGBA_TO_RGB_SHUFFLE_256);

				_mm256_storeu_si256(reinterpret_cast<__m256i*>(dstXmm), eightPixelsRGB);

				srcXmm += 32; // sizeof(RGBA) * 8 pixels
				dstXmm += 24; // sizeof(RGB) * 8 pixels
			}
		}
#elif defined AM_IMAGE_USE_SIMD
		if (totalPixels > 4) // We can't process less than 8 pixels with xmm
		{
			remainderPixels = totalPixels % 4;
			pixelsXmm = totalPixels - remainderPixels;

			// We convert every 4 RGBA pixels (128 bits) to 4 RGB pixels (96 bits)
			// xmm store function will still write all 128 bits with 32 bit padding at the end
			if (remainderPixels < 1)
			{
				pixelsXmm -= 4;
				remainderPixels += 4;
			}

			// Load 4 pixels at a time
			int xmmGroupCount = pixelsXmm / 4;
			for (int i = 0; i < xmmGroupCount; i++)
			{
				__m128i eightPixelsRGBA = _mm_loadu_si128(reinterpret_cast<__m128i*>(srcXmm));
				__m128i eightPixelsRGB = _mm_shuffle_epi8(eightPixelsRGBA, IMAGE_RGBA_TO_RGB_SHUFFLE);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(dstXmm), eightPixelsRGB);

				srcXmm += 16; // sizeof(RGBA) * 4 pixels
				dstXmm += 12; // sizeof(RGB) * 4 pixels
			}
		}
#endif

		// And then remainders without intrinsics (or all if intrinsics weren't enabled)
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
#ifdef AM_IMAGE_USE_SIMD
		// We load 4 RGB pixels and shuffle them with white alpha to RGBA
		if (totalPixels > 4)
		{
			remainderPixels = totalPixels % 4;
			pixelsXmm = totalPixels - remainderPixels;

			int xmmGroupCount = pixelsXmm / 4;
			for (int i = 0; i < xmmGroupCount; i++)
			{
				__m128i fourPixelsRGB = _mm_loadu_epi8(srcXmm);
				__m128i fourPixelsRGBA = _mm_shuffle_epi8(fourPixelsRGB, IMAGE_RGB_TO_RGBA_SHUFFLE);
				// Set RGBA alpha to 255
				fourPixelsRGBA = _mm_or_epi32(fourPixelsRGBA, IMAGE_RGBA_ALPHA_MASK);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(dstXmm), fourPixelsRGBA);

				srcXmm += 12; // sizeof(RGB) * 4 pixels
				dstXmm += 16; // sizeof(RGBA) * 4 pixels
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

	// Gray Alpha -> RGBA
	if (fromFmt == ImagePixelFormat_U16 && toFmt == ImagePixelFormat_U32)
	{
		for (int i = 0; i < remainderPixels; i++)
		{
			int k = pixelsXmm + i;
			dstData->U32[k][0] = srcData->U16[k][0];	// R
			dstData->U32[k][1] = srcData->U16[k][0];	// G
			dstData->U32[k][2] = srcData->U16[k][0];	// B
			dstData->U32[k][3] = srcData->U16[k][1];	// A
		}
		return;
	}

	// Gray -> RGB
	if (fromFmt == ImagePixelFormat_U8 && toFmt == ImagePixelFormat_U24)
	{
		for (int i = 0; i < remainderPixels; i++)
		{
			int k = pixelsXmm + i;
			dstData->U24[k][0] = srcData->U8[k][0];	// R
			dstData->U24[k][1] = srcData->U8[k][0];	// G
			dstData->U24[k][2] = srcData->U8[k][0];	// B
		}
		return;
	}

	// Gray -> RGBA
	if (fromFmt == ImagePixelFormat_U8 && toFmt == ImagePixelFormat_U32)
	{
		for (int i = 0; i < remainderPixels; i++)
		{
			int k = pixelsXmm + i;
			dstData->U32[k][0] = srcData->U8[k][0];	// R
			dstData->U32[k][1] = srcData->U8[k][0];	// G
			dstData->U32[k][2] = srcData->U8[k][0];	// B
			dstData->U32[k][3] = srcData->U8[k][0];	// A
		}
		return;
	}

	AM_UNREACHABLE("ConvertImagePixels() -> Conversion from '%s' to '%s' is not implemented.",
		Enum::GetName(fromFmt), Enum::GetName(toFmt));
}

void rageam::graphics::ImageFlipY(char* pixels, int width, int height, ImagePixelFormat fmt)
{
	EASY_FUNCTION();

	size_t rowPitch = ImageComputeRowPitch(width, fmt);
	pVoid scanlineBuffer = ImageAlloc(rowPitch);

	int halfHeight = height / 2;
	for (int y = 0; y < halfHeight; y++)
	{
		pVoid topLine = pixels + rowPitch * y;
		pVoid endLine = pixels + rowPitch * (height - 1 - y);
		// Swap lines using temporary buffer
		memcpy(scanlineBuffer, topLine, rowPitch);
		memcpy(topLine, endLine, rowPitch);
		memcpy(endLine, scanlineBuffer, rowPitch);
	}

	ImageFree(scanlineBuffer);
}

void rageam::graphics::ImageDoSwizzle(
	char* pixelData, int width, int height, ImageSwizzle r, ImageSwizzle g, ImageSwizzle b, ImageSwizzle a)
{
	EASY_FUNCTION();

	// No component will be reordered, we can safely skip
	if (r == ImageSwizzle_R &&
		g == ImageSwizzle_G &&
		b == ImageSwizzle_B &&
		a == ImageSwizzle_A)
	{
		return;
	}

	int totalPixels = width * height;
	int remainderPixels = totalPixels;
	int pixelsXmm = 0;

	ColorU32* pixels = reinterpret_cast<ColorU32*>(pixelData);

#ifdef AM_IMAGE_USE_SIMD
	if (totalPixels > 4)
	{
		remainderPixels = totalPixels % 4;
		pixelsXmm = totalPixels - remainderPixels;

		__m128i swizzle = _mm_set_epi8(
			char(a + 12), char(b + 12), char(g + 12), char(r + 12),
			char(a + 8), char(b + 8), char(g + 8), char(r + 8),
			char(a + 4), char(b + 4), char(g + 4), char(r + 4),
			char(a + 0), char(b + 0), char(g + 0), char(r + 0));

		int xmmGroupCount = pixelsXmm / 4;
		for (int i = 0; i < xmmGroupCount; i++)
		{
			__m128i fourPixels;
			fourPixels = _mm_loadu_epi8(pixels);
			fourPixels = _mm_shuffle_epi8(fourPixels, swizzle);
			_mm_storeu_si128(reinterpret_cast<__m128i*>(pixels), fourPixels);
			pixels += 4;
		}
	}

#endif
	// And then remainders without intrinsics
	for (int i = 0; i < remainderPixels; i++)
	{
		ColorU32 pixelCopy = *pixels;
		pixels->R = pixelCopy[r];
		pixels->G = pixelCopy[g];
		pixels->B = pixelCopy[b];
		pixels->A = pixelCopy[a];
		pixels++;
	}
}

void rageam::graphics::ImageAdjustBrightnessAndContrastRGBA(char* pixelData, int width, int height, int brightness, int contrast)
{
	EASY_FUNCTION();

	if (brightness == 0 && contrast == 0)
		return;

	// Contrast equation:
	// ContrastFactor = (259 * (Color + 255)) / (255 * (259 -c))
	// Color = ContrastFactor * (Color - 128) + 128

	bool  hasContrast = contrast != 0;
	bool  hasBrightness = brightness != 0;
	float contrastFactor = 259.0f * ((float)contrast + 255.0f) / (255.0f * (259.0f - (float)contrast));

	ColorU32* pixels = reinterpret_cast<ColorU32*>(pixelData);

	int totalPixels = width * height;
	int remainderPixels = totalPixels;
	int pixelsXmm = 0;

#ifdef AM_IMAGE_USE_SIMD
	if (totalPixels > 4)
	{
		remainderPixels = totalPixels % 4;
		pixelsXmm = totalPixels - remainderPixels;

		// Shuffle for unpacking every U32 pixel to 4 floats
		__m128i _bytesToFloatsShuffle[4] =
		{
			_mm_set_epi8(-128, -128, -128, 3,  /**/ -128, -128, -128, 2,  /**/ -128, -128, -128, 1,  /**/ -128, -128, -128, 0),
			_mm_set_epi8(-128, -128, -128, 7,  /**/ -128, -128, -128, 6,  /**/ -128, -128, -128, 5,  /**/ -128, -128, -128, 4),
			_mm_set_epi8(-128, -128, -128, 11, /**/ -128, -128, -128, 10, /**/ -128, -128, -128, 9,  /**/ -128, -128, -128, 8),
			_mm_set_epi8(-128, -128, -128, 15, /**/ -128, -128, -128, 14, /**/ -128, -128, -128, 13, /**/ -128, -128, -128, 12),
		};
		__m128i _floatsToBytesShuffle[4] =
		{
			_mm_set_epi8(-128, -128, -128, -128, /**/ -128, -128, -128, -128, /**/ -128, -128, -128, -128, /**/ 12, 8, 4, 0),
			_mm_set_epi8(-128, -128, -128, -128, /**/ -128, -128, -128, -128, /**/ 12, 8, 4, 0,            /**/ -128, -128, -128, -128),
			_mm_set_epi8(-128, -128, -128, -128, /**/ 12, 8, 4, 0,            /**/ -128, -128, -128, -128, /**/ -128, -128, -128, -128),
			_mm_set_epi8(12, 8, 4, 0,            /**/ -128, -128, -128, -128, /**/ -128, -128, -128, -128, /**/ -128, -128, -128, -128),
		};
		__m128i _keepPixelMasks[4] =
		{
			_mm_set_epi8(0, 0, 0, 0,     /**/ 0, 0, 0, 0,     /**/ 0, 0, 0, 0,     /**/ -1, -1, -1, -1),
			_mm_set_epi8(0, 0, 0, 0,     /**/ 0, 0, 0, 0,     /**/ -1, -1, -1, -1, /**/ 0, 0, 0, 0),
			_mm_set_epi8(0, 0, 0, 0,     /**/ -1, -1, -1, -1, /**/ 0, 0, 0, 0,     /**/ 0, 0, 0, 0),
			_mm_set_epi8(-1, -1, -1, -1, /**/ 0, 0, 0, 0,     /**/ 0, 0, 0, 0,     /**/ 0, 0, 0, 0),
		};
		__m128i _erasePixelMasks[4] = // We keep alpha here because contrast shouldn't change it
		{
			_mm_set_epi8(-1, -1, -1, -1, /**/ -1, -1, -1, -1, /**/ -1, -1, -1, -1, /**/ -1, 0, 0, 0),
			_mm_set_epi8(-1, -1, -1, -1, /**/ -1, -1, -1, -1, /**/ -1, 0, 0, 0,    /**/ -1, -1, -1, -1),
			_mm_set_epi8(-1, -1, -1, -1, /**/ -1, 0, 0, 0,    /**/ -1, -1, -1, -1, /**/ -1, -1, -1, -1),
			_mm_set_epi8(-1, 0, 0, 0,    /**/ -1, -1, -1, -1, /**/ -1, -1, -1, -1, /**/ -1, -1, -1, -1),
		};

		__m128 _128 = _mm_set_ps1(128.0f);
		__m128 _0 = _mm_set_ps1(0.0f);
		__m128 _255 = _mm_set_ps1(255.0f);
		__m128 _contrastFactor = _mm_set_ps1(contrastFactor);

		// We load absolute value because we can't do unsigned + signed saturated operation, instead we use branching
		__m128i _brightness = _mm_set1_epi8((char)abs(brightness));
		// Get rid of alpha component
		_brightness = _mm_and_epi32(_brightness, IMAGE_RGBA_RGB_MASK);

		bool addBrightness = brightness >= 0;

		int xmmGroupCount = pixelsXmm / 4;
		for (int i = 0; i < xmmGroupCount; i++)
		{
			__m128i fourPixels;
			fourPixels = _mm_loadu_epi8(pixels);

			if (hasBrightness)
			{
				if (addBrightness)  fourPixels = _mm_adds_epu8(fourPixels, _brightness);
				else				fourPixels = _mm_subs_epu8(fourPixels, _brightness);
			}

			if (hasContrast)
			{
				// We have to convert 16 pixels from U8 to floats
				// 16 floats = 64 bytes = 512 bits
				for (int k = 0; k < 4; k++)
				{
					__m128i contrastedPixel;
					__m128  floatPixel;

					// Mask out other pixels and shuffle every pixel U8 component to U32
					contrastedPixel = _mm_and_epi32(fourPixels, _keepPixelMasks[k]);
					contrastedPixel = _mm_shuffle_epi8(contrastedPixel, _bytesToFloatsShuffle[k]);

					// Color = ContrastFactor * (Color - 128) + 128
					floatPixel = _mm_cvtepi32_ps(contrastedPixel);
					floatPixel = _mm_sub_ps(floatPixel, _128);
					floatPixel = _mm_mul_ps(floatPixel, _contrastFactor);
					floatPixel = _mm_add_ps(floatPixel, _128);

					// Values must be clamped to 0-255 range
					floatPixel = _mm_min_ps(floatPixel, _255);
					floatPixel = _mm_max_ps(floatPixel, _0);

					contrastedPixel = _mm_cvtps_epi32(floatPixel);
					contrastedPixel = _mm_shuffle_epi8(contrastedPixel, _floatsToBytesShuffle[k]);
					contrastedPixel = _mm_and_epi32(contrastedPixel, IMAGE_RGBA_RGB_MASK); // Mask alpha out

					// Mask out source pixels and put new values in
					fourPixels = _mm_and_epi32(fourPixels, _erasePixelMasks[k]);
					fourPixels = _mm_or_epi32(fourPixels, contrastedPixel);
				}
			}

			_mm_storeu_si128(reinterpret_cast<__m128i*>(pixels), fourPixels);
			pixels += 4;
		}
	}

#endif
	// And then remainders without intrinsics
	for (int i = 0; i < remainderPixels; i++)
	{
		pixels->R = (u8)std::clamp<float>(contrastFactor * (float)(pixels->R + brightness - 128) + 128, 0, 255);
		pixels->G = (u8)std::clamp<float>(contrastFactor * (float)(pixels->G + brightness - 128) + 128, 0, 255);
		pixels->B = (u8)std::clamp<float>(contrastFactor * (float)(pixels->B + brightness - 128) + 128, 0, 255);
		pixels++;
	}
}

void rageam::graphics::ImageCutoutAlphaRGBA(char* const pixelData, int width, int height, int threshold)
{
	EASY_FUNCTION();

	ColorU32* pixels = reinterpret_cast<ColorU32*>(pixelData);

	int totalPixels = width * height;

#ifdef AM_IMAGE_USE_SIMD
	int scalarPixels = totalPixels % 4; // We have to process leftover 1-3 pixels in standard mode
	int vectorizedPixels = totalPixels - scalarPixels;
	totalPixels = scalarPixels;

	// Threshold is byte (although stored as int)
	// Alpha component is placed last in memory so shift by 24 bits
	__m128i thresholdCmp = _mm_set1_epi32(threshold << 24);

	int vectorizedBlocks = vectorizedPixels / 4;
	for (int i = 0; i < vectorizedBlocks; i++)
	{
		__m128i fourPixels = _mm_loadu_epi8(pixels);

		// Compare unsigned byte + mask out only alpha
		__m128i mask;
		mask = _mm_cmpeq_epi8(_mm_max_epu8(fourPixels, thresholdCmp), fourPixels);
		mask = _mm_and_si128(mask, IMAGE_RGBA_ALPHA_MASK);

		// Remove alpha component completely from source pixels
		fourPixels = _mm_and_epi32(fourPixels, IMAGE_RGBA_RGB_MASK);

		// Set alpha component from our mask
		fourPixels = _mm_or_epi32(fourPixels, mask);

		_mm_storeu_epi8(pixels, fourPixels);

		pixels += 4;
	}
#endif

	for (int i = 0; i < totalPixels; i++)
	{
		if (pixels[i].A < static_cast<u8>(threshold))
			pixels[i].A = 0;
		else
			pixels[i].A = 255;
	}
}

void rageam::graphics::ImageScaleAlphaRGBA(char* pixelData, int width, int height, float alphaScale)
{
	EASY_FUNCTION();

	ColorU32* pixels = reinterpret_cast<ColorU32*>(pixelData);
	int totalPixels = width * height;
	for (int i = 0; i < totalPixels; i++)
	{
		float newAlpha = static_cast<float>(pixels[i].A) * alphaScale;
		if (newAlpha > 255.0f) newAlpha = 255.0f;
		pixels[i].A = static_cast<u8>(newAlpha);
	}
}

float rageam::graphics::ImageAlphaTestCoverageRGBA(char* pixelData, int width, int height, int threshold, float alphaScale)
{
	ColorU32* pixels = reinterpret_cast<ColorU32*>(pixelData);

	float alphaRef = static_cast<float>(threshold) / 255.0f;

	int totalPixels = width * height;
	int testPixels = 0;
	for (int i = 0; i < totalPixels; i++)
	{
		float alpha = static_cast<float>(pixels[i].A) / 255.0f;
		alpha *= alphaScale;

		if (alpha > alphaRef)
			testPixels++;
	}

	return static_cast<float>(testPixels) / static_cast<float>(totalPixels);
}

float rageam::graphics::ImageAlphaTestFindBestScaleRGBA(char* pixelData, int width, int height, int threshold, float desiredCoverage)
{
	EASY_FUNCTION();

	// https://github.com/castano/nvidia-texture-tools/blob/master/src/nvimage/FloatImage.cpp#L1453
	float minAlphaScale = 0.0f;
	float maxAlphaScale = 4.0f;
	float alphaScale = 1.0f;
	float bestAlphaScale = 1.0f;
	float bestError = FLT_MAX;

	for (int i = 0; i < 10; i++)
	{
		float coverage = ImageAlphaTestCoverageRGBA(pixelData, width, height, threshold, alphaScale);
		float error = fabsf(coverage - desiredCoverage);
		if (error < bestError)
		{
			bestAlphaScale = alphaScale;
			bestError = error;
		}

		if (coverage > desiredCoverage)
			maxAlphaScale = alphaScale;
		else if (coverage < desiredCoverage)
			minAlphaScale = alphaScale;
		else
			break;

		alphaScale = (minAlphaScale + maxAlphaScale) * 0.5f;
	}

	return bestAlphaScale;
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
	u32 totalSize = ImageComputeTotalSizeWithMips(width, height, mipCount, fmt);
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

rageam::graphics::Image::Image(const PixelDataOwner& pixelData, ImagePixelFormat pixelFormat, int width, int height, int mipCount, bool copyPixels)
{
	m_Width = width;
	m_Height = height;
	m_MipCount = mipCount;
	m_PixelFormat = pixelFormat;

	if (copyPixels)
	{
		u32 totalSize = ImageComputeTotalSizeWithMips(width, height, mipCount, pixelFormat);
		m_PixelData = PixelDataOwner::AllocateWithSize(totalSize);
		memcpy(m_PixelData.Data(), pixelData.Data(), totalSize);
	}
	else
	{
		m_PixelData = pixelData;
	}
}

rageam::graphics::Image::Image(const PixelDataOwner& pixelData, const ImageInfo& imageInfo, bool copyPixels)
	: Image(pixelData, imageInfo.PixelFormat, imageInfo.Width, imageInfo.Height, imageInfo.MipCount, copyPixels)
{

}

rageam::graphics::ImageInfo rageam::graphics::Image::GetInfo() const
{
	ImageInfo info;
	info.Width = m_Width;
	info.Height = m_Height;
	info.MipCount = m_MipCount;
	info.PixelFormat = m_PixelFormat;
	return info;
}

u32 rageam::graphics::Image::ComputeHashKey() const
{
	// Descending by processing time...

	if (m_FastHashKey.HasValue())
		return m_FastHashKey.GetValue();

	if (!String::IsNullOrEmpty(m_FilePath))
		return ImageFactory::GetFastHashKey(m_FilePath);

	ImageCache* cache = ImageCache::GetInstance();
	return cache->ComputeImageHash(m_PixelData.Data(), ComputeTotalSizeWithMips());
}

rageam::graphics::PixelDataOwner rageam::graphics::Image::GetPixelData(int mipIndex) const
{
	if (mipIndex == 0)
		return m_PixelData;

	AM_ASSERT(mipIndex < m_MipCount, "Image::GetPixelData() -> Mip index '%i' is out of bounds (total %i mips)", mipIndex, m_MipCount);

	u32 skip = ImageComputeTotalSizeWithMips(m_Width, m_Height, mipIndex, m_PixelFormat);
	return PixelDataOwner::CreateUnowned(m_PixelData.Data()->Bytes + skip);
}

bool rageam::graphics::Image::EnsurePixelDataLoaded()
{
	EASY_FUNCTION();

	if (m_PixelData.Data())
		return true;

	ImageCache* cache = ImageCache::GetInstance();
	u32			fastHashKey = ImageFactory::GetFastHashKey(m_FilePath);
	ImagePtr	cachedImage = cache->GetFromCache(fastHashKey);
	if (!cachedImage)
	{
		EASY_EVENT("Cache miss");
		cachedImage = ImageFactory::LoadFromPath(m_FilePath);
		if (!cachedImage)
			return false;
	}
	else 
	{
		EASY_EVENT("Loaded from cache");
	}

	m_PixelData = cachedImage->GetPixelData();
	m_Width = cachedImage->m_Width;
	m_Height = cachedImage->m_Height;
	m_MipCount = cachedImage->m_MipCount;
	m_PixelFormat = cachedImage->m_PixelFormat;
	m_HasAlphaPixels.Reset();
	return true;
}

bool rageam::graphics::Image::ScanAlpha()
{
	if (m_PixelFormat != ImagePixelFormat_U32 && m_PixelFormat != ImagePixelFormat_U16)
		return false;

	if (m_HasAlphaPixels.HasValue())
		return m_HasAlphaPixels.GetValue();

	bool hasAlpha = ImageScanAlpha(m_PixelData.Data(), m_Width, m_Height, m_PixelFormat);
	m_HasAlphaPixels = hasAlpha;
	return hasAlpha;
}

rageam::graphics::ColorU32 rageam::graphics::Image::GetPixel(int x, int y, int mipIndex) const
{
	AM_DEBUG_ASSERT(x < m_Width && y < m_Height, "Image::GetPixel() -> Coors (%i, %i) are out of bounds (%ix%i)", x, y, m_Width, m_Height);
	return ImageGetPixelColor(GetPixelDataBytes(mipIndex), x, y, m_Width >> mipIndex, m_PixelFormat);
}

amPtr<rageam::graphics::Image> rageam::graphics::Image::Resize(int newWidth, int newHeight, ResizeFilter resizeFilter)
{
	EASY_FUNCTION();

	// Nothing to resize, create dummy copy
	if (m_Width == newWidth && m_Height == newHeight)
		return ImageFactory::Clone(*this);

	// Resizing is pretty expensive operation, use caching
	ImageCache* cache = ImageCache::GetInstance();

	u32 hashKey = ComputeHashKey();

	// Mix hash key with new dimensions to get new unique key
	hashKey = DataHash(&newWidth, sizeof (int), hashKey);
	hashKey = DataHash(&newHeight, sizeof (int), hashKey);

	ImagePtr cachedImage = cache->GetFromCache(hashKey);
	if (cachedImage)
	{
		EASY_EVENT("Loaded from cache");
		return cachedImage;
	}
	EASY_EVENT("Cache miss");

	ImageInfo info = GetInfo();

	PixelDataOwner resizedData = PixelDataOwner::AllocateForImage(newWidth, newHeight, info.PixelFormat);

	char* dst = resizedData.Data()->Bytes;
	char* src = GetPixelDataBytes();

	ImageResize(dst, src, resizeFilter, info.PixelFormat, info.Width, info.Height, newWidth, newHeight, ScanAlpha());

	ImagePtr resizedImage = ImageFactory::Create(resizedData, info.PixelFormat, newWidth, newHeight, false);
	// NOTE: We don't store this in file system because it would cause ridiculous spam!
	cache->Cache(resizedImage, hashKey, ComputeSlicePitch(), ImageCacheEntryFlags_Temp, Vec2S(1.0f, 1.0f));

	return resizedImage;
}

amPtr<rageam::graphics::Image> rageam::graphics::Image::PadToPowerOfTwo(Vec2S& outUvExtent) const
{
	AM_ASSERT(!ImageIsCompressedFormat(m_PixelFormat), "Image::PadToPowerOfTwo() -> Compressed pixel formats are not supported.");

	if (CanGenerateMips()) // Already power of twos
	{
		outUvExtent = { 1.0f, 1.0f };
		return ImageFactory::Clone(*this);
	}

	// Create new canvas with size aligned to next power of two, if dimension(s) are not aligned already
	int newWidth = ALIGN_POWER_OF_TWO_32(m_Width);
	int newHeight = ALIGN_POWER_OF_TWO_32(m_Height);
	u32 newSlicePitch = ImageComputeSlicePitch(newWidth, newHeight, m_PixelFormat);
	u32 oldSlicePitch = ImageComputeSlicePitch(m_Width, m_Height, m_PixelFormat);
	u32 newRowPitch = ImageComputeRowPitch(newWidth, m_PixelFormat);
	u32 oldRowPitch = ImageComputeRowPitch(m_Width, m_PixelFormat);

	PixelDataOwner bufferOwner = PixelDataOwner::AllocateWithSize(newSlicePitch);

	char* buffer = bufferOwner.Data()->Bytes;
	char* sourcePixels = GetPixelData().Data()->Bytes;

	// We fill everything with white pixels
	memset(buffer, 0xFF, newSlicePitch);

	// Then simply overlay initial image on top of extended canvas
	// Case when we have to pad X is a little bit more complex because we'll have to leave empty are at the end of each line
	// In case of padding only Y we can just use mem copy, because padded area is placed right after
	bool needToPadX = !IS_POWER_OF_TWO(m_Width);
	if (needToPadX)
	{
		// For every row...
		for (int y = 0; y < m_Height; y++)
		{
			// We copy whole source line and leave padding at the end
			char* destRow = buffer + static_cast<size_t>(newRowPitch * y);
			char* srcRow = sourcePixels + static_cast<size_t>(oldRowPitch * y);
			memcpy(destRow, srcRow, oldRowPitch);
		}
	}
	else
	{
		memcpy(buffer, sourcePixels, oldSlicePitch);
	}

	outUvExtent.X = static_cast<float>(m_Width) / static_cast<float>(newWidth);
	outUvExtent.Y = static_cast<float>(m_Height) / static_cast<float>(newHeight);

	return ImageFactory::Create(bufferOwner, m_PixelFormat, newWidth, newHeight);
}

rageam::Vec2S rageam::graphics::Image::ComputePadExtent() const
{
	int newWidth = ALIGN_POWER_OF_TWO_32(m_Width);
	int newHeight = ALIGN_POWER_OF_TWO_32(m_Height);
	float x = static_cast<float>(m_Width) / static_cast<float>(newWidth);
	float y = static_cast<float>(m_Height) / static_cast<float>(newHeight);
	return { x, y };
}

amPtr<rageam::graphics::Image> rageam::graphics::Image::ConvertPixelFormat(ImagePixelFormat formatTo) const
{
	if (m_PixelFormat == formatTo)
		return ImageFactory::Clone(*this);

	PixelDataOwner dataOwner = PixelDataOwner::AllocateForImage(m_Width, m_Height, formatTo);
	ImageConvertPixelFormat(dataOwner.Data(), GetPixelDataBytes(), m_PixelFormat, formatTo, m_Width, m_Height);

	return ImageFactory::Create(dataOwner, formatTo, m_Width, m_Height);
}

amPtr<rageam::graphics::Image> rageam::graphics::Image::GenerateMipMaps(ResizeFilter mipFilter)
{
	// Image already have mip maps or mips can't be generated (bc formats are loss and meant to be encoded only once)
	if (ImageIsCompressedFormat(m_PixelFormat) || m_MipCount > 1)
	{
		return ImageFactory::Clone(*this);
	}

	// We can't generate any mip maps, resolution is not power of two
	int mipCount = ImageComputeMaxMipCount(m_Width, m_Height);
	if (mipCount == 1)
	{
		return ImageFactory::Clone(*this);
	}

	bool hasAlphaPixels = ScanAlpha();

	PixelDataOwner mipPixelData = PixelDataOwner::AllocateForImage(m_Width, m_Height, m_PixelFormat, mipCount);
	char* mipPixelBytes = mipPixelData.Data()->Bytes;
	// We store pointer to previous mip to generate mip from it and not from the largest mip
	char* prevMipPixelBytes = GetPixelDataBytes();

	int mipWidth = m_Width;
	int mipHeight = m_Height;
	int prevMipWidth = m_Width;
	int prevMipHeight = m_Height;

	for (int i = 0; i < mipCount; i++)
	{
		u32 slicePitch = ImageComputeSlicePitch(mipWidth, mipHeight, m_PixelFormat);

		// Downsample previous mip map by 2x factor, for the first mip map
		// new/previous sizes will match and pixel data will be simply copied over
		ImageResize(
			mipPixelBytes, prevMipPixelBytes, mipFilter, m_PixelFormat,
			prevMipWidth, prevMipHeight, mipWidth, mipHeight, hasAlphaPixels);

		prevMipPixelBytes = mipPixelBytes;
		prevMipWidth = mipWidth;
		prevMipHeight = mipHeight;

		mipPixelBytes += slicePitch;
		mipWidth /= 2;
		mipHeight /= 2;
	}

	return std::make_shared<Image>(mipPixelData, m_PixelFormat, m_Width, m_Height, mipCount);
}

amPtr<rageam::graphics::Image> rageam::graphics::Image::FitMaximumResolution(int maximumResolution, ResizeFilter filter)
{
	if (maximumResolution == 0 || (m_Width <= maximumResolution && m_Height <= maximumResolution))
		return ImageFactory::Clone(*this);

	int fitWidth, fitHeight;
	ImageScaleResolution(m_Width, m_Height, maximumResolution, maximumResolution, fitWidth, fitHeight, ResolutionScalingMode_Fit);

	return Resize(fitWidth, fitHeight, filter);
}

void rageam::graphics::Image::Swizzle(ImageSwizzle r, ImageSwizzle g, ImageSwizzle b, ImageSwizzle a) const
{
	ImageDoSwizzle(m_PixelData.Data()->Bytes, m_Width, m_Height, r, g, b, a);
}

amPtr<rageam::graphics::Image> rageam::graphics::Image::AdjustBrightnessAndContrast(int brightness, int contrast) const
{
	AM_ASSERTS(m_PixelFormat == ImagePixelFormat_U32);
	ImagePtr imageCopy = ImageFactory::Copy(*this);
	ImageAdjustBrightnessAndContrastRGBA(
		imageCopy->GetPixelDataBytes(), imageCopy->m_Width, imageCopy->m_Height, brightness, contrast);
	return imageCopy;
}

bool rageam::graphics::Image::CreateDX11Resource(
	amComPtr<ID3D11ShaderResourceView>& outView,
	const ImageDX11ResourceOptions& options,
	Vec2S* outUV2,
	amComPtr<ID3D11Texture2D>* outTex)
{
	if (outUV2) *outUV2 = { 1.0f, 1.0f };

	ImageCache* cache = ImageCache::GetInstance();

	// Attempt to retrieve view from cache
	// TODO: Hash is not ideal because two options still may produce the same view
	u32 viewHash;
	viewHash = DataHash(m_PixelData.Data()->Bytes, ComputeTotalSizeWithMips());
	viewHash = DataHash(&options, sizeof ImageDX11ResourceOptions, viewHash);
	amComPtr<ID3D11ShaderResourceView> cachedView;
	amComPtr<ID3D11Texture2D> cachedTex;
	if (cache->GetFromCacheDX11(viewHash, cachedView, outUV2, &cachedTex))
	{
		outView = cachedView;
		if (outTex) *outTex = cachedTex;
		return true;
	}

	// Shallow copy to make things easier
	ImagePtr image = ImageFactory::Clone(*this);

	Timer timer = Timer::StartNew();

	// Even though that DDS can be saved with non-power-of two resolution, DX11 doesn't accept such resolutions
	if (ImageIsCompressedFormat(image->m_PixelFormat) && !ImageIsResolutionValidForMipMapsAndCompression(image->m_Width, image->m_Height))
	{
		if (options.AllowImageConversion)
		{
			PixelDataOwner decodedPixels = ImageDecodeBCToRGBA(GetPixelData(), image->m_Width, image->m_Height, image->m_PixelFormat);
			image = ImageFactory::Create(decodedPixels, ImagePixelFormat_U32, image->m_Width, image->m_Height);
		}
		else
		{
			AM_ERRF("Image::CreateDXResource() -> Can't create DX11 texture with compressed pixel format '%s' and non power-of-two resolution (%ux%u)",
				Enum::GetName(image->m_PixelFormat), image->m_Width, image->m_Height);
			return false;
		}
	}

	// U24 don't have matching DXGI format, so we have to create temporary image with U32 (RGBA) format
	if (image->m_PixelFormat == ImagePixelFormat_U24)
	{
		if (options.AllowImageConversion)
		{
			PixelDataOwner rgbaPixels = PixelDataOwner::AllocateForImage(image->m_Width, image->m_Height, ImagePixelFormat_U32);
			ImageConvertPixelFormat(rgbaPixels.Data(), GetPixelDataBytes(), ImagePixelFormat_U24, ImagePixelFormat_U32, image->m_Width, image->m_Height);
			image = ImageFactory::Create(rgbaPixels, ImagePixelFormat_U32, image->m_Width, image->m_Height);
		}
		else
		{
			AM_ERRF("Image::CreateDXResource() -> Can't create DX11 texture with RGB pixel format");
			return false;
		}
	}

	if (!ImageIsCompressedFormat(m_PixelFormat) && 
		options.MaxResolution != 0 &&
		(m_Width > options.MaxResolution || m_Height > options.MaxResolution))
	{
		int newWidth, newHeight;
		ImageScaleResolution(m_Width, m_Height, options.MaxResolution, options.MaxResolution, newWidth, newHeight, ResolutionScalingMode_Fit);
		image = Resize(newWidth, newHeight, options.MipFilter);
	}

	if (!ImageIsCompressedFormat(m_PixelFormat))
	{
		if (options.PadToPowerOfTwo)
		{
			Vec2S uv2;
			image = image->PadToPowerOfTwo(uv2);
			if (outUV2)
			{
				*outUV2 = uv2;
			}
			else
			{
				AM_WARNINGF("Image::CreateDXResource() -> PadToPowerOfTwo is True but outUV2 is NULL");
			}
		}

		if (options.CreateMips)
		{
			image = image->GenerateMipMaps(options.MipFilter);
		}
	}

	// Ensure that converted image has the same name as this image
	image->SetDebugName(GetDebugName());

	HRESULT code;
	ID3D11Device* device = RenderGetDevice();

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Format = ImagePixelFormatToDXGI(image->m_PixelFormat);
	texDesc.ArraySize = 1;
	texDesc.MipLevels = image->m_MipCount;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.Width = image->m_Width;
	texDesc.Height = image->m_Height;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;

	// Fill pixel data
	D3D11_SUBRESOURCE_DATA texData[IMAGE_MAX_MIP_MAPS] = {};
	// Mip maps are packed continuously, we have to add slice size to offset to get to next mip
	u32 pixelDataOffset = 0;
	for (int i = 0; i < image->m_MipCount; i++)
	{
		// Compute size of current mip map pixel data in bytes
		int mipWidth = image->m_Width >> i;
		int mipHeight = image->m_Height >> i;
		u32 slicePitch = ImageComputeSlicePitch(mipWidth, mipHeight, image->m_PixelFormat);
		u32 rowPitch = ImageComputeRowPitch(mipWidth, image->m_PixelFormat);

		texData[i].pSysMem = image->GetPixelDataBytes() + pixelDataOffset;
		texData[i].SysMemPitch = rowPitch;
		texData[i].SysMemSlicePitch = slicePitch;

		// Shift pointer to next mip map pixel data
		pixelDataOffset += slicePitch;
	}

	ID3D11Texture2D* pTex;
	code = device->CreateTexture2D(&texDesc, texData, &pTex);
	if (code != S_OK)
	{
		AM_ERRF(L"Image::CreateDX11Resource() -> Failed to create Texture2D for '%ls', error code: %u", GetDebugName(), code);
		return false;
	}

	SetObjectDebugName(pTex, "IImage Tex - '%ls'", image->GetDebugName());

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = texDesc.MipLevels;
	viewDesc.Texture2D.MostDetailedMip = 0;

	ID3D11ShaderResourceView* pView;
	code = device->CreateShaderResourceView(pTex, &viewDesc, &pView);

	if (code != S_OK)
	{
		AM_ERRF(L"Image::CreateDX11Resource() -> Failed to create ShaderResourceView for '%ls', error code: %u", GetDebugName(), code);
		pTex->Release();
		return false;
	}

	SetObjectDebugName(pView, "IImage View - '%ls'", image->GetDebugName());

	amComPtr tex = amComPtr(pTex);
	outView = amComPtr(pView);
	if (outTex) *outTex = tex;

	// Add image to cache
	timer.Stop();
	if (cache->ShouldStore(timer.GetElapsedMilliseconds()))
	{
		cache->CacheDX11(viewHash, outView, tex, outUV2 ? *outUV2 : Vec2S(1.0f, 1.0f));
	}

	if (options.UpdateSourceImage)
	{
		*this = std::move(*image);
	}

	return true;
}

bool rageam::graphics::ImageFactory::VerifyImageSize(const ImagePtr& image)
{
	if (image->m_Width <= 0 || image->m_Height <= 0 || image->m_MipCount <= 0)
	{
		AM_ERRF(
			"ImageFactory::VerifyImageSize() -> Loaded image has invalid dimensions and mip count: %i%i, %i mips."
			"Image is not supported and most likely corrupted", image->m_Width, image->m_Height, image->m_MipCount);
		return false;
	}

	if (image->m_MipCount > IMAGE_MAX_MIP_MAPS)
	{
		AM_ERRF(
			"ImageFactory::VerifyImageSize() -> Loaded image has '%i' mip maps, which is greater than maximum allowed '%i'"
			"Image is not supported and most likely corrupted", image->m_MipCount, IMAGE_MAX_MIP_MAPS);
		return false;
	}

	if (image->m_Width > IMAGE_MAX_RESOLUTION || image->m_Height > IMAGE_MAX_RESOLUTION)
	{
		AM_ERRF(
			"ImageFactory::VerifyImageSize() -> Loaded image has resolution greater than the maximum supported one! "
			"Image is not supported and most likely corrupted.");
		return false;
	}

	return true;
}

rageam::graphics::ImagePtr rageam::graphics::ImageFactory::TryLoadFromPath(ConstWString path, bool onlyMeta)
{
	PixelDataOwner pixelData;
	ImagePixelFormat pixelFormat;
	int width, height, mipCount;
	if (!ImageRead(path, width, height, mipCount, pixelFormat, nullptr, onlyMeta, &pixelData))
		return nullptr;

	// Some DDS have mips up to 1x1... DX11 won't allow those
	mipCount = MIN(mipCount, ImageComputeMaxMipCount(width, height));

	amPtr<Image> image = std::make_shared<Image>(pixelData, pixelFormat, width, height, mipCount);
	image->m_FilePath = path;
	image->SetDebugName(file::GetFileName(path));

	if (!VerifyImageSize(image))
		return nullptr;

	return image;
}

rageam::graphics::ImageFileKind rageam::graphics::ImageFactory::GetImageKindFromPath(ConstWString path)
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
	case Hash("webp"):	return ImageKind_WEBP;
	case Hash("jpeg"):	return ImageKind_JPEG;
	case Hash("jpg"):	return ImageKind_JPG;
	case Hash("png"):	return ImageKind_PNG;
	case Hash("tga"):	return ImageKind_TGA;
	case Hash("bmp"):	return ImageKind_BMP;
	case Hash("psd"):	return ImageKind_PSD;
	case Hash("dds"):	return ImageKind_DDS;
	case Hash("ico"):	return ImageKind_ICO;
	case Hash("svg"):	return ImageKind_SVG;

	default: return ImageKind_None;
	}
}

rageam::graphics::ImagePtr rageam::graphics::ImageFactory::LoadFromPath(ConstWString path, bool onlyMeta, bool useCache)
{
	EASY_FUNCTION();

	// Special cases for ICO and SVG
	ImageFileKind imageKind = GetImageKindFromPath(path);
	if (imageKind == ImageKind_ICO)
	{
		List<ImagePtr> icons;
		if (!LoadIco(path, icons))
			return nullptr;

		// Find ICO image that matches preferred size
		int preferredSize = tl_ImagePreferredIcoResolution;
		int bestFitIndex = ImageFindBestResolutionMatch(
			icons.GetSize(), preferredSize, preferredSize, [&icons](int i, int& outW, int& outH)
			{
				outW = icons[i]->GetWidth();
				outH = icons[i]->GetHeight();
			});
		return icons[bestFitIndex];
	}
	if(imageKind == ImageKind_SVG)
	{
		if(onlyMeta)
		{
			return Create(
				PixelDataOwner::CreateUnowned(nullptr), ImagePixelFormat_U32, tl_ImagePreferredSvgWidth, tl_ImagePreferredSvgHeight);
		}

		return LoadSvg(path, tl_ImagePreferredSvgWidth, tl_ImagePreferredSvgHeight);
	}

	ImageCache* imageCache = ImageCache::GetInstance();

	// Try to retrieve image from cache
	u32 hash = 0;
	if (useCache)
	{
		hash = GetFastHashKey(path);

		ImagePtr cachedImage = imageCache->GetFromCache(hash);
		if (cachedImage)
		{
			EASY_EVENT("Loaded from cache");
			return cachedImage;
		}
		EASY_EVENT("Cache miss");
	}

	Timer timer = Timer::StartNew();
	ImagePtr image = TryLoadFromPath(path, onlyMeta);
	timer.Stop();

	if (!image)
	{
		AM_ERRF(L"ImageFactory::LoadFromPath() -> An error occured during loading image from '%ls'", path);
		return nullptr;
	}

	if (!onlyMeta && useCache && imageCache->ShouldStore(timer.GetElapsedMilliseconds()))
	{
		u32 pixelDataSize = ImageComputeSlicePitch(image->m_Width, image->m_Height, image->m_PixelFormat);
		imageCache->Cache(image, hash, pixelDataSize, ImageCacheEntryFlags_None, Vec2S(1.0f, 1.0f));
	}

	image->m_FastHashKey = GetFastHashKey(path);

	return image;
}

rageam::graphics::ImagePtr rageam::graphics::ImageFactory::LoadFromPathAndCompress(
	ConstWString path, const ImageCompressorOptions& compOptions, CompressedImageInfo* outCompInfo, ImageCompressorToken* token)
{
	EASY_FUNCTION();

	if (GetImageKindFromPath(path) == ImageKind_DDS && !compOptions.AllowRecompress)
	{
		ImagePtr compressedImage = LoadFromPath(path);
		if (!compressedImage)
			return nullptr;

		// Image was compressed before, we don't know what options were used... just default them
		if (outCompInfo)
		{
			CompressedImageInfo compInfo = {};
			compInfo.ImageInfo = compressedImage->GetInfo();
			// We're preserving format from DDS file...
			compInfo.IsSourceCompressed = true;/*compressedImage->IsBlockCompressed();*/
			*outCompInfo = compInfo;
		}

		return compressedImage;
	}

	// We need only metadata so no reason to cache
	ImagePtr metaImage = LoadFromPath(path, true);
	if (!metaImage)
		return nullptr;

	u32 fastHash = GetFastHashKey(path);

	ImageCompressor compressor;
	return compressor.Compress(metaImage, compOptions, &fastHash, outCompInfo, token);
}

bool rageam::graphics::ImageFactory::LoadIco(ConstWString path, List<ImagePtr>& icons)
{
	// See https://en.wikipedia.org/wiki/ICO_(file_format)#cite_note-bigSize-7 for format description

	ConstWString debugName = file::GetFileName(path);

// Packing is really necessary only for IconDir::Images alignment
#pragma pack(push)
#pragma pack(2)
	struct IconImage
	{
		BYTE Width;
		BYTE Height;
		BYTE NumColors;
		BYTE Reserved;

		WORD ColorPlanes;
		WORD BitsPerPixel;

		DWORD DataSize;
		DWORD DataOffset;

		// Dimension 0 indicates that we're dealing with 256

		DWORD GetWidth() const { return Width != 0 ? Width : 256; }
		DWORD GetHeight() const { return Height != 0 ? Height : 256; }
	};

	struct IconDir
	{
		WORD Reserved;
		WORD Type;
		WORD ImageCount;
		IconImage Images[1];
	};
#pragma pack(pop)

	file::FileBytes blob;
	if (!ReadAllBytes(path, blob))
	{
		AM_ERRF(L"ImageFactory::LoadIco() -> Failed to read ICO file '%ls'", path);
		return false;
	}

	char* bytes = blob.Data.get();

	IconDir* iconDir = reinterpret_cast<IconDir*>(bytes);
	for (int i = 0; i < iconDir->ImageCount; i++)
	{
		ImagePtr image;

		IconImage& dirImage = iconDir->Images[i];

		char* imageData = bytes + dirImage.DataOffset;
		u32 imageDataSize = dirImage.DataSize;

		// ICO holds either PNG or BMP
		constexpr DWORD PNG_MAGIC = MAKEFOURCC(0x89, 'P', 'N', 'G');
		bool isPng = memcmp(imageData, &PNG_MAGIC, sizeof DWORD) == 0;
		if (isPng)
		{
			int width, height, channelCount;
			u8* stbiPixels = stbi_load_from_memory((u8*)imageData, (int)imageDataSize, &width, &height, &channelCount, 0);
			if (!stbiPixels)
			{
				AM_ERRF("ImageFactory::LoadIco() -> Failed to read PNG at slot '%i'", i);
				return false;
			}

			PixelDataOwner stbiPixelOwner = PixelDataOwner::CreateOwned(stbiPixels);
			stbiPixelOwner.DeleteFn = stbi_image_free;

			ImagePixelFormat pixelFormat;
			switch (channelCount)
			{
			case 1: pixelFormat = ImagePixelFormat_U8;	break;
			case 2: pixelFormat = ImagePixelFormat_U16;	break;
			case 3: pixelFormat = ImagePixelFormat_U24;	break;
			case 4: pixelFormat = ImagePixelFormat_U32;	break;
			}

			image = std::make_shared<Image>(stbiPixelOwner, pixelFormat, width, height, 1);
		}
		else // BMP
		{
			imageData += sizeof BITMAPINFOHEADER;

			// Non RGBA BMPs in ico are fucked up so we skip them
			if (dirImage.BitsPerPixel != 32)
			{
				AM_WARNINGF("ImageFactory::LoadIco() -> Non RGBA32 bit are not supported (slot '%i'), skipping...", i);
				continue;
			}

			int width = dirImage.Width;
			int height = dirImage.Height;
			u32 slicePitch = ImageComputeSlicePitch(width, height, ImagePixelFormat_U32);

			PixelDataOwner bmpPixelOwner = PixelDataOwner::AllocateWithSize(slicePitch);
			memcpy(bmpPixelOwner.Data(), imageData, slicePitch);

			// God knows why but BMP is flipped vertically
			ImageFlipY(bmpPixelOwner.Data()->Bytes, width, height, ImagePixelFormat_U32);
			// BMP is stored as BGRA, we need RGBA
			ImageDoSwizzle(bmpPixelOwner.Data()->Bytes, width, height,
				ImageSwizzle_B, ImageSwizzle_G, ImageSwizzle_R, ImageSwizzle_A);

			image = std::make_shared<Image>(bmpPixelOwner, ImagePixelFormat_U32, width, height, 1);
		}

		image->SetDebugName(String::FormatTemp(L"%s [Ico %i]", debugName, i));

		icons.Emplace(std::move(image));
	}

	return true;
}

rageam::graphics::ImagePtr rageam::graphics::ImageFactory::LoadSvg(ConstWString path, int width, int height)
{
	file::FileBytes fileBytes;
	if (!ReadAllBytes(path, fileBytes))
	{
		AM_ERRF(L"ImageFactory::LoadSvg() -> Failed to read file '%ls'", path);
		return nullptr;
	}

	auto document = lunasvg::Document::loadFromData(fileBytes.Data.get(), fileBytes.Size);
	auto bitmap = document->renderToBitmap(width, height);
	bitmap.convertToRGBA();
	return Create(PixelDataOwner::CreateUnowned(bitmap.data()), ImagePixelFormat_U32, width, height, true);
}

rageam::graphics::ImagePtr rageam::graphics::ImageFactory::Create(
	const PixelDataOwner& pixelData, ImagePixelFormat fmt, int width, int height, bool copyPixels)
{
	return std::make_shared<Image>(pixelData, fmt, width, height, 1, copyPixels);
}

rageam::graphics::ImagePtr rageam::graphics::ImageFactory::Clone(const Image& image)
{
	return std::make_shared<Image>(image.m_PixelData, image.GetInfo(), false);
}

rageam::graphics::ImagePtr rageam::graphics::ImageFactory::Copy(const Image& image)
{
	return std::make_shared<Image>(image.m_PixelData, image.GetInfo(), true);
}

rageam::graphics::ImagePtr rageam::graphics::ImageFactory::CreateChecker(ColorU32 color0, ColorU32 color1, int size, int tileSize)
{
	PixelDataOwner pixelDataOwner = PixelDataOwner::AllocateForImage(size, size, ImagePixelFormat_U32);

	u32* pixelData = pixelDataOwner.Data()->RGBA;

	for (int y = 0; y < size; y++)
	{
		bool isEvenRow = y / tileSize % 2 == 0;
		for (int x = 0; x < size; x++)
		{
			bool isEvenCol = x / tileSize % 2 == 0;
			u32 c0 = isEvenCol ? color0 : color1;
			u32 c1 = isEvenCol ? color1 : color0;
			u32 c = isEvenRow ? c0 : c1;

			*pixelData = c;
			++pixelData;
		}
	}

	return Create(pixelDataOwner, ImagePixelFormat_U32, size, size);
}

u32 rageam::graphics::ImageFactory::GetFastHashKey(ConstWString path)
{
	u32 hash;
	u64 fileTime = file::GetFileModifyTime(path);
	hash = Hash(path);
	hash = DataHash(&fileTime, sizeof u64, hash);
	return hash;
}

bool rageam::graphics::ImageFactory::CanBlockCompressImage(ConstWString path)
{
	int w, h, mips;
	ImagePixelFormat fmt;
	if (!ImageRead(path, w, h, mips, fmt, nullptr, true, nullptr))
		return false;

	return ImageIsResolutionValidForMipMapsAndCompression(w, h);
}

bool rageam::graphics::ImageFactory::SaveImage(ImagePtr img, ConstWString path, ImageFileKind toKind, float quality)
{
	// Save kind is not specified, first attempt to retrieve it from path or fallback to original image kind
	if (toKind == ImageKind_None)
	{
		ImageFileKind pathKind = GetImageKindFromPath(path);
		if (pathKind == ImageKind_None)
		{
			pathKind = ImageIsCompressedFormat(img->GetPixelFormat()) ? ImageKind_DDS : ImageKind_PNG;
		}
		toKind = pathKind;
	}

	if (!(ImageWriteableKinds & (1 << toKind)))
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
	if (ImageIsCompressedFormat(imgInfo.PixelFormat) && toKind != ImageKind_DDS)
	{
		img = ImageCompressor::Decompress(std::dynamic_pointer_cast<Image>(img));
		imgInfo = img->GetInfo();
	}

	// Replace extension in given path to actual extension of saving format
	file::WPath savePath = path;
	savePath = savePath.GetFilePathWithoutExtension();
	savePath += L".";
	savePath += ImageKindToFileExtensionW[toKind];

	// WEBP supports only RGBA and RGB, handle it separately
	if (toKind == ImageKind_WEBP)
	{
		bool hasAlpha = ImageIsAlphaFormat(imgInfo.PixelFormat);

		if (imgInfo.PixelFormat == ImagePixelFormat_U16)
			img = img->ConvertPixelFormat(ImagePixelFormat_U32);
		else if (imgInfo.PixelFormat == ImagePixelFormat_U8)
			img = img->ConvertPixelFormat(ImagePixelFormat_U24);
		imgInfo = img->GetInfo();

		return ImageWriteWebp(savePath, imgInfo.Width, imgInfo.Height, img->GetPixelDataBytes(), hasAlpha, quality * 100.0f);
	}

	return ImageWrite(savePath, imgInfo.Width, imgInfo.Height, imgInfo.MipCount, imgInfo.PixelFormat, img->GetPixelData(), toKind, quality);
}
