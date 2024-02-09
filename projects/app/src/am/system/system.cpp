#include "system.h"

#include "datamgr.h"
#include "am/asset/factory.h"
#include "am/asset/types/hotdrawable.h"
#include "am/asset/ui/assetwindowfactory.h"
#include "am/xml/doc.h"
#include "rage/grcore/fvf.h"
#include "exception/handler.h"

#include <easy/profiler.h>

#ifdef AM_INTEGRATED
#include "am/integration/memory/hook.h"
#endif

void rageam::System::LoadDataFromXML()
{
	file::WPath configPath = DataManager::GetAppData() / DATA_FILE_NAME;
	if (IsFileExists(configPath))
	{
		try
		{
			XmlDoc xDoc;
			xDoc.LoadFromFile(configPath);
			XmlHandle xRoot = xDoc.Root();

			XmlHandle xWindow = xRoot.GetChild("Window");
			xWindow.GetChild("Width").GetValue(Data.Window.Width);
			xWindow.GetChild("Height").GetValue(Data.Window.Height);
			xWindow.GetChild("X").GetValue(Data.Window.X);
			xWindow.GetChild("Y").GetValue(Data.Window.Y);
			xWindow.GetChild("Maximized").GetValue(Data.Window.Maximized);

			XmlHandle xUi = xRoot.GetChild("UI");
			xUi.GetChild("FontSize").GetValue(Data.UI.FontSize);
		}
		catch (const XmlException& ex)
		{
			ex.Print();
		}
		HasData = true;
	}
	else
	{
		HasData = false;
	}
}

void rageam::System::SaveDataToXML() const
{
	file::WPath configPath = DataManager::GetAppData() / DATA_FILE_NAME;
	try
	{
		XmlDoc xDoc("SystemData");
		XmlHandle xRoot = xDoc.Root();

		XmlHandle xWindow = xRoot.AddChild("Window");
		xWindow.AddChild("Width").SetValue(Data.Window.Width);
		xWindow.AddChild("Height").SetValue(Data.Window.Height);
		xWindow.AddChild("X").SetValue(Data.Window.X);
		xWindow.AddChild("Y").SetValue(Data.Window.Y);
		xWindow.AddChild("Maximized").SetValue(Data.Window.Maximized);

		XmlHandle xUi = xRoot.AddChild("UI");
		xUi.AddChild("FontSize").SetValue(Data.UI.FontSize);

		xDoc.SaveToFile(configPath);
	}
	catch (const XmlException& ex)
	{
		ex.Print();
	}
}

void rageam::System::Destroy()
{
	if (!m_Initialized)
		return;

	// Order is opposite to initialization
	rage::grcVertexDeclaration::CleanUpCache();

	// Cache GPU devices to report live objects after destruction
	auto dxDevice = amComPtr(graphics::RenderGetDevice());
	auto dxContext = amComPtr(graphics::RenderGetContext());

	// Even though render is created before UI (in order to allocate GPU objects),
	// we must destroy it now because it is currently drawing UI
	m_Render = nullptr;
	m_ImGlue = nullptr;
	m_PlatformWindow = nullptr;
	m_ImageCache = nullptr;

	// Report all live DX objects, might be not the best place to do this but this must be done
	// after rendering and ui systems are destroyed, to ensure that we'll get only actually leaked objects
#if AM_STANDALONE && DEBUG
	// Force cleanup to destroy resources with zero ref count
	if (dxContext) dxContext->Flush();
	if (dxDevice)
	{
		ID3D11Debug* debug;
		AM_ASSERT_STATUS(dxDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debug)));
		AM_ASSERT_STATUS(debug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL));
		debug->Release();
	}
#endif

	// Integration is called by ImGlue, must be destroyed after
	// Ideally we can destroy them in the right order if we add rendering function
	AM_INTEGRATED_ONLY(m_Integration = nullptr);

	asset::HotDrawable::ShutdownClass();
	asset::TxdAsset::ShutdownClass();
	asset::AssetFactory::Shutdown();
	ui::AssetWindowFactory::Shutdown();
	graphics::ImageCompressor::ShutdownClass();
	m_MainWorker = nullptr;
	ExceptionHandler::Shutdown();

	SaveDataToXML();

	AM_INTEGRATED_ONLY(Hook::Shutdown());
	AM_INTEGRATED_ONLY(m_AddressCache = nullptr);

	profiler::dumpBlocksToFile("session.prof");

	rage::SystemHeap::Shutdown();

	m_Initialized = false;
}

void rageam::System::Init(bool withUI)
{
	Timer timer = Timer::StartNew();

	EASY_PROFILER_ENABLE;
	AM_STANDALONE_ONLY(EASY_THREAD("Main Thread"));

	// Core
	m_MainWorker = std::make_unique<BackgroundWorker>("System", 8);
	BackgroundWorker::SetMainInstance(m_MainWorker.get());
	AM_INTEGRATED_ONLY(Hook::Init());
	AM_INTEGRATED_ONLY(m_AddressCache = std::make_unique<gmAddressCache>());
	m_TexturePresetManager = std::make_unique<asset::TexturePresetStore>();
	LoadDataFromXML();
	ExceptionHandler::Init();
	asset::AssetFactory::Init();
	graphics::ImageCompressor::InitClass();
	m_ImageCache = std::make_unique<graphics::ImageCache>();

	// Not a render thread in integrated mode, because called from Init launcher function
	AM_STANDALONE_ONLY((void)SetThreadDescription(GetCurrentThread(), L"[RAGEAM] Main Thread"));
	// Window (UI) + Render
	if (withUI)
		m_PlatformWindow = std::make_unique<graphics::Window>();
	m_Render = std::make_unique<graphics::Render>();
	if (withUI)
		m_ImGlue = std::make_unique<ui::ImGlue>();

	// Integration must be initialized after rendering/ui, it depends on it
	AM_INTEGRATED_ONLY(m_Integration = std::make_unique<integration::GameIntegration>());

	m_Initialized = true;

	timer.Stop();
	AM_TRACEF("[RAGEAM] Startup time: %llu ms", timer.GetElapsedMilliseconds());

	// Now both window and render are created/initialized, we can enter update loop
	if (withUI)
	{
		m_Render->EnterRenderLoop();
	}
}

void rageam::System::Update() const
{
	m_ImageCache->DeleteOldEntries();
}
