//
// File: shader.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/ptr.h"
#include "dx11.h"

namespace rageam::graphics
{
	enum ShaderType
	{
		ShaderVertex,
		ShaderPixel,
		ShaderCount
	};
	static constexpr ConstString ShaderTypeToProfile[] =
	{
		DEF_VS_PROFILE, DEF_PS_PROFILE, ""
	};

	/**
	 * \brief HLSL DX11 shader wrapper.
	 */
	class Shader
	{
		ShaderType                  m_Type;
		amComPtr<ID3D11DeviceChild> m_Object;

	public:
		Shader(ShaderType type, const amComPtr<ID3D11DeviceChild>& object)
		{
			m_Type = type;
			m_Object = object;
		}
		void Bind() const;

		// Creates from shader placed in 'data/shaders' directory
		static amPtr<Shader> CreateFromPath(ShaderType type, ConstWString shaderName, ShaderBytecode* outBytecode = nullptr);
	};
}
