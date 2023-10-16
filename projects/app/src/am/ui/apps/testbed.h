#pragma once

#include "am/ui/app.h"
#include "am/ui/extensions.h"

namespace rageam::ui
{
	class TestbedApp : public App
	{
		void OnRender() override
		{
			return;
			ImGui::Begin("rageAm Testbed");
			ImGui::End();
		}
	};
}
