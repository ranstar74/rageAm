#pragma once

#ifdef AM_INTEGRATED

#include "am/types.h"
#include "am/graphics/color.h"
#include "rage/math/math.h"
#include "am/graphics/render/engine.h"
#include "game/drawable.h"
#include "updatecomponent.h"

#include <d3d11.h>

namespace rageam::integration
{
	class GameEntity;
	class DrawList;

	// Fake drawable that we are using to render draw list easily in game.
	class DrawListDummyDrawable : public gtaDrawable
	{
		SmallList<DrawList*> m_DrawLists;

	public:
		DrawListDummyDrawable();

		void AddDrawList(DrawList* dl);
		void Draw(const Mat34V& mtx, rage::grcRenderMask mask, rage::eDrawableLod lod) override;
	};

	// NOTE: There can be only one such entity spawned!
	class DrawListDummyEntity : public IUpdateComponent
	{
		amPtr<DrawListDummyDrawable>	m_DummyDrawable;
		ComponentOwner<GameEntity>		m_GameEntity;

		void ReleaseAllRefs() override;
		void OnEarlyUpdate() override;

	public:
		void Spawn();
		void AddDrawList(DrawList* drawList) const { m_DummyDrawable->AddDrawList(drawList); }
	};

	class DrawList
	{
		friend class DrawListExecutor;

		static constexpr u32 VTX_MAX = 0x100000;
		static constexpr u32 IDX_MAX = 0x100000;

		static inline thread_local Mat44V tl_Transform = Mat44V::Identity();

		using ColorU32 = graphics::ColorU32;

#pragma pack(push, 1)
		struct Vertex
		{
			Vec3S Pos;
			Vec4S Color;
			Vec3S Normal;
		};
#pragma pack(pop)

		template<typename TVertex, size_t VertexCapacity>
		struct VertexBuffer
		{
			amComPtr<ID3D11Buffer>	Resource;
			TVertex					Vertices[VertexCapacity] = {};
			u32						VertexCount = 0;

			VertexBuffer() { Resource = amComPtr(CreateBuffer(sizeof TVertex, VertexCapacity, true)); }
			void AddVertex(const TVertex& vertex) { Vertices[VertexCount++] = vertex; }
			void Upload() const { UploadBuffer(Resource, Vertices, sizeof TVertex * VertexCount); }
			void Bind() const
			{
				ID3D11Buffer* buffer = Resource.Get();
				u32 stride = sizeof TVertex;
				u32 offset = 0;
				render::GetDeviceContext()->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
			}
		};

		template<size_t IndexCapacity>
		struct IndexBuffer
		{
			using index_t = u16;

			amComPtr<ID3D11Buffer>	Resource;
			index_t					Indices[IndexCapacity] = {};
			u32						IndexCount = 0;

			IndexBuffer() { Resource = amComPtr(CreateBuffer(sizeof index_t, IndexCapacity, false)); }
			void AddIndex(const index_t& index) { Indices[IndexCount++] = index; }
			void Upload() const { UploadBuffer(Resource, Indices, sizeof index_t * IndexCount); }
			void Bind() const { render::GetDeviceContext()->IASetIndexBuffer(Resource.Get(), DXGI_FORMAT_R16_UINT, 0); }
		};

		struct DrawData
		{
			IndexBuffer<IDX_MAX>			IDXBuffer;
			VertexBuffer<Vertex, VTX_MAX>	VTXBuffer;
			D3D11_PRIMITIVE_TOPOLOGY		TopologyType = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
			bool							IsIndexed = false;
		};

		struct DrawDatas
		{
			DrawData						Line;
			DrawData						Tris;
		};

		static auto CreateBuffer(size_t elementSize, size_t elementCount, bool vertexBuffer) -> ID3D11Buffer*;
		static void UploadBuffer(const amComPtr<ID3D11Buffer>& buffer, pConstVoid data, u32 dataSize);

		// 2 buffers allow us to compose one buffer while other is rendered, used only for deferred
		int						m_CurrDataIdx = 0;
		DrawDatas				m_DrawDatas[2];		// Present & Compose buffers
		std::recursive_mutex	m_Mutex;

		// Draw data that's we currently filling in game threads
		DrawDatas& GetBackDrawData() { return m_DrawDatas[m_CurrDataIdx]; }
		// Draw data that will be rendered
		DrawDatas& GetFrontDrawData() { return m_DrawDatas[(m_CurrDataIdx + 1) % 2]; }

		// Those are non-thread safe functions, exposed interface that calls them must use mutex

		bool VerifyBufferFitLine(const DrawData& drawData) const;
		bool VerifyBufferFitTri(const DrawData& drawData, u32 vertexCount, u32 indexCount) const;

		void DrawLine_Unsafe(const Vec3V& p1, const Vec3V& p2, const Mat44V& mtx, ColorU32 col1, ColorU32 col2);
		void DrawLine_Unsafe(const Vec3V& p1, const Vec3V& p2, const Mat44V& mtx, ColorU32 col);
		void DrawLine_Unsafe(const Vec3V& p1, const Vec3V& p2, ColorU32 col1, ColorU32 col2);
		void DrawLine_Unsafe(const Vec3V& p1, const Vec3V& p2, ColorU32 col);

		Vec3V TriNormal(const Vec3V& v1, const Vec3V& v2, const Vec3V& v3) const;
		void DrawTriFill_Unsafe(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, ColorU32 col1, ColorU32 col2, ColorU32 col3);
		void DrawTriFill_Unsafe(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, ColorU32 col1, ColorU32 col2, ColorU32 col3, const Mat44V& mtx);
		void DrawQuadFill_Unsafe(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, const Vec3V& p4, ColorU32 col, const Mat44V& mtx);

	public:
		DrawList();

		bool NoDepth = false;
		bool Unlit = false;
		bool Wireframe = false;
		bool BackfaceCull = true;

		// Must be called on the beginning of rendering
		void FlipBuffer()
		{
			std::unique_lock lock(m_Mutex);
			m_CurrDataIdx = (m_CurrDataIdx + 1) % 2; /* Flip between 0 and 1 */
		}

		// Allows to push transformation matrix for functions that don't have transform argument
		void SetTransform(const Mat44V& mtx) const { tl_Transform = mtx; }
		auto GetTransform() const -> const Mat44V& { return tl_Transform; }
		// Sets transform to identity
		void ResetTransform() const { SetTransform(Mat44V::Identity()); }

		void Clear()
		{
			for(int i = 0; i < 2 ; i++)
			{
				m_DrawDatas[i].Tris.VTXBuffer.VertexCount = 0;
				m_DrawDatas[i].Tris.IDXBuffer.IndexCount = 0;
				m_DrawDatas[i].Line.VTXBuffer.VertexCount = 0;
				m_DrawDatas[i].Line.IDXBuffer.IndexCount = 0;
			}
		}

		// --- Lines ---

		void DrawLine(const Vec3V& p1, const Vec3V& p2, const Mat44V& mtx, ColorU32 col1, ColorU32 col2);
		void DrawLine(const Vec3V& p1, const Vec3V& p2, const Mat44V& mtx, ColorU32 col);
		void DrawLine(const Vec3V& p1, const Vec3V& p2, ColorU32 col1, ColorU32 col2);
		void DrawLine(const Vec3V& p1, const Vec3V& p2, ColorU32 col);
		void DrawAABB(const AABB& bb, const Mat44V& mtx, ColorU32 col);
		void DrawAABB(const AABB& bb, ColorU32 col) { DrawAABB(bb, tl_Transform, col); }
		void DrawQuad(
			const Vec3V& pos,
			const Vec3V& normal,
			const Vec3V& tangent,
			const ScalarV& extentX,
			const ScalarV& extentY,
			ColorU32 col);
		// Normal & tangent define the plane the circle lines in,
		// Tangent also defines start of the circle, first half
		// is draw with col1 and second half is using col2
		// This is mostly used on light sphere back side is culled / draw gray
		// https://imgur.com/iQG2PMS
		void DrawCircle(
			const Vec3V& pos,
			const Vec3V& normal,
			const Vec3V& tangent,
			const ScalarV& radius,
			const Mat44V& mtx,
			ColorU32 col1, ColorU32 col2,
			float startAngle = 0.0f, float angle = rage::PI2);
		void DrawCircle(
			const Vec3V& pos,
			const Vec3V& normal,
			const Vec3V& tangent,
			const ScalarV& radius,
			ColorU32 col1, ColorU32 col2,
			float startAngle = 0.0f, float angle = rage::PI2);
		void DrawCircle(
			const Vec3V& pos,
			const Vec3V& normal,
			const Vec3V& tangent,
			const ScalarV& radius,
			ColorU32 col,
			float startAngle = 0.0f, float angle = rage::PI2);
		// Draws sphere using 3 aligned axes
		void DrawSphere(const Mat44V& mtx, ColorU32 color, float radius);
		void DrawSphere(ColorU32 color, float radius);
		void DrawSphere(const Sphere& sphere, ColorU32 color);
		void DrawCapsule() {} // TODO:

		// --- Fill ---

		void DrawTriFill(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, ColorU32 col1, ColorU32 col2, ColorU32 col3, const Mat44V& mtx);
		void DrawTriFill(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, ColorU32 col, const Mat44V& mtx);
		void DrawTriFill(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, ColorU32 col1, ColorU32 col2, ColorU32 col3);
		void DrawTriFill(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, ColorU32 col);
		void DrawQuadFill(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, const Vec3V& p4, ColorU32 col, const Mat44V& mtx);
		void DrawQuadFill(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, const Vec3V& p4, ColorU32 col);
	};

	class DrawListExecutor
	{
		static inline DrawListExecutor* s_Current = nullptr;

		struct LocalsConstantBuffer
		{
			Mat44V							ViewProj;
			int								NoDepth;
			int								Unlit;
		};

		struct Effect
		{
			amComPtr<ID3D11VertexShader>	VS;
			amComPtr<ID3D11PixelShader>		PS;
			amComPtr<ID3D11Buffer>			CB;
			pVoid							CBData = nullptr;
			size_t							CBSize = 0;
			amComPtr<ID3D11InputLayout>		VSLayout;
		};

		amComPtr<ID3D11RasterizerState>		m_RSTwoSided;
		amComPtr<ID3D11RasterizerState>		m_RSWireframe;
		amComPtr<ID3D11RasterizerState>		m_RS;
		Effect								m_Effect;

		static auto CreateCB(size_t size) -> ID3D11Buffer*;
		static void CreateShaders(
			ID3D11VertexShader** vs, ID3D11PixelShader** ps, ID3DBlob** vsBlob, ConstWString vsFileName, ConstWString psFileName);

		void BindEffect() const;
		void RenderDrawData(DrawList::DrawData& drawData) const;

		void Init();
	public:
		DrawListExecutor() { Init(); }

		void Execute(DrawList& drawList);

		static auto GetCurrent() -> DrawListExecutor* { return s_Current; }
		static void SetCurrent(DrawListExecutor* dl) { s_Current = dl; }
	};
}

#endif
