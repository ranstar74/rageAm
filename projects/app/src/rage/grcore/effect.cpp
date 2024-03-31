#include "effect.h"

#include "am/graphics/render.h"
#include "rage/grcore/txd.h"
#include "helpers/dx11.h"

#include <d3d11shader.h>
#include <d3dcompiler.h>

#include "device.h"
#include "effectmgr.h"
#include "am/graphics/render.h"
#include "rage/atl/hashstring.h"
#include "rage/file/stream.h"

// Helper function for reading string from .fxc stream
void ReadString(const rage::fiStreamPtr& stream, rage::atConstString& string, u32* hash = nullptr)
{
	int  len = stream->ReadU8();
	char buffer[256];
	stream->Read(buffer, len);
	string = buffer;
	if (hash) *hash = rage::atStringHash(buffer);
}

u32 ReadHashedString(const rage::fiStreamPtr& stream)
{
	int  len = stream->ReadU8();
	char buffer[256];
	stream->Read(buffer, len);
	return rage::atStringHash(buffer);
}

rage::grcInstanceVar::grcInstanceVar()
{
	ValueCount = 0;
	Register = 0;
	SamplerIndex = 0;
	SavedSamplerIndex = 0;
	Value = nullptr;
}

void rage::grcInstanceVar::CopyFrom(grcInstanceVar* other)
{
	// Does not guarantee that type is the same, but will save us from accident memory overrun
	AM_ASSERTS(ValueCount == other->ValueCount);

	Register = other->Register;
	SamplerIndex = other->SamplerIndex;

	if(IsTexture())
		SetTexture(other->GetTexture());
	else
		memcpy(Value, other->Value, 16 * ValueCount);
}

rage::grcCBuffer::~grcCBuffer()
{
	delete[] m_AllocatedBackingStore;
	m_AllocatedBackingStore = nullptr;
	m_AlignedBackingStore = nullptr;
}

void* rage::grcCBuffer::Lock() const
{
	AM_ASSERTS(grcDevice::IsRenderThread());
	AM_ASSERTS(!IsLocked());

	D3D11_MAPPED_SUBRESOURCE mappedSubResource;
	AM_ASSERT_STATUS(rageam::graphics::RenderGetContext()->Map(
		m_BufferObject.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource));

	Header* header = GetHeader();
	header->IsLocked = true;
	header->IsFirstUse = false;

	return mappedSubResource.pData;
}

void rage::grcCBuffer::Unlock() const
{
	AM_ASSERTS(grcDevice::IsRenderThread());
	AM_ASSERTS(IsLocked());

	rageam::graphics::RenderGetContext()->Unmap(m_BufferObject.Get(), 0);

	Header* header = GetHeader();
	header->IsLocked = false;
	header->IsDirty = false;
}

void rage::grcCBuffer::Init()
{
	constexpr u32 headerSize = ALIGN(sizeof Header, 16);
	m_BufferStride = ALIGN(m_Size + headerSize, GRC_CACHE_LINE_SIZE);

	u32 backingStoreSize = m_BufferStride + GRC_CACHE_LINE_SIZE;
	m_AllocatedBackingStore = new char[backingStoreSize];
	m_AlignedBackingStore = (char*)ALIGN((u64)m_AllocatedBackingStore, GRC_CACHE_LINE_SIZE);
	memset(m_AllocatedBackingStore, 0, backingStoreSize);

	Header* header = GetHeader();
	header->IsDirty = false;
	header->IsLocked = false;
	header->IsFirstUse = true;

	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = m_Size;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	AM_ASSERT_STATUS(rageam::graphics::RenderGetDevice()->CreateBuffer(&desc, nullptr, &m_BufferObject));
	SetObjectDebugName(m_BufferObject.Get(), m_Name);
}

void rage::grcCBuffer::Load(const fiStreamPtr& stream)
{
	m_Size = stream->ReadU32();
	for (u16 & slot : m_ShaderSlots)
		slot = stream->ReadU16();

	ReadString(stream, m_Name, &m_NameHash);
}

u32 rage::grcCBuffer::ComputeHashKey() const
{
	char name[1024];
	sprintf_s(name, "%s.%d.%d.%d.%d.%d.%d.%d",
		m_Name.GetCStr(),
		m_Size,
		m_ShaderSlots[grcShaderVS],
		m_ShaderSlots[grcShaderPS],
		m_ShaderSlots[grcShaderCS],
		m_ShaderSlots[grcShaderDS],
		m_ShaderSlots[grcShaderGS],
		m_ShaderSlots[grcShaderHS]);
	return atStringHash(name);
}

rage::grcEffectVar::Annotation::Annotation()
{
	Type = AT_INT;
	NameHash = 0;
	Int = 0;
}

rage::grcEffectVar::Annotation::~Annotation()
{
	if (Type == AT_STRING)
		String = nullptr;
}

void rage::grcEffectVar::Annotation::Load(const fiStreamPtr& stream)
{
	NameHash = ReadHashedString(stream);
	Type = AnnoType(stream->ReadU8());
	if (Type == AT_STRING)
	{
		ReadString(stream, String);
	}
	else
	{
		// Int / Float
		stream->Read(&Int, 4);
	}
}

void rage::grcEffectVar::Load(int type, u32 textureNameHash, const fiStreamPtr& stream)
{
	m_Type = static_cast<u8>(type);
	m_Count = stream->ReadU8();

	u8 registerAndTextureType = stream->ReadU8();
	u8 usageAndComparison = stream->ReadU8();
	m_Register = registerAndTextureType & (1 << TEXTURE_REGISTER_BITS) - 1;
	m_TextureType = registerAndTextureType >> TEXTURE_REGISTER_BITS;
	m_Usage = usageAndComparison & (1 << TEXTURE_USAGE_BITS) - 1;
	m_ComparisonFilter = 0;

	ReadString(stream, m_Name, &m_NameHash);
	ReadString(stream, m_Semantic, &m_SemanticHash);

	switch (type)
	{
		case VT_UAV_STRUCTURED:
			m_SamplerIndex = m_Count;
			m_Count = 1;
			break;

		case VT_TEXTURE:
			if (m_TextureType == TEXTURE_REGULAR && textureNameHash != m_NameHash)
				m_TextureType = TEXTURE_PURE;
			if (m_TextureType == TEXTURE_SAMPLER)
				m_ComparisonFilter = m_Count;
			m_Count = 0;
			break;

		default:
			if (!m_Count) 
				m_Count = 1;
			break;
	}

	m_SlotOrCBufferOffset = stream->ReadU32();
	m_CBufferNameHash = stream->ReadU32();

	m_AnnotationCount = stream->ReadU8();
	delete[] m_Annotations;
	m_Annotations = m_AnnotationCount ? new Annotation[m_AnnotationCount] : NULL;

	// if IsMaterial is present, use that to override the computed "is this a per-material or per-instance" flag
	// (the default is a material parameter for everything but textures)
	bool isPerMaterial = m_Type != VT_TEXTURE;
	for (u8 i = 0; i < m_AnnotationCount; i++)
	{
		m_Annotations[i].Load(stream);
		if (m_Annotations[i].NameHash == atStringHash("IsMaterial") && 
			m_Annotations[i].Type == Annotation::AT_INT)
			isPerMaterial = m_Annotations[i].Int;
	}
	if (isPerMaterial)
		m_Usage |= grcVarUsageMaterial;

	m_DataSize = stream->ReadU8();
	delete[] m_Data;
	m_Data = nullptr;

	if (m_Type == VT_TEXTURE && m_TextureType != TEXTURE_SAMPLER)
	{
		// TODO: Seek...
		m_DataSize = 0;
		m_SamplerIndex = 0; // INVALID_STATEBLOCK;
	}
	else
	{
		// TODO: ...	
	}

	// TODO: ...
}

void rage::grcInstanceData::Destroy()
{
	if (m_Flags & FLAG_OWN_MATERIAL) // OwnPreset
	{
		delete m_Preset.Name;
		m_Preset.Name = nullptr;
		m_Flags &= ~FLAG_OWN_MATERIAL;
	}
	m_TextureCount = 0;
}

rage::grcInstanceData::grcInstanceData(const grcEffect* effect)
{
	m_Flags = 0;

	CloneFrom(effect->GetInstanceDataTemplate());
}

void rage::grcInstanceData::CloneFrom(const grcInstanceData& other)
{
	Destroy();

	pgRscCompiler* compiler = pgRscCompiler::GetCurrent();
	pgSnapshotAllocator* virtualAllocator = pgRscCompiler::GetVirtualAllocator();

	if (compiler)
	{
		const grcEffect* otherEffect = other.GetEffect();

		char presetName[64];
		sprintf_s(presetName, sizeof presetName, "%s.sps", otherEffect->GetName());

		m_Preset.NameHash = atStringHash(presetName);
		m_Effect.NameHash = otherEffect->GetNameHash();
	}
	else
	{
		m_Effect.Ptr = other.GetEffect();
	}

	m_VarCount = other.m_VarCount;
	m_TotalSize = other.m_TotalSize;
	m_VarsSize = other.m_VarsSize;
	m_DrawBucket = other.m_DrawBucket;
	m_DrawBucketMask = other.m_DrawBucketMask;
	m_IsInstancedDraw = other.m_IsInstancedDraw;
	m_TextureCount = other.m_TextureCount;
	m_Flags = other.m_Flags & ~FLAG_OWN_MATERIAL;
	m_PhysMtl = other.m_PhysMtl;
	m_Padding = other.m_Padding;
	m_UserFlags = other.m_UserFlags;

	// Copy vars
	if (compiler)
	{
		virtualAllocator->AllocateRef(m_Vars, m_TotalSize);
	}
	else
	{
		m_Vars = (grcInstanceVar*)rage_malloc(m_TotalSize);
	}
	memcpy(m_Vars, other.m_Vars, m_TotalSize);

	// We have to manually fixup variables data pointers to newly allocated data block
	// Textures can be skipped because they point to heap
	for (u16 i = m_TextureCount; i < m_VarCount; i++)
	{
		grcInstanceVar& newVar = m_Vars[i];
		grcInstanceVar& oldVar = other.m_Vars[i];

		// This is not the cleanest way to do it but the fastest
		// We first calculate offset of variable in old data block and then add it to base pointer of new data block
		u64 offset = reinterpret_cast<u64>(oldVar.Value) - reinterpret_cast<u64>(other.m_Vars);

		newVar.Value = reinterpret_cast<char*>(reinterpret_cast<u64>(m_Vars) + offset);
	}

	// Add compiler refs to var data pointers
	if (compiler)
	{
		for (u8 i = m_TextureCount; i < m_VarCount; i++)
		{
			grcInstanceVar& var = m_Vars[i];
			virtualAllocator->AddRef(var.Value);
		}
	}
}

rage::grcInstanceData::grcInstanceData(const grcInstanceData& other)
{
	CloneFrom(other);
}

void rage::grcInstanceData::RestoreValues(const u32* origNameHashes, grcInstanceVar* origVars, u8 origVarCount) const
{
	u32* nameHashes = GetVarNameHashes();
	for (u8 i = 0; i < origVarCount; i++)
	{
		for (u8 j = 0; j < m_VarCount; j++)
		{
			// Found the same variable in new list
			if (origNameHashes[i] == nameHashes[j])
			{
				m_Vars[j].CopyFrom(&origVars[i]);
			}
		}
	}
}

// ReSharper disable CppPossiblyUninitializedMember
// ReSharper disable CppObjectMemberMightNotBeInitialized
rage::grcInstanceData::grcInstanceData(const datResource& rsc)
{
	// TODO: We need effect / preset loader for standalone...
	#ifdef AM_STANDALONE
	AM_WARNING("grcInstanceData -> Not supported in standalone.");
	return;
	#endif

	rsc.Fixup(m_Vars);

	// Resolve effect by name hash
	u32 effectNameHash = m_Effect.NameHash;
	grcEffect* effect = grcEffectMgr::FindEffectByHashKey(effectNameHash);
	m_Effect.Ptr = effect;

	AM_ASSERT(effect != nullptr, "grcInstanceData -> Effect %u is not preloaded or doesn't exists.", effectNameHash);

	// Resolve data preset
	u32 presetNameHash = m_Preset.NameHash;
	grcInstanceData* dataPreset = LookupShaderPreset(presetNameHash);
	m_Preset.Ptr = dataPreset;

	AM_ASSERT(dataPreset->GetEffect() == effect, "grcInstanceData -> Preset use different effect.");

	// Place variables
	for (u8 i = 0; i < m_VarCount; i++)
	{
		// First we have to fix up variable
		grcInstanceVar& var = m_Vars[i];
		rsc.Fixup(var.Value);
		if (var.IsTexture())
		{
			// TODO: Should only be done for texture reference... textures are already placed in embed dictionary
			// grcTexture* texture = var.GetValuePtr<grcTexture>();
			// grcTexture::Place(rsc, texture);
		}
	}

	// We back up current instance data, apply preset and existing variables
	// This done because resource may only hold few variables (possibly on edited ones)
	u32* origNameHashes = GetVarNameHashes();
	auto origVars = m_Vars;
	u8   origVarCount = m_VarCount;
	u8   origBucket = m_DrawBucket;
	u32  origBucketMask = m_DrawBucketMask;
	u8   origFlags = m_Flags;
	CloneFrom(m_Effect.Ptr->GetInstanceDataTemplate());
	RestoreValues(origNameHashes, origVars, origVarCount);
	// We don't free origVars pointer because it's part of resource memory (placed on a large memory page)
	m_DrawBucket = origBucket;
	m_DrawBucketMask = origBucketMask;
	m_Flags = origFlags;

	UpdateTessellationBucket();

	// TODO: Register / Sampler State
}
// ReSharper restore CppPossiblyUninitializedMember
// ReSharper restore CppObjectMemberMightNotBeInitialized

rage::grcInstanceData::~grcInstanceData()
{
	Destroy();

	rage_free(m_Vars);
	m_Vars = nullptr;
}

void rage::grcInstanceData::CopyVars(const grcInstanceData* to) const
{
	grcEffect* lhsEffect = GetEffect();
	grcEffect* rhsEffect = to->GetEffect();

	for (u16 i = 0; i < m_VarCount; i++)
	{
		grcEffectVar* varInfo = lhsEffect->GetVar(i);

		// Attempt to find variable in destination effect
		grcHandle rhsHandle = rhsEffect->LookupVar(varInfo->GetNameHash());
		if (!rhsHandle)
			continue;

		u16 rhsIndex = rhsHandle.GetIndex();

		grcInstanceVar* lhsVar = GetVar(i);
		grcInstanceVar* rhsVar = to->GetVar(rhsIndex);

		grcEffectVarType lhsType = varInfo->GetType();
		grcEffectVarType rhsType = rhsEffect->GetVar(rhsIndex)->GetType();

		// Although variables may have the same name, we must ensure that type matches too
		if (lhsType != rhsType)
			continue;

		// Now we can copy data to destination
		if (varInfo->GetType() == VT_TEXTURE)
		{
			rhsVar->SetTexture(lhsVar->GetTexture());
		}
		else
		{
			void* src = lhsVar->GetValuePtr<void>();
			void* dst = rhsVar->GetValuePtr<void>();
			// Data is stored in 16 byte blocks, this was most likely made to iterate them easier (game does that)
			// another possible explanation is that this is union array with Vector4 as largest element
			//
			// Also It would make sense that matrices are array of Vector4 because
			// otherwise data block would take 64 bytes instead of 16
			u8 dataSize = 16 * lhsVar->GetValueCount();
			memcpy(dst, src, dataSize);
		}
	}
}

void rage::grcInstanceData::UpdateTessellationBucket()
{
	bool tesselalationOn = false;
	grcHandle useTesselationHandle = GetEffect()->LookupVar("usetessellation");
	if (useTesselationHandle.IsValid()) // Check if shader supports tessellation at all
	{
		grcInstanceVar* useTesselationVar = GetVar(useTesselationHandle);
		tesselalationOn = useTesselationVar->GetValue<float>() == 1.0f;
	}
	m_DrawBucketMask.SetTessellated(tesselalationOn);
}

void rage::grcInstanceData::SetEffect(const grcEffect* effect, bool keepValues)
{
	u32* origNameHashes = GetVarNameHashes();
	u8   origVarCount = m_VarCount;
	auto origVars = m_Vars;

	CloneFrom(effect->GetInstanceDataTemplate());

	if (keepValues)
	{
		RestoreValues(origNameHashes, origVars, origVarCount);
	}

	rage_free(origVars);
}

rage::grcInstanceVar* rage::grcInstanceData::GetVar(u16 index) const
{
	AM_ASSERT(index < m_VarCount, "grcInstanceData::GetVar() -> Index %u is out of range.", index);
	return &m_Vars[index];
}

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

	m_Shader = amComPtr<ID3D11VertexShader>(static_cast<ID3D11VertexShader*>(
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

	// We must provide elements in the same order as they're declared in 'bit' order,
	// easiest way to do this is to recreate declaration
	grcFvf fvf;
	decl->EncodeFvf(fvf);
	delete decl;
	decl = grcVertexDeclaration::CreateFromFvf(fvf);

	if (pFvf) *pFvf = fvf;
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

	m_Shader = amComPtr<ID3D11PixelShader>(static_cast<ID3D11PixelShader*>(
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
	m_Shader = amComPtr<ID3D11ComputeShader>(static_cast<ID3D11ComputeShader*>(
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
	m_Shader = amComPtr<ID3D11DomainShader>(static_cast<ID3D11DomainShader*>(
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
		m_Shader = amComPtr<ID3D11GeometryShader>(static_cast<ID3D11GeometryShader*>(
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
	m_Shader = amComPtr<ID3D11HullShader>(static_cast<ID3D11HullShader*>(
		grcCreateProgramByTypeCached(GetName(), GRC_PROGRAM_HULL, bytecode, m_BytecodeSize)));
	m_Shader_ = m_Shader; // They're identical in release build

	rage_free(bytecode);
}

rage::grcEffectTechnique* rage::grcEffect::GetTechnique(ConstString name) const
{
	u32 hash = atStringHash(name);
	for (grcEffectTechnique& technique : m_Techniques)
	{
		if (technique.m_NameHash == hash)
			return &technique;
	}
	return nullptr;
}

rage::grcHandle rage::grcEffect::LookupVar(u32 hash)
{
	for (u16 i = 0; i < m_Locals.GetSize(); i++)
	{
		grcEffectVar* var = &m_Locals[i];
		if (var->GetNameHash() == hash || var->GetSemanticHash() == hash)
			return grcHandle(i);
	}
	return grcHandle::Null();
}

rage::grcHandle rage::grcEffect::LookupTechnique(u32 hash)
{
	for (u16 i = 0; i < m_Techniques.GetSize(); i++)
	{
		if (m_Techniques[i].m_NameHash == hash)
			return grcHandle(i);
	}
	return grcHandle::Null();
}

rage::grcEffectVar* rage::grcEffect::GetVar(ConstString name, u16* outIndex)
{
	if (outIndex) *outIndex = u16(-1);

	u32 hash = atStringHash(name);
	for (u16 i = 0; i < m_Locals.GetSize(); i++)
	{
		grcEffectVar& var = m_Locals[i];
		if (var.GetNameHash() == hash || var.GetSemanticHash() == hash)
		{
			if (outIndex) *outIndex = i;
			return &var;
		}
	}
	return nullptr;
}
