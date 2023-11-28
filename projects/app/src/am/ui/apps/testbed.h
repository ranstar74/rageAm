#pragma once

#include "am/ui/app.h"
#include "am/graphics/image/image.h"
#include <am/graphics/utils.h>

namespace rageam::ui
{
	class TestbedApp : public App
	{
		amComPtr<ID3D11ShaderResourceView> m_ImageView;
		Vec2S m_UVExtent = { 1, 1 };

		void OnRender() override
		{
			ImGui::Begin("rageAm Testbed");

			auto bc1 = DirectX::BitsPerPixel(DXGI_FORMAT_BC1_UNORM);
			auto bc2 = DirectX::BitsPerPixel(DXGI_FORMAT_BC2_UNORM);
			auto bc3 = DirectX::BitsPerPixel(DXGI_FORMAT_BC3_UNORM);
			auto bc4 = DirectX::BitsPerPixel(DXGI_FORMAT_BC4_UNORM);
			auto bc5 = DirectX::BitsPerPixel(DXGI_FORMAT_BC5_UNORM);
			auto bc7 = DirectX::BitsPerPixel(DXGI_FORMAT_BC7_UNORM);

			if(ImGui::Button("16K"))
			{
				Timer timer = Timer::StartNew();
				graphics::ImagePtr img = graphics::ImageFactory::LoadFromPath(
					L"C:/Users/falco/Desktop/ImgTestbed/Heavy/AS17-134-20489.png");
				timer.Stop();
				AM_TRACEF("AS17-134-20489.png: %llums", timer.GetElapsedMilliseconds());
			}

			static constexpr ConstWString s_BenchmarkFormats[] =
			{
				L"bmp", L"jpg", L"png", L"psd", L"tga", L"webp"
			};

			if (ImGui::Button("Benchmark"))
			{
				file::WPath basePath = L"C:/Users/falco/Desktop/ImgTestbed/Benchmark/gradient.";

				AM_TRACEF("# Image Benchmark");
				AM_TRACEF("Format; Load Time (ms)");
				for (auto ext : s_BenchmarkFormats)
				{
					file::WPath imgPath = basePath + ext;
					Timer timer = Timer::StartNew();
					graphics::ImagePtr img = graphics::ImageFactory::LoadFromPath(imgPath);
					timer.Stop();
					AM_TRACEF("%ls:\t%llu", ext, timer.GetElapsedMilliseconds());
				}
			}

			if (ImGui::Button("Load PNG and save to WEBP"))
			{
				ConstWString srcPath = L"C:/Users/falco/Desktop/ImgTestbed/gradient.png";
				ConstWString dstPath = L"C:/Users/falco/Desktop/ImgTestbed/gradient.webp";

				Timer timer = Timer::StartNew();
				graphics::ImagePtr leaf = graphics::ImageFactory::LoadFromPath(srcPath);
				graphics::ImageFactory::SaveImage(leaf, dstPath);
				timer.Stop();
				AM_TRACEF("Load PNG and save to WEBP took %llu milliseconds", timer.GetElapsedMilliseconds());
			}

			if (ImGui::Button("Image"))
			{
				//u8 pixelData[] =
				//{
				//	255, 0, 0,
				//	0, 255, 0,
				//	0, 0, 255,
				//	0, 0, 0
				//};

				//graphics::PixelDataOwner pixelDataOwner = graphics::PixelDataOwner::CreateUnowned(pixelData);
				//graphics::ImagePtr image = graphics::ImageFactory::CreateFromMemory(
				//	pixelDataOwner, graphics::ImagePixelFormat_U24, 2, 2, false);

				//image->Save(L"C:/Users/falco/Desktop/img.png");

				auto a1 = graphics::ImageFactory::IsSupportedImageFormat(L"PNG");
				auto a2 = graphics::ImageFactory::IsSupportedImageFormat(L"PND");
				auto a3 = graphics::ImageFactory::IsSupportedImageFormat(L"png");
				auto a4 = graphics::ImageFactory::IsSupportedImageFormat(L"webp");
				auto a5 = graphics::ImageFactory::IsSupportedImageFormat(L"webg");
				auto a6 = graphics::ImageFactory::IsSupportedImageFormat(L"WEBP");

				//graphics::ImagePtr image = graphics::ImageFactory::LoadFromPath(L"C:/Users/falco/Desktop/ImgTestbed/4x4.png");
				graphics::ImagePtr image = graphics::ImageFactory::LoadFromPath(
					L"C:/Users/falco/Desktop/ImgTestbed/gradient.dds");
				
				//graphics::ImagePtr resizedImage = image->Resize(2048, 2048, graphics::ResizeFilter_Box);
				//image = image->Resize(512, 512);
				//image = image->PadToPowerOfTwo(m_UVExtent);

				//image = image->Resize(512, 512);
				//graphics::ImageFactory::SaveImage(image, L"C:/Users/falco/Desktop/test_save.webp", graphics::ImageKind_WEBP, 1.0f);

				graphics::ShaderResourceOptions shaderResourceOpts = {};
				shaderResourceOpts.CreateMips = true;
				image->CreateDX11Resource(shaderResourceOpts, m_ImageView);
			}

			ImVec2 uv0(0, 0);
			ImVec2 uv1(m_UVExtent.X, m_UVExtent.Y);
			ImGui::Image(ImTextureID(m_ImageView.Get()), ImVec2(256, 256), uv0, uv1);

			ImGui::End();
		}
	};
}
