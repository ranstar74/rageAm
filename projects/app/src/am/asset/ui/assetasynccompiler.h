//
// File: assetasynccompiler.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/ui/app.h"
#include "am/types.h"
#include "am/system/worker.h"
#include "am/asset/gameasset.h"
#include "am/system/singleton.h"

#include <mutex>
#include <queue>

namespace rageam::ui
{
	/**
	 * \brief This is UI helper for compiling resources,
	 * it provides a dialog with progress bar and messages.
	 */
	class AssetAsyncCompiler : public App, public Singleton<AssetAsyncCompiler>
	{
		static constexpr ConstString SAVE_POPUP_NAME = "Exporting...##ASSET_SAVE_MODAL_DIALOG";

		std::mutex              m_ProgressMutex;
		double                  m_Progress = 0;
		List<string>            m_ProgressMessages;
		BackgroundTaskPtr       m_ActiveCompileTask;
		asset::AssetPtr			m_ActiveAsset;
		std::queue<file::WPath> m_CompileRequests;

		// Returns FALSE if there's no active task
		bool ProcessActiveRequest();
		// Returns TRUE if any request was processed
		bool ProcessNewRequest();
		void OnRender() override;
		
	public:
		~AssetAsyncCompiler() override;

		void CompileAsync(ConstWString assetPath);
	};
}
