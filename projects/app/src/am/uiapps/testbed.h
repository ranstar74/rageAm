#pragma once

#include "am/asset/factory.h"
#include "am/asset/ui/assetwindowfactory.h"
#include "am/graphics/image/imagecache.h"
#include "am/ui/app.h"
#include "am/ui/imglue.h"
#include "am/ui/slwidgets.h"
#include "helpers/format.h"

#ifdef AM_INTEGRATED
#include "am/integration/ui/modelscene.h"
#include "am/integration/ui/starbar.h"
#include "am/integration/ui/modelinspector.h"
#include "rage/paging/builder/builder.h"
#endif

namespace rageam::ui
{
	class TestbedApp : public App
	{
#define QUICK_ITD_PATH L"X:/am.ws/am.itd"
#ifdef AM_INTEGRATED
#define QUICK_SCENE_PATH L"X:/am.ws/airpump.idr"
#define QUICK_YDR_PATH L"X:/am.ws/airpump.ydr"
#define QUICK_YDR_PATH_HS "X:/am.ws/airpump.ydr"

#endif

		void OnRender() override
		{
			ImGui::Begin("rageAm Testbed");

			ImGlue* ui = GetUI();
			ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.0f);
			ImGui::SliderInt("Font Size", &ui->FontSize, 8, 24);

#ifdef AM_INTEGRATED
			SlGui::CategoryText("YDR Export");

			if (ImGui::Button("Compile IDR"))
			{
				auto dr = asset::AssetFactory::LoadFromPath<asset::DrawableAsset>(QUICK_SCENE_PATH);
				if (dr) dr->CompileToFile();
			}

			if (ImGui::Button("Build YDR"))
			{
				gtaDrawable* drawable = new gtaDrawable();
				//rage::pgRscBuilder::Load(&drawable, "X:/am.ws/v_ind_cfbucket.ydr", 165);
				rage::pgRscBuilder::Load(&drawable, QUICK_YDR_PATH_HS, 165);
				//delete drawable;
			}

			if (ImGui::Button("Inspect YDR"))
			{
				Scene::OpenWindowForSceneAndLoad(QUICK_YDR_PATH);
			}

			SlGui::CategoryText("Model Scene");

			bool loadedScene = false;
			if (ImGui::Button("Load IDR"))
			{
				Scene::OpenWindowForSceneAndLoad(QUICK_SCENE_PATH);
				loadedScene = true;
			}

			if (ImGui::Button("Load IDR + Flush Cache"))
			{
				graphics::ImageCache::GetInstance()->Clear();
				Scene::OpenWindowForSceneAndLoad(QUICK_SCENE_PATH);
				loadedScene = true;
			}

			if (loadedScene)
			{
				Scene::OpenWindowForSceneAndLoad(QUICK_SCENE_PATH);
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
