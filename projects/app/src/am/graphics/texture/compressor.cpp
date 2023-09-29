#include "compressor.h"

#include "nvtthelper.h"
#include "am/file/fileutils.h"
#include "am/graphics/render/engine.h"
#include "am/string/string.h"
#include "rage/atl/array.h"

void rageam::texture::Compressor::FindBestFormat()
{
	// Block compression (dxt/bc) require texture dimensions to be multiple of 4
	m_SupportBlockCompression = m_Width % 4 == 0 && m_Height % 4 == 0;
	if (!m_SupportBlockCompression)
	{
		m_BestFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
		return;
	}

	// Automatic compression detection
	// Color / Specular			DXT1, BC7
	// Color maps w opacity		DXT1, DXT3, DXT5, BC7
	// Normal					DXT1, BC5

	// DXT3 better fits for images that have very sharp opacity, while for DXT5 opacity has to be slowly faded
	// One good approach to choose right one is using IQR (basically - sample data distribution)

	rage::atArray<u8, u32> sample; // TODO: This is pretty expensive, do we need such detection?

	float* alpha = m_Image.channel(nvtt::Alpha);
	u32 alphaPixels = m_PixelCount / 4;
	for (u32 i = 0; i < alphaPixels; i++)
	{
		float value = *alpha;

		alpha++; // Colors are grouped by channel (see comment for nvtt::Surface)

		// We're sampling only transparent pixels, otherwise data will be biased
		if (value == 1.0f)
			continue;

		// We've got at least one transparent pixel, pre-allocate sample array
		if (sample.GetSize() == 0)
			sample.Reserve(m_PixelCount);

		sample.Add(static_cast<u8>(value * 255)); // Convert to single byte to reduce used memory
	}
	sample.Sort(); // We need samples to be sorted to calculate median

	u32 sampleSize = sample.GetSize();
	if (sampleSize != 0) // If there's at least one transparent pixel...
	{
		// Lazy median
		u8 firstQuartile = sample[sampleSize / 4];
		u8 secondQuartile = sample[sampleSize / 2 + sampleSize / 4];

		u8 iqr = secondQuartile - firstQuartile;

		m_HasAlpha = true;
		m_SharpAlpha = iqr <= 50;
	}

	if (!m_HasAlpha)		m_BestFormat = DXGI_FORMAT_BC1_UNORM;
	else if (m_SharpAlpha)	m_BestFormat = DXGI_FORMAT_BC2_UNORM;
	else					m_BestFormat = DXGI_FORMAT_BC3_UNORM;
}

bool rageam::texture::Compressor::LoadImageFromFile(const file::WPath& path)
{
	// Step 1: Load image from file system
	m_Name = String::ToAnsiTemp(path.GetFileNameWithoutExtension());

	file::FileBytes fileBytes;
	if (!AM_VERIFY(ReadAllBytes(path, fileBytes), L"TextureCompressor::Load(path: %ls) -> Failed to read file.", path.GetCStr()))
		return false;

	if (LoadImageFromMemory(fileBytes.Data.get(), fileBytes.Size))
		return true;

	AM_ERRF(L"TextureCompressor::Load() -> An error occured while loading %ls", path.GetCStr());
	return false;
}

bool rageam::texture::Compressor::LoadImageFromMemory(const char* data, u32 dataSize)
{
	// Step 1: Load image from memory
	m_Loaded = false;
	m_Image = nvtt::Surface();

	if (!m_Image.loadFromMemory(data, dataSize))
	{
		AM_ERRF("TextureCompressor::Load() -> Unable to load texture from disk...");
		return false;
	}

	// Step 2: Verify image properties
	m_Width = m_Image.width();
	m_Height = m_Image.height();
	m_PixelCount = m_Width * m_Height;

	if (min(m_Width, m_Height) < 4)
	{
		AM_ERRF("TextureCompressor::Load() -> Minimum allowed texture resolution is 4x4, actual: %ux%u)", m_Width, m_Height);
		return false;
	}

	if (!IS_POWER_OF_TWO(m_Width) || !IS_POWER_OF_TWO(m_Height))
		AM_WARNINGF("TextureCompressor::Load() -> Texture resolution is not power of two, texture will be compressed without mip maps.");

	// Step 3: Find best-fitting compression format
	FindBestFormat();

	AM_DEBUGF("TextureCompressor::Load() -> OK (w: %u h: %u), Alpha: %s (Sharp: %s), Format: %s",
		m_Width, m_Height, m_HasAlpha ? "y" : "n", m_SharpAlpha ? "y" : "n", D3D::DxgiFormatToString(m_BestFormat));

	m_Loaded = true;
	return true;
}

bool rageam::texture::Compressor::LoadFromPixelData(int width, int height, const char* data, nvtt::InputFormat inputFormat)
{
	m_Loaded = false;
	if (!m_Image.setImage(inputFormat, width, height, 1, data))
		return false;

	m_Width = width;
	m_Height = height;

	FindBestFormat();
	m_Loaded = true;
	return true;
}

DXGI_FORMAT rageam::texture::Compressor::GetFormat(DXGI_FORMAT fmt) const
{
	// If block compression not supported just use best format which is set to RGBA8 (see first lines of FindBestFormat)
	if (fmt == DXGI_FORMAT_UNKNOWN || !m_SupportBlockCompression)
		return m_BestFormat;
	return fmt;
}

DXGI_FORMAT rageam::texture::Compressor::GetFormat(const CompressionOptions& options) const
{
	return GetFormat(options.Format);
}

void rageam::texture::Compressor::GetResolution(const CompressionOptions& options, u32& width, u32& height) const
{
	if (options.MaxResolution == 0)
	{
		width = m_Width;
		height = m_Height;
		return;
	}

	u32 max = max(m_Width, m_Height);

	if (!IS_POWER_OF_TWO(max))
		max = ALIGN_POWER_OF_TWO_32(max);

	if (max <= options.MaxResolution)
	{
		width = m_Width;
		height = m_Height;
		return;
	}

	// Pseudo:
	// scaleFactor	= max / settings.MaxResolution
	// width		= m_Width * scaleFactor
	// height		= m_Height * scaleFactor

	u8 shift = BitScanR32(max) - BitScanR32(options.MaxResolution);

	width = m_Width >> shift;
	height = m_Height >> shift;
}

u32 rageam::texture::Compressor::GetMipCount(const CompressionOptions& options) const
{
	u32 width;
	u32 height;
	GetResolution(options, width, height);

	u32 mips = 1;
	if (options.MipMaps)
		mips = D3D::GetAvailableMipLevels(width, height);

	return mips;
}

u32 rageam::texture::Compressor::GetAvailableMipCount() const
{
	return D3D::GetAvailableMipLevels(m_Width, m_Height);
}

u32 rageam::texture::Compressor::ComputeSize(const CompressionOptions& options) const
{
	u32 width;
	u32 height;
	GetResolution(options, width, height);
	u32 mips = GetMipCount(options);

	u32 size;
	HRESULT result = D3D::GetTextureSize(GetFormat(options), width, height, 1, mips, size);
	AM_ASSERT(result == S_OK, "TextureCompressor::ComputeSize() -> Failed with code: %x", result);

	return size;
}

bool rageam::texture::Compressor::Compress(const CompressionOptions& options, char** ppPixelData, CompressedInfo& outInfo) const
{
	*ppPixelData = nullptr;

	NvttRawOutputHandler outputHandler;

	// We have to copy texture because it will be altered (to fit max resolution & generate mips)
	nvtt::Surface imageCopy(m_Image);

	u32 width;
	u32 height;
	GetResolution(options, width, height);
	imageCopy.resize(static_cast<int>(width), static_cast<int>(height), 1, static_cast<nvtt::ResizeFilter>(options.ResizeFilter));

	DXGI_FORMAT dxFormat = GetFormat(options);

	nvtt::Context context(true);
	nvtt::CompressionOptions compressionOptions;
	compressionOptions.setQuality(static_cast<nvtt::Quality>(options.Quality));
	compressionOptions.setFormat(DxgiFormatToNvtt(dxFormat));
	nvtt::OutputOptions outputOptions;
	outputOptions.setOutputHandler(&outputHandler);

	u32 offset = 0; // Mipmap offset
	u32 mips = GetMipCount(options);

	u32 pixelDataSize;
	HRESULT getSizeResult = D3D::GetTextureSize(dxFormat, width, height, 1, mips, pixelDataSize);
	AM_ASSERT(getSizeResult == S_OK, "TextureCompressor::Compress() -> GetTextureSize failed with code: %x", getSizeResult);

	char* pixelData = new char[pixelDataSize];

	for (u32 i = 0; i < mips; i++)
	{
		if (!context.compress(imageCopy, 0, 0, compressionOptions, outputOptions))
		{
			AM_ERRF("TextureCompressor::Compress() -> Failed to compress image %s", m_Name.GetCStr());
			delete[] pixelData;
			return false;
		}

		memcpy(pixelData + offset, outputHandler.Buffer, outputHandler.Size);
		offset += outputHandler.Size;

		if (!imageCopy.buildNextMipmap(static_cast<nvtt::MipmapFilter>(options.MipFilter)))
		{
			mips = i + 1;
			break;
		}
	}

	outInfo.Width = width;
	outInfo.Height = height;
	outInfo.MipCount = mips;
	outInfo.Size = pixelDataSize;
	outInfo.Format = dxFormat;

	*ppPixelData = pixelData;

	return true;
}

D3D11_TEXTURE2D_DESC rageam::texture::Compressor::GetDesc2D(const CompressionOptions& options) const
{
	D3D11_TEXTURE2D_DESC desc = {};
	desc.ArraySize = 1;
	desc.Format = GetFormat(options);
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	desc.MipLevels = GetMipCount(options);
	GetResolution(options, desc.Width, desc.Height);

	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	return desc;
}

ID3D11ShaderResourceView* rageam::texture::Compressor::CompressAndGetShaderView(const CompressionOptions& options, CompressedInfo& outInfo) const
{
	memset(&outInfo, 0, sizeof CompressedInfo);

	ID3D11Device* device = render::Engine::GetInstance()->GetFactory();

	D3D11_TEXTURE2D_DESC desc = GetDesc2D(options);

	char* pixelData;
	if (!Compress(options, &pixelData, outInfo))
	{
		AM_ERRF("TextureCompressor::CompressAndGetShaderView() -> Failed to compress...");
		return nullptr;
	}

	D3D11_SUBRESOURCE_DATA* subData = D3D::CreateSubData(pixelData, desc);

	ID3D11Texture2D* texture;
	HRESULT result = device->CreateTexture2D(&desc, subData, &texture);

	delete[] subData;
	delete[] pixelData;

	if (result != S_OK)
	{
		AM_ERRF("CompressAndGetShaderView::CompressAndGetShaderView() -> Failed to create texture %s, error code: %x", m_Name.GetCStr(), result);
		return nullptr;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = desc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = desc.MipLevels;
	viewDesc.Texture2D.MostDetailedMip = 0;

	ID3D11ShaderResourceView* resourceView;
	result = device->CreateShaderResourceView(texture, &viewDesc, &resourceView);
	texture->Release();

	if (result != S_OK)
	{
		AM_ERRF("CompressAndGetShaderView::CompressAndGetShaderView() -> Failed to create texture view for %s, error code: %x", m_Name.GetCStr(), result);
		return nullptr;
	}

	return resourceView;
}
