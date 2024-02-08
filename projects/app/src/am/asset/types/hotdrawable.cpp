#include "hotdrawable.h"

#include "am/graphics/render.h"
#include "am/system/worker.h"
#include "rage/grcore/texturepc.h"

rageam::List<rageam::asset::HotDrawable::TextureToRemove> rageam::asset::HotDrawable::sm_TexturesToRemove;
std::mutex rageam::asset::HotDrawable::sm_TexturesToRemoveMutex;

void rageam::asset::HotDrawable::ForAllDrawableTextures(
	const std::function<bool(rage::grcInstanceVar*)>& delegate, ConstString textureName) const
{
	for (rage::grmShader* shader : *m_CompiledDrawable->GetShaderGroup())
	{
		for (rage::grcInstanceVar& var : shader->GetTextureIterator())
		{
			ConstString varTextureName = TxdAsset::UndecorateMissingTextureName(var.GetTexture());

			if (textureName && !String::Equals(textureName, varTextureName))
				continue;

			if (!delegate(&var))
				return;
		}
	}
}

void rageam::asset::HotDrawable::AddTextureToDeleteList(rage::grcTexture* texture) const
{
	if (!texture)
		return;

	std::unique_lock lock(sm_TexturesToRemoveMutex);
	TextureToRemove textureToRemove(amUPtr<rage::grcTexture>(texture), ImGui::GetFrameCount());
	sm_TexturesToRemove.Emplace(std::move(textureToRemove));
}

void rageam::asset::HotDrawable::SetTexture(ConstString textureName, rage::grcTexture* texture) const
{
	rage::atHashValue hashKey = rage::atStringHash(textureName);

	// Add existing texture in delete list
	s32 existingTextureIndex = m_MegaDictionary->IndexOf(hashKey);
	if (existingTextureIndex != -1)
		AddTextureToDeleteList(m_MegaDictionary->MoveFrom(existingTextureIndex));

	// Set new texture in dictionary and drawable
	m_MegaDictionary->Insert(hashKey, texture);
	ReplaceDrawableTexture(textureName, texture);
}

void rageam::asset::HotDrawable::LoadTextureAsync(const TxdAssetPtr& txdAsset, TextureTune* tune)
{
	file::Path textureName;
	if (!tune->GetValidatedTextureName(textureName))
		return;

	rage::atHashValue textureHashKey = rage::atStringHash(textureName);

	// Create missing dummy texture if it was just added so user can instantly see it in the list
	if (!m_MegaDictionary->Contains(textureHashKey))
		m_MegaDictionary->Insert(textureHashKey, TxdAsset::CreateMissingTexture(textureName));

	// We must ensure that texture is not duplicated in other dictionary
	// We also should check if there's multiple textures with the same name but different extension
	TextureInfo* existingTextureInfo = m_TextureInfos.TryGetAt(textureHashKey);
	if (existingTextureInfo)
	{
		if (existingTextureInfo->IsOprhan())
		{
			// Texture was orphan, update TXD asset
			existingTextureInfo->SetTxd(txdAsset);
		}
		// Texture with the same name was already loaded from different dictionary? ...
		else if (existingTextureInfo->TxdPath != txdAsset->GetDirectoryPath())
		{
			AM_ERRF("HotDrawable::LoadTextureAsync() -> Texture '%s' exists in multiple dictionaries! '%ls' and '%ls'",
			        textureName.GetCStr(),
			        existingTextureInfo->TxdPath.GetCStr(),
			        txdAsset->GetDirectoryPath().GetCStr());
			return;
		}
	}
	else
	{
		// Create info for newly added texture
		TextureInfo textureInfo;
		textureInfo.Name = textureName;
		textureInfo.SetTxd(txdAsset);
		m_TextureInfos.EmplaceAt(textureHashKey, std::move(textureInfo));
	}

	// NOTE: We assume that txdAsset will not be touched when this task is active!
	BackgroundTaskPtr task = BackgroundWorker::Run([txdAsset, tune]
		{
			rage::pgUPtr texture = rage::pgUPtr((rage::grcTextureDX11*) txdAsset->CompileSingleTexture(*tune));
			bool success = texture != nullptr; // Check here because SetCurrentResult will move pointer
			BackgroundWorker::SetCurrentResult(texture);
			if (success)
				AM_DEBUGF("HotDrawable::LoadTextureAsync() -> Compiled texture '%ls'", tune->GetName());
			else
				AM_DEBUGF("HotDrawable::LoadTextureAsync() -> Failed to compile texture '%ls'", tune->GetName());
			return success;
		});

	task->UserDelegate = [this, task, textureName]
		{
			// We used pointer wrapper to prevent memory leak, but we don't need it anymore because ownership will move to TXD
			rage::grcTexture* texture;
			if (task->IsSuccess())
			{
				auto& texturePtr = task->GetResult<rage::pgUPtr<rage::grcTextureDX11>>();
				texture = texturePtr.Get();
				texturePtr.SuppressDelete();
			}
			else
			{
				// Texture failed to compile, create missing dummy
				texture = TxdAsset::CreateMissingTexture(textureName);
			}

			SetTexture(textureName, texture);

			m_HotFlags |= AssetHotFlags_TxdModified;
		};
	m_AsyncTasks.Emplace(std::move(task));
}

void rageam::asset::HotDrawable::LoadTextureAsync(ConstWString texturePath)
{
	TxdAssetPtr txdAsset = GetTxdAssetFromTexturePath(texturePath);
	if (!txdAsset)
		return;

	TextureTune* textureTune = txdAsset->TryFindTuneFromPath(texturePath);
	if (!textureTune)
	{
		// Texture was just added, create a new tune for it
		textureTune = &txdAsset->AddTune(texturePath);
		AM_DEBUGF(L"HotDrawable::AddTextureAsync() -> Created tune for '%ls'", texturePath);
	}

	LoadTextureAsync(txdAsset, textureTune);
}

void rageam::asset::HotDrawable::LoadTxdAsync(ConstWString txdPath)
{
	TxdAssetPtr txdAsset = GetTxdAssetFromPath(txdPath);
	if (!txdAsset)
		return;

	for (TextureTune& textureTune : txdAsset->GetTextureTunes())
	{
		LoadTextureAsync(txdAsset, &textureTune);
	}
}

rageam::asset::TxdAssetPtr rageam::asset::HotDrawable::GetTxdAssetFromTexturePath(ConstWString texturePath)
{
	file::WPath txdPath;
	if (!TxdAsset::GetTxdAssetPathFromTexture(texturePath, txdPath))
	{
		AM_ERRF(L"HotDrawable::GetTxdAssetFromTexturePath() -> Texture is not in a dictionary! '%ls'", texturePath);
		return nullptr;
	}

	return GetTxdAssetFromPath(txdPath);
}

rageam::asset::TxdAssetPtr rageam::asset::HotDrawable::GetTxdAssetFromPath(ConstWString txdPath)
{
	TxdAssetPtr  txdAsset;
	TxdAssetPtr* ppTxdAsset = m_TxdAssetStore.TryGetAt(AssetPathHashFn(txdPath));

	if (ppTxdAsset)
		return *ppTxdAsset;

	// New TXD? Load and cache it
	txdAsset = AssetFactory::LoadFromPath<TxdAsset>(txdPath, true /* Use temp config for TXD editor hot reload */);
	if (txdAsset)
	{
		m_TxdAssetStore.Insert(txdAsset);
		return txdAsset;
	}

	return nullptr;
}

void rageam::asset::HotDrawable::ReloadTxdAsset(ConstWString txdPath)
{
	// Reload it completely because temp config that we use for hot reloading might be just created or removed
	TxdAssetPtr txdAsset = AssetFactory::LoadFromPath<TxdAsset>(txdPath, true /* Use temp config for TXD editor hot reload */);
	if (txdAsset)
		m_TxdAssetStore.Insert(txdAsset);
}

void rageam::asset::HotDrawable::ReplaceDrawableTexture(ConstString textureName, rage::grcTexture* newTexture) const
{
	ForAllDrawableTextures([newTexture](rage::grcInstanceVar* var)
	{
		var->SetTexture(newTexture);
		return true;
	}, textureName);
}

bool rageam::asset::HotDrawable::IsTextureUsedInDrawable(ConstString textureName) const
{
	bool used = false;
	ForAllDrawableTextures([&used](rage::grcInstanceVar* var)
	{
		used = true;
		return false; // Stop iterating...
	}, textureName);
	return used;
}

void rageam::asset::HotDrawable::RemoveTexture(ConstString textureName)
{
	rage::atHashValue textureHashKey = rage::atStringHash(textureName);

	if (IsTextureUsedInDrawable(textureName))
	{
		rage::grcTexture* removedTexture = MarkTextureAsMissing(textureName);
		AddTextureToDeleteList(removedTexture);
	}
	else
	{
		// Texture is not used, remove completely
		rage::grcTexture* removedTexture = m_MegaDictionary->Move(textureHashKey);
		AddTextureToDeleteList(removedTexture);
		m_TextureInfos.RemoveAt(textureHashKey);
	}
}

void rageam::asset::HotDrawable::RemoveTxd(ConstWString txdPath)
{
	TxdAssetPtr txd = GetTxdAssetFromPath(txdPath);
	for (TextureTune& tune : txd->GetTextureTunes())
	{
		file::Path textureName;
		if (!tune.GetValidatedTextureName(textureName))
			continue;

		RemoveTexture(textureName);
	}
	m_TxdAssetStore.RemoveAt(txd->GetHashKey());
}

rage::grcTexture* rageam::asset::HotDrawable::MarkTextureAsMissing(ConstString textureName) const
{
	rage::atHashValue textureHashKey = rage::atStringHash(textureName);
	rage::grcTexture* oldTexture = m_MegaDictionary->Move(textureName);
	rage::grcTexture* missingTexture = TxdAsset::CreateMissingTexture(textureName);
	ReplaceDrawableTexture(textureName, missingTexture);
	m_MegaDictionary->Insert(textureHashKey, missingTexture);
	TextureInfo& textureInfo = m_TextureInfos.GetAt(textureHashKey);
	textureInfo.MakeOrphan();
	return oldTexture;
}

void rageam::asset::HotDrawable::HandleChange_Texture(const file::DirectoryChange& change)
{
	// Invalid file (non-texture) was renamed
	// If this is a bit unclear, ::HandleChange calls this function if either old or new path is valid,
	// if old path is invalid - the new one is valid
	if (!TxdAsset::IsSupportedImageFile(change.Path) /* New path is valid ... */)
	{
		LoadTextureAsync(change.NewPath);
		return;
	}

	if (change.Action == file::ChangeAction_Added || 
		change.Action == file::ChangeAction_Modified)
	{
		LoadTextureAsync(change.Path);
		return;
	}

	file::Path textureName;
	if (!TxdAsset::GetValidatedTextureName(change.Path, textureName))
		return;

	if (change.Action == file::ChangeAction_Removed)
	{
		RemoveTexture(textureName);
		return;
	}

	// The best way to handle texture renaming is ... ignore it
	// If user picked texture A, it must remain texture A.
	// Imagine if texture A gets renamed to B, and then C gets renamed to A,
	// this would completely mess up the things (and this is what paint.NET does)
	if (change.Action == file::ChangeAction_Renamed)
	{
		AM_DEBUGF("HotDrawable::HandleChange() -> Texture '%s' was renamed, used=%i",
		          textureName.GetCStr(), IsTextureUsedInDrawable(textureName));

		// Create a copy of current texture info
		// It must be done now because SetMissingTexture will unlink it from TXD
		TextureInfo newInfo = m_TextureInfos.GetAt(rage::atStringHash(textureName));

		// Replace old texture with missing
		rage::grcTexture* renamedTexture = MarkTextureAsMissing(textureName);

		// Renamed to non-image file, delete the texture
		file::Path newTextureName;
		if (!TxdAsset::IsSupportedImageFile(change.NewPath) || 
			!TxdAsset::GetValidatedTextureName(change.NewPath, newTextureName))
		{
			AddTextureToDeleteList(renamedTexture);
			return;
		}

		rage::atHashValue newTextureHashKey = rage::atStringHash(newTextureName);

		// Insert info for new name
		newInfo.Name = newTextureName;
		m_TextureInfos.Emplace(std::move(newInfo));
		
		// Rename texture
		if (TxdAsset::IsMissingTexture(renamedTexture))
			TxdAsset::SetMissingTextureName(renamedTexture, newTextureName);
		else
			renamedTexture->SetName(newTextureName);

		// Reinsert it at new slot and set it in drawable
		AddTextureToDeleteList(m_MegaDictionary->MoveIfExists(newTextureHashKey));
		m_MegaDictionary->Insert(newTextureHashKey, renamedTexture);
		ReplaceDrawableTexture(newTextureName, renamedTexture);

		return;
	}
}

void rageam::asset::HotDrawable::HandleChange_Txd(const file::DirectoryChange& change)
{
	if (change.Action == file::ChangeAction_Added)
	{
		LoadTxdAsync(change.Path);
		return;
	}

	if (change.Action == file::ChangeAction_Removed)
	{
		AM_DEBUGF(L"HotDrawable::HandleChange() -> Removing TXD '%ls'", change.Path.GetCStr());
		RemoveTxd(change.Path);
		return;
	}

	if (change.Action == file::ChangeAction_Renamed)
	{
		// Embed dictionary was renamed to wrong name, it must be called EMBED_DICT_NAME
		bool isEmbed = m_Asset->GetEmbedDictionaryPath() == change.Path;
		bool isNewEmbed = m_Asset->GetEmbedDictionaryPath() == change.NewPath;
		if (isEmbed && !isNewEmbed)
		{
			RemoveTxd(change.Path);
			AM_ERRF(
				"HotDrawable::HandleChange() -> Embed TXD in drawable director must be called '%ls%ls'",
				EMBED_DICT_NAME, ASSET_ITD_EXT);
			return;
		}

		// Update asset in store set
		HashValue    txdPathHash = AssetPathHashFn(change.Path);
		TxdAssetPtr* ppRenamedTxd = m_TxdAssetStore.TryGetAt(txdPathHash);
		if (!ppRenamedTxd) // Might be NULL if was removed before (see the case above)
		{
			LoadTxdAsync(change.NewPath);
			return;
		}

		TxdAssetPtr renamedTxd = *ppRenamedTxd;
		renamedTxd->SetNewPath(change.NewPath);
		m_TxdAssetStore.RemoveAt(txdPathHash);
		m_TxdAssetStore.Insert(renamedTxd);

		// Update txd reference in every existing texture info
		for (TextureInfo& info : m_TextureInfos)
		{
			if (info.IsOprhan())
				continue;

			if (info.TxdHashValue == txdPathHash)
				info.SetTxd(renamedTxd);
		}

		return;
	}
}

void rageam::asset::HotDrawable::HandleChange(const file::DirectoryChange& change)
{
	AM_DEBUGF(L"HotDrawable::HandleChange() -> '%hs' in '%ls' / '%ls'",
	          Enum::GetName(change.Action), change.Path.GetCStr(), change.NewPath.GetCStr());

	// Loaded asset directory (the .idr one) was changed
	if (change.Path == m_AssetPath)
	{
		return;
	}

	// Scene model file (gltf/fbx) was changed
	if (change.Path == m_Asset->GetScenePath())
	{
		// File was modified, fully reload scene
		if (change.Action == file::ChangeAction_Modified)
		{
			AM_DEBUGF("HotDrawable::HandleChange() -> Scene file was modified, reloading...");
			LoadAndCompileAsync();
			return;
		}
		return;
	}

	// A change was done to TXD directory itself
	if (AssetFactory::GetAssetType(change.Path) == AssetType_Txd ||
		AssetFactory::GetAssetType(change.NewPath) == AssetType_Txd)
	{
		HandleChange_Txd(change);
		m_HotFlags |= AssetHotFlags_TxdModified;
		return;
	}

	// TXD config was changed
	ImmutableWString pathStr = change.Path.GetCStr();
	bool isConfig = pathStr.EndsWith(ASSET_CONFIG_NAME);
	bool isTempConfig = !isConfig && pathStr.EndsWith(ASSET_CONFIG_NAME_TEMP);
	if (isConfig || isTempConfig)
	{
		file::WPath txdPath = change.Path.GetParentDirectory();
		if (AssetFactory::GetAssetType(txdPath) == AssetType_Txd)
		{
			AM_DEBUGF(L"HotDrawable::HandleChange() -> Config changed in '%ls'", txdPath.GetCStr());
			ReloadTxdAsset(txdPath);
			LoadTxdAsync(txdPath);
		}
		m_HotFlags |= AssetHotFlags_TxdModified;
		return;
	}

	// Texture was added/changed/deleted in existing TXD
	// To properly update texture we have to keep track of the:
	// - Drawable shader group variables (they directly reference grcTexture)
	// - Texture in the dictionary itself
	// - Texture tune in TXD asset
	bool isInTxdAsset = TxdAsset::IsTextureInTxdAssetDirectory(change.Path);
	bool isNewOrOldNameAnImage = 
		TxdAsset::IsSupportedImageFile(change.Path) || 
		TxdAsset::IsSupportedImageFile(change.NewPath);
	if (isInTxdAsset && isNewOrOldNameAnImage)
	{
		HandleChange_Texture(change);
		m_HotFlags |= AssetHotFlags_TxdModified;
		return;
	}
}

void rageam::asset::HotDrawable::UpdateBackgroundJobs()
{
	// Get all new changes
	file::DirectoryChange newChange;
	while (m_Watcher->GetNextChange(newChange))
		m_PendingChanges.Add(newChange);

	// For debugging...
	/*static bool s_ChangesPaused = true;
	if (ImGui::IsKeyPressed(ImGuiKey_N, false))
		s_ChangesPaused = !s_ChangesPaused;*/

	// IMPORTANT: We can't execute multiple async changes at the same time,
	// for example if user modifies a texture and then removes dictionary,
	// task order won't be preserved, and we may end up modifying texture in a dictionary that
	// was already removed, preserving order is too complicated and not worth time
	while (/*s_ChangesPaused && */m_PendingChanges.Any() && m_AsyncTasks.GetSize() == 0)
	{
		file::DirectoryChange& change = m_PendingChanges.First();
		HandleChange(change);
		m_PendingChanges.RemoveFirst();
	}

	// See if any background task has finished and add their indices to remove list
	List<u32> indicesToRemove; // Finished tasks to remove
	for (u32 i = 0; i < m_AsyncTasks.GetSize(); i++)
	{
		const BackgroundTaskPtr& task = m_AsyncTasks[i];

		// Task is not finished? We'll check again next update...
		if (!task->IsFinished())
			continue;

		// Finish up the task, apply changes...
		task->UserDelegate();

		// In reversed order, to prevent element shifting on deletion (last -> first)
		indicesToRemove.Insert(0, i);
	}

	// Clean up finished tasks
	for (u32 index : indicesToRemove)
		m_AsyncTasks.RemoveAt(index);
}

void rageam::asset::HotDrawable::UpdateDrawableCompiling()
{
	// Check if drawable is done compiling...
	if (!m_LoadingTask || !m_LoadingTask->IsFinished())
		return;

	// Will be destroyed on return
	BackgroundTaskPtr loadingTask = std::move(m_LoadingTask);
	if (!loadingTask->IsSuccess())
		return;

	amPtr<CompileDrawableResult> compileResult = loadingTask->GetResult<amPtr<CompileDrawableResult>>();
	m_CompiledDrawable = std::move(compileResult->Drawable);
	// Update asset from copy
	m_Asset->SetScene(compileResult->Scene);
	m_Asset->RefreshTunesFromScene();
	m_Asset->CompiledDrawableMap = std::move(compileResult->Map);

	// Initialize our mega dictionary with textures from embed and workspace dictionaries
	if (!m_MegaDictionary)
	{
		m_MegaDictionary = rage::pgPtr(new rage::grcTextureDictionary());

		LoadTxdAsync(m_Asset->GetEmbedDictionaryPath());

		if (m_Asset->WorkspaceTXD)
		{
			AM_DEBUGF("HotDrawable::UpdateAndApplyChanges() -> %u TXDs in workspace",
			          m_Asset->WorkspaceTXD->GetTexDictCount());

			for (u16 i = 0; i < m_Asset->WorkspaceTXD->GetTexDictCount(); i++)
			{
				ConstWString wsTxdPath = m_Asset->WorkspaceTXD->GetTexDict(i)->GetDirectoryPath();
				LoadTxdAsync(wsTxdPath);
			}
		}
	}

	// Set our mega texture dictionary in drawable and resolve textures
	if (m_CompiledDrawable)
	{
		rage::grmShaderGroup* shaderGroup = m_CompiledDrawable->GetShaderGroup();

		// Drawable is compiled without actual textures, only placeholders, resolve them if possible
		// (see DrawableAsset::tl_SkipTextures)
		rage::grcTextureDictionaryPtr dummyDictionary = shaderGroup->GetEmbedTextureDictionary();
		u16 dummyTextureCount = dummyDictionary->GetSize();
		for (u16 i = 0; i < dummyTextureCount; i++)
		{
			// Use index 0 because MoveFrom removes the texture
			auto pair = dummyDictionary->GetAt(0);
			rage::grcTexture* missingTexture = dummyDictionary->MoveFrom(0);

			// Note that we're using pair.Key here because texture->GetName is in different format
			// (see TxdAsset::CreateMissingTexture)
			if (m_MegaDictionary->Contains(pair.Key))
			{
				// Texture is already loaded, just remove this missing dummy and update it in drawable
				AddTextureToDeleteList(missingTexture);

				ConstString       textureName = TxdAsset::UndecorateMissingTextureName(missingTexture);
				rage::grcTexture* loadedTexture = m_MegaDictionary->Find(pair.Key);
				ReplaceDrawableTexture(textureName, loadedTexture);
			}
			else
			{
				// A new texture was added after loading drawable that is not in our mega dictionary
				// We can't resolve it, leave it as missing
				m_MegaDictionary->Insert(pair.Key, missingTexture);
			}
		}

		shaderGroup->SetEmbedTextureDict(m_MegaDictionary);
	}

	m_HotFlags |= AssetHotFlags_DrawableCompiled;
}

rageam::asset::HotDrawable::HotDrawable(ConstWString assetPath)
{
	m_AssetPath = assetPath;
}

void rageam::asset::HotDrawable::LoadAndCompileAsync(bool keepAsset)
{
	m_JustRequestedLoad = true;

	// Wait for existing background stuff to finish...
	// We may want cancellation token here
	BackgroundWorker::WaitFor(m_AsyncTasks);
	if (m_LoadingTask) m_LoadingTask->Wait();

	m_LoadingTask = nullptr;
	m_CompiledDrawable = nullptr;
	m_AsyncTasks.Destroy();

	if (!m_Asset || !keepAsset)
	{
		m_Asset = AssetFactory::LoadFromPath<DrawableAsset>(m_AssetPath);
		m_Asset->CompileCallback = CompileCallback;

		m_MegaDictionary = nullptr;
		m_TxdAssetStore.Destroy();
		m_TextureInfos.Destroy();

		// Failed to load...
		if (!m_Asset)
			return;
	}

	// Create FS watcher for receiving changes
	if (!m_Watcher)
	{
		ConstWString workspacePath = m_Asset->GetWorkspacePath();
		ConstWString watchPath;
		if (!String::IsNullOrEmpty(workspacePath))
		{
			watchPath = workspacePath;
			AM_DEBUGF(L"HotDrawable::LoadAndCompileAsset() -> Initialized watcher for '%ls'", workspacePath);
		}
		else
		{
			// When we are not in workspace, the only things we are watching is scene file and embed dictionary
			watchPath = m_Asset->GetDirectoryPath();
			AM_DEBUGF(L"HotDrawable::LoadAndCompileAsset() -> Asset is not in workspace, initialized watcher for '%ls'",
			          m_AssetPath.GetCStr());
		}
		// NOTE: We're using wait time 0 here because we're pulling changes in update loop
		m_Watcher = std::make_unique<file::Watcher>(watchPath, WATCHER_NOTIFY_FLAGS, file::WatcherFlags_Recurse);
	}

	// NOTE: We're copying asset here because fields like CompiledDrawableMap cannot be shared safely
	// both by update and worker thread at the same time
	amPtr assetCopy = std::make_shared<DrawableAsset>(*m_Asset);

	m_LoadingTask = BackgroundWorker::Run([assetCopy]
	{
		gtaDrawablePtr drawable = rage::pgCountedPtr(new gtaDrawable());

		bool oldSkipTextures = DrawableAsset::tl_SkipTextures;
		// We lazy load textures (after scene was loaded) and handle them different way (with single TXD)
		DrawableAsset::tl_SkipTextures = true;
		bool success = assetCopy->CompileToGame(drawable.Get());
		DrawableAsset::tl_SkipTextures = oldSkipTextures;

		if (!success)
		{
			AM_ERRF("HotDrawable::LoadAndCompileAsync() -> Failed to compile new scene...");
			return false;
		}

		amPtr result = std::make_shared<CompileDrawableResult>(
			std::move(drawable), assetCopy->GetScene(), std::move(assetCopy->CompiledDrawableMap));

		BackgroundWorker::SetCurrentResult(result);
		return true;
	});
}

void rageam::asset::HotDrawable::NotifyTextureWasUnselected(const rage::grcTexture* texture)
{
	// There are textures that were removed but still referenced in drawable, we call them
	// missing orphans. We can safely remove them only when they're not referenced anymore
	if (!texture)
		return;

	if (!TxdAsset::IsMissingTexture(texture))
		return;

	ConstString       textureName = TxdAsset::UndecorateMissingTextureName(texture);
	rage::atHashValue textureHashKey = rage::atStringHash(textureName);
	TextureInfo&      textureInfo = m_TextureInfos.GetAt(textureHashKey);

	// Only orphan texture can be removed when it's no longer referenced
	if (!textureInfo.IsOprhan())
		return;

	if (IsTextureUsedInDrawable(textureName))
		return;

	// Missing orphan texture is not referenced in drawable anymore, we can remove it for good
	AddTextureToDeleteList(m_MegaDictionary->Move(textureHashKey));
	m_TextureInfos.RemoveAt(textureHashKey);
}

rageam::asset::HotDrawableInfo rageam::asset::HotDrawable::UpdateAndApplyChanges()
{
	m_HotFlags = AssetHotFlags_None;

	UpdateBackgroundJobs();
	UpdateDrawableCompiling();

	// We have to set flag via m_JustRequestedLoad because Load function can be called from anywhere
	if (m_JustRequestedLoad)
	{
		m_HotFlags |= AssetHotFlags_DrawableUnloaded;
		m_JustRequestedLoad = false;
	}

	HotDrawableInfo info;
	info.IsLoading = m_LoadingTask != nullptr;
	info.Drawable = m_CompiledDrawable;
	info.DrawableAsset = m_Asset;
	info.HotFlags = m_HotFlags;
	info.MegaDictionary = m_MegaDictionary.Get();
	return info;
}

ConstWString rageam::asset::HotDrawable::GetTxdPathFromTexture(ConstString textureName, HashValue* outTxdHashValue) const
{
	TextureInfo* info = m_TextureInfos.TryGetAt(rage::atStringHash(textureName));
	if (info)
	{
		if (outTxdHashValue) *outTxdHashValue = info->TxdHashValue;
		return info->TxdPath;
	}
	return L"";
}

rage::grcTexture* rageam::asset::HotDrawable::LookupTexture(ConstString name) const
{
	if (String::IsNullOrEmpty(name))
		return nullptr;

	return m_MegaDictionary->Find(name);
}

void rageam::asset::HotDrawable::RemoveTexturesFromRenderThread(bool forceAll)
{
	std::unique_lock lock(sm_TexturesToRemoveMutex);
	if (sm_TexturesToRemove.GetSize() == 0)
		return;

	List<u32> indices;
	for (u32 i = 0; i < sm_TexturesToRemove.GetSize(); i++)
	{
		TextureToRemove& entry = sm_TexturesToRemove[i];

		// We check that texture was removed at least 4 frames ago because
		//  game's draw list may still hold references on it, system is quite complicated
		//  and this is the easiest way to ensure that texture is not referenced anymore
		// We take absolute value because frame count may overflow eventually
		if (forceAll || abs(entry.FrameIndex - ImGui::GetFrameCount()) > 16)
			indices.Insert(0, i);
	}

	if (!indices.Any())
		return;

	AM_DEBUGF("HotDrawable::RemoveTexturesFromRenderThread() -> Removing %u textures", indices.GetSize());
	for (u32 index : indices)
	{
		sm_TexturesToRemove.RemoveAt(index);
	}
}
