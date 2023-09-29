//
// File: image.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "imgui.h"
#include "am/file/path.h"
#include "am/graphics/texture/compressor.h"
#include "am/graphics/texture/surface.h"
#include "am/system/ptr.h"
#include "am/task/worker.h"
#include "common/types.h"
#include "rage/math/math.h"

namespace rageam::ui
{
	/**
	 * \brief Simple image that can be loaded from file and rendered, used mostly for icons.
	 */
	class Image
	{
		amPtr<BackgroundTask>				m_LoadTask;
		std::mutex							m_LoadMutex;
		texture::Surface					m_Surface;
		amComPtr<ID3D11ShaderResourceView>	m_View;
	public:
		Image() = default;
		Image(const Image&) = delete;
		~Image()
		{
			if (m_LoadTask)
				m_LoadTask->Wait();
		}

		u32 GetWidth() const { return IsLoading() ? 0 : m_Surface.GetInfo().Width; }
		u32 GetHeight() const { return IsLoading() ? 0 : m_Surface.GetInfo().Height; }

		const texture::Surface& GetSurface() const { return m_Surface; }

		void SetView(ID3D11ShaderResourceView* view)
		{
			std::unique_lock lock(m_LoadMutex);
			m_View.Reset();
			m_View = view;
		}

		ID3D11ShaderResourceView* GetView() const { return m_View.Get(); }

		bool Load(const file::WPath& path, const texture::CompressionOptions& options, texture::CompressedInfo& outInfo)
		{
			ID3D11ShaderResourceView* view;
			m_Surface.SetCompressOptions(options);
			if (!m_Surface.LoadToGpu(path, &view))
				return false;

			outInfo = m_Surface.GetInfo();

			std::unique_lock lock(m_LoadMutex);
			m_View.Reset();
			m_View = view;

			return true;
		}

		bool Load(const file::WPath& path, u32 maxResolution = 0, bool compress = true)
		{
			texture::CompressedInfo info{};
			texture::CompressionOptions options{};
			options.MipMaps = true;
			options.Quality = texture::COMPRESSOR_QUALITY_NORMAL;
			options.MaxResolution = maxResolution;
			if (!compress)
				options.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

			return Load(path, options, info);
		}

		void LoadAsync(const file::WPath& path, u32 maxResolution = 0, bool compress = true)
		{
			m_LoadTask = BackgroundWorker::Run([=, this]
				{
					return Load(path, maxResolution, compress);
				}, L"Load UI Image %ls", path.GetCStr());
		}

		void LoadAsync(const file::WPath& path, const texture::CompressionOptions& options, texture::CompressedInfo& outInfo)
		{
			m_LoadTask = BackgroundWorker::Run([path, options, &outInfo, this]
				{
					return Load(path, options, outInfo);
				}, L"Load UI Image %ls", path.GetCStr());
		}

		bool IsLoading() const { return m_LoadTask && !m_LoadTask->IsFinished(); }
		bool IsLoaded() const { return m_LoadTask->IsFinished(); }
		bool FailedToLoad() const { return m_LoadTask && !m_LoadTask->IsSuccess(); }

		void Render(float width, float height) const
		{
			// To prevent layout issues while it's loading, maybe replace it on some loading indicator?
			if (IsLoading() || !m_View)
			{
				ImGui::Dummy(ImVec2(width, height));
				return;
			}

			ImGui::Image(ImTextureID(m_View.Get()), ImVec2(width, height));
		}

		void Render(u32 maxSize = 0) const
		{
			if (IsLoading())
				return;

			float maxSizef = static_cast<float>(maxSize);
			if (!m_View)
			{
				// Image is not loaded yet but we have to draw placeholder...
				Render(maxSizef, maxSizef);
				return;
			}

			const texture::CompressedInfo& info = m_Surface.GetInfo();

			float width = static_cast<float>(info.Width);
			float height = static_cast<float>(info.Height);
			if (maxSize != 0)
			{
				float scaleFactor = maxSizef / rage::Math::Max(width, height);

				width *= scaleFactor;
				height *= scaleFactor;
			}

			Render(width, height);
		}

		ImTextureID GetID() const
		{
			if (IsLoading())
				return ImTextureID(nullptr);

			return ImTextureID(m_View.Get());
		}
	};
}
