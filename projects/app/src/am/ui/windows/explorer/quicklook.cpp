#include "quicklook.h"

#include "am/graphics/texture/imageformat.h"
#include "am/ui/styled/slgui.h"
#include "helpers/format.h"
#include "rage/math/math.h"

rageam::ui::QuickLookImage::QuickLookImage(const ExplorerEntryPtr& entry) : QuickLookType(entry)
{
	file::WPath path = file::PathConverter::Utf8ToWide(m_Entry->GetPath());

	image.LoadAsync(path, 0, false /* We want best quality preview */);
}

void rageam::ui::QuickLookImage::Render()
{
	SlGui::PushFont(SlFont_Medium);

	ImGui::AlignTextToFramePadding();

	ImGui::Text("%s ", m_Entry->GetFullName());
	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine();
	if (image.IsLoading())
	{
		ImGui::Text(" ... ");
	}
	else if (image.FailedToLoad())
	{
		ImGui::Text(" Failed to open image");
	}
	else
	{
		char sizeBuffer[16];
		FormatBytes(sizeBuffer, 16, m_Entry->GetSize());
		ImGui::Text(" %ux%u, %s", image.GetWidth(), image.GetHeight(), sizeBuffer);
	}

	ImGui::PopFont();

	image.Render(512);
}

void rageam::ui::QuickLook::Open(const ExplorerEntryPtr& entry)
{
	// TODO: Better way to register file types
	ConstString ext = entry->GetType();

	if (texture::IsImageFormat(ext))
		m_LookType = std::make_unique<QuickLookImage>(entry);
	else return;

	m_Opening = true;
	ImGui::OpenPopup(POPUP_NAME);
}

void rageam::ui::QuickLook::Close()
{
	m_Opening = false;
	m_Measured = false;
	m_LookType.reset();
}

bool rageam::ui::QuickLook::IsOpened() const
{
	return ImGui::IsPopupOpen(POPUP_NAME);
}

void rageam::ui::QuickLook::Render()
{
	if (!m_LookType)
		return;

	// Wait until displayed data is fully loaded
	if (!m_LookType->IsLoaded())
		return;

	// See exact description of animation in header file

	if (m_Opening && m_Measured)
	{
		float by = GImGui->IO.DeltaTime * OPEN_ANIM_SPEED;
		m_AnimatedSize = ImLerp(m_AnimatedSize, m_WindowSize, by);

		// Now animated size is close enough to actual, we can finish animation
		if (rage::Math::AlmostEquals(m_AnimatedSize.x, m_WindowSize.x))
		{
			m_AnimatedSize = {};

			// We have to reset forced window size to allow popup to resize itself
			ImGui::SetWindowSize(ImVec2(0, 0), ImGuiCond_Always);
			m_Opening = false;
		}

		ImGui::SetNextWindowSize(m_AnimatedSize, ImGuiCond_Always);
	}

	// Measure pass is done with fully transparent window
	if (!m_Measured)
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, FLT_MIN);

	ImGuiWindowFlags popupFlags = 0;
	popupFlags |= ImGuiWindowFlags_NoScrollbar;
	popupFlags |= ImGuiWindowFlags_AlwaysAutoResize;

	ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 2);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	bool popupOpened = ImGui::BeginPopup(POPUP_NAME, popupFlags);
	ImGui::PopStyleVar(2);

	if (!popupOpened)
	{
		Close();
		return;
	}

	m_LookType->Render();

	if (!m_Measured)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();

		// We have to wait second frame to get exact size measurements
		if (!window->Appearing)
		{
			m_WindowSize = window->Size;
			m_Measured = true;
		}

		ImGui::PopStyleVar();
	}

	ImGui::EndPopup();
}
