//
// File: blit.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "render.h"
#include "shader.h"

namespace rageam::graphics
{
	/**
	 * \brief Blit is a term for rendering 2D images.
	 * This class allows to render shader views on the screen.
	 * You can override pixel shader to apply various post-processing effects, such as blur.
	 */
	class Blit
	{
		amPtr<Shader>               m_VS, m_PS;
		amComPtr<ID3D11Buffer>      m_ScreenPlane;
		amComPtr<ID3D11InputLayout> m_InputLayout;
		
	public:
		Blit()
		{
			ShaderBytecode vsBytecode;

			m_VS = Shader::CreateFromPath(ShaderVertex, L"im_blit_vs.hlsl", &vsBytecode);
			m_PS = Shader::CreateFromPath(ShaderPixel, L"im_blit_ps.hlsl");

			// Create 2 triangles that occlude whole screen
			Vec3S screenVerts[] =
			{
				Vec3S(-1, -1, 0), // Bottom Left
				Vec3S( 1,  1, 0), // Top Right
				Vec3S(-1,  1, 0), // Top Left
				Vec3S( 1, -1, 0), // Bottom Right
				Vec3S( 1,  1, 0), // Top Right
				Vec3S(-1, -1, 0), // Bottom Left
			};
			m_ScreenPlane = DX11::CreateBuffer(D3D11_BIND_VERTEX_BUFFER, sizeof screenVerts, false, screenVerts);

			InputLayoutBuilder layoutBuilder;
			layoutBuilder.Add("Position", 0, DXGI_FORMAT_R32G32B32_FLOAT);
			m_InputLayout = layoutBuilder.CreateLayout(vsBytecode);
		}

		// Binds default pixel shader, this is done in Bind() function so needs to be called only to reset state
		void BindDefPS() const
		{
			m_PS->Bind();
		}

		// If pixel shader override is not specified, default one (without post-processing) is used instead
		void Bind(const amPtr<Shader>* psOverride = nullptr) const
		{
			const amPtr<Shader>& ps = psOverride ? *psOverride : m_PS;
			ps->Bind();
			m_VS->Bind();
			DXCONTEXT->IASetInputLayout(m_InputLayout.Get());
			DX11::SetTopologyType(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			DX11::SetVertexBuffer(m_ScreenPlane, sizeof Vec3S);
		}

		void Draw(const amComPtr<ID3D11ShaderResourceView>& texture) const
		{
			DX11::SetPSTexture(texture, 0);
			DXCONTEXT->Draw(6, 0);
		}
	};
}
