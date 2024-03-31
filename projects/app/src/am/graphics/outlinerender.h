// File: outlinerender.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/grcore/device.h"
#include "am/system/ptr.h"
#include "blit.h"

#include <d3d11.h>

namespace rageam::graphics
{
	class OutlineRender : public Singleton<OutlineRender>
	{
		// GeomColor  - Where game geometries are drawn to
		// SobelColor - Geometries after outline pass
		// FinalColor - Outline after gauss pass

		amComPtr<ID3D11Texture2D>          m_FinalColor;
		amComPtr<ID3D11ShaderResourceView> m_FinalColorView;
		amComPtr<ID3D11RenderTargetView>   m_FinalColorRT;
		amComPtr<ID3D11Texture2D>          m_SobelColor;
		amComPtr<ID3D11ShaderResourceView> m_SobelColorView;
		amComPtr<ID3D11RenderTargetView>   m_SobelColorRT;
		amComPtr<ID3D11Texture2D>          m_GeomColor;
		amComPtr<ID3D11ShaderResourceView> m_GeomColorView;
		amComPtr<ID3D11RenderTargetView>   m_GeomColorRT;
		amComPtr<ID3D11BlendState>		   m_BlendState;
		amComPtr<ID3D11BlendState>		   m_BlendStateAlpha1;
		amComPtr<ID3D11ShaderResourceView> m_WhiteTexView;
		Blit                               m_Blit;
		amPtr<Shader>                      m_SobelPS;
		amPtr<Shader>                      m_GaussPS;
		u32                                m_Width = 0;
		u32                                m_Height = 0;
		bool							   m_Initialized = false;
		bool                               m_Began = false;
		bool                               m_RenderingOutline = false;
		// State Backup
		ID3D11RenderTargetView*			   m_OldRTs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
		ID3D11DepthStencilView*			   m_OldDepthStencilView = nullptr;
		ID3D11BlendState*				   m_OldBlendState = {};
		float							   m_OldBlendFactor[4] = {};
		u32								   m_OldSampleMask = 0;

		void CreateDeviceObjects();
	public:
		OutlineRender();
		~OutlineRender() override;

		void NewFrame();
		void EndFrame();

		// Any object rendered within scope of Begin/End will be drawn to outline render target
		bool Begin();
		void End();

		bool IsRenderingOutline() const { return m_RenderingOutline; }
		auto GetWhiteTexture() const { return m_WhiteTexView.Get(); }
	};
}
