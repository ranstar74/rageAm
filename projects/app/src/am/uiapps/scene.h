//
// File: scene.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/asset/factory.h"
#include "am/ui/extensions.h"
#include "am/ui/imglue.h"
#include "am/ui/window.h"

namespace rageam::ui
{
	enum SceneType
	{
		Scene_Invalid,
#ifdef AM_INTEGRATED
		Scene_Editor,
		Scene_Inspector,
#endif
	};

	/**
	 * \brief Base class for viewable 3D scenes. For simplicity, there can be only one scene created at a time.
	 * At the moment we have SceneInGame, SceneEditor and SceneInspector (for .IDR and .YDR)
	 */
	class Scene : public Window
	{
		static inline Scene* sm_Instance = nullptr;

	protected:
		// Can only construct from OpenWindowForSceneAndLoad
		Scene() = default;
	public:
		ConstString GetTitle() const override
		{
			ConstString modelName = GetModelName();
			if (String::IsNullOrEmpty(modelName)) // Nothing is loaded yet...
				return ImGui::FormatTemp("Scene %s", GetName());
			return ImGui::FormatTemp("%s - %s", GetName(), GetModelName());
		}
		ConstString GetID() const override { return "rageam::ui::Scene"; }

		virtual void LoadFromPath(ConstWString path) = 0;

		virtual const Vec3V& GetPosition() const = 0;
		virtual void SetPosition(const Vec3V& pos) = 0;

		// Name of the loaded scene - for e.g. 'Duck.idr'
		virtual ConstString GetModelName() const = 0;
		// Name of the scene - 'Editor', 'Inspector'
		virtual ConstString GetName() const = 0;

		// Sets active debug camera (if there's any) position to target spawned prop
		virtual void FocusCamera() = 0;

		static SceneType GetSceneType(ConstWString path)
		{
#ifdef AM_INTEGRATED
			if (file::MatchExtension(path, L"ydr"))
				return Scene_Inspector;
			if (asset::AssetFactory::GetAssetType(path) == asset::AssetType_Drawable)
				return Scene_Editor;
#endif
			return Scene_Invalid;
		}

		// Closes currently opened scene window (if there's any)
		// and tries to open new one for specified path
		static void OpenWindowForSceneAndLoad(ConstWString path);

		// Gets currently opened scene window (there can be only one at a time)
		static Scene* GetCurrent() { return sm_Instance; }
		static inline Vec3V DefaultSpawnPosition = { 0 ,0 , 0 };
	};
}
