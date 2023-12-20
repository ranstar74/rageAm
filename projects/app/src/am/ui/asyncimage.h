//
// File: image.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "imgui.h"
#include "am/task/worker.h"
#include "am/graphics/image/image.h"

namespace rageam::ui
{
	/**
	 * \brief Image with async loading + rendering functions for ImGui.
	 */
	class AsyncImage
	{
		ImVec2								m_ImageUV2 = { 1.0f, 1.0f };
		amComPtr<ID3D11ShaderResourceView>	m_ImageView;
		graphics::ImageInfo					m_ImageInfo;
		amPtr<BackgroundTask>				m_LoadTask;
		std::mutex							m_LoadMutex;

	public:
		AsyncImage() = default;
		AsyncImage(const AsyncImage&) = delete;
		~AsyncImage();

		int GetWidth() const { return m_ImageInfo.Width; }
		int GetHeight() const { return m_ImageInfo.Height; }
		auto GetView() const { return m_ImageView.Get(); }

		bool Load(const file::WPath& path, int maxResolution = 0);
		void LoadAsync(const file::WPath& path, int maxResolution = 0);

		bool IsLoading() const { return m_LoadTask && !m_LoadTask->IsFinished(); }
		bool IsLoaded() const { return m_LoadTask->IsFinished(); }
		bool FailedToLoad() const { return m_LoadTask && !m_LoadTask->IsSuccess(); }

		void Render(float width, float height) const;
		void Render(int maxSize = 0) const;

		ImVec2 GetUV2() const { return m_ImageUV2; }

		ImTextureID GetID() const;
	};
}
