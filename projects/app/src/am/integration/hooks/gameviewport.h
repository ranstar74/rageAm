#pragma once

#include "common/types.h"
#include "d3d11.h"

namespace hooks
{
	/**
	 * \brief
	 */
	class GameViewport
	{
		static inline ID3D11Texture2D* sm_GameTexture = nullptr;
		static inline ID3D11ShaderResourceView* sm_GameShaderView = nullptr;

		static inline bool sm_Enabled = false;

		// Creates texture that game will use instead of SwapChain back buffer and shader view for it
		static void CreateGameBuffer();

		// Releases game texture and it's shader view
		static void DestroyGameBuffer();

		// Resizes game buffer and feeds it to game instead of back buffer,
		// has to be called once viewport resolution changed or viewport was enabled
		static void DoUpdate();
	public:
		static void Init();

		// Those has to be called only from render thread!

		static void Enable();
		static void Disable();
		static void SetResolution(u32 width, u32 height);

		static ID3D11ShaderResourceView* GetGameShaderView() { return sm_GameShaderView; }
		static ID3D11Texture2D* GetGameTexture() { return sm_GameTexture; }
	};
}
