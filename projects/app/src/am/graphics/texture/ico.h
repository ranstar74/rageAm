#pragma once


#include <Windows.h>
#include <d3d11.h>

#include "am/file/path.h"
#include "am/system/ptr.h"

namespace rageam::texture
{
	// See https://en.wikipedia.org/wiki/ICO_(file_format)#cite_note-bigSize-7 for format description

	/**
	 * \brief Utility to read .ico files.
	 * ICO file may contain multiple images of different size, similar to mip-map system.
	 */
	class Ico
	{
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

		file::Path m_FileName;
		IconDir* m_Dir = nullptr;
		char* m_FileBuffer = nullptr;
	public:
		Ico() = default;
		~Ico();

		// Loads .ico file from given path
		bool Load(const wchar_t* path);
		// Helper function to create texture & shader view with mip-maps from this icon file
		ID3D11ShaderResourceView* CreateView(int resolution) const;
	};
}
