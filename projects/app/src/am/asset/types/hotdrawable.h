//
// File: hotdrawable.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "drawable.h"
#include "am/asset/factory.h"
#include "am/file/watcher.h"
#include "am/system/worker.h"

// What changes were applied by hot reload
enum AssetHotFlags_ : u32
{
	AssetHotFlags_None				= 0,		// No change was applied
	AssetHotFlags_DrawableCompiled	= 1 << 0,	// Scene was loaded and drawable compiled
	AssetHotFlags_TxdModified		= 1 << 1,	// Txd or texture was added or removed
};
typedef u32 AssetHotFlags;

namespace rageam::asset
{
	struct HotDrawableInfo
	{
		DrawableTxdSet*		TXDs;			// All texture dictionaries in the workspace + embed
		DrawableAssetPtr	DrawableAsset;
		amPtr<gtaDrawable>	Drawable;
		bool				IsLoading;		// Drawable asset compiling?
		AssetHotFlags		HotFlags;		// For current frame only!
	};

	/**
	 * \brief Drawable asset that automatically syncs up with file changes.
	 */
	class HotDrawable
	{
		static constexpr file::eNotifyFlags WATCHER_NOTIFY_FLAGS =
			file::NotifyFlags_DirectoryName |
			file::NotifyFlags_FileName |
			file::NotifyFlags_LastWrite;

		struct CompileDrawableResult
		{
			amPtr<gtaDrawable>			Drawable;
			// Those are moved from drawable asset copy
			graphics::ScenePtr			Scene;
			amUPtr<DrawableAssetMap>	Map;
		};

		amUPtr<file::Watcher>		m_Watcher;
		file::WPath					m_AssetPath;
		BackgroundTaskPtr			m_LoadingTask;
		// Those are all background tasks we execute, changes are synced with game via UserLambda delegate
		Tasks						m_Tasks;
		DrawableAssetPtr			m_Asset;
		amPtr<gtaDrawable>			m_CompiledDrawable;
		// Holds every compiled TXD from workspace including embed (for simpler access)
		// Key is the full wide path to the TXD
		DrawableTxdSet				m_TXDs;
		// Those textures were left without dictionary (dict was removed)...
		// We have to keep track of them to prevent memory leaks
		rage::grcTextureDictionary	m_OrphanMissingTextures;
		AssetHotFlags				m_HotFlags = AssetHotFlags_None;

		// Iterates all textures in drawable shader group variables
		// If textureName is not NULL, only textures with given name are yielded, excluding vars with NULL texture
		void IterateDrawableTextures(
			const std::function<void(rage::grcInstanceVar*)>& delegate, ConstString textureName = nullptr) const;

		void CompileTxdAsync(DrawableTxd* txd);
		void AddNewTxdAsync(ConstWString path);
		void CompileTexAsync(DrawableTxd* txd, TextureTune* tune);

		// Replaces all textures with name of given texture with the texture
		void ReplaceTexture(rage::grcTexture* newTexture) const;
		// Replaces all missing textures (with name of given texture) in drawable with given one
		void ResolveMissingTexture(rage::grcTexture* newTexture);
		// Creates missing texture & replaces existing references in drawable
		void MarkTextureAsMissing(const rage::grcTexture* texture, const DrawableTxd& hotTxd) const;
		void MarkTextureAsMissing(const TextureTune& tune, const DrawableTxd& hotTxd) const;

		void HandleChange_Texture(const file::DirectoryChange& change);
		void HandleChange_Txd(const file::DirectoryChange& change);
		void HandleChange_Scene(const file::DirectoryChange& change);
		void HandleChange(const file::DirectoryChange& change);

		void UpdateBackgroundJobs();
		void CheckIfDrawableHasCompiled();

	public:
		HotDrawable(ConstWString assetPath);

		// Flushes current state and fully reloads asset
		void LoadAndCompileAsync(bool keepAsset = true);

		// Must be called on early update/tick for synchronization of drawable changes
		HotDrawableInfo UpdateAndApplyChanges();

		const file::WPath& GetPath() const { return m_AssetPath; }

		// Drawable compile callback
		// NOTE: This delegate is not thread-safe!
		std::function<AssetCompileCallback> CompileCallback;
	};
	using HotDrawablePtr = amPtr<HotDrawable>;
}
