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
	AssetHotFlags_DrawableUnloaded	= 1 << 0,	// Scene was unloaded
	AssetHotFlags_DrawableCompiled	= 1 << 1,	// Scene was loaded and drawable compiled
	AssetHotFlags_TxdModified		= 1 << 2,	// Txd or texture was added or removed
};
typedef u32 AssetHotFlags;

namespace rageam::asset
{
	class HotDrawable;

	struct HotDrawableInfo
	{
		DrawableAssetPtr            DrawableAsset;
		gtaDrawablePtr              Drawable;
		bool                        IsLoading;		// Drawable asset compiling?
		AssetHotFlags               HotFlags;		// For current frame only!
		rage::grcTextureDictionary* MegaDictionary;
	};

	/**
	 * \brief Since HotDrawable stores all textures in single dictionary (MegaDictionary),
	 * we provide this map for convenient way to access/iterate TXD textures.
	 * \n This can be seen as 'virtual' dictionary.
	 */
	struct HotDictionary
	{
		file::WPath TxdAssetPath;
		file::Path	TxdName;
		List<u16>   Indices;
		List<u32>	Hashes;
		bool		IsEmbed;
		bool		IsOrphan;
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
			gtaDrawablePtr			  Drawable;
			// Those are moved from drawable asset copy
			graphics::ScenePtr        Scene;
			amUPtr<DrawableAssetMap>  Map;
		};

		struct TextureInfo
		{
			// Empty if texture is missing orphan (has no parent TXD, but still used in drawable)
			file::WPath TxdPath;
			HashValue	TxdHashValue;
			string		Name;

			bool IsOprhan() const { return String::IsNullOrEmpty(TxdPath); }
			void SetTxd(const TxdAssetPtr& txdAsset)
			{
				TxdPath = txdAsset->GetDirectoryPath();
				TxdHashValue = txdAsset->GetHashKey();
			}
			void MakeOrphan()
			{
				TxdPath = L"";
				TxdHashValue = 0;
			}
		};
		struct TextureInfoHashFn
		{
			u32 operator()(const TextureInfo& info) const { return atStringHash(info.Name); }
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
		AssetHotFlags					m_HotFlags = AssetHotFlags_None;
		List<file::DirectoryChange>		m_PendingChanges;
		bool							m_JustRequestedLoad = false;

		// We store all the textures in the single dictionary. This simplifies access and editing interface.
		// All the extra metadata for the specific texture is stored in separate hashmap m_TextureInfos.
		// This allows us to lazy load textures after entity was spawned, greatly reducing loading time
		rage::grcTextureDictionaryPtr			m_MegaDictionary;
		HashSet<TextureInfo, TextureInfoHashFn>	m_TextureInfos;
		HashSet<TxdAssetPtr, TxdAssetHashFn>	m_TxdAssetStore;	// Instead of loading TXD asset every time, we cache them here
		List<HotDictionary>						m_HotDicts;			// Mapping between TXD and textures in mega dictionary
		bool									m_HotDictsDirty = false; // Flag indicating that hot dictionaries must be rebuilt on request

		// Iterates all textures in drawable shader group variables
		// If textureName is not NULL, only textures with given name are yielded, excluding vars with NULL texture
		// Return false from delegate to stop iterating
		void ForAllDrawableTextures(
			const std::function<bool(rage::grcInstanceVar*)>& delegate, ConstString textureName = nullptr) const;

		// Every created grcTexture must be not deleted manually but added in delete list
		// Texture will only be removed when draw list does not reference it anymore
		void AddTextureToDeleteList(rage::grcTexture* texture) const;

		void SetTexture(ConstString textureName, rage::grcTexture* texture) const;

		void LoadTextureAsync(const TxdAssetPtr& txdAsset, TextureTune* tune);
		void LoadTextureAsync(ConstWString texturePath);
		void LoadTxdAsync(ConstWString txdPath);

		TxdAssetPtr GetTxdAssetFromTexturePath(ConstWString texturePath);
		TxdAssetPtr GetTxdAssetFromPath(ConstWString txdPath);
		void		ReloadTxdAsset(ConstWString txdPath);

		// Replaces existing and missing textures
		void ReplaceDrawableTexture(ConstString textureName, rage::grcTexture* newTexture) const;
		bool IsTextureUsedInDrawable(ConstString textureName) const;
		// Removes texture and sets missing one if it's still used in drawable
		void RemoveTexture(ConstString textureName);
		void RemoveTxd(ConstWString txdPath);
		// Puts missing texture and returns the texture that was used
		rage::grcTexture* MarkTextureAsMissing(ConstString textureName) const;

		void HandleChange_Texture(const file::DirectoryChange& change);
		void HandleChange_Txd(const file::DirectoryChange& change);
		void HandleChange(const file::DirectoryChange& change);

		void UpdateBackgroundJobs();
		void UpdateDrawableCompiling();

	public:
		HotDrawable(ConstWString assetPath);

		// Flushes current state and fully reloads asset
		void LoadAndCompileAsync(bool keepAsset = true);
		// Event when texture was unset in shader variable
		void NotifyTextureWasUnselected(const rage::grcTexture* texture);
		// Must be called on early update/tick for synchronization of drawable changes
		HotDrawableInfo UpdateAndApplyChanges();

		// Path of loaded scene asset (DrawableAsset)
		const file::WPath& GetPath() const { return m_AssetPath; }
		// Attempts to retrieve TXD asset path from texture name
		ConstWString GetTxdPathFromTexture(ConstString textureName, HashValue* outTxdHashValue = nullptr) const;
		// Maps path hash to loaded TXD asset
		const auto&	GetTxdAssets() { return m_TxdAssetStore; }
		// Looks up cached txd asset by path
		const auto& GetTxdAsset(ConstWString path) { return GetTxdAssetFromPath(path); }
		// Maps TXD to textures in HotDrawableInfo::MegaDictionary
		const List<HotDictionary>& GetHotDictionaries();

		rage::grcTexture* LookupTexture(ConstString name) const;

		// All removed textures are added in single list and removed on the end of the frame to prevent
		// removing textures that are still currently in draw list
		static void RemoveTexturesFromRenderThread(bool forceAll);
		static void ShutdownClass() { RemoveTexturesFromRenderThread(true); }

		// Drawable compile callback
		// NOTE: This delegate is not thread-safe!
		std::function<AssetCompileCallback> CompileCallback;
	};
	using HotDrawablePtr = amPtr<HotDrawable>;
}
