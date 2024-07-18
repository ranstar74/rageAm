//
// File: image.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/enum.h"
#include "am/graphics/color.h"
#include "am/system/nullable.h"
#include "am/system/ptr.h"
#include "am/types.h"

#include <d3d11.h>

// TODO:
// - HSV / Levels
// - Large allocator for images, currently we use malloc
// - Alpha test coverage in encoder is done even if texture has no alpha
//
// Formats
// - TIFF

namespace rageam::graphics
{
	struct ImageCompressorToken;
	struct CompressedImageInfo;
	struct ImageCompressorOptions;
	class PixelDataOwner;
	class Image;

	// Enables mix of SSE and AVX2 image processing
	// AVX2 (AM_IMAGE_USE_AVX2) is enabled if project is created with --avx2 options
	// NOTE: Third party libraries will use SSE2 regardless! This option exists only for testing / benchmarking
#define AM_IMAGE_USE_SIMD

	static constexpr int	IMAGE_MAX_RESOLUTION = 1 << 14;	// Maximum supported by DX11
	static constexpr int	IMAGE_MAX_MIP_MAPS = 14;			// 16384 -> 1
	static constexpr size_t IMAGE_RGBA_PITCH = 4;
	static constexpr size_t IMAGE_RGB_PITCH = 3;

	// We use those memory allocation functions because system heap is limited and not suitable for this

	pVoid ImageAlloc(u32 size);
	pVoid ImageAllocTemp(u32 size);
	pVoid ImageReAlloc(pVoid block, u32 newSize);
	pVoid ImageReAllocTemp(pVoid block, u32 newSize);
	void  ImageFree(pVoid block);
	void  ImageFreeTemp(pVoid block);

	enum ImageFileKind
	{
		ImageKind_None,
		ImageKind_WEBP,
		ImageKind_JPEG,
		ImageKind_JPG,
		ImageKind_PNG,
		ImageKind_TGA,
		ImageKind_BMP,
		ImageKind_PSD,
		ImageKind_DDS,
		ImageKind_ICO,
		ImageKind_SVG,
	};

	// Formats that are supported for writing (saving) to file
	static constexpr u32 ImageWriteableKinds =
		1 << ImageKind_WEBP |
		1 << ImageKind_JPEG |
		1 << ImageKind_JPG |
		1 << ImageKind_PNG |
		1 << ImageKind_TGA |
		1 << ImageKind_BMP |
		1 << ImageKind_DDS;

	static constexpr ConstWString ImageKindToFileExtensionW[] =
	{
		L"", L"webp", L"jpeg", L"jpg", L"png", L"tga", L"bmp", L"psd", L"dds", L"ico"
	};

	static constexpr ConstString ImageKindToFileExtension[] =
	{
		"", "webp", "jpeg", "jpg", "png", "tga", "bmp", "psd", "dds", "ico"
	};

	enum ResizeFilter
	{
		ResizeFilter_Box,
		ResizeFilter_Triangle,
		ResizeFilter_CubicBSpline,
		ResizeFilter_CatmullRom,
		ResizeFilter_Mitchell,
		ResizeFilter_Point,
	};

	enum ImagePixelFormat
	{
		ImagePixelFormat_None,

		ImagePixelFormat_U32,	// RGBA			U8[4]
		ImagePixelFormat_U24,	// RGB			U8[3]
		ImagePixelFormat_U16,	// Gray Alpha	U8[2]
		ImagePixelFormat_U8,	// Gray			U8[1]
		ImagePixelFormat_A8,	// Alpha		U8[1]

		// All compressed formats are UNORM

		ImagePixelFormat_BC1,
		ImagePixelFormat_BC2,
		ImagePixelFormat_BC3,
		ImagePixelFormat_BC4,
		ImagePixelFormat_BC5,
		ImagePixelFormat_BC7,
	};

	static constexpr ConstString ImagePixelFormatToName[] =
	{
		"None",
		"RGBA", "RGB", "Gray Alpha (GA16)", "Gray (U8)", "Alpha (A8)",
		"BC1", "BC2", "BC3", "BC4", "BC5", "BC7"
	};

	static constexpr u32 ImagePixelFormatToFourCC[] =
	{
		0, 0, 0, 0, 0, 0,			// None, U32, U24, U16, U8, A8
		FOURCC('D', 'X', 'T', '1'),	// BC1
		FOURCC('D', 'X', 'T', '2'), // BC2
		FOURCC('D', 'X', 'T', '5'), // BC3
		FOURCC('A', 'T', 'I', '1'), // BC4
		FOURCC('A', 'T', 'I', '2'), // BC5
		0							// BC7
	};

	static constexpr int ImagePixelFormatBitsPerPixel[] =
	{
		0,
		32, 24, 16, 8, 8,		// U32, U24, U16, U8, A8
		4, 8, 8, 4, 8, 8,		// BC1, BC2, BC3, BC4, BC5, BC7
	};

	static_assert(Enum::GetCount<ImagePixelFormat>() == sizeof(ImagePixelFormatToName) / sizeof(ConstString));
	static_assert(Enum::GetCount<ImagePixelFormat>() == sizeof(ImagePixelFormatToFourCC) / sizeof(u32));
	static_assert(Enum::GetCount<ImagePixelFormat>() == sizeof(ImagePixelFormatBitsPerPixel) / sizeof(int));

	enum ResolutionScalingMode
	{
		ResolutionScalingMode_Fill,
		ResolutionScalingMode_Fit,
		ResolutionScalingMode_Stretch,
	};

	enum ImageSwizzle
	{
		ImageSwizzle_R,
		ImageSwizzle_G,
		ImageSwizzle_B,
		ImageSwizzle_A,
	};

	// Scales given resolution used specified constraints, mostly used for UI
	void ImageScaleResolution(int wIn, int hIn, int wTo, int hTo, int& wOut, int& hOut, ResolutionScalingMode mode);
	// If rectSize is equal to 0, original size is returned
	void ImageFitInRect(int wIn, int hIn, int rectSize, int& wOut, int& hOut);

	// Finds closest sized image in enumeration
	// -1 if not found (only for empty list)
	int ImageFindBestResolutionMatch(int imageCount, int width, int height,
		const std::function<void(int i, int& w, int& h)>& getter);

	// Gets whether given format is one of ImagePixelFormat_BC (block compressed)
	bool ImageIsCompressedFormat(ImagePixelFormat fmt);
	// Gets whether format supports alpha channel
	bool ImageIsAlphaFormat(ImagePixelFormat fmt);

	// NOTE: Not every pixel format (such as U24) can be converted to DXGI,
	// DXGI_FORMAT_UNKNOWN will be returned for such
	DXGI_FORMAT ImagePixelFormatToDXGI(ImagePixelFormat fmt);
	ImagePixelFormat ImagePixelFormatFromDXGI(DXGI_FORMAT fmt);

	// Total number of bytes image takes with given resolution and format
	u32 ImageComputeSlicePitch(int width, int height, ImagePixelFormat pixelFormat);
	// Total number of bytes one pixel row (scanline) takes with given resolution and format
	// Also known as 'stride'
	u32 ImageComputeRowPitch(int width, ImagePixelFormat pixelFormat);
	// Accumulates total size of image and mip maps
	u32 ImageComputeTotalSizeWithMips(int width, int height, int mipCount, ImagePixelFormat pixelFormat);

	// Makes sure that resolution is power of two and greater than 4x4 (lowest size for block compression)
	bool ImageIsResolutionValidForMipMapsAndCompression(int width, int height);
	// Non-power of two images cannot be properly scaled down
	bool ImageIsResolutionValidForMipMaps(int width, int height);

	// Gets num of mip maps up until 4x4 (lowest size for block compression)
	int ImageComputeMaxMipCount(int width, int height);

	// Checks if any pixel has transparent alpha value (non 255)
	// BC formats are not supported
	bool ImageScanAlpha(pVoid data, int width, int height, ImagePixelFormat fmt);

	// Helper utility for stb_image_write
	// If kind is ImageKind_None, it is defaulted to ImageKind_PNG
	// Quality is only used on JPEG
	bool ImageWriteStb(ConstWString path, ImageFileKind kind, int w, int h, int c, pVoid data, int quality = 100);
	// NOTE: Data must be in RGBA U32 or RGB U24 format (hasAlpha param)! If quality is 100.0, lossless format is used
	bool ImageWriteWebp(ConstWString path, int w, int h, pVoid data, bool hasAlpha, float quality = 100.0f);
	bool ImageWriteDDS(ConstWString path, int w, int h, int mips, ImagePixelFormat fmt, pVoid data);
	bool ImageWriteDDS(ConstWString path, int w, int h, int mips, DXGI_FORMAT fmt, pVoid data);

	bool ImageReadStb(ConstWString path, int& w, int& h, ImagePixelFormat& fmt, bool onlyMeta, PixelDataOwner* pixels);
	// Decoded format is always RGBA
	bool ImageReadWebp(ConstWString path, int& w, int& h, ImagePixelFormat& fmt, bool onlyMeta, PixelDataOwner* pixels);
	bool ImageReadDDS(ConstWString path, int& w, int& h, int& mips, ImagePixelFormat& fmt, bool onlyMeta, PixelDataOwner* pixels);

	// Out pixels must not be NULL if onlyMeta is set to false
	// NOTE: This function does not support ICO!
	bool ImageRead(ConstWString path, int& w, int& h, int& mips, ImagePixelFormat& fmt, ImageFileKind* outKind, bool onlyMeta, PixelDataOwner* outPixels);
	// NOTE: Image extension in path is ignored! Image type is picked based on 'kind' parameter
	// Mip count is ignored for every format except DDS
	bool ImageWrite(ConstWString path, int w, int h, int mips, ImagePixelFormat fmt, const PixelDataOwner& pixelData, ImageFileKind kind, float quality = 0.95f);

	// Low level function to resize array of image pixels
	// Images without alpha are faster to process because alpha channel is processed separately and can be ignored
	void ImageResize(
		pVoid dst, pVoid src, ResizeFilter filter, ImagePixelFormat fmt,
		int xFrom, int yFrom, int xTo, int yTo, bool hasAlphaPixels);

	// Only for non-compressed formats
	char* ImageGetPixel(char* pixelData, int x, int y, int width, ImagePixelFormat fmt);

	// Converts any pixel format (including block compressed formats) color to RGBA32
	ColorU32 ImageGetPixelColor(char* pixelData, int x, int y, int width, ImagePixelFormat fmt);

	// Converts pixels from one format to another, BC formats are not supported
	// Currently implemented:
	// RGBA -> RGB
	// RGB -> RGBA
	// Gray Alpha -> RGBA
	// Gray -> RGB
	void ImageConvertPixelFormat(pVoid dst, pVoid src, ImagePixelFormat fromFmt, ImagePixelFormat toFmt, int width, int height);

	// Flips image upside down
	void ImageFlipY(char* pixels, int width, int height, ImagePixelFormat fmt);

	// Swaps RGBA pixel component in given order
	void ImageDoSwizzle(char* pixelData, int width, int height, ImageSwizzle r, ImageSwizzle g, ImageSwizzle b, ImageSwizzle a);

	// NOTE: Following functions with 'RGBA' postfix expect input pixels in RGBA 32 bit format

	void ImageAdjustBrightnessAndContrastRGBA(char* pixelData, int width, int height, int brightness, int contrast);
	// Sets all pixels below threshold to 0 and all greater or equal to 255
	// Threshold must be between 0 and 255
	void ImageCutoutAlphaRGBA(char* pixelData, int width, int height, int threshold);
	void ImageScaleAlphaRGBA(char* pixelData, int width, int height, float alphaScale);
	// http://the-witness.net/news/2010/09/computing-alpha-mipmaps/
	float ImageAlphaTestCoverageRGBA(char* pixelData, int width, int height, int threshold, float alphaScale = 1.0f);
	// Uses brute force method to find most optimal alpha scaling to achieve desired coverage, idea is taken from NVTT
	float ImageAlphaTestFindBestScaleRGBA(char* pixelData, int width, int height, int threshold, float desiredCoverage);

	struct ImageInfo
	{
		ImagePixelFormat	PixelFormat;
		int					Width;
		int					Height;
		int					MipCount;
	};

	union ImagePixelData_
	{
		u8		U32[1][4];
		u8		U24[1][3];
		u8		U16[1][2];
		u8		U8[1][1];
		char	Bytes[1];
		u32		RGBA[1];
	};
	using ImagePixelData = ImagePixelData_*;

	// Options for creating ID3D11Texture2D and ID3D11ShaderResourceView
	struct ImageDX11ResourceOptions
	{
		// Mip maps will be created only if they don't exist 
		// already and pixel format is not compressed (BC#)
		bool CreateMips = true;
		// Image will be fit into given size rect
		int	 MaxResolution = 0;
		// Filter for generating mip maps, Box is good and fast option
		ResizeFilter	MipFilter = ResizeFilter_Box;
		// Image will be converted to suitable for displaying format if needed,
		// for example BC encoded images with size non power of two, or RGB images
		// Those needs to be converted to RGBA before creating DX resource
		bool AllowImageConversion = true;
		bool PadToPowerOfTwo = false;
		// Replaces initial pixel data with new if image was converted/padded/mip-mapped
		// A bit hacky but useful in some scenarios when we want to have full copy of GPU texture
		bool UpdateSourceImage = false;
	};

	// Optionally ref counted pixel data pointer
	// In some cases we may pass pixel data without moving ownership (for example if it's stack allocated)
	// NOTE: Pointer must be allocated via ImageAlloc. If it's not, see DeleteFn
	class PixelDataOwner
	{
		using pRefs = int*;

		pRefs m_RefCount; // Shared between all owned copies, pixel data is deleted when last instance is released
		pChar m_Data;
		bool  m_Owned;

		void AddRef() const;
		int  DecRef() const;

	public:
		PixelDataOwner();
		PixelDataOwner(const PixelDataOwner& other);
		PixelDataOwner(PixelDataOwner&& other) noexcept;
		~PixelDataOwner() { Release(); }

		ImagePixelData Data() const { return reinterpret_cast<ImagePixelData>(m_Data); }
		bool IsOwner() const { return m_Owned; }

		// Releases ref counter if owns memory and deletes pointer if ref count became 0
		// If pointer is not owned, data is set to NULL without deleting
		void Release();

		// Passed pixel data ownership remains untouched
		static PixelDataOwner CreateUnowned(pVoid data);
		// Passed pixel data ownership moves to this class instance 
		// and pointer will be deleted on destruction (when ref count becomes 0)
		static PixelDataOwner CreateOwned(pVoid data);

		static PixelDataOwner AllocateWithSize(u32 size);
		static PixelDataOwner AllocateForImage(int width, int height, ImagePixelFormat fmt, int mipCount = 1);

		PixelDataOwner& operator=(const PixelDataOwner& other);
		PixelDataOwner& operator=(PixelDataOwner&& other) noexcept;

		// Custom pointer delete function
		std::function<void(pVoid block)> DeleteFn;
	};

	class Image
	{
		friend class ImageFactory;

		wstring			 m_DebugName;
		Nullable<u32>	 m_FastHashKey;
		file::WPath		 m_FilePath;		// In case if image was loaded from file
		int				 m_Width;
		int				 m_Height;
		int				 m_MipCount;
		ImagePixelFormat m_PixelFormat;
		PixelDataOwner	 m_PixelData;
		Nullable<bool>	 m_HasAlphaPixels;

	public:
		Image(const PixelDataOwner& pixelData, ImagePixelFormat pixelFormat, int width, int height, int mipCount, bool copyPixels = false);
		Image(const PixelDataOwner& pixelData, const ImageInfo& imageInfo, bool copyPixels = false);
		Image(const Image& other) = delete;
		Image(Image&&) = default;

		ImageInfo        GetInfo() const;
		int              GetWidth() const { return m_Width; }
		int              GetHeight() const { return m_Height; }
		int              GetMipCount() const { return m_MipCount; }
		ImagePixelFormat GetPixelFormat() const { return m_PixelFormat; }

		bool IsBlockCompressed() const { return ImageIsCompressedFormat(m_PixelFormat); }

		u32 ComputeRowPitch() const { return ImageComputeRowPitch(m_Width, m_PixelFormat); }
		u32 ComputeSlicePitch() const { return ImageComputeSlicePitch(m_Width, m_Height, m_PixelFormat); }
		u32 ComputeTotalSizeWithMips() const { return ImageComputeTotalSizeWithMips(m_Width, m_Height, m_MipCount, m_PixelFormat); }

		u32 ComputeHashKey() const;

		// Set to image file name with extension by default
		// For memory images, 'None' is default
		ConstWString GetDebugName() const { return m_DebugName; }
		void		 SetDebugName(ConstWString newName) { m_DebugName = newName; }

		// NOTE: Might be null if pixel data was not loaded / previously failed to load
		// In case of DDS mip maps are placed next to each other
		PixelDataOwner GetPixelData(int mipIndex = 0) const;

		// Allows to lazy-load pixel data after loading image with just metadata
		bool EnsurePixelDataLoaded();

		// Path image was loaded from, only present if loaded using ImageFactory::LoadFromFile,
		// otherwise empty string (not null) will be returned
		ConstWString GetFilePath() const { return m_FilePath; }

		// Image may not have pixel data if loading failed
		bool HasPixelData() const { return GetPixelData().Data() != nullptr; }
		// Shortcut for GetPixelData().Data.Bytes
		char* GetPixelDataBytes(int mipIndex = 0) const { return GetPixelData(mipIndex).Data()->Bytes; }

		// See IsResolutionValidForMipMapsAndCompression
		bool CanBeCompressed() const { return ImageIsResolutionValidForMipMapsAndCompression(m_Width, m_Height); }
		// See IsResolutionValidForMipMaps
		bool CanGenerateMips() const { return ImageIsResolutionValidForMipMaps(m_Width, m_Height); }

		// Scans image for at least one non-opaque (transparent) pixel
		// BC formats are not currently supported, returned value is cached
		bool ScanAlpha();

		// Gets color of pixel at given coordinates,
		// works with any image format (including block compressed)
		ColorU32 GetPixel(int x, int y, int mipIndex = 0) const;

		// Creates a new image resized using specified method (filter)
		// NOTE: This image will create a new image even if dimensions match!
		amPtr<Image> Resize(int newWidth, int newHeight, ResizeFilter resizeFilter = ResizeFilter_Mitchell);
		// For cases when we want to generate mip maps for image that has non power-of-two resolution
		// (which is the case for most regular pictures from for e.g. internet)
		// the solution is to add empty padding pixels and draw using adjusted UV coordinates
		// If image is already sized power of two, clone is returned
		amPtr<Image> PadToPowerOfTwo(Vec2S& outUvExtent) const;
		Vec2S		 ComputePadExtent() const;
		// If format matches current, shallow clone is returned
		amPtr<Image> ConvertPixelFormat(ImagePixelFormat formatTo) const;
		// Will return existing pixel data if image already have more than 1 mip map or pixels are block compressed
		amPtr<Image> GenerateMipMaps(ResizeFilter mipFilter = ResizeFilter_Box);
		// Makes sure that image resolution is smaller equal to given constraint,
		// If maximumResolution is set to 0, shallow clone is returned
		amPtr<Image> FitMaximumResolution(int maximumResolution, ResizeFilter filter = ResizeFilter_Box);
		amPtr<Image> AdjustBrightnessAndContrast(int brightness, int contrast) const;

		// NOTE: This operation alters pixel data of THIS image!
		void Swizzle(ImageSwizzle r, ImageSwizzle g, ImageSwizzle b, ImageSwizzle a) const;

		// tex is optional parameter and reference is released if set to NULL
		bool CreateDX11Resource(
			amComPtr<ID3D11ShaderResourceView>& outView,
			const ImageDX11ResourceOptions& options = {},
			Vec2S* outUV2 = nullptr,
			amComPtr<ID3D11Texture2D>* outTex = nullptr);

		Image& operator=(const Image& other) const = delete;
		Image& operator=(Image&&) = default;
	};
	using ImagePtr = amPtr<Image>;

	/**
	 * \brief Image creator functions, must be used instead of calling constructor directly.
	 */
	class ImageFactory
	{
		// Makes sure that image resolution is less or equal than IMAGE_MAX_RESOLUTION, such images are not supported
		static bool		VerifyImageSize(const ImagePtr& image);
		static ImagePtr TryLoadFromPath(ConstWString path, bool onlyMeta);

	public:
		// Checks if image format (e.g. 'PNG') is supported and can be loaded,
		// given path may contain full absolute path or just file extension
		static ImageFileKind GetImageKindFromPath(ConstWString path);
		// See GetImageKindFromPath
		static bool IsSupportedImageFormat(ConstWString path) { return GetImageKindFromPath(path) != ImageKind_None; }

		// Loads image from file system, cache option keeps image in memory
		// See tl_ImagePreferredIcoResolution for ICO and tl_ImagePreferredSvgWidth & tl_ImagePreferredSvgHeight for SVG files
		static ImagePtr LoadFromPath(ConstWString path, bool onlyMeta = false, bool useCache = true);

		// Helper to load compressed image, at first it loads only image metadata to find matching compressed image in cache
		// without fully loading image, and loads image only in case if it's not in cache
		// NOTE: Pixels loaded from DDS, even uncompressed (for example RGBA) are returned as is, unless ImageCompressorOptions::AllowRecompress is set!
		static ImagePtr LoadFromPathAndCompress(
			ConstWString path, const ImageCompressorOptions& compOptions, CompressedImageInfo* outCompInfo = nullptr, ImageCompressorToken* token = nullptr);

		// Exists as separate loader because format does not quite fit in existing architecture
		// Only PNG and 32Bit BMP formats are supported
		static bool LoadIco(ConstWString path, List<ImagePtr>& icons);

		static ImagePtr LoadSvg(ConstWString path, int width, int height);

		// Copying pixels is only reasonable if given buffer data is temporary and may change
		static ImagePtr Create(const PixelDataOwner& pixelData, ImagePixelFormat fmt, int width, int height, bool copyPixels = false);

		// Creates image with reference on the existing pixel data without copying
		static ImagePtr Clone(const Image& image);
		// Copies pixel data, creating fully independent instance
		static ImagePtr Copy(const Image& image);

		// Purple/black tile, just like in source engine
		static ImagePtr CreateChecker_NotFound(int size = 512, int tileSize = 8) { return CreateChecker(COLOR_BLACK, COLOR_PINK, size, tileSize); }
		// Checker from photoshop / paint.net
		static ImagePtr CreateChecker_Opacity(int size = 512, int tileSize = 8) { return CreateChecker(COLOR_GRAY, COLOR_WHITE, size, tileSize); }
		static ImagePtr CreateChecker(ColorU32 color0, ColorU32 color1, int size = 512, int tileSize = 8);

		// atStringHash of path mixed with file modify time
		static u32 GetFastHashKey(ConstWString path);

		static bool CanBlockCompressImage(ConstWString path);

		// Format specific information:
		// - DDS:
		// Quality is ignored if given image is DDS and save file type is DDS.
		// If image is not DDS, it will be compressed using BC7 encoder and specified quality.
		// If image is DDS and converted to any other format, pixel data is decoded and only first mip map is written
		//
		// Quality used for:
		// - WEBP, lossless encoding if 1.0 is specified
		// - JPEG/JPEG
		//
		// If kind is ImageKind_None, image format will be parsed from given path
		// See WritableImageKinds for supported saving formats
		static bool SaveImage(ImagePtr img, ConstWString path, ImageFileKind toKind = ImageKind_None, float quality = 0.95f);

		// Hint for image resolution to load from .ICO file using LoadFromPath / LoadFromPathAndCompress function's
		// For ICO: If there's no such resolution, closest one is picked
		// If 0 is specified, largest available layer is selected
		static inline thread_local int tl_ImagePreferredIcoResolution = 0;
		static inline thread_local int tl_ImagePreferredSvgWidth = 256;
		static inline thread_local int tl_ImagePreferredSvgHeight = 256;
	};

#ifdef AM_IMAGE_USE_SIMD
	static const __m128i IMAGE_MASK_ALL = _mm_set_epi8(
		-1, -1, -1, -1,
		-1, -1, -1, -1,
		-1, -1, -1, -1,
		-1, -1, -1, -1);
	static const __m128i IMAGE_RGBA_ALPHA_MASK = _mm_set_epi8(
		-1, 0, 0, 0,
		-1, 0, 0, 0,
		-1, 0, 0, 0,
		-1, 0, 0, 0
	);
	static const __m128i IMAGE_RGBA_RGB_MASK = _mm_set_epi8(
		0, -1, -1, -1,
		0, -1, -1, -1,
		0, -1, -1, -1,
		0, -1, -1, -1
	);
	static const __m128i IMAGE_RGB_TO_RGBA_SHUFFLE = _mm_set_epi8(
		// Alpha value doesn't matter here
		0, 11, 10, 9,
		0, 8, 7, 6,
		0, 5, 4, 3,
		0, 2, 1, 0
	);
	static const __m128i IMAGE_RGBA_TO_RGB_SHUFFLE = _mm_set_epi8(
		// Remainder from 4 alpha channels
		0, 0, 0, 0,
		14, 13, 12,
		10, 9, 8,
		6, 5, 4,
		2, 1, 0
	);
#ifdef AM_IMAGE_USE_AVX2
	static const __m256i IMAGE_RGBA_TO_RGB_SHUFFLE_256 = _mm256_set_epi8(
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
#endif // AM_IMAGE_USE_AVX2
#endif // AM_IMAGE_USE_SIMD
}
