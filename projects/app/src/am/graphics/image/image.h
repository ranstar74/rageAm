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
#include "common/types.h"

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
		0,			// None
		4, 3, 2, 1,	// U32, U24, U16, U8
		// TODO:
	};

	// NOTE: Not every pixel format (such as U24) can be converted to DXGI,
	// DXGI_FORMAT_UNKNOWN will be returned for such
	DXGI_FORMAT ImagePixelFormatToDXGI(ImagePixelFormat fmt);

	u32 ComputeImageByteWidth(int width, int height, ImagePixelFormat pixelFormat);

	// Helper utility for stb_image_write
	// If kind is ImageKind_None, it is defaulted to ImageKind_PNG
	bool WriteImageStb(ConstWString path, ImageKind kind, int w, int h, int c, pVoid data);

	struct ImageInfo
	{
		ImagePixelFormat	PixelFormat; // For correctly interpreting ImageData
		int					Width;
		int					Height;
	};
	
	union ImageData
	{
		u8		U32[4][1];
		u8		U24[3][1];
		u8		U16[2][1];
		u8		U8[1][1];
		pVoid	Bytes;
	};

	class IImage
	{
		friend class ImageFactory;

		u32			m_ByteWidth = 0;
		ImageKind	m_Kind = ImageKind_None;

	protected:
		// Must be called after width/height/pixelformat changed
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
		// Metadata information of the image file
		virtual auto GetInfo() const -> ImageInfo = 0;
		// NOTE: Might be null if pixel data was not loaded / previously failed to load 
		virtual auto GetPixelData() const -> ImageData* = 0;
		// Image is block compressed (DDS)
		virtual bool IsCompressed() const { return false; }

		// See ImageCompressor::CanBeCompressed
		bool CanBeCompressed() const;

		// Number of bytes whole image takes
		auto GetByteWidth() const { return m_ByteWidth; }
		// Kind of loaded image (png/jpeg/dds)
		auto GetKind() const { return m_Kind; }

		// Creates a new image resized using Lanczos method 
		amPtr<IImage> Resize(int newWidth, int newHeight) const;

		// tex is optional parameter and reference is released if set to NULL
		bool CreateDX11Resource(
			amComPtr<ID3D11ShaderResourceView>& view, 
			amComPtr<ID3D11Texture2D>* tex = nullptr);
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
		ImageData* GetPixelData() const override;
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
		bool Save(ConstWString path, ImageKind kind) override;

		bool LoadPixelData(ConstWString path) override;
		bool LoadMetaData(ConstWString path) override;
		ImageInfo GetInfo() const override;
		ImageData* GetPixelData() const override;
	};

	class Image_Webp : public IImageFile
	{
		// TODO: 
	};

	class Image_DDS : public IImageFile
	{
	public:
		Image_DDS() = default;
		Image_DDS(pVoid pixelData, ImagePixelFormat fmt, int width, int height, int mipCount, bool moveMemory);

		bool IsCompressed() const override { return true; }
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
