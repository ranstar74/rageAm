#include "surface.h"

#include "d3dutils.h"
#include "ddsutils.h"
#include "imageformat.h"

bool rageam::texture::Surface::LoadToGpu(ConstWString path, ID3D11ShaderResourceView** ppView, bool forceCompress)
{
	char* pixelData;
	if (!LoadToRam(path, &pixelData, forceCompress))
		return false;

	return CreateShaderView(m_Info, pixelData, ppView);
}

bool rageam::texture::Surface::LoadToRam(ConstWString path, char** ppPixelData, bool forceCompress)
{
	m_IsPreCompressed = IsDDS(path);

	// .dds files are already compressed so just use them as is
	if (m_IsPreCompressed && !forceCompress)
	{
		AM_DEBUGF("Surface::LoadToRam() -> Texture is pre-compressed, loading from .dds (%ls)", path);

		return LoadDDSFromFile(path, m_Info, ppPixelData);
	}
	AM_DEBUGF("Surface::LoadToRam() -> Raw texture, compressing (%ls)", path);

	if (!m_Compressor.LoadImageFromFile(path))
		return false;

	if (!m_Compressor.Compress(m_CompressOptions, ppPixelData, m_Info))
		return false;

	return true;
}
