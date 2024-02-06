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
	class HotDrawable;

	struct HotDrawableInfo
	{
		DrawableTxdSet*  TXDs;			// All texture dictionaries in the workspace + embed
		DrawableAssetPtr DrawableAsset;
		gtaDrawablePtr   Drawable;
		bool             IsLoading;		// Drawable asset compiling?
		AssetHotFlags    HotFlags;		// For current frame only!
	};

	/**
	 * \brief Drawable asset that automatically syncs up with file changes.
	 */
	class HotDrawable
	{
		enum TaskID
		{
			TaskID_None,
			TaskID_CompileTex,
			TaskID_CompileTxd,
		};

		static constexpr file::eNotifyFlags WATCHER_NOTIFY_FLAGS =
			file::NotifyFlags_DirectoryName |
			file::NotifyFlags_FileName |
			file::NotifyFlags_LastWrite;

		struct CompileDrawableResult
		{
			gtaDrawablePtr			  Drawable;
			// Those are moved from drawable asset copy
			graphics::ScenePtr        Scene;
			amUPtr<DrawableAssetMap>  Map;
		};

		// When we remove texture we must be sure that it is not referenced in draw list
		// Easiest way to do this - remove them on the end of the frame
		struct TextureToRemove { amUPtr<rage::grcTexture> Tex; int FrameIndex; };
		static List<TextureToRemove>	sm_TexturesToRemove;
		static std::mutex				sm_TexturesToRemoveMutex;

		amUPtr<file::Watcher>			m_Watcher;
		file::WPath						m_AssetPath;
		BackgroundTaskPtr				m_LoadingTask;
		// Those are all background tasks we execute, changes are synced with game via UserLambda delegate
		Tasks							m_AsyncTasks;
		DrawableAssetPtr				m_Asset;
		gtaDrawablePtr					m_CompiledDrawable;
		// Holds every compiled TXD from workspace including embed (for simpler access)
		// Key is the full wide path to the TXD
		DrawableTxdSet					m_TXDs;
		// Those textures were left without dictionary (dict was removed)...
		// We have to keep track of them to prevent memory leaks
		rage::grcTextureDictionaryPtr	m_OrphanMissingTextures;
		AssetHotFlags					m_HotFlags = AssetHotFlags_None;
		List<file::DirectoryChange>		m_PendingChanges;

		// Iterates all textures in drawable shader group variables
		// If textureName is not NULL, only textures with given name are yielded, excluding vars with NULL texture
		// Return false from delegate to stop iterating
		void ForAllDrawableTextures(
			const std::function<bool(rage::grcInstanceVar*)>& delegate, ConstString textureName = nullptr) const;

		// If orphan missing texture is not referenced anywhere anymore, remove it
		void RemoveFromMissingOrphans(ConstString textureName) const;

		// See explanation comments for RemoveTexturesFromRenderThread & sm_TexturesToRemove
		void AddTextureToDeleteList(rage::grcTexture* texture) const;
		void AddTxdTexturesToDeleteList(const rage::grcTextureDictionaryPtr& dict) const;

		void AddNewTxdAsync(const file::WPath& path);
		void CompileTxdAsync(DrawableTxd* txd);
		void CompileTexAsync(DrawableTxd* txd, TextureTune* tune);

		// Marks all the textures as missing orphans and removes TXD from the store
		void DeleteTxdAndRemoveFromTheList(const DrawableTxd& txd);
		void DeleteTxdAndRemoveFromTheList(ConstWString path);

		bool IsMissingOrphanTexture(ConstString textureName) const { return m_OrphanMissingTextures->Contains(textureName); }
		// Texture's parent dictionary is gone, and we must remove the texture, but
		// if it still referenced somewhere, we must create missing texture and add it to orphans
		void MarkTextureAsMissingOrphan(ConstString textureName) const;
		void MarkTextureAsMissing(ConstString textureName, const DrawableTxd& txd) const;
		void MarkTextureAsMissing(const TextureTune& textureTune, const DrawableTxd& txd) const;
		// Replaces existing and missing textures
		void ReplaceDrawableTexture(ConstString textureName, rage::grcTexture* newTexture) const;
		bool IsTextureUsedInDrawable(ConstString textureName) const;

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
		// Texture is not used by drawable anymore and if it was missing we can safely remove it from list now
		void NotifyTextureWasUnselected(const rage::grcTexture* texture) const;

		// Must be called on early update/tick for synchronization of drawable changes
		HotDrawableInfo UpdateAndApplyChanges();

		rage::grcTexture* LookupTexture(ConstString name) const;

		// All removed textures are added in single list and removed on the end of the frame to prevent
		// removing textures that are still currently in draw list
		static void RemoveTexturesFromRenderThread(bool forceAll);
		static void ShutdownClass() { RemoveTexturesFromRenderThread(true); }

		const file::WPath&          GetPath() const { return m_AssetPath; }
		rage::grcTextureDictionary& GetOrphanMissingTextures() { return *m_OrphanMissingTextures; }

		// Drawable compile callback
		// NOTE: This delegate is not thread-safe!
		std::function<AssetCompileCallback> CompileCallback;
	};
	using HotDrawablePtr = amPtr<HotDrawable>;
}
