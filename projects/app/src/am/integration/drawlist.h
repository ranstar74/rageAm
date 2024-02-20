//
// File: drawlist.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "am/graphics/color.h"
#include "am/types.h"

#include <d3d11.h>

namespace rageam::integration
{
	/**
	 * \brief Buffer with primitive vertex data to draw.
	 */
	class DrawList
	{
		friend class DrawListExecutor;

		// Thread local for simplicity of API, for example we can push entity matrix just once,
		// and it will be shared between all draw lists
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

		struct VertexBuffer
		{
			amComPtr<ID3D11Buffer> Resource;
			amUPtr<Vertex[]>       Vertices;
			u32                    Capacity;
			u32                    Size;
			u32                    GPUSize;

			void Init(u32 capacity);
			void Upload();
			void Bind() const;
		};

		struct IndexBuffer
		{
			using Index_t = u16;

			amComPtr<ID3D11Buffer> Resource;
			amUPtr<Index_t[]>      Indices;
			u32                    Capacity;
			u32                    Size;
			u32                    GPUSize;

			void Init(u32 capacity);
			void Upload();
			void Bind() const;
		};

		// Buffer for specific primitive topology
		struct DrawData
		{
			IndexBuffer              IDXBuffer;
			VertexBuffer             VTXBuffer;
			D3D11_PRIMITIVE_TOPOLOGY TopologyType = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
			bool                     IsIndexed = false;
		};

		static ID3D11Buffer* CreateBuffer(size_t elementSize, size_t elementCount, bool vertexBuffer);
		static void UploadBuffer(const amComPtr<ID3D11Buffer>& buffer, pConstVoid data, u32 dataSize);

		DrawData             m_Lines;
		DrawData             m_Tris;
		ConstString          m_Name;
		std::recursive_mutex m_Mutex;
		bool				 m_HasLinesToDraw = false;
		bool				 m_HasTrisToDraw = false;

		// Those are non-thread safe functions, exposed interface that calls them must use mutex

		bool VerifyBufferFitLine() const;
		bool VerifyBufferFitTri(u32 vertexCount, u32 indexCount) const;

		void DrawLine_Unsafe(const Vec3V& p1, const Vec3V& p2, const Mat44V& mtx, ColorU32 col1, ColorU32 col2);
		void DrawLine_Unsafe(const Vec3V& p1, const Vec3V& p2, const Mat44V& mtx, ColorU32 col);
		void DrawLine_Unsafe(const Vec3V& p1, const Vec3V& p2, ColorU32 col1, ColorU32 col2);
		void DrawLine_Unsafe(const Vec3V& p1, const Vec3V& p2, ColorU32 col);

		Vec3V TriNormal(const Vec3V& v1, const Vec3V& v2, const Vec3V& v3) const;
		void DrawTriFill_Unsafe(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, ColorU32 col1, ColorU32 col2, ColorU32 col3);
		void DrawTriFill_Unsafe(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, ColorU32 col1, ColorU32 col2, ColorU32 col3, const Mat44V& mtx);
		void DrawQuadFill_Unsafe(const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, const Vec3V& p4, ColorU32 col, const Mat44V& mtx);

	public:
		DrawList(ConstString name, u32 maxTris, u32 maxLines);

		bool NoDepth = false;
		bool Unlit = false;
		bool Wireframe = false;
		bool BackfaceCull = true;

		// Uses 3D polygons facing camera to simulate effect, thickness is measured in world units
		// static inline float LineThickness = 0.003f;

		void EndFrame();

		// Allows to push transformation matrix for functions that don't have transform argument
		static void SetTransform(const Mat44V& mtx) { tl_Transform = mtx; }
		static auto GetTransform() -> const Mat44V& { return tl_Transform; }
		// Sets transform to identity
		static void ResetTransform() { SetTransform(Mat44V::Identity()); }

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
		class Shader
		{
			amComPtr<ID3D11VertexShader> m_VS;
			amComPtr<ID3D11PixelShader>  m_PS;
			amComPtr<ID3D11InputLayout>  m_VSLayout;
			ConstWString				 m_VsName;
			ConstWString				 m_PsName;

		public:
			Shader(ConstWString vsName, ConstWString psName);
			virtual ~Shader() = default;

			virtual void Create();
			virtual void Bind();
		};

		class DefaultShader : public Shader
		{
			struct Locals
			{
				int	Unlit;
			};

			amComPtr<ID3D11Buffer> m_LocalsCB;
		public:
			DefaultShader() : Shader(L"default_vs.hlsl", L"default_ps.hlsl") {}

			void Create() override;
			void Bind() override;

			Locals Locals = {};
		};

		class ImageBlitShader : public Shader
		{
			amComPtr<ID3D11Buffer> m_ScreenVerts;

		public:
			ImageBlitShader() : Shader(L"im_blit_vs.hlsl", L"im_blit_ps.hlsl") {}

			void Create() override;

			void Blit(ID3D11ShaderResourceView* view) const;
		};

		u32                                m_ScreenWidth = 0, m_ScreenHeight = 0, m_SampleCount = 0;
		amComPtr<ID3D11Texture2D>          m_BackbufMs;
		amComPtr<ID3D11RenderTargetView>   m_BackbufMsRt;
		amComPtr<ID3D11Texture2D>          m_Backbuf;
		amComPtr<ID3D11ShaderResourceView> m_BackbufView;
		amComPtr<ID3D11BlendState>		   m_BlendState;
		amComPtr<ID3D11DepthStencilState>  m_DSS;
		amComPtr<ID3D11DepthStencilState>  m_DSSNoDepth;
		amComPtr<ID3D11RasterizerState>    m_RSTwoSided;
		amComPtr<ID3D11RasterizerState>    m_RSWireframe;
		amComPtr<ID3D11RasterizerState>    m_RS;
		DefaultShader                      m_DefaultShader;
		ImageBlitShader                    m_ImBlitShader;

		static ID3D11Buffer* CreateCB(size_t size);
		static void CreateShaders(
			ID3D11VertexShader** vs, 
			ID3D11PixelShader** ps,
			ID3DBlob** vsBlob, 
			ConstWString vsFileName, 
			ConstWString psFileName);

		void RenderDrawData(const DrawList::DrawData& drawData) const;
		void CreateBackbuf();

	public:
		DrawListExecutor();

		void Execute(DrawList& drawList);
	};
}

#endif
