#include "apps.h"

#include "am/system/exception/handler.h"
#include "imgui_internal.h"
#include "apps/statusbar.h"
#include "apps/testbed.h"
#include "apps/windowmgr.h"
#include "windows/memstat.h"
#include "windows/styleeditor.h"
#include "font_icons/icons_am.h"
#include "common/logger.h"
#include "extensions.h"
#include "windows/explorer/explorer.h"

#ifndef AM_STANDALONE
#include "am/desktop/window_integrated.h"
#include "apps/integration/modelscene.h"
#endif

void rageam::ui::Apps::RegisterSystemApps()
{
	std::unique_lock lock(m_Mutex);

	AddApp(new TestbedApp());
	AddApp(new StatusBar());
	AddApp(new WindowManager());
#ifndef AM_STANDALONE
	AddApp(new ModelSceneApp());
#endif
}

bool rageam::ui::Apps::UpdateAll()
{
	std::unique_lock lock(m_Mutex);

	// We display UI disabled if game viewport is focused (active) in integration mode
#ifndef AM_STANDALONE
	static bool gameDisabled = false;
	if (ImGui::IsKeyPressed(ImGuiKey_F10))
	{
		gameDisabled = !gameDisabled;
	}

	if (gameDisabled)
	{
		static gmAddress disabledAllControlsCommand = gmAddress::Scan("40 53 48 83 EC 20 33 DB 85 C9 75 09");
		disabledAllControlsCommand.To<void(*)(int)>()(0);
	}

	// TODO: Bring back cursor clip hook
	GImGui->IO.MouseDrawCursor = gameDisabled;

	bool disabled = false;// sm_GameApp->IsFocused();
#else
	bool disabled = false;
#endif

	if (disabled) ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
#ifdef AM_STANDALONE

	static ConstString ABOUT_POPUP_ID = "About##ABOUT_POPUP_ID";
	bool openAbout = false;

	// Dock space in integration mode will cover all game viewport, we don't want that
	ImGui::BeginDockSpace(); // TODO: In integration mode this creates debug window...

	if (ImGui::BeginMenuBar())
	{
		static WindowManager* windowMgr = FindAppByType<WindowManager>();

		ImGui::MenuItem("File");

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem(ICON_AM_COLOR_PALETTE "  Style Editor"))
				windowMgr->EnsureOpened<StyleEditor>();

			if (ImGui::MenuItem(ICON_AM_PERFORMANCE_REPORT"  System Performance"))
				windowMgr->EnsureOpened<SystemMonitor>();

			if (ImGui::MenuItem(ICON_AM_FOLDER_BOTTOM_PANEL"  Explorer"))
				windowMgr->EnsureOpened<Explorer>();

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem(ICON_AM_ABOUT_BOX"  About"))
			{
				openAbout = true;
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	if (openAbout)
		ImGui::OpenPopup(ABOUT_POPUP_ID);

	if (ImGui::BeginPopupModal(ABOUT_POPUP_ID, 0, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static Image* amIcon = Gui->Icons.GetIcon("am", IconSize_256);
		static int currentYear = DateTime::Now().Year();

		SlGui::PushFont(SlFont_Medium);

		amIcon->Render(64);
		ImGui::SameLine();
		ImGui::Text("The 'rageAm' RAGE Research Project");
		ImGui::SnapToPrevious();
		ImGui::Text(U8("\u00a9")); // Trademark
		ImGui::SameLine(0, 0);
		ImGui::Text(" %i ranstar74.", currentYear);

		ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * 1.75f));

		if (ImGui::Button("Continue"))
			ImGui::CloseCurrentPopup();

		ImGui::PopFont();

		ImGui::EndPopup();
	}
#endif

	// Update apps
	{
		for (amUniquePtr<App>& app : m_Apps)
		{
			m_LastApp = app.get();

			if (ExceptionHandler::ExecuteSafe(UpdateModule, app.get()))
				continue;

			AM_ERRF("Apps::UpdateAll() -> An error occured during updating %s", app->GetDebugName());
			return false;
		}
	}

#ifdef AM_STANDALONE
	ImGui::End(); // DockSpace
#endif

	if (disabled) ImGui::PopItemFlag();

	return true;
}

void rageam::ui::Apps::AddApp(App* app)
{
	std::unique_lock lock(m_Mutex);

	m_Apps.Construct(app);
}
