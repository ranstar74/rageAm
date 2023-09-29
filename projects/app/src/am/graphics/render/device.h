//
// File: device.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <d3d11.h>

namespace rageam::render
{
	/**
	 * \brief Holds DirectX device, context and swap chain.
	 * \n Depending on build type either creates new DirectX device or hooks up to game one.
	 */
	class Device
	{
		ID3D11Device* m_Factory = nullptr;
		ID3D11DeviceContext* m_Context = nullptr;
		IDXGISwapChain* m_Swapchain = nullptr;

	public:
		void Create(bool useWindow);
		void Destroy();

		ID3D11Device* GetFactory() const { return m_Factory; }
		ID3D11DeviceContext* GetContext() const { return m_Context; }
		IDXGISwapChain* GetSwapchain() const { return m_Swapchain; }
	};
}
