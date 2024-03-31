//
// File: dx11.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "am/system/ptr.h"
#include "am/string/string.h"
#include "am/system/asserts.h"
#include "am/types.h"

#include <d3d11.h>

namespace rageam::graphics
{
#define AM_DX_DEBUG

	using ShaderBytecode = amComPtr<ID3DBlob>;

	static constexpr ConstString DEF_VS_PROFILE = "vs_4_0";
	static constexpr ConstString DEF_PS_PROFILE = "ps_4_0";
	static constexpr ConstString DEF_ENTRY_POINT = "main";

	enum CreateTextureType
	{
		CreateTexture_Default,
		CreateTexture_Dynamic,		// For transferring texture from CPU to GPU
		CreateTexture_RenderTarget,	// For rendering to the texture
		CreateTexture_Staging,		// For transferring texture from GPU to CPU
		CreateTexture_Depth,		// For depth buffer
	};

	class InputLayoutBuilder
	{
		static constexpr int MAX_INPUT_ELEMENTS = 16;

		UINT                     m_Offset = 0;
		UINT                     m_Count = 0;
		D3D11_INPUT_ELEMENT_DESC m_Items[MAX_INPUT_ELEMENTS] = {};
	public:
		void Add(ConstString semanticName, UINT semanticIndex, DXGI_FORMAT format);

		amComPtr<ID3D11InputLayout> CreateLayout(const ShaderBytecode& vsBytecode) const;
	};

	class DX11
	{
		static constexpr int MAX_DEBUG_NAME = 256;

	public:
		// Compiles .hlsl shader from source code string
		static ShaderBytecode CompileShaderFromCode(ConstString shader, UINT shaderLen, ConstString profile, ConstString entryPoint = DEF_ENTRY_POINT);
		static ShaderBytecode CompileShaderFromPath(ConstWString path, ConstString profile, ConstString entryPoint = DEF_ENTRY_POINT);

		static amComPtr<ID3D11VertexShader> CreateVertexShader(ConstString name, const ShaderBytecode& bytecode);
		static amComPtr<ID3D11PixelShader>  CreatePixelShader(ConstString name, const ShaderBytecode& bytecode);

		// Initial data must be specified if buffer is not dynamic
		static amComPtr<ID3D11Buffer> CreateBuffer(D3D11_BIND_FLAG bindFlag, UINT size, bool dynamic, const void* initialData = nullptr);
		// Buffer must be created with dynamic flag
		static void SetBufferData(const amComPtr<ID3D11Buffer>& buffer, const void* data, UINT size);
		static void SetVertexBuffer(const amComPtr<ID3D11Buffer>& buffer, UINT vertexStride);
		static void SetTopologyType(D3D11_PRIMITIVE_TOPOLOGY topology);
		static void SetPSTexture(const amComPtr<ID3D11ShaderResourceView>& view, UINT slot);

		// Initial data must be null for render target or staging textures
		static amComPtr<ID3D11Texture2D>		  CreateTexture(ConstString name, CreateTextureType type, int width, int height, int mipCount, DXGI_FORMAT format, const void* initialData = nullptr);
		static amComPtr<ID3D11ShaderResourceView> CreateTextureView(ConstString name, const amComPtr<ID3D11Texture2D>& texture);
		static amComPtr<ID3D11RenderTargetView>   CreateRenderTargetView(ConstString name, const amComPtr<ID3D11Texture2D>& texture);

		static void ClearRenderTarget(const amComPtr<ID3D11RenderTargetView>& rt);
		static void SetRenderTarget(const amComPtr<ID3D11RenderTargetView>& rt, const amComPtr<ID3D11DepthStencilView>& depth = nullptr);

		// Returned string points to thread local buffer, valid until next call in this thread
		template<typename T>
		static ConstString GetDebugName(const amComPtr<T>& object);
		// This name can be seen in RenderDoc or leaked objects log
		template<typename T>
		static void SetDebugName(const amComPtr<T>& object, ConstString fmt, ...);
	};

	template<typename T>
	ConstString DX11::GetDebugName(const amComPtr<T>& object)
	{
		thread_local char tl_Buffer[MAX_DEBUG_NAME];
		UINT              size = MAX_DEBUG_NAME;
		AM_ASSERT_STATUS(object->GetPrivateData(WKPDID_D3DDebugObjectName, &size, tl_Buffer));
		return tl_Buffer;
	}

	template<typename T>
	void DX11::SetDebugName(const amComPtr<T>& object, ConstString fmt, ...)
	{
		char    name[MAX_DEBUG_NAME];
		va_list args;
		va_start(args, fmt);
		String::FormatVA(name, sizeof name, fmt, args);
		va_end(args);
		AM_ASSERT_STATUS(object->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(name), name));
	}
}
