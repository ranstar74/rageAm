#include "drawlist.h"

#ifdef AM_INTEGRATED

#include "am/graphics/buffereditor.h"
#include "am/system/datamgr.h"
#include "game/viewport.h"
#include "helpers/com.h"
#include "rage/grcore/device.h"
#include "rage/grcore/effect/effectmgr.h"

#include <d3dcompiler.h>

rageam::integration::DrawListDummyDrawable::DrawListDummyDrawable()
{
	rage::grcEffect* effect = rage::grcEffectMgr::FindEffect("default");
	rage::grcVertexFormatInfo vertexFormatInfo;
	vertexFormatInfo.FromEffect(effect, false);

	rage::spdAABB bb = AABB(Vec3V(-3, -3, -3), Vec3V(3, 3, 3));

	char* dummyVertexData = new char[(size_t)vertexFormatInfo.Decl->Stride] {};
	u16* dummmyIndexData = new u16[]{ 0, 0, 0 };

	// Add dummy geometry
	rage::grmVertexData vertexData;
	vertexData.VertexCount = 1;
	vertexData.IndexCount = 3;
	vertexData.Vertices = dummyVertexData;
	vertexData.Indices = dummmyIndexData;
	vertexData.Info = std::move(vertexFormatInfo);
	rage::pgUPtr geometry = new rage::grmGeometryQB();
	geometry->SetVertexData(vertexData);
	rage::grmModel* model = new rage::grmModel();
	model->AddGeometry(geometry, bb);
	rage::grmModels& lodModels = m_LodGroup.GetLod(0)->GetModels();
	lodModels.Emplace(model);

	rage::grmShader* shader = new rage::grmShader(effect);
	m_ShaderGroup->AddShader(shader);

	GetLodGroup().CalculateExtents();
	ComputeBucketMask(); // NOTE: We except mask to set to 0xFFFF!
}

void rageam::integration::DrawListDummyDrawable::AddDrawList(DrawList* dl)
{
	m_DrawLists.Add(dl);
}

void rageam::integration::DrawListDummyDrawable::Draw(const Mat34V& mtx, rage::grcRenderMask mask, rage::eDrawableLod lod)
{
	// Let game to render our dummy geometry to setup rendering for us
	gtaDrawable::Draw(mtx, mask, lod);

	DrawListExecutor* executor = DrawListExecutor::GetCurrent();

	for (DrawList* dl : m_DrawLists)
	{
		if (mask & (1 << 0)) // Diffuse bucket
		{
			if (!dl->Unlit) executor->Execute(*dl);
		}

		if (mask & (1 << 1)) // Alpha bucket
		{
			if (dl->Unlit) executor->Execute(*dl);
		}
	}
}

void rageam::integration::DrawListDummyEntity::ReleaseAllRefs()
{
	m_GameEntity.Release();
}

void rageam::integration::DrawListDummyEntity::OnEarlyUpdate()
{
	// We keep entity always around camera because otherwise
	// game will cull it out on bounding check
	m_GameEntity->SetPosition(CViewport::GetCameraPos());
}

void rageam::integration::DrawListDummyEntity::Spawn()
{
	static constexpr u32 DUMMY_NAME_HASH = rage::joaat("amDrawListDummy");

	m_DummyDrawable = std::make_shared<DrawListDummyDrawable>();
	auto archetypeDef = std::make_shared<CBaseArchetypeDef>();
	const AABB& bb = m_DummyDrawable->GetBoundingBox();
	const Sphere& bs = m_DummyDrawable->GetBoundingSphere();
	archetypeDef->BoundingBox = bb;
	archetypeDef->BsCentre = bs.GetCenter();
	archetypeDef->BsRadius = bs.GetRadius().Get();
	archetypeDef->Name = DUMMY_NAME_HASH;
	archetypeDef->AssetName = DUMMY_NAME_HASH;
	archetypeDef->Flags = rage::ADF_STATIC;
	archetypeDef->LodDist = archetypeDef->BsRadius;

	m_GameEntity.Create();
	m_GameEntity->Spawn(m_DummyDrawable, archetypeDef, rage::VEC_ORIGIN);
}

bool rageam::integration::DrawList::VerifyBufferFitLine(const DrawData& drawData) const
{
	bool can = drawData.VTXBuffer.VertexCount + 2 <= VTX_MAX;
	if (!can) AM_WARNINGF("OverlayRender -> Line buffer size must be extended!");
	return can;
}

bool rageam::integration::DrawList::VerifyBufferFitTri(const DrawData& drawData, u32 vertexCount, u32 indexCount) const
{
	bool can =
		drawData.VTXBuffer.VertexCount + vertexCount <= VTX_MAX &&
		drawData.IDXBuffer.IndexCount + indexCount <= IDX_MAX;
	if (!can) AM_WARNINGF("OverlayRender -> Tri buffer size must be extended!");
	return can;
}

rageam::integration::DrawList::DrawList()
{
	for (int i = 0; i < 2; i++) // Back & Front buffers
	{
		m_DrawDatas[i].Line.IsIndexed = false;
		m_DrawDatas[i].Line.TopologyType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
		m_DrawDatas[i].Tris.IsIndexed = true;
		m_DrawDatas[i].Tris.TopologyType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
}

ID3D11Buffer* rageam::integration::DrawList::CreateBuffer(size_t elementSize, size_t elementCount, bool vertexBuffer)
{
	D3D11_BUFFER_DESC desc;
	desc.BindFlags = vertexBuffer ? D3D11_BIND_VERTEX_BUFFER : D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = elementSize * elementCount;
	desc.StructureByteStride = elementSize;
	desc.MiscFlags = 0;

	ID3D11Buffer* object;
	AM_ASSERT_STATUS(graphics::RenderGetDevice()->CreateBuffer(&desc, NULL, &object));
	return object;
}

void rageam::integration::DrawList::UploadBuffer(const amComPtr<ID3D11Buffer>& buffer, pConstVoid data, u32 dataSize)
{
	ID3D11DeviceContext* context = graphics::RenderGetContext();
	D3D11_MAPPED_SUBRESOURCE mapped;
	AM_ASSERT_STATUS(context->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
	memcpy(mapped.pData, data, dataSize);
	context->Unmap(buffer.Get(), 0);
}

void rageam::integration::DrawList::DrawLine_Unsafe(
	const rage::Vec3V& p1, const rage::Vec3V& p2, const rage::Mat44V& mtx, ColorU32 col1, ColorU32 col2)
{
	if (col1.A == 0 && col2.A == 0)
		return;

	DrawData& drawData = GetBackDrawData().Line;
	if (!VerifyBufferFitLine(drawData))
		return;

	rage::Vec3V pt1 = p1.Transform(mtx);
	rage::Vec3V pt2 = p2.Transform(mtx);
	drawData.VTXBuffer.Vertices[drawData.VTXBuffer.VertexCount++] = Vertex(pt1, col1.ToVec4());
	drawData.VTXBuffer.Vertices[drawData.VTXBuffer.VertexCount++] = Vertex(pt2, col2.ToVec4());
}

void rageam::integration::DrawList::DrawLine_Unsafe(
	const rage::Vec3V& p1, const rage::Vec3V& p2, const rage::Mat44V& mtx, ColorU32 col)
{
	DrawLine_Unsafe(p1, p2, mtx, col, col);
}

void rageam::integration::DrawList::DrawLine_Unsafe(
	const rage::Vec3V& p1, const rage::Vec3V& p2, ColorU32 col1, ColorU32 col2)
{
	if (col1.A == 0 && col2.A == 0)
		return;

	DrawData& drawData = GetBackDrawData().Line;
	if (!VerifyBufferFitLine(drawData))
		return;

	drawData.VTXBuffer.Vertices[drawData.VTXBuffer.VertexCount++] = Vertex(p1, col1.ToVec4());
	drawData.VTXBuffer.Vertices[drawData.VTXBuffer.VertexCount++] = Vertex(p2, col2.ToVec4());
}

void rageam::integration::DrawList::DrawLine_Unsafe(const rage::Vec3V& p1, const rage::Vec3V& p2, ColorU32 col)
{
	DrawLine_Unsafe(p1, p2, col, col);
}

rageam::Vec3V rageam::integration::DrawList::TriNormal(const Vec3V& v1, const Vec3V& v2, const Vec3V& v3) const
{
	rage::Vec3V p1 = v3 - v1;
	rage::Vec3V p2 = v3 - v2;
	return p1.Cross(p2).Normalized();
}

void rageam::integration::DrawList::DrawTriFill_Unsafe(
	const Vec3V& p1, const Vec3V& p2, const Vec3V& p3,
	ColorU32 col1, ColorU32 col2, ColorU32 col3)
{
	if (col1.A == 0 && col2.A == 0 && col3.A == 0)
		return;

	DrawData& drawData = GetBackDrawData().Tris;
	if (!VerifyBufferFitTri(drawData, 3, 3))
		return;

	Vec3V n = TriNormal(p1, p2, p3);

	u32 startVertex = drawData.VTXBuffer.VertexCount;
	drawData.VTXBuffer.Vertices[drawData.VTXBuffer.VertexCount++] = Vertex(p1, col1.ToVec4(), n);
	drawData.VTXBuffer.Vertices[drawData.VTXBuffer.VertexCount++] = Vertex(p2, col2.ToVec4(), n);
	drawData.VTXBuffer.Vertices[drawData.VTXBuffer.VertexCount++] = Vertex(p3, col3.ToVec4(), n);
	drawData.IDXBuffer.Indices[drawData.IDXBuffer.IndexCount++] = startVertex + 0;
	drawData.IDXBuffer.Indices[drawData.IDXBuffer.IndexCount++] = startVertex + 1;
	drawData.IDXBuffer.Indices[drawData.IDXBuffer.IndexCount++] = startVertex + 2;
}

void rageam::integration::DrawList::DrawTriFill_Unsafe(
	const Vec3V& p1, const Vec3V& p2, const Vec3V& p3,
	ColorU32 col1, ColorU32 col2, ColorU32 col3,
	const Mat44V& mtx)
{
	return DrawTriFill_Unsafe(p1.Transform(mtx), p2.Transform(mtx), p3.Transform(mtx), col1, col2, col3);
}

void rageam::integration::DrawList::DrawQuadFill_Unsafe(
	const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, const Vec3V& p4, ColorU32 col, const Mat44V& mtx)
{
	if (col.A == 0)
		return;

	DrawData& drawData = GetBackDrawData().Line;
	if (!VerifyBufferFitTri(drawData, 4, 6))
		return;

	Vec4S colVec = col.ToVec4();
	Vec3V n1 = TriNormal(p1, p2, p3);
	Vec3V n2 = TriNormal(p2, p3, p4);
	Vec3V n = (n1 + n2) * rage::S_HALF; // Average

	u32 startVertex = drawData.VTXBuffer.VertexCount;
	drawData.VTXBuffer.Vertices[drawData.VTXBuffer.VertexCount++] = Vertex(p1, colVec, n);
	drawData.VTXBuffer.Vertices[drawData.VTXBuffer.VertexCount++] = Vertex(p2, colVec, n);
	drawData.VTXBuffer.Vertices[drawData.VTXBuffer.VertexCount++] = Vertex(p3, colVec, n);
	drawData.VTXBuffer.Vertices[drawData.VTXBuffer.VertexCount++] = Vertex(p4, colVec, n);
	drawData.IDXBuffer.Indices[drawData.IDXBuffer.IndexCount++] = startVertex + 0;
	drawData.IDXBuffer.Indices[drawData.IDXBuffer.IndexCount++] = startVertex + 1;
	drawData.IDXBuffer.Indices[drawData.IDXBuffer.IndexCount++] = startVertex + 2;
	drawData.IDXBuffer.Indices[drawData.IDXBuffer.IndexCount++] = startVertex + 0;
	drawData.IDXBuffer.Indices[drawData.IDXBuffer.IndexCount++] = startVertex + 3;
	drawData.IDXBuffer.Indices[drawData.IDXBuffer.IndexCount++] = startVertex + 2;
}

void rageam::integration::DrawList::DrawLine(const rage::Vec3V& p1, const rage::Vec3V& p2, const rage::Mat44V& mtx, ColorU32 col1, ColorU32 col2)
{
	std::unique_lock lock(m_Mutex);
	DrawLine_Unsafe(p1, p2, mtx, col1, col2);
}

void rageam::integration::DrawList::DrawLine(const rage::Vec3V& p1, const rage::Vec3V& p2, const rage::Mat44V& mtx, ColorU32 col)
{
	DrawLine(p1, p2, mtx, col, col);
}

void rageam::integration::DrawList::DrawLine(const rage::Vec3V& p1, const rage::Vec3V& p2, ColorU32 col1, ColorU32 col2)
{
	std::unique_lock lock(m_Mutex);
	DrawLine_Unsafe(p1, p2, tl_Transform, col1, col2);
}

void rageam::integration::DrawList::DrawLine(const rage::Vec3V& p1, const rage::Vec3V& p2, ColorU32 col)
{
	DrawLine(p1, p2, tl_Transform, col, col);
}

void rageam::integration::DrawList::DrawAABB(const rage::spdAABB& bb, const rage::Mat44V& mtx, ColorU32 col)
{
	std::unique_lock lock(m_Mutex);
	// Top Face
	DrawLine_Unsafe(bb.TTL(), bb.TTR(), mtx, col);
	DrawLine_Unsafe(bb.TTR(), bb.TBR(), mtx, col);
	DrawLine_Unsafe(bb.TBR(), bb.TBL(), mtx, col);
	DrawLine_Unsafe(bb.TBL(), bb.TTL(), mtx, col);
	// Bottom Face
	DrawLine_Unsafe(bb.BTL(), bb.BTR(), mtx, col);
	DrawLine_Unsafe(bb.BTR(), bb.BBR(), mtx, col);
	DrawLine_Unsafe(bb.BBR(), bb.BBL(), mtx, col);
	DrawLine_Unsafe(bb.BBL(), bb.BTL(), mtx, col);
	// Vertical Edges
	DrawLine_Unsafe(bb.BTL(), bb.TTL(), mtx, col);
	DrawLine_Unsafe(bb.BTR(), bb.TTR(), mtx, col);
	DrawLine_Unsafe(bb.BBL(), bb.TBL(), mtx, col);
	DrawLine_Unsafe(bb.BBR(), bb.TBR(), mtx, col);
}

void rageam::integration::DrawList::DrawQuad(
	const rage::Vec3V& pos,
	const rage::Vec3V& normal,
	const rage::Vec3V& tangent,
	const rage::ScalarV& extentX,
	const rage::ScalarV& extentY,
	ColorU32 col)
{
	rage::Vec3V biNormal = normal.Cross(tangent);

	// If looking at plane where tangent is right and binormal is top
	rage::Vec3V tr = pos + tangent * extentX + biNormal * extentY;
	rage::Vec3V tl = pos - tangent * extentX + biNormal * extentY;
	rage::Vec3V bl = pos - tangent * extentX - biNormal * extentY;
	rage::Vec3V br = pos + tangent * extentX - biNormal * extentY;

	std::unique_lock lock(m_Mutex);
	DrawLine_Unsafe(tr, tl, col);
	DrawLine_Unsafe(tl, bl, col);
	DrawLine_Unsafe(bl, br, col);
	DrawLine_Unsafe(br, tr, col);
}

void rageam::integration::DrawList::DrawCircle(
	const rage::Vec3V& pos,
	const rage::Vec3V& normal,
	const rage::Vec3V& tangent,
	const rage::ScalarV& radius,
	const rage::Mat44V& mtx,
	const ColorU32 col1,
	const ColorU32 col2,
	float startAngle,
	float angle)
{
	static constexpr int NUM_SEGMENTS = 48;
	float SEGMENT_STEP = angle / NUM_SEGMENTS;

	// We need tangent and binormal for orientation of the circle
	rage::Vec3V biNormal = normal.Cross(tangent);
	rage::Vec3V prevPoint;
	// We drawing + 1 more segment to connect last & first point
	for (int i = 0; i < NUM_SEGMENTS + 1; i++)
	{
		float theta = startAngle + SEGMENT_STEP * static_cast<float>(i);
		float cos = cosf(theta);
		float sin = sinf(theta);

		// Transform coordinate on circle plane
		rage::Vec3V point = pos + tangent * cos * radius + biNormal * sin * radius;

		// Draw second half with different color
		ColorU32 col = i > (NUM_SEGMENTS / 2) ? col2 : col1;

		if (i != 0) DrawLine(prevPoint, point, mtx, col);
		prevPoint = point;
	}
}

void rageam::integration::DrawList::DrawCircle(
	const rage::Vec3V& pos,
	const rage::Vec3V& normal,
	const rage::Vec3V& tangent,
	const rage::ScalarV& radius,
	const ColorU32 col1, const ColorU32 col2,
	float startAngle, float angle)
{
	DrawCircle(pos, normal, tangent, radius, tl_Transform, col1, col2, startAngle, angle);
}

void rageam::integration::DrawList::DrawCircle(
	const rage::Vec3V& pos,
	const rage::Vec3V& normal,
	const rage::Vec3V& tangent,
	const rage::ScalarV& radius,
	const ColorU32 col,
	float startAngle, float angle)
{
	DrawCircle(pos, normal, tangent, radius, tl_Transform, col, col, startAngle, angle);
}

void rageam::integration::DrawList::DrawSphere(const rage::Mat44V& mtx, ColorU32 color, float radius)
{
	DrawCircle(rage::S_ZERO, rage::VEC_FORWARD, rage::VEC_UP, radius, mtx, color, color);
	DrawCircle(rage::S_ZERO, rage::VEC_RIGHT, rage::VEC_UP, radius, mtx, color, color);
	DrawCircle(rage::S_ZERO, rage::VEC_UP, rage::VEC_FORWARD, radius, mtx, color, color);
}

void rageam::integration::DrawList::DrawSphere(ColorU32 color, float radius)
{
	DrawCircle(rage::S_ZERO, rage::VEC_FORWARD, rage::VEC_UP, radius, tl_Transform, color, color);
	DrawCircle(rage::S_ZERO, rage::VEC_RIGHT, rage::VEC_UP, radius, tl_Transform, color, color);
	DrawCircle(rage::S_ZERO, rage::VEC_UP, rage::VEC_FORWARD, radius, tl_Transform, color, color);
}

void rageam::integration::DrawList::DrawSphere(const Sphere& sphere, ColorU32 color)
{
	Mat44V sphereWorld = Mat44V::Translation(sphere.GetCenter()) * tl_Transform;
	DrawSphere(sphereWorld, color, sphere.GetRadius().Get());
}

void rageam::integration::DrawList::DrawTriFill(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, ColorU32 col1, ColorU32 col2, ColorU32 col3, const Mat44V& mtx)
{
	std::unique_lock lock(m_Mutex);
	DrawTriFill_Unsafe(p1, p2, p3, col1, col2, col3, mtx);
}

void rageam::integration::DrawList::DrawTriFill(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, ColorU32 col, const Mat44V& mtx)
{
	DrawTriFill(p1, p2, p3, col, col, col, mtx);
}

void rageam::integration::DrawList::DrawTriFill(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, ColorU32 col1, ColorU32 col2, ColorU32 col3)
{
	DrawTriFill(p1, p2, p3, col1, col2, col3, tl_Transform);
}

void rageam::integration::DrawList::DrawTriFill(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, ColorU32 col)
{
	DrawTriFill(p1, p2, p3, col, col, col);
}

void rageam::integration::DrawList::DrawQuadFill(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, const Vec3V& p4, ColorU32 col, const Mat44V& mtx)
{
	std::unique_lock lock(m_Mutex);
	DrawQuadFill_Unsafe(p1, p2, p3, p4, col, mtx);
}

void rageam::integration::DrawList::DrawQuadFill(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, const Vec3V& p4, ColorU32 col)
{
	DrawQuadFill(p1, p2, p3, p4, col, tl_Transform);
}

// DX11 state backup
struct OldState
{
	ID3D11BlendState* BlendState;
	float BlendFactor[4];
	UINT BlendMask;
	ID3D11DepthStencilState* DepthStencilState;
	UINT StencilRef;
	UINT NumViewports, NumScissors;
	D3D11_VIEWPORT Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	D3D11_RECT Scissors[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	ID3D11RasterizerState* RasteriszerState;
	D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology;
	ID3D11PixelShader* PS;
	ID3D11VertexShader* VS;
	UINT PSInstancesCount, VSInstancesCount;
	ID3D11ClassInstance* PSInstances[256], * VSInstances[256];
	ID3D11Buffer* IndexBuffer, * VertexBuffer, * VSConstantBuffer;
	UINT IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
	DXGI_FORMAT IndexBufferFormat;
	ID3D11InputLayout* InputLayout;
};

static void BackupOldState(OldState& oldState)
{
	memset(&oldState, 0, sizeof OldState);
	ID3D11DeviceContext* context = rageam::graphics::RenderGetContext();
	oldState.NumScissors = oldState.NumViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
	context->OMGetBlendState(&oldState.BlendState, oldState.BlendFactor, &oldState.BlendMask);
	context->OMGetDepthStencilState(&oldState.DepthStencilState, &oldState.StencilRef);
	context->RSGetViewports(&oldState.NumViewports, oldState.Viewports);
	context->RSGetScissorRects(&oldState.NumScissors, oldState.Scissors);
	context->RSGetState(&oldState.RasteriszerState);
	context->IAGetPrimitiveTopology(&oldState.PrimitiveTopology);
	oldState.PSInstancesCount = oldState.VSInstancesCount = 256;
	context->PSGetShader(&oldState.PS, oldState.PSInstances, &oldState.PSInstancesCount);
	context->VSGetShader(&oldState.VS, oldState.VSInstances, &oldState.VSInstancesCount);
	context->VSGetConstantBuffers(1, 1, &oldState.VSConstantBuffer);
	context->IAGetInputLayout(&oldState.InputLayout);
	context->IAGetIndexBuffer(&oldState.IndexBuffer, &oldState.IndexBufferFormat, &oldState.IndexBufferOffset);
	context->IAGetVertexBuffers(0, 1, &oldState.VertexBuffer, &oldState.VertexBufferStride, &oldState.VertexBufferOffset);
}

static void RestoreOldState(OldState& oldState)
{
	ID3D11DeviceContext* context = rageam::graphics::RenderGetContext();
	context->OMSetBlendState(oldState.BlendState, oldState.BlendFactor, oldState.BlendMask);
	context->OMSetDepthStencilState(oldState.DepthStencilState, oldState.StencilRef);
	context->RSSetViewports(oldState.NumViewports, oldState.Viewports);
	context->RSSetScissorRects(oldState.NumScissors, oldState.Scissors);
	context->RSSetState(oldState.RasteriszerState);
	context->IASetPrimitiveTopology(oldState.PrimitiveTopology);
	context->PSSetShader(oldState.PS, oldState.PSInstances, oldState.PSInstancesCount);
	context->VSSetShader(oldState.VS, oldState.VSInstances, oldState.VSInstancesCount);
	context->VSSetConstantBuffers(1, 1, &oldState.VSConstantBuffer);
	context->IASetInputLayout(oldState.InputLayout);
	context->IASetIndexBuffer(oldState.IndexBuffer, oldState.IndexBufferFormat, oldState.IndexBufferOffset);
	context->IASetVertexBuffers(0, 1, &oldState.VertexBuffer, &oldState.VertexBufferStride, &oldState.VertexBufferOffset);
	SAFE_RELEASE(oldState.BlendState);
	SAFE_RELEASE(oldState.DepthStencilState);
	SAFE_RELEASE(oldState.RasteriszerState);
	SAFE_RELEASE(oldState.PS);
	SAFE_RELEASE(oldState.VS);
	for (UINT i = 0; i < oldState.PSInstancesCount; i++) SAFE_RELEASE(oldState.PSInstances[i]);
	for (UINT i = 0; i < oldState.VSInstancesCount; i++) SAFE_RELEASE(oldState.VSInstances[i]);
	SAFE_RELEASE(oldState.IndexBuffer);
	SAFE_RELEASE(oldState.VertexBuffer);
	SAFE_RELEASE(oldState.VSConstantBuffer);
	SAFE_RELEASE(oldState.InputLayout);
}

ID3D11Buffer* rageam::integration::DrawListExecutor::CreateCB(size_t size)
{
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = size;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = size;

	ID3D11Buffer* buffer;
	AM_ASSERT_STATUS(graphics::RenderGetDevice()->CreateBuffer(&desc, NULL, &buffer));
	return buffer;
}

void rageam::integration::DrawListExecutor::CreateShaders(
	ID3D11VertexShader** vs, ID3D11PixelShader** ps, ID3DBlob** vsBlob,
	ConstWString vsFileName, ConstWString psFileName)
{
	ID3D11Device* device = graphics::RenderGetDevice();

	file::WPath shaders = DataManager::GetDataFolder() / L"shaders";
	file::WPath vsPath = shaders / vsFileName;
	file::WPath psPath = shaders / psFileName;

	// VS
	{
		ID3DBlob* blob;
		ID3DBlob* errors;
		AM_ASSERT_STATUS(D3DCompileFromFile(vsPath, NULL, NULL, "main", "vs_5_0", 0, 0, &blob, &errors));
		AM_ASSERT_STATUS(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, vs));
		SAFE_RELEASE(errors);
		*vsBlob = blob;
	}

	// PS
	{
		ID3DBlob* blob;
		ID3DBlob* errors;
		AM_ASSERT_STATUS(D3DCompileFromFile(psPath, NULL, NULL, "main", "ps_5_0", 0, 0, &blob, &errors));
		AM_ASSERT_STATUS(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, ps));
		SAFE_RELEASE(errors);
		SAFE_RELEASE(blob);
	}
}

void rageam::integration::DrawListExecutor::BindEffect() const
{
	ID3D11DeviceContext* context = graphics::RenderGetContext();

	// Viewport
	u32 width, height;
	rage::grcDevice::GetScreenSize(width, height);
	D3D11_VIEWPORT viewport = {};
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MaxDepth = 1.0;
	context->RSSetViewports(1, &viewport);

	// Scissors
	D3D11_RECT scissors = {};
	scissors.right = static_cast<LONG>(width);
	scissors.bottom = static_cast<LONG>(height);
	context->RSSetScissorRects(1, &scissors);
	
	// Shaders
	context->IASetInputLayout(m_Effect.VSLayout.Get());
	context->VSSetShader(m_Effect.VS.Get(), NULL, 0);
	context->PSSetShader(m_Effect.PS.Get(), NULL, 0);

	// Constant buffer
	DrawList::UploadBuffer(m_Effect.CB, m_Effect.CBData, m_Effect.CBSize);
	ID3D11Buffer* cb = m_Effect.CB.Get();
	context->VSSetConstantBuffers(1, 1, &cb);
}

void rageam::integration::DrawListExecutor::RenderDrawData(DrawList::DrawData& drawData) const
{
	ID3D11DeviceContext* context = graphics::RenderGetContext();

	context->IASetPrimitiveTopology(drawData.TopologyType);
	if (drawData.IsIndexed)
	{
		drawData.VTXBuffer.Upload();
		drawData.IDXBuffer.Upload();
		drawData.VTXBuffer.Bind();
		drawData.IDXBuffer.Bind();

		context->DrawIndexed(drawData.IDXBuffer.IndexCount, 0, 0);
	}
	else
	{
		drawData.VTXBuffer.Upload();
		drawData.VTXBuffer.Bind();

		context->Draw(drawData.VTXBuffer.VertexCount, 0);
	}

	// Reset state for next frame
	drawData.VTXBuffer.VertexCount = 0;
	drawData.IDXBuffer.IndexCount = 0;
}


void rageam::integration::DrawListExecutor::Init()
{
	ID3D11Device* device = graphics::RenderGetDevice();

	// Resterizer State
	{
		ID3D11RasterizerState* rasterizerState;
		D3D11_RASTERIZER_DESC rsDesc;
		rsDesc.CullMode = D3D11_CULL_BACK;
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = 8000; // To prevent z figthing on collision if it matches geometry
		rsDesc.DepthBiasClamp = 0;
		rsDesc.SlopeScaledDepthBias = 0;
		rsDesc.DepthClipEnable = true;
		rsDesc.ScissorEnable = true;
		rsDesc.AntialiasedLineEnable = false;
		rsDesc.MultisampleEnable = true;
		// Default
		{
			AM_ASSERT_STATUS(device->CreateRasterizerState(&rsDesc, &rasterizerState));
			m_RS = amComPtr(rasterizerState);
		}
		// Two sided
		{
			rsDesc.CullMode = D3D11_CULL_NONE;
			AM_ASSERT_STATUS(device->CreateRasterizerState(&rsDesc, &rasterizerState));
			m_RSTwoSided = amComPtr(rasterizerState);
		}
		// Wireframe
		{
			rsDesc.CullMode = D3D11_CULL_BACK;
			rsDesc.FillMode = D3D11_FILL_WIREFRAME;
			AM_ASSERT_STATUS(device->CreateRasterizerState(&rsDesc, &rasterizerState));
			m_RSWireframe = amComPtr(rasterizerState);
		}
	}

	// Effect
	{
		ID3DBlob* vsBlob;

		ID3D11Buffer* cb = CreateCB(sizeof LocalsConstantBuffer);

		ID3D11VertexShader* vs;
		ID3D11PixelShader* ps;
		CreateShaders(&vs, &ps, &vsBlob, L"deferred_vs.hlsl", L"deferred_ps.hlsl");
		m_Effect.CB = amComPtr(cb);
		m_Effect.VS = amComPtr(vs);
		m_Effect.PS = amComPtr(ps);

		D3D11_INPUT_ELEMENT_DESC inputDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		ID3D11InputLayout* vsLayout;
		AM_ASSERT_STATUS(device->CreateInputLayout(inputDesc, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &vsLayout));
		SAFE_RELEASE(vsBlob);
		m_Effect.VSLayout = amComPtr(vsLayout);
	}
}

void rageam::integration::DrawListExecutor::Execute(DrawList& drawList)
{
	DrawList::DrawDatas& drawDatas = drawList.GetFrontDrawData();
	// Check if there's anything to render
	if (drawDatas.Line.VTXBuffer.VertexCount == 0 &&
		drawDatas.Tris.VTXBuffer.VertexCount == 0)
		return;

	// NOTE: We can't use projection matrix from CViewport, game uses
	// inverse depth projection for actual rendering,
	// usually near clip is set to 7333 and far is 0.1
	static char** currViewport = gmAddress::Scan("48 8B 05 ?? ?? ?? ?? 0F 28 69 30").GetRef(3).To<char**>();

	// We can't use worldViewProj because world is set at camera position, recompute WorldViewProject without World
	const Mat44V& worldView = (*(rage::Mat44V*)(*currViewport + 0x40));
	const Mat44V& worldViewProj = *(rage::Mat44V*)(*currViewport + 0x80);
	Mat44V proj = worldView.Inverse() * worldViewProj;
	Mat44V viewProj = CViewport::GetViewMatrix() * proj;

	// Setup effect
	LocalsConstantBuffer cb;
	cb.ViewProj = viewProj;
	cb.Unlit = drawList.Unlit;
	cb.NoDepth = drawList.NoDepth;
	m_Effect.CBSize = sizeof LocalsConstantBuffer;
	m_Effect.CBData = &cb;

	// Prepare state & Draw
	OldState oldState;
	BackupOldState(oldState);
	{
		ID3D11DeviceContext* context = graphics::RenderGetContext();
		if (drawList.Wireframe)
			context->RSSetState(m_RSWireframe.Get());
		else if (!drawList.BackfaceCull)
			context->RSSetState(m_RSTwoSided.Get());
		else
			context->RSSetState(m_RS.Get());

		BindEffect();
		RenderDrawData(drawDatas.Line);
		RenderDrawData(drawDatas.Tris);
	}
	RestoreOldState(oldState);
}

#endif
