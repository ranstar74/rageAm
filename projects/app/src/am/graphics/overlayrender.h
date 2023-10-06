#pragma once

#include <d3dcompiler.h>

#include "color.h"
#include "rage/math/vec.h"
#include "rage/spd/aabb.h"

#include <mutex>

#include "BufferHelpers.h"
#include "am/system/datamgr.h"
#include "am/system/ptr.h"
#include "game/viewport.h"
#include "rage/math/math.h"
#include "render/engine.h"

namespace rageam::graphics
{
	// TODO: Support non-overlay render
	// TODO: Add opacity settings of overlay

	// We support rendering two types of primitives -
	// 1) Line - Unlit, 2 colors
	// 2) Triangle - Lit or Unlit, wireframe or solid

#pragma pack(push, 1)
	struct VertexUnlit
	{
		rage::Vector3 Pos;
		rage::Vector4 Color;
	};
#pragma pack(pop)

	struct UnlitConstantBuffer
	{
		rage::Mat44V ViewProj;
	};

	class OverlayRender
	{
		static constexpr u32 LINE_BUFFER_MAX = 0x10000;

		//amComPtr<ID3D11RenderTargetView>	m_BackBufferRt;
		amComPtr<ID3D11BlendState>			m_BlendState;
		amComPtr<ID3D11Buffer>				m_UnlitConstantBuffer;
		amComPtr<ID3DBlob>					m_UnlitVSBlob;
		amComPtr<ID3D11InputLayout>			m_UnlitLayoutVS;
		amComPtr<ID3D11VertexShader>		m_UnlitVS;
		amComPtr<ID3D11PixelShader>			m_UnlitPS;
		amComPtr<ID3D11RasterizerState>		m_Resterizer;
		amComPtr<ID3D11Buffer>				m_LineDeviceBuffer;
		VertexUnlit							m_LineBuffer[LINE_BUFFER_MAX] = {};
		u32									m_LineVertexCount = 0;
		std::recursive_mutex				m_Mutex;
		u32									m_Width = 0;
		u32									m_Height = 0;
		rage::Mat44V						m_CurrentMatrix = rage::Mat44V::Identity();

		void CreateConstantBuffer()
		{
			ID3D11Device* device = render::GetDevice();

			ID3D11Buffer* buffer;
			D3D11_BUFFER_DESC desc;
			desc.ByteWidth = sizeof UnlitConstantBuffer;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			device->CreateBuffer(&desc, NULL, &buffer);

			m_UnlitConstantBuffer = buffer;
		}

		void CreateVertexBuffer(amComPtr<ID3D11Buffer>& buffer, u32 vertexStride, u32 vertexCount) const
		{
			ID3D11Device* device = render::GetDevice();
			ID3D11Buffer* object;
			D3D11_BUFFER_DESC desc;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.ByteWidth = vertexStride * vertexCount;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = vertexStride;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			AM_ASSERT_D3D(device->CreateBuffer(&desc, NULL, &object));
			buffer = object;
		}

		void UploadBuffer(const amComPtr<ID3D11Buffer>& buffer, pVoid data, u32 dataSize) const
		{
			ID3D11DeviceContext* context = render::GetDeviceContext();
			D3D11_MAPPED_SUBRESOURCE mapped;
			AM_ASSERT_D3D(context->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
			memcpy(mapped.pData, data, dataSize);
			context->Unmap(buffer.Get(), 0);
		}

		void CreateBlendState()
		{
			ID3D11Device* device = render::GetDevice();

			D3D11_BLEND_DESC desc = {};
			desc.RenderTarget[0].BlendEnable = TRUE;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].RenderTargetWriteMask = 0xF;
			desc.AlphaToCoverageEnable = FALSE;
			desc.IndependentBlendEnable = TRUE;

			ID3D11BlendState* blendState;
			AM_ASSERT_D3D(device->CreateBlendState(&desc, &blendState));

			m_BlendState = blendState;
		}

		void CreateBackBuffer()
		{
			//ID3D11Device* device = render::GetDevice();

			//ID3D11Texture2D* tx;
			//D3D11_TEXTURE2D_DESC txDesc = {};
			//txDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			//txDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
			//txDesc.ArraySize = 1;
			//txDesc.MipLevels = 1;
			//txDesc.Width = m_Width;
			//txDesc.Height = m_Height;
			//txDesc.SampleDesc.Count = 1;
			//txDesc.Usage = D3D11_USAGE_DYNAMIC;
			//AM_ASSERT_D3D(device->CreateTexture2D(&txDesc, NULL, &tx));

			//ID3D11RenderTargetView* rt;
			//D3D11_RENDER_TARGET_VIEW_DESC rtDesc = {};
			//rtDesc.Format = txDesc.Format;
			//rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			//AM_ASSERT_D3D(device->CreateRenderTargetView(tx, &rtDesc, &rt));

			//m_BackBufferRt = rt;

			//tx->Release();
		}

		void CreateRasterizer()
		{
			ID3D11Device* device = render::GetDevice();

			ID3D11RasterizerState* rs;
			D3D11_RASTERIZER_DESC rasterizerState;
			rasterizerState.FillMode = D3D11_FILL_SOLID;
			rasterizerState.CullMode = D3D11_CULL_FRONT;
			rasterizerState.FrontCounterClockwise = true;
			rasterizerState.DepthBias = false;
			rasterizerState.DepthBiasClamp = 0;
			rasterizerState.SlopeScaledDepthBias = 0;
			rasterizerState.DepthClipEnable = true;
			rasterizerState.ScissorEnable = true;
			rasterizerState.AntialiasedLineEnable = false;
			rasterizerState.MultisampleEnable = true;
			AM_ASSERT_D3D(device->CreateRasterizerState(&rasterizerState, &rs));

			m_Resterizer = rs;
		}

		void CreateUnlitShader()
		{
			ID3D11Device* device = render::GetDevice();

			// VS
			{
				file::WPath shaderPath = DataManager::GetDataFolder() / L"shaders" / L"col_overlay_unlit_vs.hlsl";
				ID3DBlob* vsBlob;
				ID3DBlob* errors;
				D3DCompileFromFile(shaderPath, NULL, NULL, "main", "vs_5_0", 0, 0, &vsBlob, &errors);
				if (errors) errors->Release();

				ID3D11VertexShader* vs;
				device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &vs);
				m_UnlitVS = vs;

				m_UnlitVSBlob = vsBlob;
			}

			// PS
			{
				file::WPath shaderPath = DataManager::GetDataFolder() / L"shaders" / L"col_overlay_unlit_ps.hlsl";
				ID3DBlob* psBlob;
				ID3DBlob* errors;
				D3DCompileFromFile(shaderPath, NULL, NULL, "main", "ps_5_0", 0, 0, &psBlob, &errors);

				ID3D11PixelShader* ps;
				device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &ps);
				m_UnlitPS = ps;

				if (errors) errors->Release();
				if (psBlob) psBlob->Release();
			}

			D3D11_INPUT_ELEMENT_DESC inputDesc[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			ID3D11InputLayout* vsLayout;
			device->CreateInputLayout(inputDesc, 2, m_UnlitVSBlob->GetBufferPointer(), m_UnlitVSBlob->GetBufferSize(), &vsLayout);

			m_UnlitLayoutVS = vsLayout;
		}

		// Those are non-thread safe functions, exposed interface that calls them must use mutex

		void DrawLine_Unsafe(const rage::Vec3V& p1, const rage::Vec3V& p2, const rage::Mat44V& mtx, ColorU32 col1, ColorU32 col2)
		{
			if (col1.A == 0 && col2.A == 0)
				return;

			rage::Vec3V pt1 = p1.Transform(mtx);
			rage::Vec3V pt2 = p2.Transform(mtx);
			m_LineBuffer[m_LineVertexCount++] = VertexUnlit(pt1, col1.ToVec4());
			m_LineBuffer[m_LineVertexCount++] = VertexUnlit(pt2, col2.ToVec4());
		}

		void DrawLine_Unsafe(const rage::Vec3V& p1, const rage::Vec3V& p2, const rage::Mat44V& mtx, ColorU32 col)
		{
			DrawLine_Unsafe(p1, p2, mtx, col, col);
		}

	public:
		void Init()
		{
			rage::grcDevice::GetScreenSize(m_Width, m_Height);

			CreateVertexBuffer(m_LineDeviceBuffer, sizeof VertexUnlit, LINE_BUFFER_MAX);
			CreateBackBuffer();
			CreateRasterizer();
			CreateUnlitShader();
			CreateConstantBuffer();
			CreateBlendState();
		}

		// Used on draw functions that have no matrix argument
		void SetCurrentMatrix(const rage::Mat44V& mtx)
		{
			m_CurrentMatrix = mtx;
		}

		void ResetCurrentMatrix()
		{
			m_CurrentMatrix = rage::Mat44V::Identity();
		}

		void DrawLine(const rage::Vec3V& p1, const rage::Vec3V& p2, const rage::Mat44V& mtx, ColorU32 col1, ColorU32 col2)
		{
			std::unique_lock lock(m_Mutex);
			DrawLine_Unsafe(p1, p2, mtx, col1, col2);
		}

		void DrawLine(const rage::Vec3V& p1, const rage::Vec3V& p2, const rage::Mat44V& mtx, ColorU32 col)
		{
			DrawLine(p1, p2, mtx, col, col);
		}

		void DrawLine(const rage::Vec3V& p1, const rage::Vec3V& p2, ColorU32 col1, ColorU32 col2)
		{
			std::unique_lock lock(m_Mutex);
			DrawLine_Unsafe(p1, p2, m_CurrentMatrix, col1, col2);
		}

		void DrawLine(const rage::Vec3V& p1, const rage::Vec3V& p2, ColorU32 col)
		{
			DrawLine(p1, p2, m_CurrentMatrix, col, col);
		}

		void DrawAABB(const rage::spdAABB& bb, const rage::Mat44V& mtx, ColorU32 col)
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

		// Normal & tangent define the plane the circle lines in,
		// Tangent also defines start of the circle, first half
		// is draw with col1 and second half is using col2
		// This is mostly used on light sphere back side is culled / draw gray
		// https://imgur.com/iQG2PMS
		void DrawCircle(
			const rage::Vec3V& pos,
			const rage::Vec3V& normal,
			const rage::Vec3V& tangent,
			const rage::ScalarV& radius,
			const rage::Mat44V& mtx,
			const ColorU32 col1,
			const ColorU32 col2,
			float startAngle = 0.0f, float angle = rage::PI2)
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

		void DrawCircle(
			const rage::Vec3V& pos,
			const rage::Vec3V& normal,
			const rage::Vec3V& tangent,
			const rage::ScalarV& radius,
			const ColorU32 col1,
			const ColorU32 col2,
			float startAngle = 0.0f, float angle = rage::PI2)
		{
			DrawCircle(pos, normal, tangent, radius, m_CurrentMatrix, col1, col2, startAngle, angle);
		}

		void DrawCircle(
			const rage::Vec3V& pos,
			const rage::Vec3V& normal,
			const rage::Vec3V& tangent,
			const rage::ScalarV& radius,
			const ColorU32 col,
			float startAngle = 0.0f, float angle = rage::PI2)
		{
			DrawCircle(pos, normal, tangent, radius, m_CurrentMatrix, col, col, startAngle, angle);
		}

		// Draws sphere using 3 axes
		void DrawSphere(const rage::Mat44V& mtx, ColorU32 color, float radius)
		{
			DrawCircle(rage::S_ZERO, rage::VEC_FORWARD, rage::VEC_UP, radius, mtx, color, color);
			DrawCircle(rage::S_ZERO, rage::VEC_RIGHT, rage::VEC_UP, radius, mtx, color, color);
			DrawCircle(rage::S_ZERO, rage::VEC_UP, rage::VEC_FORWARD, radius, mtx, color, color);
		}

		void DrawSphere(ColorU32 color, float radius)
		{
			DrawCircle(rage::S_ZERO, rage::VEC_FORWARD, rage::VEC_UP, radius, m_CurrentMatrix, color, color);
			DrawCircle(rage::S_ZERO, rage::VEC_RIGHT, rage::VEC_UP, radius, m_CurrentMatrix, color, color);
			DrawCircle(rage::S_ZERO, rage::VEC_UP, rage::VEC_FORWARD, radius, m_CurrentMatrix, color, color);
		}

		void DrawCapsule() {}

		void UpdateMatrices() const
		{
			UnlitConstantBuffer cb;
			cb.ViewProj = CViewport::GetViewProjectionMatrix();
			UploadBuffer(m_UnlitConstantBuffer, &cb, sizeof UnlitConstantBuffer);
		}

		void Render()
		{
			std::unique_lock lock(m_Mutex);

			if (!m_LineVertexCount)
				return;

			UploadBuffer(m_LineDeviceBuffer, m_LineBuffer, m_LineVertexCount * sizeof VertexUnlit);
			UpdateMatrices();

			// TODO: Render to back buffer & blend

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
				UINT  PSInstancesCount, VSInstancesCount;
				ID3D11ClassInstance* PSInstances[256], * VSInstances[256];
				ID3D11Buffer* IndexBuffer, * VertexBuffer, * VSConstantBuffer;
				UINT IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
				DXGI_FORMAT IndexBufferFormat;
				ID3D11InputLayout* InputLayout;
			};
			OldState oldState{};

			ID3D11DeviceContext* context = render::GetDeviceContext();

			// Backup old state
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
			context->VSGetConstantBuffers(0, 1, &oldState.VSConstantBuffer);
			context->IAGetInputLayout(&oldState.InputLayout);
			context->IAGetIndexBuffer(&oldState.IndexBuffer, &oldState.IndexBufferFormat, &oldState.IndexBufferOffset);
			context->IAGetVertexBuffers(0, 1, &oldState.VertexBuffer, &oldState.VertexBufferStride, &oldState.VertexBufferOffset);

			// Setup state
			static float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			context->OMSetBlendState(m_BlendState.Get(), blendFactor, 0xffffffff);
			context->OMSetDepthStencilState(NULL, 0);
			D3D11_VIEWPORT vp = {};
			vp.Width = (float)m_Width;
			vp.Height = (float)m_Height;
			vp.MaxDepth = 1.0;
			context->RSSetViewports(1, &vp);
			D3D11_RECT scissors = {};
			scissors.right = (LONG)m_Width;
			scissors.bottom = (LONG)m_Height;
			context->RSSetScissorRects(1, &scissors);
			context->RSSetState(m_Resterizer.Get());

			// Draw lines
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
			ID3D11Buffer* lb = m_LineDeviceBuffer.Get();
			unsigned int stride = sizeof VertexUnlit;
			unsigned int offset = 0;
			context->IASetVertexBuffers(0, 1, &lb, &stride, &offset);
			context->IASetIndexBuffer(NULL, DXGI_FORMAT_UNKNOWN, 0);
			context->IASetInputLayout(m_UnlitLayoutVS.Get());
			context->VSSetShader(m_UnlitVS.Get(), NULL, 0);
			context->PSSetShader(m_UnlitPS.Get(), NULL, 0);
			ID3D11Buffer* constantBuffer = m_UnlitConstantBuffer.Get();
			context->VSSetConstantBuffers(0, 1, &constantBuffer);
			context->Draw(m_LineVertexCount, 0);

			// Restore state
			context->OMSetBlendState(oldState.BlendState, oldState.BlendFactor, oldState.BlendMask);
			context->OMSetDepthStencilState(oldState.DepthStencilState, oldState.StencilRef);
			context->RSSetViewports(oldState.NumViewports, oldState.Viewports);
			context->RSSetScissorRects(oldState.NumScissors, oldState.Scissors);
			context->RSSetState(oldState.RasteriszerState);
			context->IASetPrimitiveTopology(oldState.PrimitiveTopology);
			context->PSSetShader(oldState.PS, oldState.PSInstances, oldState.PSInstancesCount);
			context->VSSetShader(oldState.VS, oldState.VSInstances, oldState.VSInstancesCount);
			context->VSSetConstantBuffers(0, 1, &oldState.VSConstantBuffer);
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

			m_LineVertexCount = 0;
		}
	};
}
