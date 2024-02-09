#pragma once

#include "am/asset/factory.h"
#include "am/asset/ui/assetwindowfactory.h"
#include "am/graphics/image/imagecache.h"
#include "am/ui/app.h"
#include "helpers/format.h"

#ifdef AM_INTEGRATED
#include "am/integration/ui/modelscene.h"
#include "am/integration/ui/starbar.h"
#endif

namespace rageam::ui
{
	class TestbedApp : public App
	{
#define QUICK_ITD_PATH	 L"X:/am.ws/am.itd"
#ifdef AM_INTEGRATED
#define QUICK_SCENE_PATH L"X:/am.ws/airpump.idr"

		integration::ModelScene* m_ModelScene = nullptr;
		bool                     m_CameraSet = false;

		void OnStart() override
		{
			ImGlue* ui = GetUI();
			m_ModelScene = ui->FindAppByType<integration::ModelScene>();
			m_ModelScene->LoadFromPatch(QUICK_SCENE_PATH);
			ui->FindAppByType<integration::StarBar>()->SetCameraEnabled(true);
		}
#endif

		void OnRender() override
		{
			ImGui::Begin("rageAm Testbed");

			ImGlue* ui = GetUI();
			ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.0f);
			ImGui::SliderInt("Font Size", &ui->FontSize, 8, 24);

#ifdef AM_INTEGRATED
			if (!m_CameraSet && m_ModelScene->IsLoaded())
			{
				m_ModelScene->ResetCameraPosition();
				m_CameraSet = true;
			}

			if (ImGui::Button("Load IDR"))
			{
				m_ModelScene->LoadFromPatch(QUICK_SCENE_PATH);
			}

			if (ImGui::Button("Load IDR + Flush Cache"))
			{
				graphics::ImageCache::GetInstance()->Clear();
				m_ModelScene->Unload(false);
				m_ModelScene->LoadFromPatch(QUICK_SCENE_PATH);
			}
#endif

			if (ImGui::Button("ITD"))
			{
				using namespace asset;

				AssetPtr txd = AssetFactory::LoadFromPath(QUICK_ITD_PATH);
				AssetWindowFactory::OpenNewOrFocusExisting(txd);
			}

			if (ImGui::CollapsingHeader("ImageCache", ImGuiTreeNodeFlags_DefaultOpen))
			{
				graphics::ImageCache*     imageCache = graphics::ImageCache::GetInstance();
				graphics::ImageCacheState ics = imageCache->GetState();

				ImGui::BulletText("RAM: %s/%s", FormatSize(ics.SizeRamUsed), FormatSize(ics.SizeRamBudget));
				ImGui::BulletText("FS: %s/%s", FormatSize(ics.SizeFsUsed), FormatSize(ics.SizeFsBudget));
				ImGui::BulletText("Image counts:");
				ImGui::Indent();
				ImGui::Text("- RAM: %u", ics.ImageCountRam);
				ImGui::Text("- FS: %u", ics.ImageCountFs);
				ImGui::Text("- DX11 Views: %u", ics.DX11ViewCount);
				if (ImGui::Button("Clear"))
					imageCache->Clear();
				ImGui::Unindent();
			}

			ImGui::End();
		}
	};
}
