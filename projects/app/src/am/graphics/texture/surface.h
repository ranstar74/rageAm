#pragma once

#include "compressor.h"

namespace rageam::texture
{
	/**
	 * \brief Just a texture, can load compressed (.dds) or raw images (.png / whatever).
	 * For raw images compression options can be set, or default will be used.
	 */
	class Surface
	{
		Compressor			m_Compressor;
		CompressionOptions	m_CompressOptions;
		CompressedInfo		m_Info = {};
		bool				m_IsPreCompressed = false;
	public:
		// Sets compression options that will be used for loading next images if they are not pre-compressed.
		void SetCompressOptions(const CompressionOptions& options) { m_CompressOptions = options; }
		// Gets information about currently loaded texture.
		const CompressedInfo& GetInfo() const { return m_Info; }
		// .dds files are pre-compressed textures.
		bool IsPreCompressed() const { return m_IsPreCompressed; }
		// Can be used only if image is not pre-compressed!
		const Compressor& GetCompressor() const { return m_Compressor; }
		// Same as LoadToRam but creates ID3D11Texture & View for it.
		bool LoadToGpu(ConstWString path, ID3D11ShaderResourceView** ppView, bool forceCompress = false);
		// Loads image from disk and performs block compression (for all raw images).
		// For already compressed images (.dds) uses already existing data, unless forceCompress is set to true.
		// Warning: Pixel data is a raw pointer!
		bool LoadToRam(ConstWString path, char** ppPixelData, bool forceCompress = false);
	};
}
