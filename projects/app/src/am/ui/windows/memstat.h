#pragma once

#include "implot.h"
#include "am/ui/extensions.h"
#include "am/ui/window.h"
#include "am/ui/font_icons/icons_am.h"
#include "am/ui/styled/slgui.h"
#include "helpers/format.h"

namespace rageam::ui
{
	class SystemMonitor : public Window
	{
		static constexpr int ALLOCATOR_COUNT = 3;
		static constexpr double ALLOC_MAX_SECONDS = 5; // Time to the oldest time point in allocations plot, in seconds
		static constexpr int ALLOC_BUFFER_COUNT = (int)ALLOC_MAX_SECONDS * 60 + 120; // Assuming 60 fps + padding

		static constexpr ConstString sm_AllocatorNames[] = { "General", "Physical", "Virtual" };
		static constexpr int sm_AllocatorColors[] = { 0, 4, 8 };

		ImPlot::ScrollingBuffer m_AllocDatas[ALLOCATOR_COUNT] =
		{
			{ ALLOC_BUFFER_COUNT }, { ALLOC_BUFFER_COUNT }, { ALLOC_BUFFER_COUNT }
		};

		static int FormatAllocatorTime(double value, char* buffer, int bufferSize, void* userData)
		{
			double timeNow = ImGui::GetTime();
			double timeSince = timeNow - value;
			if (timeSince < 1.0)
				return sprintf_s(buffer, bufferSize, "Now");
			return sprintf_s(buffer, bufferSize, "%.0f second(s) ago", timeSince);
		}

		void PlotAllocator(rage::eAllocatorType type)
		{
			ImPlot::ScrollingBuffer& buffer = m_AllocDatas[type];
			rage::sysMemAllocator* allocator = GetMultiAllocator()->GetAllocator(type);
			ConstString name = sm_AllocatorNames[type];

			double total = static_cast<double>(allocator->GetHeapSize());
			double used = static_cast<double>(allocator->GetMemoryUsed());

			char totalText[16];
			char usedText[16];
			FormatBytes(totalText, 16, total);
			FormatBytes(usedText, 16, used);

			// Convert from bytes to mb
			total /= 1024 * 1024;
			used /= 1024 * 1024;

			double timeNow = ImGui::GetTime();
			buffer.AddPoint(timeNow, used);

			if (ImPlot::BeginPlot(ImGui::FormatTemp("%s\t%s / %s", name, usedText, totalText), ImVec2(-1, 170)))
			{
				ImVec4 col = ImPlot::GetColormapColor(sm_AllocatorColors[type]);

				ImPlotAxisFlags xFlags = ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoHighlight;
				ImPlotAxisFlags yFlags = ImPlotAxisFlags_NoHighlight;

				ImPlot::SetupAxes("##Seconds", "##Used", xFlags, yFlags);
				ImPlot::SetupAxisLimits(ImAxis_X1, timeNow - ALLOC_MAX_SECONDS, timeNow, ImGuiCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0, total);
				ImPlot::SetupAxisFormat(ImAxis_Y1, "%.2f");
				ImPlot::SetupAxisFormat(ImAxis_X1, FormatAllocatorTime);

				ImPlotLineFlags lineFlags = ImPlotLineFlags_Shaded;

				ImPlot::SetNextLineStyle(col);
				ImPlot::SetNextFillStyle(col, 0.25);
				ImPlot::PlotLine("##Used", &buffer.Data[0].x, &buffer.Data[0].y, buffer.Data.size(), lineFlags, buffer.Offset, 2 * sizeof(float));

				ImPlot::EndPlot();
			}
		}

	public:
		void OnRender() override
		{
			if (ImGui::BeginTabBar("MemStatTabBar"))
			{
				if (ImGui::BeginTabItem(ICON_AM_REPORT"  Brief"))
				{
					ImPlot::PushColormap(ImPlotColormap_Cool);
					PlotAllocator(rage::ALLOC_TYPE_GENERAL);
					PlotAllocator(rage::ALLOC_TYPE_VIRTUAL);
					PlotAllocator(rage::ALLOC_TYPE_PHYSICAL);
					ImPlot::PopColormap();

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(ICON_AM_HISTOGRAM_VISUALIZER"  Memory Visualizer"))
				{
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(ICON_AM_SHAPE"  Pool Manager"))
				{
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}

		ConstString GetTitle() const override { return "System Monitor"; }
		ImVec2 GetDefaultSize() override { return { 425, 570 }; }
	};
}
