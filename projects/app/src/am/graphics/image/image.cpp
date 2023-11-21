#include "image.h"

#include "bc.h"
#include "resize.h"
#include "am/types.h"
#include "am/file/fileutils.h"
#include "am/file/pathutils.h"
#include "am/system/enum.h"

#include "stb_image_write.h"

DXGI_FORMAT rageam::graphics::ImagePixelFormatToDXGI(ImagePixelFormat fmt)
{
	switch (fmt)
	{
	case ImagePixelFormat_U32:	return DXGI_FORMAT_R8G8B8A8_UNORM;
	case ImagePixelFormat_U24:	return DXGI_FORMAT_UNKNOWN;
	case ImagePixelFormat_U8:	return DXGI_FORMAT_A8_UNORM;

	case ImagePixelFormat_BC1:	return DXGI_FORMAT_BC1_UNORM;
	case ImagePixelFormat_BC2:	return DXGI_FORMAT_BC2_UNORM;
	case ImagePixelFormat_BC3:	return DXGI_FORMAT_BC3_UNORM;
	case ImagePixelFormat_BC4:	return DXGI_FORMAT_BC4_UNORM;
	case ImagePixelFormat_BC5:	return DXGI_FORMAT_BC5_UNORM;
	case ImagePixelFormat_BC7:	return DXGI_FORMAT_BC7_UNORM;

	case ImagePixelFormat_None:
	case ImagePixelFormat_U16:	return DXGI_FORMAT_R8G8_UNORM;
	}
	AM_UNREACHABLE("ImagePixelFormatToDXGI() -> Format '%s' is not implemented.", Enum::GetName(fmt));
}

u32 rageam::graphics::ComputeImageByteWidth(int width, int height, ImagePixelFormat pixelFormat)
{
	u32 pixelStride = ImagePixelFormatToSize[pixelFormat];
	return pixelStride * width * height;
}

bool rageam::graphics::WriteImageStb(ConstWString path, ImageKind kind, int w, int h, int c, pVoid data)
{
	if (kind == ImageKind_DDS || kind == ImageKind_PSD)
	{
		AM_ERRF("ImageMemory::Save() -> Formats DDS and PSD are not supported.");
		return false;
	}

	if (kind == ImageKind_None)
		kind = ImageKind_PNG;

	file::U8Path uPath = file::PathConverter::WideToUtf8(path);

	switch (kind)
	{
	case ImageKind_PNG:		stbi_write_png(uPath, w, h, c, data, w * c);	break;
	case ImageKind_BMP:		stbi_write_bmp(uPath, w, h, c, data);			break;
	case ImageKind_JPEG:
	case ImageKind_JPG:		stbi_write_jpg(uPath, w, h, c, data, 100);		break;
	case ImageKind_TGA:		stbi_write_tga(uPath, w, h, c, data);			break;
	case ImageKind_WEBP:	AM_UNREACHABLE("ImageMemory::Save() -> Webp is not implemented."); break; // TODO: ...

	default: break;
	}
	return true;
}

void rageam::graphics::IImage::PostLoadCompute()
{
	ImageInfo info = GetInfo();
	m_ByteWidth = ComputeImageByteWidth(info.Width, info.Height, info.PixelFormat);
}

bool rageam::graphics::IImage::CanBeCompressed() const
{
	ImageInfo info = GetInfo();
	return ImageCompressor::CanBeCompressed(info.Width, info.Height);
}

amPtr<rageam::graphics::IImage> rageam::graphics::IImage::Resize(int newWidth, int newHeight) const
{
	// Allocate buffer for rescaled image, resize it and create new image from this buffer
	ImageInfo info = GetInfo();
	u32 newSize = ComputeImageByteWidth(newWidth, newHeight, info.PixelFormat);
	char* buffer = new char[newSize];
	ImageResizer::Resample(GetPixelData(), buffer, info.PixelFormat, info.Width, info.Height, newWidth, newHeight);
	return ImageFactory::CreateFromMemory(buffer, info.PixelFormat, newWidth, newHeight, true); // Pass pointer ownership
}

bool rageam::graphics::IImage::CreateDX11Resource(amComPtr<ID3D11ShaderResourceView>& view, amComPtr<ID3D11Texture2D>* tex)
{
	return false; // TODO: ...
}

rageam::graphics::ImageMemory::ImageMemory(pVoid pixelData, ImagePixelFormat fmt, int width, int height, bool moveMemory)
{
	m_Width = width;
	m_Height = height;
	m_PixelFormat = fmt;

	if (moveMemory)
	{
		m_PixelData = amUniquePtr<char>(static_cast<char*>(pixelData));
	}
	else
	{
		u32 sliceWidth = GetByteWidth();
		m_PixelData = amUniquePtr<char>(new char[sliceWidth]);
		memcpy(m_PixelData.get(), pixelData, sliceWidth);
	}
	IImage::PostLoadCompute();
}

bool rageam::graphics::ImageMemory::Save(ConstWString path, ImageKind kind)
{
	int channelCount = static_cast<int>(ImagePixelFormatToSize[m_PixelFormat]);

	return WriteImageStb(path, kind, m_Width, m_Height, channelCount, m_PixelData.get());
}

rageam::graphics::ImageInfo rageam::graphics::ImageMemory::GetInfo() const
{
	ImageInfo info;
	info.Width = m_Width;
	info.Height = m_Height;
	info.PixelFormat = m_PixelFormat;
	return info;
}

rageam::graphics::ImageData* rageam::graphics::ImageMemory::GetPixelData() const
{
	return reinterpret_cast<ImageData*>(m_PixelData.get());
}

rageam::graphics::Image_Stb::~Image_Stb()
{
	stbi_image_free(m_PixelData);
	m_PixelData = nullptr;
}

bool rageam::graphics::Image_Stb::Save(ConstWString path, ImageKind kind)
{
	return WriteImageStb(path, kind, m_NumChannels, m_Width, m_Height, m_PixelData);
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
	file::CloseFileStream(fs);

	PostLoadCompute();

	// Pixel data will be null if image is corrupted
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

rageam::graphics::ImageData* rageam::graphics::Image_Stb::GetPixelData() const
{
	return reinterpret_cast<ImageData*>(m_PixelData);
}

rageam::graphics::Image_DDS::Image_DDS(pVoid pixelData, ImagePixelFormat fmt, int width, int height, int mipCount, bool moveMemory)
{

}

rageam::graphics::ImagePtr rageam::graphics::ImageFactory::LoadFromPath(ConstWString path, bool onlyMeta)
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
		image = std::make_shared<Image_Stb>(); break;

	case Hash("webp"):
		kind = ImageKind_WEBP;
		// image = std::make_shared<Image_Webp>(); break;
		break;
	case Hash("dds"):
		kind = ImageKind_DDS;
		// image = std::make_shared<Image_DDS>(); break;
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
	return image;
}

rageam::graphics::ImagePtr rageam::graphics::ImageFactory::CreateFromMemory(
	pVoid pixelData, ImagePixelFormat fmt, int width, int height, bool moveMemory)
{
	return std::make_shared<ImageMemory>(pixelData, fmt, width, height, moveMemory);
}
