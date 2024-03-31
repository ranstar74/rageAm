#pragma once

#include "am/ui/app.h"

namespace rageam::ui
{
	class TestbedApp : public App
	{
		void OnStart() override;
		void OnRender() override;
	};
}
