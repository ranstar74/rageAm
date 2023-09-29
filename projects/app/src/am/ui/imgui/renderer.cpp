#include "renderer.h"

#include "am/desktop/window.h"
#include "am/graphics/render/device.h"
#include "am/graphics/render/engine.h"
#include "am/ui/imgui/imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "helpers/com.h"

void rageam::ui::Renderer::CreateRT()
{
	DestroyRT();

	render::Engine* engine = render::Engine::GetInstance();
	ID3D11Device* factory = engine->GetFactory();
	IDXGISwapChain* swapchain = engine->GetSwapchain();

	// Create RT for back buffer
	ID3D11Texture2D* backbuf;
	swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuf));
	factory->CreateRenderTargetView(backbuf, NULL, &m_RenderTarget);
	backbuf->Release();
}

void rageam::ui::Renderer::DestroyRT()
{
	SAFE_RELEASE(m_RenderTarget);
}

rageam::ui::Renderer::Renderer()
{
	Window* window = WindowFactory::GetWindow();
	window->OnResizeBegin.Listen(this, [this]
	{
		DestroyRT();
	});
	window->OnResizeEnd.Listen(this, [this](WindowSize)
	{
		CreateRT();
	});

	CreateRT();
}

void rageam::ui::Renderer::DestroyContext() const
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
}

void rageam::ui::Renderer::InitImGuiBackEnds() const
{
	render::Engine* engine = render::Engine::GetInstance();
	ID3D11Device* factory = engine->GetFactory();
	ID3D11DeviceContext* context = engine->GetDeviceContext();

	ImGui_ImplWin32_Init(WindowFactory::GetWindowHandle());
	ImGui_ImplDX11_Init(factory, context);
}

void rageam::ui::Renderer::BeginFrame() const
{
	render::Engine* engine = render::Engine::GetInstance();
	ID3D11DeviceContext* context = engine->GetDeviceContext();

	context->OMSetRenderTargets(1, &m_RenderTarget, NULL);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void rageam::ui::Renderer::EndFrame() const
{
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
