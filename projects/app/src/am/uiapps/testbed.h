#pragma once

#include "am/graphics/image/imagecache.h"
#include "am/ui/app.h"
#include "helpers/format.h"

namespace rageam::ui
{
	class TestbedApp : public App
	{
		void OnRender() override
		{
			ImGui::Begin("rageAm Testbed");

			graphics::ImageCache* imageCache = graphics::ImageCache::GetInstance();
			graphics::ImageCacheState ics = imageCache->GetState();
			ImGui::Text("ImageCache");
			ImGui::BulletText("RAM: %s/%s", FormatSize(ics.SizeRamUsed), FormatSize(ics.SizeRamBudget));
			ImGui::BulletText("FS: %s/%s", FormatSize(ics.SizeFsUsed), FormatSize(ics.SizeFsBudget));
			ImGui::BulletText("Image counts:");
			ImGui::Indent();
			ImGui::Text("- RAM: %u", ics.ImageCountRam);
			ImGui::Text("- FS: %u", ics.ImageCountFs);
			ImGui::Text("- DX11 Views: %u", ics.DX11ViewCount);
			ImGui::Unindent();

			ImGui::End();
		}
	};
}
