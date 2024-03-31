#include "dx11.h"
#include "am/file/fileutils.h"
#include "dxgi_utils.h"
#include "render.h"

#include <d3dcompiler.h>

void rageam::graphics::InputLayoutBuilder::Add(ConstString semanticName, UINT semanticIndex, DXGI_FORMAT format)
{
	AM_ASSERT(m_Count < MAX_INPUT_ELEMENTS, "InputLayoutBuilder::Add() -> Maximum %i elements allowed!", MAX_INPUT_ELEMENTS);
	m_Items[m_Count].Format = format;
	m_Items[m_Count].AlignedByteOffset = m_Offset;
	m_Items[m_Count].SemanticName = semanticName;
	m_Items[m_Count].SemanticIndex = semanticIndex;
	m_Items[m_Count].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	m_Count++;
	m_Offset += DXGI::BytesPerPixel(format);
}

amComPtr<ID3D11InputLayout> rageam::graphics::InputLayoutBuilder::CreateLayout(const ShaderBytecode& vsBytecode) const
{
	if (!AM_VERIFY(vsBytecode, "InputLayoutBuilder::CreateLayout() -> Bytecode was NULL!")) // Allow NULL in case if shader failed to compile
		return nullptr;
	amComPtr<ID3D11InputLayout> inputLayout;
	AM_ASSERT_STATUS(DXDEVICE->CreateInputLayout(m_Items, m_Count, vsBytecode->GetBufferPointer(), vsBytecode->GetBufferSize(), &inputLayout));
	return inputLayout;
}

rageam::graphics::ShaderBytecode rageam::graphics::DX11::CompileShaderFromCode(ConstString shader, UINT shaderLen, ConstString profile, ConstString entryPoint)
{
	amComPtr<ID3DBlob> shaderBlob;
	amComPtr<ID3DBlob> errorBlob;

	UINT	flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
	HRESULT result = D3DCompile(shader, shaderLen, NULL, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, profile, flags, 0, &shaderBlob, &errorBlob);

	if (FAILED(result))
	{
		// TODO: Is there only one or many strings?
		ConstString error = static_cast<ConstString>(errorBlob->GetBufferPointer());
		AM_ERRF("DX11::CompileShaderFromPath() -> Failed to compile shader, errors:");
		AM_ERR(error);
	}

	return shaderBlob;
}

rageam::graphics::ShaderBytecode rageam::graphics::DX11::CompileShaderFromPath(ConstWString path, ConstString profile, ConstString entryPoint)
{
	file::FileBytes bytes;
	if (!ReadAllBytes(path, bytes))
	{
		AM_ERRF(L"DX11::CompileShaderFromPath() -> Failed to open file '%ls'", path);
		return nullptr;
	}

	ShaderBytecode bytecode = CompileShaderFromCode(bytes.Data.get(), bytes.Size, profile, entryPoint);
	if (!bytecode)
	{
		// In addition to error from CompileShaderFromCode
		AM_ERRF(L"DX11::CompileShaderFromPath() -> '%ls'", path);
	}
	return bytecode;
}

amComPtr<ID3D11VertexShader> rageam::graphics::DX11::CreateVertexShader(ConstString name, const ShaderBytecode& bytecode)
{
	AM_ASSERTS(bytecode);
	amComPtr<ID3D11VertexShader> shader;
	AM_ASSERT_STATUS(DXDEVICE->CreateVertexShader(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), nullptr, &shader));
	SetDebugName(shader, name);
	return shader;
}

amComPtr<ID3D11PixelShader> rageam::graphics::DX11::CreatePixelShader(ConstString name, const ShaderBytecode& bytecode)
{
	AM_ASSERTS(bytecode);
	amComPtr<ID3D11PixelShader> shader;
	AM_ASSERT_STATUS(DXDEVICE->CreatePixelShader(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), nullptr, &shader));
	SetDebugName(shader, name);
	return shader;
}

amComPtr<ID3D11Buffer> rageam::graphics::DX11::CreateBuffer(D3D11_BIND_FLAG bindFlag, UINT size, bool dynamic, const void* initialData)
{
	if (!dynamic) AM_ASSERT(initialData != nullptr, "DX11::CreateBuffer() -> Initial data is not specified for immutable buffer!");

	D3D11_BUFFER_DESC desc = {};
	desc.BindFlags = bindFlag;
	desc.ByteWidth = size;

	if (dynamic)
	{
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.Usage = D3D11_USAGE_DYNAMIC;
	}
	else
	{
		desc.Usage = D3D11_USAGE_IMMUTABLE;
	}

	D3D11_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pSysMem = initialData;

	// Don't pass empty D3D11_SUBRESOURCE_DATA if we create dynamic buffer without initial data
	D3D11_SUBRESOURCE_DATA* pSubresourceData = initialData ? &subresourceData : nullptr;

	amComPtr<ID3D11Buffer> buffer;
	AM_ASSERT_STATUS(DXDEVICE->CreateBuffer(&desc, pSubresourceData, &buffer));
	return buffer;
}

void rageam::graphics::DX11::SetBufferData(const amComPtr<ID3D11Buffer>& buffer, const void* data, UINT size)
{
	AM_ASSERTS(buffer);
	
#ifdef AM_DX_DEBUG
	D3D11_BUFFER_DESC desc;
	buffer->GetDesc(&desc);
	AM_ASSERTS(desc.Usage == D3D11_USAGE_DYNAMIC);
	AM_ASSERTS(size <= desc.ByteWidth);
#endif

	D3D11_MAPPED_SUBRESOURCE subresource;
	AM_ASSERT_STATUS(DXCONTEXT->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource));
	memcpy(subresource.pData, data, size);
	DXCONTEXT->Unmap(buffer.Get(), 0);
}

void rageam::graphics::DX11::SetVertexBuffer(const amComPtr<ID3D11Buffer>& buffer, UINT vertexStride)
{
	UINT offset = 0;
	UINT stride = vertexStride;
	DXCONTEXT->IASetVertexBuffers(0, 1, buffer.GetAddressOf(), &stride, &offset);
}

void rageam::graphics::DX11::SetTopologyType(D3D11_PRIMITIVE_TOPOLOGY topology)
{
	DXCONTEXT->IASetPrimitiveTopology(topology);
}

void rageam::graphics::DX11::SetPSTexture(const amComPtr<ID3D11ShaderResourceView>& view, UINT slot)
{
	DXCONTEXT->PSSetShaderResources(slot, 1, view.GetAddressOf());
}

amComPtr<ID3D11Texture2D> rageam::graphics::DX11::CreateTexture(ConstString name, CreateTextureType type, int width, int height, int mipCount, DXGI_FORMAT format, const void* initialData)
{
	if (initialData)
		AM_ASSERT(type == CreateTexture_Default || type == CreateTexture_Dynamic, "DX11::CreateTexture() -> Initial data is only allowed on default and dynamic textures!");

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = mipCount;
	textureDesc.Format = format;
	textureDesc.ArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA resourceData;
	resourceData.pSysMem = initialData;
	resourceData.SysMemPitch = DXGI::ComputeRowPitch(width, format);
	resourceData.SysMemSlicePitch = 0;

	// Don't pass resource data without valid pixel data
	D3D11_SUBRESOURCE_DATA* pResourceData = initialData ? &resourceData : nullptr;

	switch (type)
	{
	case CreateTexture_Default:
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		break;

	case CreateTexture_Dynamic:
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		textureDesc.Usage = D3D11_USAGE_DYNAMIC;
		break;

	case CreateTexture_RenderTarget:
		textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		break;

	case CreateTexture_Staging:
		textureDesc.Usage = D3D11_USAGE_STAGING;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		break;

	case CreateTexture_Depth:
		textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		break;

	default:
		AM_UNREACHABLE("DX11::CreateTexture() -> Create type '%i' is not implemented!", type);
	}

	amComPtr<ID3D11Texture2D> texture;
	AM_ASSERT_STATUS(DXDEVICE->CreateTexture2D(&textureDesc, pResourceData, &texture));
	SetDebugName(texture, name);
	return texture;
}

amComPtr<ID3D11ShaderResourceView> rageam::graphics::DX11::CreateTextureView(ConstString name, const amComPtr<ID3D11Texture2D>& texture)
{
	if (!AM_VERIFY(texture, "DX11::CreateTextureView() -> Texture was NULL!"))
		return nullptr;

#ifdef AM_DX_DEBUG
	D3D11_TEXTURE2D_DESC textureDesc;
	texture->GetDesc(&textureDesc);
	AM_ASSERT(textureDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE, "Texture can't be used as shader resource!");
#endif

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = -1;
	viewDesc.Texture2D.MostDetailedMip = 0;

	amComPtr<ID3D11ShaderResourceView> view;
	AM_ASSERT_STATUS(DXDEVICE->CreateShaderResourceView(texture.Get(), &viewDesc, &view));
	SetDebugName(view, name);
	return view;
}

amComPtr<ID3D11RenderTargetView> rageam::graphics::DX11::CreateRenderTargetView(ConstString name, const amComPtr<ID3D11Texture2D>& texture)
{
	if (!AM_VERIFY(texture, "DX11::CreateRenderTargetView() -> Texture was NULL!"))
		return nullptr;

#ifdef AM_DX_DEBUG
	D3D11_TEXTURE2D_DESC textureDesc;
	texture->GetDesc(&textureDesc);
	AM_ASSERT(textureDesc.BindFlags & D3D11_BIND_RENDER_TARGET, "Texture can't be used as render target!");
#endif

	D3D11_RENDER_TARGET_VIEW_DESC viewDesc;
	viewDesc.Format = DXGI_FORMAT_UNKNOWN;
	viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipSlice = 0;

	amComPtr<ID3D11RenderTargetView> view;
	AM_ASSERT_STATUS(DXDEVICE->CreateRenderTargetView(texture.Get(), &viewDesc, &view));
	SetDebugName(view, name);
	return view;
}

void rageam::graphics::DX11::ClearRenderTarget(const amComPtr<ID3D11RenderTargetView>& rt)
{
	float clearColor[] = { 0, 0, 0, 0 };
	DXCONTEXT->ClearRenderTargetView(rt.Get(), clearColor);
}

void rageam::graphics::DX11::SetRenderTarget(const amComPtr<ID3D11RenderTargetView>& rt, const amComPtr<ID3D11DepthStencilView>& depth)
{
	DXCONTEXT->OMSetRenderTargets(1, rt.GetAddressOf(), depth.Get());
}
