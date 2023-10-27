//
// File: hotdrawable.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "drawable.h"
#include "am/asset/factory.h"
#include "am/file/watcher.h"
#include "am/system/thread.h"

// What changes were applied by hot reload
enum AssetHotFlags_ : u32
{
	AssetHotFlags_None = 0,						// No change was applied
	AssetHotFlags_DrawableCompiled = 1 << 0,	// Scene was loaded and drawable compiled
	AssetHotFlags_TxdModified = 1 << 1,			// Txd or texture was added or removed
};
typedef u32 AssetHotFlags;

namespace rageam::asset
{
	// TODO: We should show 'not found' texture icon instead of deleting it completely (on delete / loaded & not found)
	// TODO: We should apply changes on the beginning of frame when nothing is using it!
	// TODO: How do we save changes on reload?

	struct HotTxd
	{
		TxdAssetPtr						Asset;
		rage::grcTextureDictionaryPtr	Dict; // May be null if failed to compile
		bool							IsEmbed;

		bool TryCompile();
	};

	struct HotTxdHashFn
	{
		u32 operator()(const HotTxd& hotTxd) const { return hotTxd.Asset->GetHashKey(); }
	};
	using HotTxdSet = HashSet<HotTxd, HotTxdHashFn>;

	struct HotDrawableInfo
	{
		HotTxdSet*			TXDs;			// All texture dictionaries in the workspace + embed
		DrawableAssetPtr	DrawableAsset;
		amPtr<gtaDrawable>	Drawable;
		bool				IsLoading;		// Drawable asset compiling
	};

	/**
	 * \brief Hot reloads drawable asset changes.
	 */
	class HotDrawable
	{
		static constexpr file::eNotifyFlags WATCHER_NOTIFY_FLAGS =
			file::NotifyFlags_DirectoryName |
			file::NotifyFlags_FileName |
			file::NotifyFlags_LastWrite;
		
		// We have to sync two things - changes applied to already loaded drawable and drawable loading
		// - Drawable load request can be queued from any thread at any moment, worker thread is instantly
		//		switched from waiting for changes to loading drawable
		// - Drawable changes are applied only on beginning of game early update

		amUniquePtr<Thread>			m_WorkerThread;
		std::condition_variable		m_RequestCondVar;
		std::mutex					m_RequestMutex;
		std::condition_variable		m_ApplyChangesCondVar;
		std::mutex					m_ApplyChangesMutex;
		bool						m_ApplyChangesWaiting = false;
		bool						m_LoadRequested = false;
		bool						m_IsLoading = false;
		amUniquePtr<file::Watcher>	m_Watcher;
		file::WPath					m_AssetPath;
		DrawableAssetPtr			m_Asset;
		amPtr<gtaDrawable>			m_Drawable;
		// Holds every compiled TXD from workspace including embed
		// Key is the full wide path to the TXD
		HotTxdSet					m_TXDs;
		std::atomic<AssetHotFlags>	m_HotChanges = AssetHotFlags_None;
		// Those textures were left without dictionary (dict was removed)... We have to keep track of them to prevent memory leaks
		rage::grcTextureDictionary	m_OrphanMissingTextures;

		// Iterates all textures in drawable shader group variables
		// If textureName is not NULL, only textures with given name are yielded, excluding vars with NULL texture
		void IterateDrawableTextures(const std::function<void(rage::grcInstanceVar*)>& delegate, ConstString textureName = nullptr) const;
		bool LoadAndCompileAsset();

		// Replaces all missing textures (with name of given texture) in drawable with given one
		void ResolveMissingTextures(rage::grcTexture* newTexture);
		// Creates missing texture & replaces existing references in drawable
		void MarkTextureAsMissing(const rage::grcTexture* texture, const HotTxd& hotTxd) const;
		void MarkTextureAsMissing(ConstString textureName, const HotTxd& hotTxd) const;
		void HandleChange_Texture(const file::DirectoryChange& change);
		void HandleChange_Txd(const file::DirectoryChange& change);
		void HandleChange_Scene(const file::DirectoryChange& change);
		void HandleChange_Asset(const file::DirectoryChange& change);
		void HandleChange(const file::DirectoryChange& change);

		// To safely apply changes we sync with main game thread and apply them on early update
		void WaitForApplyChangesSignal();
		void SignalApplyChannels();
		bool IsWaitingForApplyChanges();

		void StopWatcherAndWorkerThread();

		static u32 WorkerThreadEntryPoint(const ThreadContext* ctx);

	public:
		HotDrawable(ConstWString assetPath);
		~HotDrawable();

		// Flushes current state and fully reloads asset
		void Load();
		// Must be called on early update/tick for synchronization of drawable changes
		AssetHotFlags ApplyChanges();
		HotDrawableInfo GetInfo();

		// Drawable compile callback
		// NOTE: This delegate is not thread-safe!
		std::function<AssetCompileCallback> CompileCallback;
	};
	using HotDrawablePtr = amPtr<HotDrawable>;
}
