#pragma once

#include "am/ui/app.h"
#include "am/ui/extensions.h"
#include "am/ui/imglue.h"
#include "am/ui/font_icons/icons_am.h"
#include "helpers/utf8.h"

#include "lunasvg.h"
#include "am/asset/factory.h"
#include "am/asset/ui/assetwindowfactory.h"

namespace rageam::ui
{
	// For testing how different text scripts are displayed in UI
	class UITestbed : public App
	{
		ImImage* m_DesktopIcon = nullptr;
		ImImage m_SvgImage;
		int m_SvgSize = 16;

		void OnStart() override
		{
			m_DesktopIcon = GetUI()->GetIconByHash(Hash("desktop"));
			m_SvgImage.Load(L"C:/Users/falco/Desktop/Save.svg");
		}

		void OnRender() override
		{
			ImGlue* ui = GetUI();

			ImGui::Begin("UI Testbed");

			static bool s_ShowDemoWindow = false;
			ImGui::Checkbox("Show Demo Window", &s_ShowDemoWindow);
			if (s_ShowDemoWindow)
				ImGui::ShowDemoWindow();

			if (ImGui::Button("ITD"))
			{
				using namespace asset;

				AssetPtr txd = AssetFactory::LoadFromPath(
					//L"C:/Users/falco/Desktop/vehshare.itd");
					//L"C:/Users/falco/Desktop/texWorks.ws/tiny.itd");
					L"X:/am.ws/am.itd");
				// txd = AssetFactory::LoadFromPath(L"C:/Users/falco/Desktop/light_txd.itd");
				AssetWindowFactory::OpenNewOrFocusExisting(txd);
			}

			if (ImGui::Button("Quick ITD"))
			{
				using namespace asset;

				//AssetPtr txd = AssetFactory::LoadFromPath(L"C:/Users/falco/Desktop/texWorks.ws/tiny.itd");
				///AssetPtr txd = AssetFactory::LoadFromPath(L"C:/Users/falco/Desktop/light_txd.itd");
				AssetPtr txd = AssetFactory::LoadFromPath(L"X:/am.ws/am.itd");
				txd->CompileCallback = [](ConstWString msg, double p)
					{
						AM_TRACEF(L"[%.02f]: %s", p, msg);
					};
				txd->CompileToFile();
			}

			ImGui::SliderInt("Font Size", &ui->FontSize, 8, 24);
			m_DesktopIcon->Render(ui->FontSize);

			ImGui::Button(ICON_AM_SAVE" Save");

			// ImGui::SliderInt("Svg", &m_SvgSize, 8, 256);
			// m_SvgImage.Render(m_SvgSize, m_SvgSize);

			if (ImGui::CollapsingHeader("Extra Languages"))
			{
				ImGui::CheckboxFlags("Cyrillic", &ui->ExtraFontRanges, ImExtraFontRanges_Cyrillic);
				ImGui::CheckboxFlags("Chinese", &ui->ExtraFontRanges, ImExtraFontRanges_Chinese);
				ImGui::CheckboxFlags("Japanese", &ui->ExtraFontRanges, ImExtraFontRanges_Japanese);
				ImGui::CheckboxFlags("Greek", &ui->ExtraFontRanges, ImExtraFontRanges_Greek);
				ImGui::CheckboxFlags("Thai", &ui->ExtraFontRanges, ImExtraFontRanges_Thai);
				ImGui::CheckboxFlags("Korean", &ui->ExtraFontRanges, ImExtraFontRanges_Korean);
				ImGui::CheckboxFlags("Vietnamese", &ui->ExtraFontRanges, ImExtraFontRanges_Vietnamese);
			}

			if (ImGui::CollapsingHeader("Bank Font"))
			{
				ImGui::Text("[5]");
				ImGui::Text("Spd: 14.36");
				ImGui::Text("Dist: 37.7");
				ImGui::Text("<Vehicle>");
			}

			if (ImGui::CollapsingHeader("Encodings"))
			{
				// China
				ImGui::DebugTextEncoding(U8("一个好的建筑师在拿起第一块砖之前，已经在自己的脑海中完成了房屋的建造"));
				// Japan
				ImGui::DebugTextEncoding(U8("すべてのベストは日本で作られています"));
				// Cyrillic
				ImGui::DebugTextEncoding(U8("Он сделал свой выбор еще 10 минут назад"));
				// Latin
				ImGui::DebugTextEncoding(U8("Eighty eight miles per hour!"));
			}

			ImGui::End();
		}
	};
}
