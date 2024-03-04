#pragma once

#include "am/asset/factory.h"
#include "am/asset/ui/assetwindowfactory.h"
#include "am/graphics/image/imagecache.h"
#include "am/ui/app.h"
#include "am/ui/slwidgets.h"
#include "helpers/format.h"
#include "rage/paging/builder/builder.h"

#ifdef AM_INTEGRATED
#include "am/integration/ui/modelscene.h"
#include "am/integration/ui/starbar.h"
#include "am/integration/ui/modelinspector.h"
#endif

namespace rageam::ui
{
	class TestbedApp : public App
	{
#define QUICK_ITD_PATH	 L"X:/am.ws/am.itd"
#ifdef AM_INTEGRATED
#define QUICK_SCENE_PATH L"X:/am.ws/airpump.idr"
#define QUICK_YDR_PATH L"X:/am.ws/airpump.ydr"
#define QUICK_YDR_PATH_HS "X:/am.ws/airpump.ydr"

		integration::ModelScene*	 m_ModelScene = nullptr;
		integration::ModelInspector* m_ModelInspector = nullptr;
		bool                         m_CameraSet = true;

		void OnStart() override
		{
			ImGlue* ui = GetUI();
			m_ModelScene = ui->FindAppByType<integration::ModelScene>();
			m_ModelInspector = ui->FindAppByType<integration::ModelInspector>();
		}
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
				m_ModelInspector->LoadFromPath(QUICK_YDR_PATH);
			}

			SlGui::CategoryText("Model Scene");

			bool loadedScene = false;
			if (ImGui::Button("Load IDR"))
			{
				m_ModelScene->LoadFromPatch(QUICK_SCENE_PATH);
				loadedScene = true;
			}

			if (ImGui::Button("Load IDR + Flush Cache"))
			{
				graphics::ImageCache::GetInstance()->Clear();
				m_ModelScene->Unload(false);
				m_ModelScene->LoadFromPatch(QUICK_SCENE_PATH);
				loadedScene = true;
			}

			if (loadedScene)
			{
				m_ModelScene->LoadFromPatch(QUICK_SCENE_PATH);
				ui->FindAppByType<integration::StarBar>()->SetCameraEnabled(true);
				m_CameraSet = false;
			}

			// Reset camera once drawable loads
			if (!m_CameraSet && m_ModelScene->IsLoaded())
			{
				m_ModelScene->ResetCameraPosition();
				m_CameraSet = true;
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
