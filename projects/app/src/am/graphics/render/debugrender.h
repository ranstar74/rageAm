#pragma once

#include "am/graphics/buffereditor.h"
#include "am/graphics/render/engine.h"
#include "rage/physics/bounds/boundbase.h"
#include "rage/physics/bounds/composite.h"
#include "rage/physics/bounds/geometry.h"
#include "rage/grcore/buffer.h"
#include "rage/grcore/device.h"
#include "rage/grcore/effect/effectmgr.h"
#include "rage/grm/shadergroup.h"
#include "rage/math/mathv.h"
#include "rage/math/mtxv.h"
#include "rage/creature/skeletondata.h"

class gtaDrawable;

namespace rageam::graphics
{
	// May not be most optimal way, we can improve performance by:
	// - Caching geometry bounds mesh
	// - Batch all primitive bounds in single call

	class DebugRender
	{
		static constexpr u32 VERTEX_BUFFER_LENGTH = UINT16_MAX;
		static constexpr u32 INDEX_BUFFER_LENGTH = UINT16_MAX;

		amComPtr<ID3D11Texture2D> m_RenderTexture;
		amComPtr<ID3D11ShaderResourceView> m_RenderTextureView;
		amComPtr<ID3D11RenderTargetView> m_RenderTargetView;

		amUniquePtr<rage::grmShader>	m_Shader;
		rage::grcVertexBufferD3D11		m_VertexBuffer;
		rage::grcIndexBufferD3D11		m_IndexBuffer;
		rage::grcTextureDX11* m_Texture;

		rageam::graphics::VertexDeclaration m_VertexDecl;
		int width, height;
		void CreateShader()
		{
			rage::grcEffect* fx = rage::grcEffectMgr::FindEffect("emissivestrong");
			m_Shader = std::make_unique<rage::grmShader>(fx);

			// Make it glow
			rage::fxHandle_t emissiveMult = fx->LookupVarByName("emissiveMultiplier");
			m_Shader->GetVar(emissiveMult)->SetValue(60.0f);

			u32 glowColor = 0xFF00FF00; // Green
			m_Texture = new rage::grcTextureDX11(1, 1, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &glowColor);

			rage::fxHandle_t diffuse = fx->LookupVarByName("DiffuseTex");
			m_Shader->GetVar(diffuse)->SetTexture(m_Texture);

			//auto var = m_Shader->GetVar(diffuse);
			//auto varInfo = fx->GetVar("DiffuseTex");

			m_VertexDecl.FromEffect(fx, false);







			auto swapchain = rageam::render::Engine::GetInstance()->GetSwapchain();
			auto device = rageam::render::Engine::GetInstance()->GetFactory();

			D3D11_TEXTURE2D_DESC gameTextureDesc;

			// Use original back buffer desc for our custom one
			ID3D11Texture2D* pBackBuffer;
			swapchain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
			pBackBuffer->GetDesc(&gameTextureDesc);
			pBackBuffer->Release();

			// Override resolution
			/*gameTextureDesc.Width = s_GameWidth;
			gameTextureDesc.Height = s_GameHeight;*/
			gameTextureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

			ID3D11Texture2D* texture;
			ID3D11ShaderResourceView* textureView;
			device->CreateTexture2D(&gameTextureDesc, NULL, &texture);

			D3D11_SHADER_RESOURCE_VIEW_DESC gameTextureViewDesc = {};
			gameTextureViewDesc.Format = gameTextureDesc.Format;
			gameTextureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			gameTextureViewDesc.Texture2D.MipLevels = 1;
			device->CreateShaderResourceView(texture, &gameTextureViewDesc, &textureView);

			m_RenderTexture = texture;
			m_RenderTextureView = textureView;

			D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc{};
			renderTargetViewDesc.Format = gameTextureDesc.Format;
			renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

			ID3D11RenderTargetView* rtv;
			device->CreateRenderTargetView(texture, &renderTargetViewDesc, &rtv);
			m_RenderTargetView = rtv;

			width = gameTextureDesc.Width;
			height = gameTextureDesc.Height;
		}

		ID3D11RenderTargetView* oldRT;
		ID3D11DepthStencilView* oldDS;
		void SetRT()
		{
			auto context = rageam::render::Engine::GetInstance()->GetDeviceContext();

			context->OMGetRenderTargets(1, &oldRT, &oldDS);

			ID3D11RenderTargetView* views = m_RenderTargetView.Get();
			context->OMSetRenderTargets(1, &views, NULL);
		}

		void UnsetRT()
		{
			auto context = rageam::render::Engine::GetInstance()->GetDeviceContext();
			context->OMSetRenderTargets(1, &oldRT, oldDS);
		}


		void CreateBuffers()
		{
			m_VertexBuffer.CreateDynamic(m_VertexDecl.GrcInfo.Fvf, VERTEX_BUFFER_LENGTH);
			m_IndexBuffer.CreateDynamic(INDEX_BUFFER_LENGTH);
		}

		void RenderBoundGeometry(const rage::phBoundGeometry* bound)
		{
			amUniquePtr<char> vertexBuffer = bound->ComposeVertexBuffer(m_VertexDecl.GrcInfo);
			amUniquePtr<u16> indices = bound->GetIndices();

			pVoid vb = m_VertexBuffer.GetBuffer()->Bind();
			pVoid ib = m_IndexBuffer.GetBuffer()->Bind();

			memcpy(vb, vertexBuffer.get(), m_VertexDecl.Stride * bound->GetVertexCount());
			memcpy(ib, indices.get(), sizeof u16 * bound->GetIndexCount());

			m_IndexBuffer.GetBuffer()->Unbind();
			m_VertexBuffer.GetBuffer()->Unbind();

			rage::grcDevice::DrawIndexedPrimitive(
				rage::GRC_DRAW_TRIANGLELIST, m_VertexDecl.GrcInfo.Decl, &m_VertexBuffer, &m_IndexBuffer, bound->GetIndexCount());
		}

		void DispatchPrimitive(rage::phBound* bound, const rage::Mat44V& mtx)
		{
			rage::grcDevice::SetWorldMtx(*(rage::Mat34V*) & mtx);

			switch (bound->GetShapeType())
			{
			case rage::PH_BOUND_GEOMETRY: RenderBoundGeometry((rage::phBoundGeometry*)bound);	break;
			case rage::PH_BOUND_COMPOSITE:
			{
				rage::phBoundComposite* composite = (rage::phBoundComposite*)bound;
				u16 numBounds = composite->GetNumBounds();
				for (u16 i = 0; i < numBounds; i++)
				{
					rage::Mat44V boundWorld = composite->GetMatrix(i) * mtx;
					DispatchPrimitive(composite->GetBound(i).Get(), boundWorld);
				}

				break;
			}

			default: break;
			}
		}

	public:
		DebugRender() = default;

		void Init()
		{
			CreateShader();
			CreateBuffers();
		}

		void  DrawLine(rage::Vec3V v1, rage::Vec3V v2, u32 col)
		{
			static gmAddress addr = gmAddress::Scan("48 89 5C 24 08 44 89 44 24 18 57 48 83 EC 20 48 8B FA BA");
			static auto grcDrawLine = addr.To<void(*)(rage::Vec3V & from, rage::Vec3V & to, u32 color)>();

			//v1 = DirectX::XMVector3Transform(v1, mtx);
			//v2 = DirectX::XMVector3Transform(v2, mtx);

			grcDrawLine(v1, v2, col);
		}

		void RenderBoneRecurse(rage::crSkeletonData* skel, const rage::crBoneData* rootBone, u32 col)
		{
			rage::crBoneData* childBone = skel->GetFirstChildBone(rootBone->GetIndex());
			while (childBone)
			{
				DrawLine(rootBone->GetTranslation(), childBone->GetTranslation(), col);
				RenderBoneRecurse(skel, childBone, col);
				childBone = skel->GetBone(childBone->GetNextIndex());
			}
		}

		void RenderToScreen()
		{
			// ImGui::Image(m_RenderTextureView.Get(), /*ImVec2(width, height)*/ ImVec2(128, 128));
		}

		void Render(rage::crSkeletonData* skel, const rage::Mat34V& mtx)
		{
			if (!skel)
				return;

			rage::grcDevice::SetWorldMtx(mtx);

			u16 passCount = m_Shader->BeginDraw();
			for (u16 i = 0; i < passCount; i++)
			{
				m_Shader->BeginPass(i);

				u32 col = 0xFF0000FF;

				for (u16 k = 0; k < skel->GetBoneCount(); k++)
				{
					RenderBoneRecurse(skel, skel->GetBone(k), col);
				}

				m_Shader->EndPass();
			}
			m_Shader->EndDraw();
		}

		void Render(const rage::spdAABB& bb, const rage::Mat34V& mtx, u32 col)
		{
			//SetRT();
			rage::grcDevice::SetWorldMtx(mtx);

			u16 passCount = m_Shader->BeginDraw();
			for (u16 i = 0; i < passCount; i++)
			{
				m_Shader->BeginPass(i);

				// Top Face
				DrawLine(bb.TTL(), bb.TTR(), col);
				DrawLine(bb.TTR(), bb.TBR(), col);
				DrawLine(bb.TBR(), bb.TBL(), col);
				DrawLine(bb.TBL(), bb.TTL(), col);
				// Bottom Face
				DrawLine(bb.BTL(), bb.BTR(), col);
				DrawLine(bb.BTR(), bb.BBR(), col);
				DrawLine(bb.BBR(), bb.BBL(), col);
				DrawLine(bb.BBL(), bb.BTL(), col);
				// Vertical Edges
				DrawLine(bb.BTL(), bb.TTL(), col);
				DrawLine(bb.BTR(), bb.TTR(), col);
				DrawLine(bb.BBL(), bb.TBL(), col);
				DrawLine(bb.BBR(), bb.TBR(), col);

				m_Shader->EndPass();
			}
			m_Shader->EndDraw();
			//UnsetRT();
		}

		void Render(rage::phBound* bound, const rage::Mat34V& mtx)
		{
			if (!bound)
				return;

			//if (bRenderBoundExtents)
			//	Render(bound->GetBoundingBox(), mtx, 0xFF0000FF);

			rage::grcDevice::SetWorldMtx(mtx);

			u16 passCount = m_Shader->BeginDraw();
			for (u16 i = 0; i < passCount; i++)
			{
				m_Shader->BeginPass(i);
				DispatchPrimitive(bound, mtx.To44());
				m_Shader->EndPass();
			}
			m_Shader->EndDraw();
		}

		void RenderDrawable(gtaDrawable* drawable, const rage::Mat44V& mtx);

		bool bRenderSkeleton = true;
		bool bRenderLodGroupExtents = true;
		bool bRenderBoundExtents = true;
		bool bRenderGeometryExtents = true;
		bool bRenderBoundMesh = false;
	};
}
