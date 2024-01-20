//
// File: image.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "imgui.h"
#include "am/system/worker.h"
#include "am/graphics/image/image.h"

namespace rageam::ui
{
	/**
	 * \brief UI image with async loading and dynamic ICO + SVG support
	 * \n For .ICO files mipmap is automatically chosen based on rendering size.
	 * \n For .SVG files image is re-rasterized every time it's rendered with at new size.
	 */
	class ImImage
	{
		// We have multiple layers only for dynamic .ICO picking
		struct ImageLayer
		{
			ImVec2                             ImageUV2 = {1.0f, 1.0f};
			amComPtr<ID3D11ShaderResourceView> ImageView;
			graphics::ImageInfo                ImageInfo;
		};

		// Pending are used for synchronization purposes, we must not destroy
		// the texture in between of frames because image is referenced in draw list until next frame

		bool				  m_HasPending = false;
		List<ImageLayer>      m_LayersPending;
		ImageLayer            m_LargestLayer = {};
		List<ImageLayer>      m_Layers;
		amPtr<BackgroundTask> m_LoadTask;
		mutable std::mutex    m_Mutex;
		int                   m_LastRenderWidth = 0;
		int                   m_LastRenderHeight = 0;
		file::WPath           m_LoadedPath;
		bool                  m_IsSvg = false;
		bool                  m_IsLoading = false;
		bool                  m_FailedToLoad = false;

		graphics::ImageDX11ResourceOptions GetResourceOptions(int maxResolution) const;
		ImageLayer& FindBestMatchLayer(List<ImageLayer>& layers, int width, int height) const;
		bool        LoadInternal(const file::WPath& path, int maxResolution = 0);

	public:
		ImImage() = default;
		ImImage(const ImImage&) = delete;
		~ImImage();

		// For those functions we return largest available layer (in case of .ICO), no difference for other image files
		int         GetWidth()  const { return m_LargestLayer.ImageInfo.Width; }
		int         GetHeight() const { return m_LargestLayer.ImageInfo.Height; }
		auto        GetView()   const { return m_LargestLayer.ImageView; }
		ImVec2      GetUV2()    const { return m_LargestLayer.ImageUV2; }
		ImTextureID GetID()     const;

		void Load(const file::WPath& path, int maxResolution = 0);
		void Set(const graphics::ImagePtr& image, int maxResolution = 0);

		bool IsLoading() const { return m_IsLoading; }
		bool FailedToLoad() const { return m_FailedToLoad; }

		void Render(float width, float height);
		void Render(int maxSize = 0);
	};
}
