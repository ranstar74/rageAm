#pragma once

#include "common/types.h"
#include "imgui.h"
#include "am/system/enum.h"
#include "am/ui/extensions.h"

namespace rageam::integration::widgets // rage-specific extensions for ImGui
{
	// Allow unknown creates checkbox to show all enum flags (using reflection)
	// Returns true if user didn't choose to show all flags
	template<typename TFlagsEnum>
	bool EnumFlags(ConstString idStr, ConstString prefix, u32* flags, bool allowUnknown = true)
	{
		ImGuiID id = ImGui::GetID(idStr);
		ImGui::PushID(id);

		ImGuiStorage* stateStorage = ImGui::GetStateStorage();

		bool showAllFlags = stateStorage->GetBool(id, false);
		if (allowUnknown)
		{
			if (ImGui::Checkbox("Show All", &showAllFlags))
				stateStorage->SetBool(id, showAllFlags);
			ImGui::SameLine();
			ImGui::HelpMarker("Show all flags including unknown, for research purposes.");
			ImGui::Separator();
		}

		if (showAllFlags)
		{
			static constexpr int numBits = sizeof(TFlagsEnum) * 8;
			for (int i = 0; i < numBits; i++)
			{
				TFlagsEnum flag = TFlagsEnum(1 << i);
				ConstString name = Enum::GetName(flag);
				if (!name)
					name = ImGui::FormatTemp("%s_UNKNOWN_%i", prefix, i);
				ImGui::CheckboxFlags(name, flags, flag);
			}
		}

		ImGui::PopID();

		return showAllFlags == false;
	}
}
