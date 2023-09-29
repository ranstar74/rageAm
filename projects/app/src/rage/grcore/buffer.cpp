#include "buffer.h"

#include "device.h"

void rage::grcBufferD3D11::Create(const void* data, u32 elementCount, u32 stride, u32 bindFlags, u32 miscFlags, D3D11_USAGE usage, u32 cpuAccess, bool keepData)
{
	// TODO: There's a lot other things happen in this function, this covers it really basically

	m_Size = elementCount * stride;

	D3D11_BUFFER_DESC desc;
	desc.StructureByteStride = stride;
	desc.BindFlags = bindFlags;
	desc.MiscFlags = miscFlags;
	desc.ByteWidth = m_Size;
	desc.CPUAccessFlags = cpuAccess;
	desc.Usage = usage;

	D3D11_SUBRESOURCE_DATA initData;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;
	initData.pSysMem = data;

	auto pInitData = &initData;
	if (usage == D3D11_USAGE_DYNAMIC && data == nullptr)
		pInitData = nullptr;

	ID3D11Buffer* buffer;
	ID3D11Device* factory = GetDeviceD3D11();
	HRESULT result = factory->CreateBuffer(&desc, pInitData, &buffer);
	if (result != S_OK)
	{
		AM_ERRF("grcBufferD3D11::Create() -> Failed to create buffer, error code: %u", result);
		return;
	}

	m_Object = buffer;

	// Make data copy if present
	if (keepData && data)
	{
		char* dataCopy = (char*)rage_malloc(m_Size);
		memcpy(dataCopy, data, m_Size);
		m_DataCopy = dataCopy;
	}
}

pVoid rage::grcBufferD3D11::Bind() const
{
	D3D11_MAPPED_SUBRESOURCE mapped;
	AM_ASSERT(GetDeviceContextD3D11()->Map(m_Object.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped) == S_OK,
		"grcBufferD3D11::Bind() -> Failed to bind buffer.");
	return mapped.pData;
}

void rage::grcBufferD3D11::Unbind() const
{
	GetDeviceContextD3D11()->Unmap(m_Object.Get(), 0);
}

rage::grcVertexBuffer::grcVertexBuffer()
{
	m_VertexStride = 0;
	m_VertexCount = 0;

	m_UnknownA = 0;
	m_UnknownB = 0;
	m_Unknown10 = 0;
	m_Unknown28 = 0;
}

const rage::grcFvf* rage::grcVertexBuffer::GetVertexFormat() const
{
	return m_VertexFormat.Get();
}

void rage::grcVertexBuffer::SetVertexFormat(const grcFvf& format)
{
	m_VertexFormat = new grcFvf(format);
	m_VertexStride = format.Stride;
}

void rage::grcVertexBuffer::SetVertexCount(u32 count)
{
	m_VertexCount = count;
}

void rage::grcVertexBufferD3D11::CreateWithData(const grcFvf& fvf, const void* vertices, u32 vertexCount)
{
	SetVertexCount(vertexCount);
	SetVertexFormat(fvf);
	m_Buffer.Create(vertices, vertexCount, fvf.Stride, D3D11_BIND_VERTEX_BUFFER, 0, D3D11_USAGE_DEFAULT, 0, true);

	//// Copy data so we can serialize it later
	//u32 verticesSize = vertexCount * fvf.Stride;
	//char* dataCopy = new char[verticesSize];
	//memcpy(dataCopy, vertices, verticesSize);
	//m_VertexData = dataCopy;
	m_VertexData.Set((char*)m_Buffer.GetInitialData());
}

void rage::grcVertexBufferD3D11::CreateDynamic(const grcFvf& fvf, u32 vertexCount)
{
	SetVertexFormat(fvf);
	m_Buffer.Create(nullptr, vertexCount, fvf.Stride, D3D11_BIND_VERTEX_BUFFER, 0, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void rage::grcIndexBufferD3D11::CreateWithData(const u16* indices, u32 indexCount)
{
	SetIndexCount(indexCount);
	m_Buffer.Create(indices, indexCount, sizeof u16, D3D11_BIND_INDEX_BUFFER, 0, D3D11_USAGE_DEFAULT);

	//// Copy data so we can serialize it later
	//u32 indicesSize = indexCount * sizeof u16;
	//u16* dataCopy = new u16[indexCount];
	//memcpy(dataCopy, indices, indicesSize);
	//m_Indices = dataCopy;
	m_Indices.Set((u16*)m_Buffer.GetInitialData());
}

void rage::grcIndexBufferD3D11::CreateDynamic(u32 indexCount)
{
	m_Buffer.Create(nullptr, indexCount, sizeof u16, D3D11_BIND_INDEX_BUFFER, 0, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}
