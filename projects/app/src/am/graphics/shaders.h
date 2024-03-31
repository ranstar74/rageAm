#pragma once

#include "am/system/ptr.h"
#include "common/types.h"

#include <d3d11.h>

namespace rageam::graphics
{
	static ID3D11Buffer* CreateCB(size_t size);
	static void CreateShaders(
		ID3D11VertexShader** vs,
		ID3D11PixelShader** ps,
		ID3DBlob** vsBlob,
		ConstWString vsFileName,
		ConstWString psFileName);

	ID3D11Buffer* CreateBuffer(size_t elementSize, size_t elementCount, bool vertexBuffer);

	void UploadBuffer(const amComPtr<ID3D11Buffer>& buffer, pConstVoid data, u32 dataSize);

	class Shader1
	{
		amComPtr<ID3D11VertexShader> m_VS;
		amComPtr<ID3D11PixelShader>  m_PS;
		amComPtr<ID3D11InputLayout>  m_VSLayout;
		ConstWString				 m_VsName;
		ConstWString				 m_PsName;

	public:
		Shader1(ConstWString vsName, ConstWString psName);
		virtual ~Shader1() = default;

		virtual void Create();
		virtual void Bind();
	};

	class DefaultShader : public Shader1
	{
		struct Locals
		{
			int	Unlit;
		};

		amComPtr<ID3D11Buffer> m_LocalsCB;
	public:
		DefaultShader() : Shader1(L"default_vs.hlsl", L"default_ps.hlsl") {}

		void Create() override;
		void Bind() override;

		Locals Locals = {};
	};

	class ImageBlitShader : public Shader1
	{
		amComPtr<ID3D11Buffer> m_ScreenVerts;

	public:
		ImageBlitShader() : Shader1(L"im_blit_vs.hlsl", L"im_blit_ps.hlsl") {}

		void Create() override;
		void Blit(ID3D11ShaderResourceView* view) const;
	};
}
