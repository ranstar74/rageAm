#pragma once

#include "am/system/enum.h"
#include "am/ui/app.h"
#include "am/ui/window.h"
#include "am/ui/font_icons/icons_awesome.h"
#include "am/ui/styled/slwidgets.h"
#include "am/xml/doc.h"
#include "helpers/utf8.h"

namespace rageam::ui
{
	class StyleEditor : public Window
	{
		static constexpr auto sm_ImColors = magic_enum::enum_names<ImGuiCol_>();
		static constexpr auto sm_SlColors = magic_enum::enum_names<SlGuiCol_>();

	public:

		void OnRender() override
		{
			ImGuiStyle& imStyle = ImGui::GetStyle();
			SlGuiStyle& slStyle = SlGui::GetStyle();

			if (ImGui::BeginMenuBar())
			{
				if (ImGui::MenuItem(ICON_FA_FILE_EXPORT"Save"))
				{
					// xml::Document::CreateNew("Style");
				}

				ImGui::EndMenuBar();
			}

			if (ImGui::BeginTabBar("Colors"))
			{
				// Note that in both cases we skip the last item _COUNT (with size() - 1)

				if (ImGui::BeginTabItem("All"))
				{
					ImGui::ShowStyleEditor();
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("ImGui"))
				{
					for (size_t i = 0; i < sm_ImColors.size() - 1; i++)
					{
						float* colors = reinterpret_cast<float*>(&imStyle.Colors[i]);
						ImGui::ColorEdit4(sm_ImColors[i].data(), colors);
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("SlGui"))
				{
					for (size_t i = 0; i < sm_SlColors.size() - 1; i++)
					{
						SlGui::ColorEditGradient(sm_SlColors[i].data(), slStyle.Colors[i]);
					}
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}

		ConstString GetTitle() const override { return "Style Editor"; }
	};
}
