#include "shader.h"
#include "render.h"
#include "am/system/datamgr.h"

void rageam::graphics::Shader::Bind() const
{
	switch (m_Type)
	{
		case ShaderVertex: DXCONTEXT->VSSetShader(reinterpret_cast<ID3D11VertexShader*>(m_Object.Get()), nullptr, 0); break;
		case ShaderPixel:  DXCONTEXT->PSSetShader(reinterpret_cast<ID3D11PixelShader*>(m_Object.Get()), nullptr, 0);  break;
		default: break;
	}
}

amPtr<rageam::graphics::Shader> rageam::graphics::Shader::CreateFromPath(ShaderType type, ConstWString shaderName, ShaderBytecode* outBytecode)
{
	file::WPath shaderPath = DataManager::GetShadersFolder() / shaderName;
	ShaderBytecode shaderBytecode = DX11::CompileShaderFromPath(shaderPath, ShaderTypeToProfile[type]);

	// Return valid pointer even if compilation failed,
	// so our program doesn't crash and we can quickly fix the issue and reload shaders
	amComPtr<ID3D11DeviceChild> object;
	if (shaderBytecode)
	{
		ConstString shaderDebugName = String::ToAnsiTemp(shaderName);
		switch (type)
		{
			case ShaderVertex: object = DX11::CreateVertexShader(shaderDebugName, shaderBytecode); break;
			case ShaderPixel:  object = DX11::CreatePixelShader(shaderDebugName, shaderBytecode);  break;
			default: AM_UNREACHABLE("Shader::CreateFromPath() -> Type '%i' is not supported!", type);
		}

		if (outBytecode)
			*outBytecode = shaderBytecode;
	}

	return std::make_shared<Shader>(type, object);
}
