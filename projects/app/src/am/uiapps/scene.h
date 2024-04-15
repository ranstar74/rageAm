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
		// Use weak because scene window will be closed from outside - via close button or from window manager
		static inline amWeakPtr<Scene> sm_OpenedSceneWeakRef;
		static inline file::WPath      sm_PendingScenePath;
		static inline Vec3V			   sm_PendingScenePosition;

	protected:
		// Can only construct from OpenWindowForSceneAndLoad
		Scene() = default;
	public:
		ConstString GetTitle() const override
		{
			static constexpr int MAX_TITLE = 256;
			static char s_Buffer[MAX_TITLE];

			ConstString modelName = GetModelName();
			if (String::IsNullOrEmpty(modelName)) // No model is loaded yet
				sprintf_s(s_Buffer, MAX_TITLE, "Scene %s", GetName());
			else
				sprintf_s(s_Buffer, MAX_TITLE, "Scene %s - %s", GetName(), modelName);
		
			return s_Buffer;
		}
		ConstString GetID() const override { return "rageam::ui::Scene"; }

		virtual void LoadFromPath(ConstWString path) = 0;

		virtual const Vec3V& GetRotation() const = 0;
		virtual const Vec3V& GetPosition() const = 0;
		virtual void SetPosition(const Vec3V& pos) = 0;
		virtual void SetRotation(const Vec3V& angle) = 0;

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

		static void TryOpenPendingSceneWindow();

		// Closes currently opened scene window (if there's any)
		// and tries to open new one for specified path
		static void ConstructFor(ConstWString path);

		// Gets currently opened scene window (there can be only one at a time)
		static Scene* GetCurrent() { return sm_OpenedSceneWeakRef.lock().get(); }

		static inline Vec3V DefaultSpawnPosition = { 0 ,0 , 0 };
	};
}
