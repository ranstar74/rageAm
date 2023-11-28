//
// File: image.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <d3d11.h>
#include <DirectXTex.h>
#include <stb_image.h>

#include "am/system/ptr.h"
#include "am/types.h"

namespace rageam::graphics
{
	// TODO:
	// - Cutout alpha
	// - Resize threading
	// - Alpha test coverage
	// - We can't track memory allocated by stb/webp/dds loaders
	// - tiff
	// - 'Large' allocator for images?

#define AM_IMAGE_USE_AVX2_RESIZE

	class PixelDataOwner;

	static constexpr int MAX_IMAGE_RESOLUTION = 1 << 14;	// Maximum supported by DX11
	static constexpr int MAX_MIP_MAPS = 13;					// 16384 -> 4

	// Those allocation functions must be used for correct memory use tracking
	pVoid ImageAlloc(u32 size);
	pVoid ImageAllocTemp(u32 size);
	void ImageFree(pVoid block);
	void ImageFreeTemp(pVoid block);

	enum ImageKind
	{
		ImageKind_None,			// Not loaded or created from memory
		ImageKind_WEBP,
		ImageKind_JPEG,
		ImageKind_JPG,
		ImageKind_PNG,
		ImageKind_TGA,
		ImageKind_BMP,
		ImageKind_PSD,
		ImageKind_DDS,
		ImageKind_ICO,

		ImageKind_COUNT
	};

	// Formats that are supported for writing (saving) to file
	static constexpr u32 WriteableImageKinds =
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
	};

	enum ImagePixelFormat
	{
		ImagePixelFormat_None,

		ImagePixelFormat_U32,	// RGBA			U8[4]
		ImagePixelFormat_U24,	// RGB			U8[3]
		ImagePixelFormat_U16,	// Gray Alpha	U8[2]
		ImagePixelFormat_U8,	// Gray			U8[1]

		// All compressed formats are UNORM

		ImagePixelFormat_BC1,
		ImagePixelFormat_BC2,
		ImagePixelFormat_BC3,
		ImagePixelFormat_BC4,
		ImagePixelFormat_BC5,
		ImagePixelFormat_BC7,
	};

	static constexpr int ImagePixelFormatBitsPerPixel[] =
	{
		0,
		32, 24, 16, 8,			// U32, U24, U16, U8
		4, 8, 8, 4, 8, 8,		// BC1, BC2, BC3, BC4, BC5, BC7
	};

	// Gets whether given format is one of ImagePixelFormat_BC (block compressed)
	bool IsCompressedPixelFormat(ImagePixelFormat fmt);
	// Gets whether format supports alpha channel
	bool IsAlphaPixelFormat(ImagePixelFormat fmt);

	// NOTE: Not every pixel format (such as U24) can be converted to DXGI,
	// DXGI_FORMAT_UNKNOWN will be returned for such
	DXGI_FORMAT ImagePixelFormatToDXGI(ImagePixelFormat fmt);
	ImagePixelFormat ImagePixelFormatFromDXGI(DXGI_FORMAT fmt);

	// Total number of bytes image takes with given resolution and format
	u32 ComputeImageSlicePitch(int width, int height, ImagePixelFormat pixelFormat);
	// Total number of bytes one pixel row (scanline) takes with given resolution and format
	// Also known as 'stride'
	u32 ComputeImageRowPitch(int width, ImagePixelFormat pixelFormat);
	// Accumulates total size of image and mip maps
	u32 ComputeImageSizeWithMips(int width, int height, int mipCount, ImagePixelFormat pixelFormat);

	// Makes sure that resolution is power of two and greater than 4x4 (lowest size for block compression)
	bool IsResolutionValidForMipMapsAndCompression(int width, int height);
	// Non-power of two images cannot be properly scaled down
	bool IsResolutionValidForMipMaps(int width, int height);

	// Gets num of mip maps up until 4x4 (lowest size for block compression)
	int ComputeMaximumMipCountForResolution(int width, int height);

	// Checks if any pixel has transparent alpha value (non 255)
	// TODO: BC Decoding
	bool ScanPixelsForAlpha(pVoid data, int width, int height, ImagePixelFormat fmt);

	// Helper utility for stb_image_write
	// If kind is ImageKind_None, it is defaulted to ImageKind_PNG
	// Quality is only used on JPEG
	bool WriteImageStb(ConstWString path, ImageKind kind, int w, int h, int c, pVoid data, int quality = 100);
	// NOTE: Data must be in RGBA U32 or RGB U24 format! If quality is 100.0, lossless format is used
	bool WriteImageWebp(ConstWString path, int w, int h, pVoid data, bool hasAlpha, float quality = 100.0f);
	
	// Low level function to resize array of image pixels
	// Images without alpha are faster to process
	void ResizeImagePixels(
		pVoid dst, pVoid src, ResizeFilter filter, ImagePixelFormat fmt, 
		int xFrom, int yFrom, int xTo, int yTo, bool hasAlphaPixels);

	// Converts pixels from one format to another, BC formats are not supported
	// Currently implemented:
	// RGBA -> RGB
	// RGB -> RGBA
	void ConvertImagePixels(pVoid dst, pVoid src, ImagePixelFormat fromFmt, ImagePixelFormat toFmt, int width, int height);

	struct ImageInfo
	{
		ImagePixelFormat	PixelFormat;	// For correctly interpreting PixelData
		int					Width;
		int					Height;
		int					MipCount = 1;	// Used on DDS and Ico
	};

	union PixelData_
	{
		u8		U32[1][4];
		u8		U24[1][3];
		u8		U16[1][2];
		u8		U8[1][1];
		char	Bytes[1];
	};
	using PixelData = PixelData_*;

	// Options for creating ID3D11Texture2D and ID3D11ShaderResourceView
	struct ShaderResourceOptions
	{
		// Mip maps will be created only if they don't exist 
		// already and pixel format is not compressed (BC#)
		bool			CreateMips;
		// For .Ico mip maps are not really 'scaled' down versions of the image,
		// if we use them as actual 'mip maps', they would look really, really, weird
		// As solution for this we generate mip maps based on specified mip index
		int				BaseMipIndex = -1;
		// Filter for generating mip maps, Box is good and fast option
		ResizeFilter	MipFilter = ResizeFilter_Box;
	};

	// Optionally ref counted pixel data pointer
	// In some cases we may pass pixel data without moving ownership (for example if it's stack allocated)
	// NOTE: Pointer must be allocated via ImageAlloc. If it's not, see DeleteFn
	class PixelDataOwner
	{
		using pRefs = int*;

		pRefs	m_RefCount; // Shared between all owned copies, pixel data is deleted when last instance is released
		pChar	m_Data;
		bool	m_Owned;

		void AddRef() const;
		int DecRef() const;

	public:
		PixelDataOwner();
		PixelDataOwner(const PixelDataOwner& other);
		PixelDataOwner(PixelDataOwner&& other) noexcept;
		~PixelDataOwner() { Release(); }

		PixelData Data() const { return reinterpret_cast<PixelData>(m_Data); }
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

	class IImage
	{
		friend class ImageFactory;

		u32			m_Stride = 0;
		u32			m_Slice = 0;
		u32			m_SizeWithMips = 0;
		ImageKind	m_Kind = ImageKind_None;
		wstring		m_DebugName = L"None";
		bool		m_HasAlpha = false;

	protected:
		// Must be called after any image property was changed
		void PostLoadCompute();
		// forceHasAlpha allows to directly set value without scanning
		void ComputeHasAlpha(const bool* forceHasAlpha = nullptr);

	public:
		virtual ~IImage() = default;

		// Full metadata information of the image file
		virtual ImageInfo GetInfo() const = 0;
		virtual int GetWidth() const = 0;
		virtual int GetHeight() const = 0;
		// NOTE: Might be null if pixel data was not loaded / previously failed to load
		// In case of DDS and Ico, mip maps are placed next to each other
		virtual PixelDataOwner GetPixelData(int mipIndex = 0) const = 0;
		// If generateMips is true, attempts to generate them (if possible)
		// Generating mip maps will fail if image resolution is not power-of-two
		// NOTE: DDS and Ico do not generate mip maps
		// Generated mip data is not cached anywhere and created every call
		virtual PixelDataOwner CreatePixelDataWithMips(bool generateMips, ResizeFilter mipFilter, int& outMipCount) const;

		// Image may not have pixel data if loading failed
		bool HasPixelData() const { return GetPixelData().Data() != nullptr; }

		// By default set to image file name with extension. 
		// For memory images, 'None' is default
		ConstWString GetDebugName() const { return m_DebugName; }
		void SetDebugName(ConstWString newName) { m_DebugName = newName; }

		// See IsResolutionValidForMipMapsAndCompression
		bool CanBeCompressed() const;
		// See IsResolutionValidForMipMaps
		bool CanGenerateMips() const;

		// Number of bytes one pixel row (scanline) takes
		auto GetStride() const { return m_Stride; }
		// Number of bytes whole image takes
		auto GetSlicePitch() const { return m_Slice; }
		// Number of bytes all mip maps take, will be different from GetSlicePitch()
		// only for DDS and Ico because they may have multiple mip maps
		auto GetSizeWithMips() const { return m_SizeWithMips; }
		// Kind of loaded image (png/jpeg/dds)
		auto GetKind() const { return m_Kind; }
		// Whether at least one pixel has non 255 alpha value
		// NOTE: This function will always return false for DDS!
		bool HasAlpha() const { return m_HasAlpha; }

		// Creates a new image resized using specified method (filter)
		amPtr<IImage> Resize(int newWidth, int newHeight, ResizeFilter resizeFilter = ResizeFilter_Mitchell) const;
		// For cases when we want to generate mip maps for image that has non power-of-two resolution
		// (which is the case for most regular pictures from for e.g. internet)
		// the solution is to add empty padding pixels and draw using adjusted UV coordinates
		amPtr<IImage> PadToPowerOfTwo(Vec2S& outUvExtent) const;

		// tex is optional parameter and reference is released if set to NULL
		bool CreateDX11Resource(
			const ShaderResourceOptions& options,
			amComPtr<ID3D11ShaderResourceView>& view,
			amComPtr<ID3D11Texture2D>* tex = nullptr) const;
	};
	using ImagePtr = amPtr<IImage>;

	/**
	 * \brief Image that can be loaded from file system.
	 */
	class IImageFile : public IImage
	{
	public:
		virtual bool LoadPixelData(ConstWString path) = 0;
		// Loads ONLY meta information from image file,
		// much faster way if pixel data is not required
		virtual bool LoadMetaData(ConstWString path) = 0;
	};

	/**
	 * \brief Image created from pixel array.
	 */
	class ImageMemory : public IImage
	{
		PixelDataOwner		m_PixelData;
		int					m_Width;
		int					m_Height;
		ImagePixelFormat	m_PixelFormat;
	public:
		ImageMemory(const PixelDataOwner& pixelData, ImagePixelFormat fmt, int width, int height, bool copyPixels = false);

		ImageInfo GetInfo() const override;
		int GetWidth() const override { return m_Width; }
		int GetHeight() const override { return m_Height; }
		PixelDataOwner GetPixelData(int mipIndex = 0) const override;
	};

	/**
	 * \brief stb_image.h wrapper, supported formats: JPEG, PNG, TGA, BMP, PSD.
	 */
	class Image_Stb : public IImageFile
	{
		stbi_uc* m_PixelData = nullptr;

		int m_Width = 0;
		int m_Height = 0;
		int m_NumChannels = 0;
	public:
		~Image_Stb() override;

		bool LoadPixelData(ConstWString path) override;
		bool LoadMetaData(ConstWString path) override;
		ImageInfo GetInfo() const override;
		int GetWidth() const override { return m_Width; }
		int GetHeight() const override { return m_Height; }
		PixelDataOwner GetPixelData(int mipIndex) const override;
	};

	/**
	 * \brief Windows .ico files, only 32 bit PNG is supported + mip maps.
	 */
	class Image_Ico : public IImageFile
	{
		// TODO:
	};

	class Image_Webp : public IImageFile
	{
		static constexpr ImagePixelFormat WEBP_PIXEL_FORMAT = ImagePixelFormat_U32; // RGBA

		int				m_Width = 0;
		int				m_Height = 0;
		PixelDataOwner	m_PixelData;
	public:

		bool LoadMetaData(ConstWString path) override;
		bool LoadPixelData(ConstWString path) override;
		ImageInfo GetInfo() const override;
		int GetWidth() const override { return m_Width; }
		int GetHeight() const override { return m_Height; }
		PixelDataOwner GetPixelData(int mipIndex) const override { return m_PixelData; }
	};

	/**
	 * \brief Block compressed (in most cases) image format that includes mip maps.
	 */
	class Image_DDS : public IImageFile
	{
		int						m_Width = 0;
		int						m_Height = 0;
		int						m_MipCount = 0;
		ImagePixelFormat		m_PixelFormat = ImagePixelFormat_None;
		PixelDataOwner			m_PixelData;	// When created from memory
		DirectX::ScratchImage	m_ScratchImage;	// When loaded from .DDS file
		bool					m_LoadedFromFile = false;

		void SetFromMetadata(const DirectX::TexMetadata& meta);

	public:
		Image_DDS() = default;
		Image_DDS
		(const PixelDataOwner& pixelData, ImagePixelFormat fmt,
			int width, int height, int mipCount, bool copyPixels = false);

		bool LoadMetaData(ConstWString path) override;
		bool LoadPixelData(ConstWString path) override;

		ImageInfo GetInfo() const override;
		int GetWidth() const override { return m_Width; }
		int GetHeight() const override { return m_Height; }

		PixelDataOwner GetPixelData(int mipIndex = 0) const override;
		PixelDataOwner CreatePixelDataWithMips(bool generateMips, ResizeFilter mipFilter, int& outMipCount) const override;
	};

	/**
	 * \brief Image creator functions, must be used instead of calling constructor directly.
	 */
	class ImageFactory
	{
		// Makes sure that image resolution is less or equal than MAX_IMAGE_RESOLUTION, such images are not supported
		static bool VerifyImageSize(const ImagePtr& img);
		static ImagePtr TryLoadFromPath(ConstWString path, bool onlyMeta);

	public:
		// Checks if image format (e.g. 'PNG') is supported and can be loaded,
		// given path may contain full absolute path or just file extension
		static ImageKind GetImageKindFromPath(ConstWString path);
		// See GetImageKindFromPath
		static bool IsSupportedImageFormat(ConstWString path) { return GetImageKindFromPath(path) != ImageKind_None; }

		// Loads image from file system
		static ImagePtr LoadFromPath(ConstWString path, bool onlyMeta = false);

		// Copying pixels is only reasonable if given buffer data is temporary and may change
		static ImagePtr CreateFromMemory(
			const PixelDataOwner& pixelData, ImagePixelFormat fmt, int width, int height, bool copyPixels = false);

		// False is returned if:
		// - Pixel data is NULL
		// - Given format was incompatible
		// - Failed to open file for writing
		// NOTE: Given file extension in path is ignored and replaced to format one!
		//
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
		// If kind is ImageKind_None, image format is attempt to be parsed from given path, if failed, original image format is used
		// See WritableImageKinds for supported saving formats
		static bool SaveImage(const ImagePtr& img, ConstWString path, ImageKind toKind = ImageKind_None, float quality = 0.95f);
	};
}
