#pragma once

#include "am/ui/app.h"
#include "am/graphics/image/image.h"
#include <am/graphics/utils.h>

namespace rageam::ui
{
	class TestbedApp : public App
	{
		amComPtr<ID3D11ShaderResourceView> m_ImageView;

		void OnRender() override
		{
			ImGui::Begin("rageAm Testbed");

			auto bc1 = DirectX::BitsPerPixel(DXGI_FORMAT_BC1_UNORM);
			auto bc2 = DirectX::BitsPerPixel(DXGI_FORMAT_BC2_UNORM);
			auto bc3 = DirectX::BitsPerPixel(DXGI_FORMAT_BC3_UNORM);
			auto bc4 = DirectX::BitsPerPixel(DXGI_FORMAT_BC4_UNORM);
			auto bc5 = DirectX::BitsPerPixel(DXGI_FORMAT_BC5_UNORM);
			auto bc7 = DirectX::BitsPerPixel(DXGI_FORMAT_BC7_UNORM);

			if (ImGui::Button("Image"))
			{
				u8 pixelData[] =
				{
					255, 0, 0, 
					0, 255, 0,
					0, 0, 255,
					0, 0, 0
				};
				
				graphics::ImagePtr image = graphics::ImageFactory::CreateFromMemory(
					pixelData, graphics::ImagePixelFormat_U24, 2, 2, false);

				image->Save(L"C:/Users/falco/OneDrive/Рабочий стол/img.png");

				rageam::graphics::ShaderResourceOptions shaderResourceOpts = {};
				shaderResourceOpts.CreateMips = true;
				image->CreateDX11Resource(shaderResourceOpts, m_ImageView);
			}

			ImGui::Image(ImTextureID(m_ImageView.Get()), ImVec2(256, 256));

			ImGui::End();
		}
	};
}
