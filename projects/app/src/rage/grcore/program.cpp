#include "program.h"

#include <d3d11shader.h>
#include <d3dcompiler.h>

#include "device.h"
#include "am/graphics/render.h"
#include "rage/atl/hashstring.h"
#include "rage/file/stream.h"

ID3D11DeviceChild* rage::grcCreateProgramByTypeCached(ConstString name, grcProgramType type, pVoid bytecode, u32 bytecodeSize)
{
	// TODO: Implement caching

	ID3D11Device* factory = rageam::graphics::RenderGetDevice();
	ID3D11DeviceChild* program = nullptr;
	HRESULT result;
	switch (type)
	{
	case GRC_PROGRAM_VERTEX:
		result = factory->CreateVertexShader(bytecode, bytecodeSize, nullptr, (ID3D11VertexShader**)&program); break;
	case GRC_PROGRAM_PIXEL:
		result = factory->CreatePixelShader(bytecode, bytecodeSize, nullptr, (ID3D11PixelShader**)&program); break;
	case GRC_PROGRAM_COMPUTE:
		result = factory->CreateComputeShader(bytecode, bytecodeSize, nullptr, (ID3D11ComputeShader**)&program); break;
	case GRC_PROGRAM_GEOMETRY:
		result = factory->CreateGeometryShader(bytecode, bytecodeSize, nullptr, (ID3D11GeometryShader**)&program); break;
	case GRC_PROGRAM_HULL:
		result = factory->CreateHullShader(bytecode, bytecodeSize, nullptr, (ID3D11HullShader**)&program); break;
	case GRC_PROGRAM_DOMAIN:
		result = factory->CreateDomainShader(bytecode, bytecodeSize, nullptr, (ID3D11DomainShader**)&program); break;
	default:
		AM_UNREACHABLE("");
	}

	if (result != S_OK)
	{
		AM_ERRF("Unable to create shader program %s", name);
		return nullptr;
	}
	return program;
}

u32 rage::ReadStringAndComputeHash(const fiStreamPtr& stream)
{
	char buffer[265];
	u8 size = stream->ReadU8();
	stream->Read(buffer, size);
	return atStringHash(buffer);
}

rage::grcProgram::~grcProgram()
{
	if (m_Name)
	{
		delete m_Name;
		m_Name = nullptr;
	}
}

void rage::grcProgram::Deserialize(const fiStreamPtr& stream, ConstString fileName)
{
	char fullName[256];
	char programName[256];
	u8 nameSize = stream->ReadU8();
	stream->Read(programName, nameSize);

	// Format name as 'FILENAME:SHADERNAME'
	int length = sprintf_s(fullName, 256, "%s:%s", fileName, programName);
	m_Name = new char[length + 1];
	memcpy(m_Name, fullName, length + 1);

	// Read hashes of all shader variables
	u8 varCount = stream->ReadU8();
	m_VarNameHashes.Reserve(varCount);
	for (u8 i = 0; i < varCount; i++)
		m_VarNameHashes.Add(ReadStringAndComputeHash(stream));

	// TODO: Research more
	u8 variable2Count = stream->ReadU8();
	for (u8 i = 0; i < variable2Count; i++)
	{
		ReadStringAndComputeHash(stream);
		stream->ReadU8();
		stream->ReadU8();
	}
}

rage::grcVertexProgram::~grcVertexProgram()
{
	if (m_Bytecode)
	{
		rage_free(m_Bytecode);
		m_Bytecode = nullptr;
	}
}

void rage::grcVertexProgram::Deserialize(const fiStreamPtr& stream, ConstString fileName)
{
	grcProgram::Deserialize(stream, fileName);

	m_BytecodeSize = stream->ReadU32();
	if (m_BytecodeSize == 0) // Some shaders are empty
		return;

	m_Bytecode = rage_malloc(m_BytecodeSize);
	stream->Read(m_Bytecode, m_BytecodeSize);

	// Game does version check here but we just assume that user pc can run the game
	// vs_5_0
	[[maybe_unused]] u8 shaderModelOnes = stream->ReadU8();
	[[maybe_unused]] u8 shaderModelTenths = stream->ReadU8();

	m_Shader = amComPtr(static_cast<ID3D11VertexShader*>(
		grcCreateProgramByTypeCached(GetName(), GRC_PROGRAM_VERTEX, m_Bytecode, m_BytecodeSize)));
	m_Shader_ = m_Shader; // They're identical in release build
}

void rage::grcVertexProgram::ReflectVertexFormat(grcVertexDeclaration** ppDecl, grcFvf* pFvf) const
{
	ID3D11ShaderReflection* reflection;
	HRESULT reflectResult =
		D3DReflect(m_Bytecode, m_BytecodeSize, IID_ID3D11ShaderReflection, (void**)&reflection);
	AM_ASSERT(reflectResult == S_OK, "grcVertexProgram::ReflectVertexFormat() -> D3DReflect failed with %u", reflectResult);

	D3D11_SHADER_DESC desc;
	HRESULT descResult = reflection->GetDesc(&desc);
	AM_ASSERT(descResult == S_OK, "grcVertexProgram::ReflectVertexFormat() -> GetDesc failed with %u", descResult);

	D3D11_SIGNATURE_PARAMETER_DESC signatureParameterDescs[16];
	for (UINT i = 0; i < desc.InputParameters; i++)
	{
		HRESULT inputDescResult = reflection->GetInputParameterDesc(i, &signatureParameterDescs[i]);
		AM_ASSERT(inputDescResult == S_OK, "grcVertexProgram::ReflectVertexFormat() -> GetInputParameterDesc failed with %u", inputDescResult);
	}

	D3D11_INPUT_ELEMENT_DESC elementDescs[16];
	u16 elementCount = 0;
	for (UINT i = 0; i < desc.InputParameters; i++)
	{
		if (signatureParameterDescs[i].SystemValueType == D3D_NAME_INSTANCE_ID)
			continue;

		elementCount++;

		D3D11_SIGNATURE_PARAMETER_DESC& paramDesc = signatureParameterDescs[i];
		D3D11_INPUT_ELEMENT_DESC& elementDesc = elementDescs[i];

		// fill out input element desc
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		// determine DXGI format
		if (paramDesc.Mask == 1)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else if (paramDesc.Mask <= 3)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		}
		else if (paramDesc.Mask <= 7)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if (paramDesc.Mask <= 15)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}
	}

	grcVertexDeclaration* decl = grcDevice::CreateVertexDeclaration(elementDescs, elementCount);
	if (pFvf) decl->EncodeFvf(*pFvf);

	*ppDecl = decl;
}

void rage::grcFragmentProgram::Deserialize(const fiStreamPtr& stream, ConstString fileName)
{
	// Very similar to grcVertexProgram::Deserialize but we don't keep bytecode
	// because on vertex shader it is only kept for D3D Reflector

	grcProgram::Deserialize(stream, fileName);

	m_BytecodeSize = stream->ReadU32();
	if (m_BytecodeSize == 0) // Some shaders are empty
		return;

	pVoid bytecode = rage_malloc(m_BytecodeSize);
	stream->Read(bytecode, m_BytecodeSize);

	// Game does version check here but we just assume that user pc can run the game
	// ps_5_0
	[[maybe_unused]] u8 shaderModelOnes = stream->ReadU8();
	[[maybe_unused]] u8 shaderModelTenths = stream->ReadU8();

	m_Shader = amComPtr(static_cast<ID3D11PixelShader*>(
		grcCreateProgramByTypeCached(GetName(), GRC_PROGRAM_PIXEL, bytecode, m_BytecodeSize)));
	m_Shader_ = m_Shader; // They're identical in release build

	rage_free(bytecode);
}

void rage::grcComputeProgram::Deserialize(const fiStreamPtr& stream, ConstString fileName)
{
	grcProgram::Deserialize(stream, fileName);

	m_BytecodeSize = stream->ReadU32();
	if (m_BytecodeSize == 0) // Some shaders are empty
		return;

	pVoid bytecode = rage_malloc(m_BytecodeSize);
	stream->Read(bytecode, m_BytecodeSize);

	// TODO: Game does hardware support check before creating it
	m_Shader = amComPtr(static_cast<ID3D11ComputeShader*>(
		grcCreateProgramByTypeCached(GetName(), GRC_PROGRAM_COMPUTE, bytecode, m_BytecodeSize)));
	m_Shader_ = m_Shader; // They're identical in release build

	rage_free(bytecode);
}

void rage::grcDomainProgram::Deserialize(const fiStreamPtr& stream, ConstString fileName)
{
	grcProgram::Deserialize(stream, fileName);

	m_BytecodeSize = stream->ReadU32();
	if (m_BytecodeSize == 0) // Some shaders are empty
		return;

	pVoid bytecode = rage_malloc(m_BytecodeSize);
	stream->Read(bytecode, m_BytecodeSize);

	// TODO: Game does hardware support check before creating it
	m_Shader = amComPtr(static_cast<ID3D11DomainShader*>(
		grcCreateProgramByTypeCached(GetName(), GRC_PROGRAM_DOMAIN, bytecode, m_BytecodeSize)));
	m_Shader_ = m_Shader; // They're identical in release build

	rage_free(bytecode);
}

rage::grcGeometryProgram::~grcGeometryProgram()
{
	delete[] m_UnknownDatas;
	m_UnknownDatas = nullptr;
}

void rage::grcGeometryProgram::Deserialize(const fiStreamPtr& stream, ConstString fileName)
{
	grcProgram::Deserialize(stream, fileName);

	m_UnknownDataCount = stream->ReadU8();
	if (m_UnknownDataCount != 0)
	{
		m_UnknownDatas = new UnknownData[m_UnknownDataCount];
		for (u32 i = 0; i < m_UnknownDataCount; i++)
		{
			UnknownData& data = m_UnknownDatas[i];

			stream->Read(data.Unknown0, sizeof data.Unknown0);
			data.Unknown16 = stream->ReadU8();
			data.Unknown17 = stream->ReadU8();
			data.Unknown18 = stream->ReadU8();
			data.Unknown19 = stream->ReadU8();
		}
	}

	m_BytecodeSize = stream->ReadU32();
	if (m_BytecodeSize == 0)
		return;

	pVoid bytecode = rage_malloc(m_BytecodeSize);
	stream->Read(bytecode, m_BytecodeSize);

	[[maybe_unused]] u8 shaderModelOnes = stream->ReadU8();
	[[maybe_unused]] u8 shaderModelTenths = stream->ReadU8();

	if (m_UnknownDataCount == 0) // TODO: Figure out why
	{
		m_Shader = amComPtr(static_cast<ID3D11GeometryShader*>(
			grcCreateProgramByTypeCached(GetName(), GRC_PROGRAM_GEOMETRY, bytecode, m_BytecodeSize)));
		m_Shader_ = m_Shader;
	}

	rage_free(bytecode);
}

void rage::grcHullProgram::Deserialize(const fiStreamPtr& stream, ConstString fileName)
{
	grcProgram::Deserialize(stream, fileName);

	m_BytecodeSize = stream->ReadU32();
	if (m_BytecodeSize == 0) // Some shaders are empty
		return;

	pVoid bytecode = rage_malloc(m_BytecodeSize);
	stream->Read(bytecode, m_BytecodeSize);

	// TODO: Game does hardware support check before creating it
	m_Shader = amComPtr(static_cast<ID3D11HullShader*>(
		grcCreateProgramByTypeCached(GetName(), GRC_PROGRAM_HULL, bytecode, m_BytecodeSize)));
	m_Shader_ = m_Shader; // They're identical in release build

	rage_free(bytecode);
}
