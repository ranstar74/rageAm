#include "ddsutils.h"

#include "compressor.h"
#include "DirectXTex.h"

bool rageam::texture::LoadDDSFromFile(const wchar_t* path, CompressedInfo& info, char** ppPixelData)
{
	DirectX::TexMetadata metadata;
	DirectX::ScratchImage scratchImage;
	HRESULT loadResult = LoadFromDDSFile(path, DirectX::DDS_FLAGS_NONE, &metadata, scratchImage);
	if (loadResult != S_OK)
	{
		AM_ERRF(L"LoadDDSFromFile() -> Failed to load dds texture %ls, failed with code: %x", path, loadResult);
		return false;
	}

	u32 pixelsSize = static_cast<u32>(scratchImage.GetPixelsSize());

	info.Format = metadata.format;
	info.Width = metadata.width;
	info.Height = metadata.height;
	info.MipCount = metadata.mipLevels;
	info.Size = scratchImage.GetPixelsSize();

	// Re-allocate pixel data on our hea
	char* pixels = new char[pixelsSize]();
	memcpy(pixels, scratchImage.GetPixels(), pixelsSize);

	*ppPixelData = pixels;

	return true;
}
