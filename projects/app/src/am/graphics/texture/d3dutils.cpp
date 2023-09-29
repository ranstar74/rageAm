#include "d3dutils.h"

#include "compressor.h"
#include "am/graphics/render/engine.h"
#include "rage/grcore/texture/utils.h"

#include <d3d11.h>

void rageam::texture::GetDesc2D(const CompressedInfo& info, D3D11_TEXTURE2D_DESC& outDesc)
{
	memset(&outDesc, 0, sizeof D3D11_TEXTURE2D_DESC);

	outDesc.ArraySize = 1;
	outDesc.Format = info.Format;
	outDesc.Usage = D3D11_USAGE_DEFAULT;
	outDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	outDesc.MipLevels = info.MipCount;
	outDesc.Width = info.Width;
	outDesc.Height = info.Height;
	outDesc.SampleDesc.Count = 1;
	outDesc.SampleDesc.Quality = 0;
}

bool rageam::texture::CreateShaderView(const CompressedInfo& info, char* pixelData, ID3D11ShaderResourceView** ppView)
{
	ID3D11Device* device = render::Engine::GetInstance()->GetFactory();

	D3D11_TEXTURE2D_DESC desc;
	GetDesc2D(info, desc);

	D3D11_SUBRESOURCE_DATA* subData = D3D::CreateSubData(pixelData, desc);

	ID3D11Texture2D* texture;
	HRESULT result = device->CreateTexture2D(&desc, subData, &texture);

	delete[] subData;
	delete[] pixelData;

	if (result != S_OK)
	{
		AM_ERRF("CreateShaderView() -> Failed to create texture, error code: %x", result);
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.Format = desc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = desc.MipLevels;
	viewDesc.Texture2D.MostDetailedMip = 0;

	result = device->CreateShaderResourceView(texture, &viewDesc, ppView);
	texture->Release();

	if (result != S_OK)
	{
		AM_ERRF("CreateShaderView() -> Failed to create texture view, error code: %x", result);
		return false;
	}

	return true;
}
