#include "geometry.h"

rage::grcVertexBufferD3D11* rage::grmGeometryQB::GetPrimaryVertexBuffer() const
{
	// The rest (3) ones are almost never used, this is un-researched so force to the first one
	return m_VertexBuffers[0].Get();
}

rage::grcIndexBufferD3D11* rage::grmGeometryQB::GetPrimaryIndexBuffer() const
{
	// Same as in GetVertexBuffer()
	return m_IndexBuffers[0].Get();
}

rage::grmGeometryQB::grmGeometryQB()
{
	m_VertexDeclaration = nullptr;
	m_VertexStride = 0;

	m_IndexCount = 0;
	m_PrimitiveCount = 0;
	m_VertexCount = 0;
	m_PrimitiveType = PRIM_TRIANGLES;
	m_QuadTreeData = 0;
	m_BoneIDsCount = 0;

	m_Unknown10 = 0;
	m_Unknown14 = 0;
	m_Unknown71 = 0;
	m_Unused78 = 0;
	m_UnusedVertexDeclaration = nullptr;

	m_VertexBuffers[0] = new grcVertexBufferD3D11();
	m_IndexBuffers[0] = new grcIndexBufferD3D11();
}

// ReSharper disable once CppPossiblyUninitializedMember
rage::grmGeometryQB::grmGeometryQB(const datResource& rsc)
{
	const grcFvf* fvf = GetPrimaryVertexBuffer()->GetVertexFormat();
	m_VertexDeclaration = grcVertexDeclaration::CreateFromFvf(*fvf);
}

void rage::grmGeometryQB::SetVertexData(const grmVertexData& vertexData)
{
	grcVertexDeclarationPtr decl = vertexData.Info.Decl;
	const grcFvf& fvf = vertexData.Info.Fvf;

	m_VertexDeclaration = decl;
	m_VertexStride = static_cast<u8>(decl->Stride);
	m_VertexCount = static_cast<u16>(vertexData.VertexCount);
	m_PrimitiveCount = vertexData.VertexCount / 3;
	m_IndexCount = vertexData.IndexCount;

	GetPrimaryVertexBuffer()->CreateWithData(fvf, vertexData.Vertices, vertexData.VertexCount);
	GetPrimaryIndexBuffer()->CreateWithData(vertexData.Indices, vertexData.IndexCount);
}

void rage::grmGeometryQB::Draw()
{
	grcVertexBuffer* vertexBuffer = GetPrimaryVertexBuffer();
	grcIndexBuffer* indexBuffer = GetPrimaryIndexBuffer();
	if (!vertexBuffer || !indexBuffer)
		return;

	grcDevice::DrawIndexedPrimitive(
		(grcDrawMode)m_PrimitiveType, m_VertexDeclaration, vertexBuffer, indexBuffer, m_IndexCount);
}

rage::grcIndexBuffer* rage::grmGeometryQB::GetIndexBuffer(int quad) const
{
	return m_IndexBuffers[quad].Get();
}

u32 rage::grmGeometryQB::GetTotalVertexCount() const
{
	u32 vtxCount = 0;
	for (int i = 0; i < GRID_SIZE; i++)
	{
		grcVertexBufferD3D11* vtxBuffer = m_VertexBuffers[i].Get();
		if (vtxBuffer) vtxCount += vtxBuffer->GetVertexCount();
	}
	return vtxCount;
}
