#pragma once

struct D3D11_TEXTURE2D_DESC;
class ID3D11ShaderResourceView;

namespace rageam::texture
{
	struct CompressedInfo;

	// Converts compressed info into DirectX 11 desc struct.
	void GetDesc2D(const CompressedInfo& info, D3D11_TEXTURE2D_DESC& outDesc);
	// Creates shader resource view from given pixel data.
	// Note: pixel data pointer is freed up after creating view.
	bool CreateShaderView(const CompressedInfo& info, char* pixelData, ID3D11ShaderResourceView** ppView);
}
