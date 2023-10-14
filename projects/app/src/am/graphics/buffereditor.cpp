#include "buffereditor.h"

#include "utils.h"

void rageam::graphics::VertexBufferEditor::AllocateBuffer(u32 vertexCount)
{
	u32 bufferSize = vertexCount * m_Decl.Stride;
	m_Buffer = new char[bufferSize];
	m_OwnBufferMemory = true;
	m_VertexCount = vertexCount;
}

void rageam::graphics::VertexBufferEditor::SetBuffer(char* buffer, u32 vertexCount)
{
	m_Buffer = buffer;
	m_VertexCount = vertexCount;
	m_OwnBufferMemory = false;
}

void rageam::graphics::VertexBufferEditor::SetData(u32 vertexIndex, const VertexAttribute* attribute, DXGI_FORMAT format, pVoid in) const
{
	pVoid dest = (char*)m_Buffer + GetBufferOffset(vertexIndex, attribute->Offset);
	memcpy(dest, in, attribute->SizeInBytes);
}

bool rageam::graphics::VertexBufferEditor::CanConvertColor(DXGI_FORMAT toFormat, DXGI_FORMAT inFormat) const
{
	if (toFormat == inFormat)
		return true;

	// float[4] to u32
	if (inFormat == DXGI_FORMAT_R32G32B32A32_FLOAT && toFormat == DXGI_FORMAT_R8G8B8A8_UNORM)
		return true;

	// u32 to float[4]
	if (inFormat == DXGI_FORMAT_R8G8B8A8_UNORM && toFormat == DXGI_FORMAT_R32G32B32A32_FLOAT)
		return true;

	// u16[4] to float[4]
	if (inFormat == DXGI_FORMAT_R16G16B16A16_UINT && toFormat == DXGI_FORMAT_R32G32B32A32_FLOAT)
		return true;

	return false;
}

bool rageam::graphics::VertexBufferEditor::CanConvertSetBlendWeight(DXGI_FORMAT toFormat, DXGI_FORMAT inFormat) const
{
	if (toFormat == inFormat)
		return true;

	return false;
}

bool rageam::graphics::VertexBufferEditor::CanConvertSetBlendIndices(DXGI_FORMAT toFormat, DXGI_FORMAT inFormat) const
{
	if (toFormat == inFormat)
		return true;

	// In V blend indices are stored in float[] and then remapped to 0-255 and used as index in gBoneMtx
	// 'D3DCOLORtoUBYTE4(indices + offset)
	if (toFormat == DXGI_FORMAT_R32G32B32A32_FLOAT && inFormat == DXGI_FORMAT_R8G8B8A8_UINT)
		return true;

	return false;
}

void rageam::graphics::VertexBufferEditor::Destroy()
{
	if (m_OwnBufferMemory)
		delete m_Buffer;

	m_Buffer = nullptr;
	m_OwnBufferMemory = false;
}

rageam::graphics::VertexBufferEditor::VertexBufferEditor(const VertexDeclaration& decl)
{
	m_Decl = decl;

}

rageam::graphics::VertexBufferEditor::VertexBufferEditor(const rage::grcVertexFormatInfo& vtxInfo)
{
	m_Decl.SetGrcInfo(vtxInfo);
}

rageam::graphics::VertexBufferEditor::~VertexBufferEditor()
{
	Destroy();
}

void rageam::graphics::VertexBufferEditor::Init(u32 vertexCount, char* buffer)
{
	m_VertexCount = vertexCount;
	if (buffer)
	{
		m_Buffer = buffer;
		m_OwnBufferMemory = false;
	}
	else
	{
		u64 bufferSize = ComputeBufferSize();
		m_Buffer = new char[bufferSize];
		m_OwnBufferMemory = true;
	}
}

void rageam::graphics::VertexBufferEditor::SetFromGeometry(const SceneGeometry* geometry)
{
	for (const VertexAttribute& attribute : m_Decl.Attributes)
	{
		SceneData packedData;
		if (!geometry->GetAttribute(packedData, attribute.Semantic, attribute.SemanticIndex))
			continue; // Geometry contains no such vertex data

		switch (attribute.Semantic)
		{
		case POSITION:
			if (packedData.Format == DXGI_FORMAT_R32G32B32_FLOAT)
				SetPositions(packedData.GetBufferAs<rage::Vector3>());
			else
				AM_WARNINGF("VertexBufferEditor() -> Unsupported position data type '%s'.", Enum::GetName(packedData.Format));
			break;

		case NORMAL:
			if (packedData.Format == DXGI_FORMAT_R32G32B32_FLOAT)
				SetNormals(packedData.GetBufferAs<rage::Vector3>());
			else
				AM_WARNINGF("VertexBufferEditor() -> Unsupported normal data type '%s'.", Enum::GetName(packedData.Format));
			break;

		case TANGENT:
			if (packedData.Format == DXGI_FORMAT_R32G32B32A32_FLOAT)
				SetTangents(packedData.GetBufferAs<rage::Vector4>());
			else
				AM_WARNINGF("VertexBufferEditor() -> Unsupported tangent data type '%s'.", Enum::GetName(packedData.Format));
			break;

		case TEXCOORD:
			if (packedData.Format == DXGI_FORMAT_R32G32_FLOAT)
				SetTexcords(attribute.SemanticIndex, packedData.GetBufferAs<rage::Vector2>());
			else
				AM_WARNINGF("VertexBufferEditor() -> Unsupported texcoord data type '%s'.", Enum::GetName(packedData.Format));
			break;

		case COLOR:
			SetColor(attribute.SemanticIndex, packedData.Buffer, packedData.Format);
			break;

			// See function comment in header
		case BLENDINDICES:
			//	SetBlendIndices(packedData.Buffer, packedData.Format);
			break;

		case BLENDWEIGHT:
			SetBlendWeights(packedData.Buffer, packedData.Format);
			break;

		default:
			AM_WARNINGF("VertexBufferEditor() -> Semantic '%s' is not supported.", Enum::GetName(attribute.Semantic));
		}
	}
}

u64 rageam::graphics::VertexBufferEditor::GetBufferOffset(u32 vertexIndex, u64 vertexOffset) const
{
	return (u64)m_Decl.Stride * (u64)vertexIndex + vertexOffset;
}

void rageam::graphics::VertexBufferEditor::SetColorSingle(u32 semanticIndex, u32 color)
{
	static constexpr DXGI_FORMAT inFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	const VertexAttribute* attr = m_Decl.FindAttribute(COLOR, semanticIndex);

	if (!CanConvertColor(attr->Format, inFormat))
	{
		AM_ERRF("VertexBufferEditor() -> Can't convert color from '%s' to '%s'!",
			Enum::GetName(inFormat), Enum::GetName(attr->Format));
		return;
	}
	
	// Instead of converting color million times we set it for the first vertex
	// and then just copy to remaining elements
	ConvertAndSetColor(0, attr, attr->Format, inFormat, &color);
	pVoid colorSrc = m_Buffer + attr->Offset; // In the first vertex
	for (u32 i = 1; i < m_VertexCount; i++)
	{
		pVoid colorDst = m_Buffer + GetBufferOffset(i, attr->Offset);
		memcpy(colorDst, colorSrc, attr->SizeInBytes);
	}

	SetSemantic(COLOR, semanticIndex);
}

void rageam::graphics::VertexBufferEditor::SetColor(u32 semanticIndex, pVoid colors, DXGI_FORMAT inFormat)
{
	const VertexAttribute* attr = m_Decl.FindAttribute(COLOR, semanticIndex);

	if (!CanConvertColor(attr->Format, inFormat))
	{
		AM_ERRF("VertexBufferEditor() -> Can't convert color from '%s' to '%s'!",
			Enum::GetName(inFormat), Enum::GetName(attr->Format));
		return;
	}

	size_t inStride = FormatStride(inFormat);
	for (u32 i = 0; i < m_VertexCount; i++)
	{
		char* inColor = static_cast<char*>(colors) + inStride * i;
		ConvertAndSetColor(i, attr, attr->Format, inFormat, inColor);
	}
	SetSemantic(COLOR, semanticIndex);
}

void rageam::graphics::VertexBufferEditor::SetBlendWeights(pVoid weights, DXGI_FORMAT inFormat)
{
	const VertexAttribute* attr = m_Decl.FindAttribute(BLENDWEIGHT, 0);

	if (!CanConvertSetBlendWeight(attr->Format, inFormat))
	{
		AM_ERRF("VertexBufferEditor() -> Can't convert blend weight from '%s' to '%s'!",
			Enum::GetName(inFormat), Enum::GetName(attr->Format));
		return;
	}

	size_t inStride = FormatStride(inFormat);
	for (u32 i = 0; i < m_VertexCount; i++)
	{
		char* inWeight = static_cast<char*>(weights) + inStride * i;
		ConvertAndSetBlendWeights(i, attr, attr->Format, inFormat, inWeight);
	}
	SetSemantic(BLENDWEIGHT, 0);
}

void rageam::graphics::VertexBufferEditor::SetBlendIndices(pVoid indices, DXGI_FORMAT inFormat)
{
	const VertexAttribute* attr = m_Decl.FindAttribute(BLENDINDICES, 0);

	if (!CanConvertSetBlendIndices(attr->Format, inFormat))
	{
		AM_ERRF("VertexBufferEditor() -> Can't convert blend indices from '%s' to '%s'!",
			Enum::GetName(inFormat), Enum::GetName(attr->Format));
		return;
	}

	size_t inStride = FormatStride(inFormat);
	for (u32 i = 0; i < m_VertexCount; i++)
	{
		char* inIndices = static_cast<char*>(indices) + inStride * i;
		ConvertAndSetBlendIndices(i, attr, attr->Format, inFormat, inIndices);
	}
	SetSemantic(BLENDINDICES, 0);
}
