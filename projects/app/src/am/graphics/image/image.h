//
// File: image.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <dxgiformat.h>
#include <d3d11.h>

#include "stb_image.h"
#include "am/system/ptr.h"
#include "am/types.h"

namespace rageam::graphics
{
	enum ImageKind : u32
	{
		// Formats are present in form of bit mask to quickly check
		// if image implementation supports such extension

		ImageKind_None = 0,			// Not loaded or created from memory
		ImageKind_WEBP = 1 << 0,
		ImageKind_JPEG = 1 << 1,
		ImageKind_JPG = 1 << 2,
		ImageKind_PNG = 1 << 3,
		ImageKind_TGA = 1 << 4,
		ImageKind_BMP = 1 << 5,
		ImageKind_PSD = 1 << 6,
		ImageKind_DDS = 1 << 7,
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

	// Num bytes
	static constexpr u32 ImagePixelFormatToSize[] = 
	{
		0,					// None
		4, 3, 2, 1,			// U32, U24, U16, U8
		// TODO:
	};

	static constexpr bool IsCompressedPixelFormat[] =
	{
		0,					// None
		0, 0, 0, 0,			// U32, U24, U16, U8
		1, 1, 1, 1, 1, 1	// BC1, BC2, BC3, BC4, BC5
	};

	// NOTE: Not every pixel format (such as U24) can be converted to DXGI,
	// DXGI_FORMAT_UNKNOWN will be returned for such
	DXGI_FORMAT ImagePixelFormatToDXGI(ImagePixelFormat fmt);

	// Total number of bytes image takes with given resolution and format
	u32 ComputeImageSlicePitch(int width, int height, ImagePixelFormat pixelFormat);
	// Total number of bytes one pixel row (scanline) takes with given resolution and format
	u32 ComputeImageStride(int width, int height, ImagePixelFormat pixelFormat);

	// Makes sure that resolution is power of two and greater than 4x4 (lowest size for block compression)
	bool IsResolutionValidForMipMapsAndCompression(int width, int height);
	// Non-power of two images canno't be properly scaled down
	bool IsResolutionValidForMipMaps(int width, int height);

	// Gets num of mip maps up until 4x4 (lowest size for block compression)
	int GetMaximumMipCountForResolution(int width, int height);

	// Helper utility for stb_image_write
	// If kind is ImageKind_None, it is defaulted to ImageKind_PNG
	bool WriteImageStb(ConstWString path, ImageKind kind, int w, int h, int c, pVoid data);

	struct ImageInfo
	{
		ImagePixelFormat	PixelFormat;	// For correctly interpreting PixelData
		int					Width;
		int					Height;
		int					MipCount = 1;	// Used on DDS and Ico
	};
	
	union PixelData_
	{
		u8		U32[4][1];
		u8		U24[3][1];
		u8		U16[2][1];
		u8		U8[1][1];
		pChar	Bytes;
	};
	using PixelData = PixelData_*;

	// Options for creating ID3D11Texture2D and ID3D11ShaderResourceView
	struct ShaderResourceOptions
	{
		// Mip maps will be created only if they don't exist 
		// already and pixel format is not compressed (BC#)
		bool CreateMips;
	};

	// In some cases we may pass pixel data without moving ownership (for example if it's stack allocated)
	// NOTE: Pointer must be allocated via operator new[]
	struct PixelDataOwner
	{
		PixelData	Data;
		bool		Owned;

		~PixelDataOwner()
		{
			if (!Owned) delete[] Data;
			Data = nullptr;
			Owned = false;
		}

		// Passed pixel data ownership remains untouched
		static PixelDataOwner CreateUnowned(pVoid data)
		{
			PixelDataOwner owner;
			owner.Owned = false;
			owner.Data = reinterpret_cast<PixelData>(data);
			return owner;
		}

		// Passed pixel data ownership moves to this class instance 
		// and pointer will be deleted on destruction
		static PixelDataOwner CreateOwned(pVoid data)
		{
			PixelDataOwner owner;
			owner.Owned = true;
			owner.Data = reinterpret_cast<PixelData>(data);
			return owner;
		}
	};

	class IImage
	{
		friend class ImageFactory;

		u32			m_Stride = 0;
		u32			m_Slice = 0;
		ImageKind	m_Kind = ImageKind_None;
		wstring		m_DebugName = L"None";

	protected:
		// Must be called after any image property was changed
		virtual void PostLoadCompute();

	public:
		virtual ~IImage() = default;

		// False is returned if:
		// - Pixel data is NULL
		// - Given format was incompatible
		// - Failed to open file for writing
		// NOTE: Given file extension in path is ignored and replaced to format one!
		// If kind is ImageKind_None, image format is chosen automatically
		virtual bool Save(ConstWString path, ImageKind kind = ImageKind_None) = 0;
		// Full metadata information of the image file
		virtual ImageInfo GetInfo() const = 0;
		virtual int GetWidth() const = 0;
		virtual int GetHeight() const = 0;
		// NOTE: Might be null if pixel data was not loaded / previously failed to load
		// In case of DDS and Ico, mip maps are placed next to each other
		virtual PixelDataOwner GetPixelData(int mipIndex = 0) const = 0;
		// Image is block compressed (DDS)
		virtual bool IsCompressed() const { return false; }
		// If generateMips is true, attempts to generate them (if possible)
		// Generating mip maps will fail if image resolution is not power-of-two
		// NOTE: DDS and Ico do not generate mip maps
		// Generated mip data is not cached anywhere and created every call
		virtual PixelDataOwner CreatePixelDataWithMips(bool generateMips) const;

		// By default set to image file name with extension. 
		// For memory images, 'None' is default
		ConstWString GetDebugName() const { return m_DebugName; }
		void SetDebugName(ConstWString newName) { m_DebugName = newName; }

		// See IsResolutionValidForMipMapsAndCompression
		bool CanBeCompressed() const;

		// Number of bytes one pixel row (scanline) takes
		auto GetStride() const { m_Stride; }
		// Number of bytes whole image takes
		auto GetSlicePitch() const { return m_Slice; }
		// Kind of loaded image (png/jpeg/dds)
		auto GetKind() const { return m_Kind; }

		// Creates a new image resized using Lanczos method 
		amPtr<IImage> Resize(int newWidth, int newHeight) const;

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
		amUniquePtr<char>	m_PixelData;
		int					m_Width;
		int					m_Height;
		ImagePixelFormat	m_PixelFormat;
	public:
		// Move memory transfers pixel data pointer ownership to this class instance
		// Otherwise pixel data is copied
		ImageMemory(pVoid pixelData, ImagePixelFormat fmt, int width, int height, bool moveMemory);

		// Not supported types: DDS, PSD
		// Automatic format: PNG
		bool Save(ConstWString path, ImageKind kind = ImageKind_None) override;

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

		// Not supported types: DDS, PSD
		// Automatic format: PNG
		bool Save(ConstWString path, ImageKind kind = ImageKind_None) override;

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
		// TODO: 
	};

	/**
	 * \brief Block compressed (in most cases) image format that includes mip maps.
	 */
	class Image_DDS : public IImageFile
	{
	public:
		Image_DDS() = default;
		Image_DDS(pVoid pixelData, ImagePixelFormat fmt, int width, int height, int mipCount, bool moveMemory);

		bool IsCompressed() const override { return true; }

		PixelDataOwner CreatePixelDataWithMips(bool generateMips) const override;
	};

	class ImageFactory
	{
	public:
		// Loads image from file system
		static ImagePtr LoadFromPath(ConstWString path, bool onlyMeta = false);
		// See ImageMemory constructor for moveMemory param
		static ImagePtr CreateFromMemory(pVoid pixelData, ImagePixelFormat fmt, int width, int height, bool moveMemory);
	};
}
