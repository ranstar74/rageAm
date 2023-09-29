#include "ico.h"

#include <filesystem>
#include <fstream>

#include "compressor.h"
#include "am/graphics/render/engine.h"
#include "am/system/asserts.h"
#include "rage/atl/array.h"

rageam::texture::Ico::~Ico()
{
	delete[] m_FileBuffer;
}

bool rageam::texture::Ico::Load(const wchar_t* path)
{
	std::ifstream stream(path, std::ifstream::binary);
	if (!AM_VERIFY(stream.is_open(), L"Ico::Load() -> Unable to open file %ls", path))
		return false;

	std::streamsize fileSize = static_cast<std::streamsize>(std::filesystem::file_size(path));
	m_FileBuffer = new char[fileSize];
	stream.read(m_FileBuffer, fileSize);

	m_Dir = reinterpret_cast<IconDir*>(m_FileBuffer);
	m_FileName = PATH_TO_UTF8(file::GetFileName(path)); // Need really just for logs in compressor

	return true;
}

ID3D11ShaderResourceView* rageam::texture::Ico::CreateView(int resolution) const
{
	CompressedInfo info;
	CompressionOptions options;
	options.MipMaps = true;
	options.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	Compressor compressor;
	compressor.SetDebugName(m_FileName);

	// Step 1:  Find closest mipmap that satisfies given size
	WORD closestIndex = 0;
	for (; closestIndex < m_Dir->ImageCount - 1; closestIndex++)
	{
		bool currentFits = m_Dir->Images[closestIndex].GetWidth() >= (DWORD)resolution;
		bool nextNotFits = m_Dir->Images[closestIndex + 1].GetWidth() < (DWORD)resolution;
		if (currentFits && nextNotFits)
			break;
	}

	// Step 2: Read image data
	// For PNG we use texture compressor that will basically just decode it
	// For BMP we just use existing data
	IconImage& dirImage = m_Dir->Images[closestIndex];
	char* data = m_FileBuffer + dirImage.DataOffset;
	DWORD dataSize = dirImage.DataSize;

	// Verify whether image is PNG or BMP
	constexpr DWORD PNG_MAGIC = MAKEFOURCC(0x89, 'P', 'N', 'G');
	bool isPng = memcmp(data, &PNG_MAGIC, sizeof DWORD) == 0;
	if (isPng)
	{
		compressor.LoadImageFromMemory(data, dataSize);
	}
	else
	{
		data += sizeof BITMAPINFOHEADER;
		dataSize -= sizeof BITMAPINFOHEADER;

		int width = dirImage.Width;
		int height = dirImage.Height;

		DWORD stride = width * 4;

		// Flip image vertically
		char* copy = new char[dataSize];
		for (DWORD x = 0; x < width; x++)
		{
			for (DWORD y = 0; y < height; y++)
			{
				DWORD srcOffset = x * 4 + y * stride;
				DWORD cpyOffset = x * 4 + (height - y - 1) * stride;

				*(DWORD*)(copy + cpyOffset) = *(DWORD*)(data + srcOffset); // NOLINT(clang-diagnostic-cast-align)
			}
		}

		memcpy(data, copy, dataSize);
		delete[] copy;

		compressor.LoadFromPixelData(width, height, data, nvtt::InputFormat::InputFormat_BGRA_8UB);
	}

	// Step 3: Create texture & view from parsed images, using them as mip-maps
	return compressor.CompressAndGetShaderView(options, info);
}
