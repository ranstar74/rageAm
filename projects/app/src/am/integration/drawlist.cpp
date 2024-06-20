#include "drawlist.h"

#ifdef AM_INTEGRATED

#include "am/graphics/buffereditor.h"
#include "am/system/datamgr.h"
#include "rage/grcore/device.h"
#include "game/viewport.h"
#include "helpers/com.h"
#include "am/graphics/render.h"

#include <d3dcompiler.h>

bool rageam::integration::DrawList::VerifyBufferFitLine() const
{
	bool can = m_Lines.VTXBuffer.Size + 2 <= m_Lines.VTXBuffer.Capacity;
	if (!can) AM_WARNINGF("DrawList '%s' -> Line buffer size must be extended!", m_Name);
	return can;
}

bool rageam::integration::DrawList::VerifyBufferFitTri(u32 vertexCount, u32 indexCount) const
{
	bool can =
		m_Tris.VTXBuffer.Size + vertexCount <= m_Tris.VTXBuffer.Capacity &&
		m_Tris.IDXBuffer.Size + indexCount <= m_Tris.IDXBuffer.Capacity;
	if (!can) AM_WARNINGF("DrawList '%s' -> Tri buffer size must be extended!", m_Name);
	return can;
}

rageam::integration::DrawList::DrawList(ConstString name, u32 maxTris, u32 maxLines)
{
	m_Name = name;

	m_Lines.IsIndexed = false;
	m_Lines.TopologyType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	m_Lines.VTXBuffer.Init(maxLines);

	m_Tris.IsIndexed = true;
	m_Tris.TopologyType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_Tris.VTXBuffer.Init(maxTris);
	m_Tris.IDXBuffer.Init(maxTris * 3);
}

void rageam::integration::DrawList::VertexBuffer::Init(u32 capacity)
{
	Resource = amComPtr<ID3D11Buffer>(graphics::CreateBuffer(sizeof Vertex, capacity, true));
	Vertices = amUPtr<Vertex[]>(new Vertex[capacity]);
	Capacity = capacity;
}

void rageam::integration::DrawList::VertexBuffer::Upload()
{
	GPUSize = Size;
	graphics::UploadBuffer(Resource, Vertices.get(), sizeof Vertex * Size);
}

void rageam::integration::DrawList::VertexBuffer::Bind() const
{
	ID3D11Buffer* buffer = Resource.Get();
	u32           stride = sizeof Vertex;
	u32           offset = 0;
	graphics::RenderGetContext()->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
}

void rageam::integration::DrawList::IndexBuffer::Init(u32 capacity)
{
	Resource = amComPtr<ID3D11Buffer>(graphics::CreateBuffer(sizeof Index_t, capacity, false));
	Indices = amUPtr<Index_t[]>(new Index_t[capacity]);
	Capacity = capacity;
}

void rageam::integration::DrawList::IndexBuffer::Upload()
{
	GPUSize = Size;
	graphics::UploadBuffer(Resource, Indices.get(), sizeof Index_t * Size);
}

void rageam::integration::DrawList::IndexBuffer::Bind() const
{
	graphics::RenderGetContext()->IASetIndexBuffer(Resource.Get(), DXGI_FORMAT_R16_UINT, 0);
}

void rageam::integration::DrawList::DrawLine_Unsafe(
	const rage::Vec3V& p1, const rage::Vec3V& p2, const rage::Mat44V& mtx, ColorU32 col1, ColorU32 col2)
{
	EASY_BLOCK("DrawList::DrawLine_Unsafe");
	if (col1.A == 0 && col2.A == 0)
		return;

	if (!VerifyBufferFitLine())
		return;

	rage::Vec3V pt1 = p1.Transform(mtx);
	rage::Vec3V pt2 = p2.Transform(mtx);
	rage::Vector4 col1v = col1.ToVec4();
	rage::Vector4 col2v = col2.ToVec4();

	m_Lines.VTXBuffer.Vertices[m_Lines.VTXBuffer.Size++] = Vertex(pt1, col1v);
	m_Lines.VTXBuffer.Vertices[m_Lines.VTXBuffer.Size++] = Vertex(pt2, col2v);
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

	if (!VerifyBufferFitLine())
		return;

	m_Lines.VTXBuffer.Vertices[m_Lines.VTXBuffer.Size++] = Vertex(p1, col1.ToVec4());
	m_Lines.VTXBuffer.Vertices[m_Lines.VTXBuffer.Size++] = Vertex(p2, col2.ToVec4());
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

	if (!VerifyBufferFitTri(3, 3))
		return;

	Vec3V n = TriNormal(p1, p2, p3);

	u32 startVertex = m_Tris.VTXBuffer.Size;
	m_Tris.VTXBuffer.Vertices[m_Tris.VTXBuffer.Size++] = Vertex(p1, col1.ToVec4(), n);
	m_Tris.VTXBuffer.Vertices[m_Tris.VTXBuffer.Size++] = Vertex(p2, col2.ToVec4(), n);
	m_Tris.VTXBuffer.Vertices[m_Tris.VTXBuffer.Size++] = Vertex(p3, col3.ToVec4(), n);
	m_Tris.IDXBuffer.Indices[m_Tris.IDXBuffer.Size++] = startVertex + 0;
	m_Tris.IDXBuffer.Indices[m_Tris.IDXBuffer.Size++] = startVertex + 1;
	m_Tris.IDXBuffer.Indices[m_Tris.IDXBuffer.Size++] = startVertex + 2;
}

void rageam::integration::DrawList::DrawTriFill_Unsafe(
	const Vec3V& p1, const Vec3V& p2, const Vec3V& p3,
	ColorU32 col1, ColorU32 col2, ColorU32 col3,
	const Mat44V& mtx)
{
	return DrawTriFill_Unsafe(p1.Transform(mtx), p2.Transform(mtx), p3.Transform(mtx), col1, col2, col3);
}

void rageam::integration::DrawList::DrawQuadFill_Unsafe(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, const Vec3V& p4, ColorU32 col)
{
	if (col.A == 0)
		return;

	if (!VerifyBufferFitTri(4, 6))
		return;

	Vec4S colVec = col.ToVec4();
	Vec3V n1 = TriNormal(p1, p2, p3);
	Vec3V n2 = TriNormal(p2, p3, p4);
	Vec3V n = (n1 + n2) * rage::S_HALF; // Average

	u32 startVertex = m_Tris.VTXBuffer.Size;
	m_Tris.VTXBuffer.Vertices[m_Tris.VTXBuffer.Size++] = Vertex(p1, colVec, n);
	m_Tris.VTXBuffer.Vertices[m_Tris.VTXBuffer.Size++] = Vertex(p2, colVec, n);
	m_Tris.VTXBuffer.Vertices[m_Tris.VTXBuffer.Size++] = Vertex(p3, colVec, n);
	m_Tris.VTXBuffer.Vertices[m_Tris.VTXBuffer.Size++] = Vertex(p4, colVec, n);
	m_Tris.IDXBuffer.Indices[m_Tris.IDXBuffer.Size++] = startVertex + 0;
	m_Tris.IDXBuffer.Indices[m_Tris.IDXBuffer.Size++] = startVertex + 2;
	m_Tris.IDXBuffer.Indices[m_Tris.IDXBuffer.Size++] = startVertex + 1;
	m_Tris.IDXBuffer.Indices[m_Tris.IDXBuffer.Size++] = startVertex + 2;
	m_Tris.IDXBuffer.Indices[m_Tris.IDXBuffer.Size++] = startVertex + 0;
	m_Tris.IDXBuffer.Indices[m_Tris.IDXBuffer.Size++] = startVertex + 3;
}

void rageam::integration::DrawList::DrawQuadFill_Unsafe(
	const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, const Vec3V& p4, ColorU32 col, const Mat44V& mtx)
{
	DrawQuadFill_Unsafe(p1.Transform(mtx), p2.Transform(mtx), p3.Transform(mtx), p4.Transform(mtx), col);
}

void rageam::integration::DrawList::EndFrame()
{
	std::unique_lock lock(m_Mutex);

	m_HasLinesToDraw = m_Lines.VTXBuffer.Size > 0;
	m_HasTrisToDraw = m_Tris.VTXBuffer.Size > 0;

	// TODO:
	// For some reason not clearing buffer causes
	// weird graphical artefacts, need to investigate this...

	if (1 || m_HasLinesToDraw)
		m_Lines.VTXBuffer.Upload();

	if (1 || m_HasTrisToDraw)
	{
		m_Tris.VTXBuffer.Upload();
		m_Tris.IDXBuffer.Upload();
	}

	// Resets stats for the next frame
	m_Lines.VTXBuffer.Size = 0;
	m_Tris.VTXBuffer.Size = 0;
	m_Tris.IDXBuffer.Size = 0;
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

void rageam::integration::DrawList::DrawLineFast(const Vec3V& p1, const Vec3V& p2, ColorU32 col)
{
	EASY_BLOCK("DrawList::DrawLineFast");
	if (col.A == 0)
		return;

	if (!VerifyBufferFitLine())
		return;

	rage::Vector4 colv = col.ToVec4();
	m_Lines.VTXBuffer.Vertices[m_Lines.VTXBuffer.Size++] = Vertex(p1, colv);
	m_Lines.VTXBuffer.Vertices[m_Lines.VTXBuffer.Size++] = Vertex(p2, colv);
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
	DrawLine_Unsafe(tr, tl, tl_Transform, col);
	DrawLine_Unsafe(tl, bl, tl_Transform, col);
	DrawLine_Unsafe(bl, br, tl_Transform, col);
	DrawLine_Unsafe(br, tr, tl_Transform, col);
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
	EASY_BLOCK("DrawList::DrawCircle");
	Vec3V front;
	CViewport::GetCamera(&front, 0, 0, 0);

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

void rageam::integration::DrawList::DrawCapsule(float radius, float halfExtent, ColorU32 color)
{
	rage::Vec3V extentFrom = rage::VEC_UP * halfExtent;
	rage::Vec3V extentTo = rage::VEC_DOWN * halfExtent;

	DrawLine(extentFrom, extentTo, color);

	// Half sphere tops
	DrawCircle(extentTo, rage::VEC_UP, rage::VEC_RIGHT, radius, color);
	DrawCircle(extentFrom, rage::VEC_UP, rage::VEC_RIGHT, radius, color);

	// Draw two half spheres (arcs)
	DrawCircle(extentTo, rage::VEC_FRONT, rage::VEC_RIGHT, radius, color, 0, rage::PI);
	DrawCircle(extentTo, -rage::VEC_RIGHT, rage::VEC_FRONT, radius, color, 0, rage::PI);
	DrawCircle(extentFrom, -rage::VEC_FRONT, rage::VEC_RIGHT, radius, color, 0, rage::PI);
	DrawCircle(extentFrom, rage::VEC_RIGHT, rage::VEC_FRONT, radius, color, 0, rage::PI);

	// Draw lines connecting half spheres (arcs)
	DrawLine(extentTo + rage::VEC_FRONT * radius, extentFrom + rage::VEC_FRONT * radius, color);
	DrawLine(extentTo + rage::VEC_RIGHT * radius, extentFrom + rage::VEC_RIGHT * radius, color);
	DrawLine(extentTo - rage::VEC_FRONT * radius, extentFrom - rage::VEC_FRONT * radius, color);
	DrawLine(extentTo - rage::VEC_RIGHT * radius, extentFrom - rage::VEC_RIGHT * radius, color);
}

void rageam::integration::DrawList::DrawCapsule(float radius, const Vec3V& normal, const Vec3V& extentFrom, const Vec3V& extentTo, ColorU32 color)
{
	DrawLine(extentFrom, extentTo, color);

	Vec3V tangent, biNormal;
	normal.TangentAndBiNormal(tangent, biNormal);

	// Half sphere tops
	DrawCircle(extentTo, normal, tangent, radius, color);
	DrawCircle(extentFrom, normal, tangent, radius, color);

	// Draw two half spheres (arcs)
	DrawCircle(extentTo, biNormal, tangent, radius, color, 0, rage::PI);
	DrawCircle(extentTo, -tangent, biNormal, radius, color, 0, rage::PI);
	DrawCircle(extentFrom, -biNormal, tangent, radius, color, 0, rage::PI);
	DrawCircle(extentFrom, tangent, biNormal, radius, color, 0, rage::PI);

	// Draw lines connecting half spheres (arcs)
	DrawLine(extentTo + biNormal * radius, extentFrom + biNormal * radius, color);
	DrawLine(extentTo + tangent * radius, extentFrom + tangent * radius, color);
	DrawLine(extentTo - biNormal * radius, extentFrom - biNormal * radius, color);
	DrawLine(extentTo - tangent * radius, extentFrom - tangent * radius, color);
}

void rageam::integration::DrawList::DrawCylinder(float radius, const Vec3V& normal, const Vec3V& extentFrom, const Vec3V& extentTo, ColorU32 color)
{
	DrawLine(extentFrom, extentTo, color);

	Vec3V tangent, biNormal;
	normal.TangentAndBiNormal(tangent, biNormal);

	// Half sphere tops
	DrawCircle(extentTo, normal, tangent, radius, color);
	DrawCircle(extentFrom, normal, tangent, radius, color);

	// Draw lines connecting half spheres (arcs)
	DrawLine(extentTo + biNormal * radius, extentFrom + biNormal * radius, color);
	DrawLine(extentTo + tangent * radius, extentFrom + tangent * radius, color);
	DrawLine(extentTo - biNormal * radius, extentFrom - biNormal * radius, color);
	DrawLine(extentTo - tangent * radius, extentFrom - tangent * radius, color);
}

void rageam::integration::DrawList::DrawCylinder(float radius, float halfExtent, ColorU32 color)
{
	rage::Vec3V extentFrom = rage::VEC_UP * halfExtent;
	rage::Vec3V extentTo = rage::VEC_DOWN * halfExtent;

	DrawLine(extentFrom, extentTo, color);

	// Half top cylinders
	DrawCircle(extentTo, rage::VEC_UP, rage::VEC_RIGHT, radius, color);
	DrawCircle(extentFrom, rage::VEC_UP, rage::VEC_RIGHT, radius, color);

	// Draw lines connecting half spheres (arcs)
	DrawLine(extentTo + rage::VEC_FRONT * radius, extentFrom + rage::VEC_FRONT * radius, color);
	DrawLine(extentTo + rage::VEC_RIGHT * radius, extentFrom + rage::VEC_RIGHT * radius, color);
	DrawLine(extentTo - rage::VEC_FRONT * radius, extentFrom - rage::VEC_FRONT * radius, color);
	DrawLine(extentTo - rage::VEC_RIGHT * radius, extentFrom - rage::VEC_RIGHT * radius, color);
}

void rageam::integration::DrawList::DrawAlignedBox(const Vec3V& center, const Vec3V& extent, ColorU32 col)
{
	DrawAABB(AABB(center - extent, center + extent), col);
}

void rageam::integration::DrawList::DrawAlignedBoxFill(const Vec3V& center, const Vec3V& extent, ColorU32 col)
{
	rage::spdAABB bb(center - extent, center + extent);
	rage::Mat44V& mtx = tl_Transform;
	// Top Face
	DrawTriFill_Unsafe(bb.TTL(), bb.TBL(), bb.TTR(), col, col, col, mtx);
	DrawTriFill_Unsafe(bb.TBL(), bb.TBR(), bb.TTR(), col, col, col, mtx);
	// Bottom Face
	DrawTriFill_Unsafe(bb.BTL(), bb.BBL(), bb.BTR(), col, col, col, mtx);
	DrawTriFill_Unsafe(bb.BBL(), bb.BBR(), bb.BTR(), col, col, col, mtx);
	// Side Faces
	DrawTriFill_Unsafe(bb.BBL(), bb.TBL(), bb.BTL(), col, col, col, mtx);
	DrawTriFill_Unsafe(bb.BTL(), bb.TBL(), bb.TTL(), col, col, col, mtx);
	DrawTriFill_Unsafe(bb.BTL(), bb.TTL(), bb.BTR(), col, col, col, mtx);
	DrawTriFill_Unsafe(bb.BTR(), bb.TTL(), bb.TTR(), col, col, col, mtx);
	DrawTriFill_Unsafe(bb.BTR(), bb.TBR(), bb.BBR(), col, col, col, mtx);
	DrawTriFill_Unsafe(bb.BTR(), bb.TTR(), bb.TBR(), col, col, col, mtx);
	DrawTriFill_Unsafe(bb.BBR(), bb.TBR(), bb.TBL(), col, col, col, mtx);
	DrawTriFill_Unsafe(bb.BBL(), bb.BBR(), bb.TBL(), col, col, col, mtx);
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
	ID3D11BlendState*        BlendState;
	float                    BlendFactor[4];
	UINT                     BlendMask;
	ID3D11DepthStencilState* DepthStencilState;
	UINT                     StencilRef;
	UINT                     NumViewports, NumScissors;
	D3D11_VIEWPORT           Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	D3D11_RECT               Scissors[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	ID3D11RasterizerState*   RasteriszerState;
	D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology;
	ID3D11PixelShader*       PS;
	ID3D11VertexShader*      VS;
	UINT                     PSInstancesCount, VSInstancesCount;
	ID3D11ClassInstance*     PSInstances[256];
	ID3D11ClassInstance*     VSInstances[256];
	ID3D11Buffer*            IndexBuffer;
	ID3D11Buffer*            VertexBuffer;
	ID3D11Buffer*            VSConstantBuffer;
	UINT                     IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
	DXGI_FORMAT              IndexBufferFormat;
	ID3D11InputLayout*       InputLayout;
	ID3D11RenderTargetView*  RenderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
	ID3D11DepthStencilView*  DepthStencil;
};

void BackupOldState(OldState& oldState)
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
	context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, oldState.RenderTargets, &oldState.DepthStencil);
}

void RestoreOldState(OldState& oldState)
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
	context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, oldState.RenderTargets, oldState.DepthStencil);
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
	for (ID3D11RenderTargetView*& rt : oldState.RenderTargets) SAFE_RELEASE(rt);
	SAFE_RELEASE(oldState.DepthStencil);
}

void rageam::integration::DrawListExecutor::RenderDrawData(const DrawList::DrawData& drawData) const
{
	ID3D11DeviceContext* context = graphics::RenderGetContext();

	context->IASetPrimitiveTopology(drawData.TopologyType);
	if (drawData.IsIndexed)
	{
		drawData.VTXBuffer.Bind();
		drawData.IDXBuffer.Bind();
		context->DrawIndexed(drawData.IDXBuffer.GPUSize, 0, 0);
	}
	else
	{
		drawData.VTXBuffer.Bind();
		context->Draw(drawData.VTXBuffer.GPUSize, 0);
	}
}

void rageam::integration::DrawListExecutor::CreateBackbuf()
{
	ID3D11Device* device = graphics::RenderGetDevice();

	u32 sampleCount = m_SampleCount;
	if (sampleCount == 0)
		sampleCount = 1; // 0 is not valid for DX11

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = m_ScreenWidth;
	texDesc.Height = m_ScreenHeight;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.SampleDesc.Count = sampleCount;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	AM_ASSERT_STATUS(device->CreateTexture2D(&texDesc, nullptr, &m_BackbufMs));

	if (sampleCount > 1)
	{
		D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
		rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
		rtDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
		AM_ASSERT_STATUS(device->CreateRenderTargetView(m_BackbufMs.Get(), &rtDesc, &m_BackbufMsRt));
	}
	else
	{
		D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
		rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtDesc.Texture2D.MipSlice = 0;
		AM_ASSERT_STATUS(device->CreateRenderTargetView(m_BackbufMs.Get(), &rtDesc, &m_BackbufMsRt));
	}

	// Non multi sampled for resolving 
	texDesc.SampleDesc.Count = 1;
	AM_ASSERT_STATUS(device->CreateTexture2D(&texDesc, nullptr, &m_Backbuf));

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = 1;
	viewDesc.Texture2D.MostDetailedMip = 0;
	AM_ASSERT_STATUS(device->CreateShaderResourceView(m_Backbuf.Get(), &viewDesc, &m_BackbufView));
}

rageam::integration::DrawListExecutor::DrawListExecutor()
{
	m_DefaultShader.Create();
	m_ImBlitShader.Create();

	m_GaussVS = graphics::Shader::CreateFromPath(graphics::ShaderPixel, L"im_gauss_ps.hlsl");

	ID3D11Device* device = graphics::RenderGetDevice();

	// Depth Stencil State
	{
		D3D11_DEPTH_STENCIL_DESC dsDesc;
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.DepthFunc = D3D11_COMPARISON_GREATER;
		dsDesc.StencilEnable = true;
		dsDesc.StencilReadMask = 0xFF;
		dsDesc.StencilWriteMask = 0xFF;
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		AM_ASSERT_STATUS(device->CreateDepthStencilState(&dsDesc, &m_DSS));
		dsDesc.DepthEnable = false;
		AM_ASSERT_STATUS(device->CreateDepthStencilState(&dsDesc, &m_DSSNoDepth));
	}

	// Resterizer State
	{
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
			AM_ASSERT_STATUS(device->CreateRasterizerState(&rsDesc, &m_RS));
		}
		// Two-sided
		{
			rsDesc.CullMode = D3D11_CULL_NONE;
			AM_ASSERT_STATUS(device->CreateRasterizerState(&rsDesc, &m_RSTwoSided));
		}
		// Wireframe
		{
			rsDesc.CullMode = D3D11_CULL_BACK;
			rsDesc.FillMode = D3D11_FILL_WIREFRAME;
			AM_ASSERT_STATUS(device->CreateRasterizerState(&rsDesc, &m_RSWireframe));
		}
	}

	// Blend state
	{
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		AM_ASSERT_STATUS(device->CreateBlendState(&blendDesc, &m_BlendState));
	}
}

void rageam::integration::DrawListExecutor::Execute(DrawList& drawList)
{
	std::unique_lock lock(drawList.m_Mutex);

	// Check if there's anything to render
	if (!drawList.m_HasTrisToDraw && !drawList.m_HasLinesToDraw)
		return;

	u32 width, height, sampleCount;
	rage::grcDevice::GetScreenSize(width, height);
	sampleCount = rage::grcDevice::GetMSAA().Mode;

	// Recreate MSAA back buffer and render target
	if (m_ScreenWidth != width || m_ScreenHeight != height || m_SampleCount != sampleCount)
	{
		m_ScreenWidth = width;
		m_ScreenHeight = height;
		m_SampleCount = sampleCount;

		AM_DEBUGF("DrawListExecutor -> Creating backbuf for %ux%u and %u samples", 
			m_ScreenWidth, m_ScreenHeight, m_SampleCount);

		CreateBackbuf();
	}

	// Game will update matrices constant buffer for us
	static auto grcWorldIdentity = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"E8 ?? ?? ?? ?? 48 8D 45 30",
#else
		"E8 ?? ?? ?? ?? 4C 63 2D",
#endif
		"rage::grcWorldIdentity")
		.GetCall().ToFunc<void()>();
	grcWorldIdentity();

	// Prepare state & Draw
	OldState oldState;
	BackupOldState(oldState);
	{
		ID3D11DeviceContext* context = graphics::RenderGetContext();

		context->OMSetBlendState(m_BlendState.Get(), nullptr, 0xFFFFFFFF);

		// Viewport
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
		
		// Obtain scene depth stencil
		static auto getTargetView = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
			"0F B7 40 7C 8B 4C 24 40", "rage::grcRenderTargetDX11::GetTargetView+0x17")
			.GetAt(-0x17)
#else
			"0F B7 41 7C 41", "rage::grcRenderTargetDX11::GetTargetView")
#endif
			.ToFunc<ID3D11ShaderResourceView*(pVoid rt, u32 mip, u32 layer)>();
		static auto getDepthBuffer = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
			"88 4C 24 08 48 8B", "CRenderTargets::GetDepthBuffer")
#else
			"E8 ?? ?? ?? ?? EB 20 83 3D", "CRenderTargets::GetDepthBuffer")
			.GetCall()
#endif
			.ToFunc<pVoid(bool unused)>();
		ID3D11DepthStencilView* depthBuffer = (ID3D11DepthStencilView*) getTargetView(getDepthBuffer(false), 0, 0);

		if (!drawList.WriteOnlyDepth)
		{
			// We render to our MSAA back-buffer because buffer that's set in
			// CVisualEffects::RenderMarkers is not g buffer and don't have multisampling
			float clearColor[] = { 0, 0, 0, 0 };
			ID3D11RenderTargetView* backbufRt = m_BackbufMsRt.Get();
			context->OMSetRenderTargets(1, &backbufRt, depthBuffer);
			context->ClearRenderTargetView(backbufRt, clearColor);
		}
		else
		{
			// No RT view, just depth
			context->OMSetRenderTargets(0, nullptr, depthBuffer);
		}

		if (drawList.Wireframe)
			context->RSSetState(m_RSWireframe.Get());
		else if (!drawList.BackfaceCull)
			context->RSSetState(m_RSTwoSided.Get());
		else
			context->RSSetState(m_RS.Get());

		if (drawList.NoDepth)
			context->OMSetDepthStencilState(m_DSSNoDepth.Get(), 1);
		else
			context->OMSetDepthStencilState(m_DSS.Get(), 1);

		// Shading in wireframe mode doesn't look good...
		m_DefaultShader.Locals.Unlit = drawList.Wireframe ? true : drawList.Unlit;
		m_DefaultShader.Bind();
		RenderDrawData(drawList.m_Lines);
		RenderDrawData(drawList.m_Tris);

		// Resolve multi sampled back buffer
		context->ResolveSubresource(
			m_Backbuf.Get(), D3D11CalcSubresource(0, 0, 1),
			m_BackbufMs.Get(), D3D11CalcSubresource(0, 0, 1),
			DXGI_FORMAT_R8G8B8A8_UNORM);

		// Set game's render target back and blit our back buffer to it
		context->RSSetState(m_RS.Get()); // Make sure we don't have wireframe rasterizer set...
		context->OMSetDepthStencilState(m_DSSNoDepth.Get(), 1); // We don't need depth
		context->OMSetRenderTargets(1, oldState.RenderTargets, nullptr);
		m_ImBlitShader.Bind();
		m_GaussVS->Bind(); // Glowing outline
		m_ImBlitShader.Blit(m_BackbufView.Get());
	}
	RestoreOldState(oldState);
}

#endif
