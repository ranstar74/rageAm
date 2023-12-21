#include "hotdrawable.h"

#include "drawablehelpers.h"

bool rageam::asset::HotTxd::TryCompile()
{
	Dict = nullptr;

	rage::grcTextureDictionary* dict = new rage::grcTextureDictionary();
	if (!Asset->CompileToGame(dict))
	{
		AM_ERRF(L"HotTxd::TryCompile() -> Failed to compile workspace TXD '%ls'", Asset->GetDirectoryPath().GetCStr());
		delete dict;
		return false;
	}

	Dict = rage::pgPtr(dict);
	return true;
}

void rageam::asset::HotDrawable::IterateDrawableTextures(const std::function<void(rage::grcInstanceVar*)>& delegate, ConstString textureName) const
{
	// Apply to textures in drawable
	rage::grmShaderGroup* shaderGroup = m_Drawable->GetShaderGroup();
	for (u16 i = 0; i < shaderGroup->GetShaderCount(); i++)
	{
		rage::grmShader* shader = shaderGroup->GetShader(i);
		// Textures always placed first 
		for (u16 k = 0; k < shader->GetTextureVarCount(); k++)
		{
			rage::grcInstanceVar* var = shader->GetVarByIndex(k);

			if (textureName)
			{
				rage::grcTexture* varTex = var->GetTexture();
				if (!varTex)
					continue; // Variable has no texture set

				if (!String::Equals(varTex->GetName(), textureName, true))
					continue; // Name didn't match
			}

			delegate(var);
		}
	}
}

bool rageam::asset::HotDrawable::LoadAndCompileAsset()
{
	// We've got request, load asset & compile
	DrawableAssetPtr asset;

	// Use existing drawable asset or load if wasn't loaded before
	if (m_Asset)
		asset = m_Asset;
	else
		asset = AssetFactory::LoadFromPath<DrawableAsset>(m_AssetPath);

	// Failed to load asset...
	if (!asset)
		return false;

	asset->CompileCallback = CompileCallback;

	auto drawable = std::make_shared<gtaDrawable>();
	if (!asset->CompileToGame(drawable.get()))
		return false;

	// Add embed dictionary to TXDs
	if (asset->HasEmbedTXD())
	{
		const amPtr<TxdAsset>& embedTxdAsset = asset->GetEmbedDictionary();
		HotTxd embedHotTxd(embedTxdAsset, drawable->GetShaderGroup()->GetEmbedTextureDictionary(), true);
		m_TXDs.EmplaceAt(Hash(embedTxdAsset->GetDirectoryPath()), std::move(embedHotTxd));
	}

	// Compile all workshop TXDs
	AM_DEBUGF("HotDrawable::LoadAndCompileAsset() -> %u TXDs in workspace", asset->WorkspaceTXD->GetTexDictCount());
	for (u16 i = 0; i < asset->WorkspaceTXD->GetTexDictCount(); i++)
	{
		const amPtr<TxdAsset>& txdAsset = asset->WorkspaceTXD->GetTexDict(i);

		HotTxd hotTxd(txdAsset);
		hotTxd.TryCompile();

		m_TXDs.EmplaceAt(Hash(txdAsset->GetDirectoryPath()), std::move(hotTxd));
	}

	// Synchronize with getter
	std::unique_lock lock(m_RequestMutex);

	// Start watching on asset workspace directory (if asset is in workspace), otherwise we just watch asset directory itself
	ConstWString workspacePath = asset->GetWorkspacePath();
	if (!String::IsNullOrEmpty(workspacePath))
	{
		m_Watcher = std::make_unique<file::Watcher>(workspacePath, WATCHER_NOTIFY_FLAGS, file::WatcherFlags_Recurse);
		AM_DEBUGF(L"HotDrawable::LoadAndCompileAsset() -> Initialized watcher for '%ls'", workspacePath);
	}
	else
	{
		AM_DEBUGF(L"HotDrawable::LoadAndCompileAsset() -> Asset is not in workspace, initialized watcher for '%ls'", m_AssetPath.GetCStr());
	}

	m_AssetMap = DrawableAssetMap(*asset->CompiledDrawableMap);
	m_DrawableScene = asset->GetScene();
	m_Asset = std::move(asset);
	m_Drawable = std::move(drawable);

	m_HotChanges |= AssetHotFlags_DrawableCompiled;

	return true;
}

void rageam::asset::HotDrawable::ResolveMissingTextures(rage::grcTexture* newTexture)
{
	ConstString textureName = newTexture->GetName();
	IterateDrawableTextures([&](rage::grcInstanceVar* var)
		{
			ConstString missingTexName = GetMissingTextureName(var->GetTexture());
			if (missingTexName && String::Equals(missingTexName, textureName, true))
				var->SetTexture(newTexture);
		});

	// Orphan missing texture is not referenced anywhere anymore, remove it
	if (m_OrphanMissingTextures.Contains(textureName))
		m_OrphanMissingTextures.Remove(textureName);
}

void rageam::asset::HotDrawable::MarkTextureAsMissing(const rage::grcTexture* texture, const HotTxd& hotTxd) const
{
	if (!texture)
	{
		AM_WARNINGF("HotDrawable::MarkTextureAsMissing() -> Texture was NULL");
		return;
	}

	ConstString textureName = texture->GetName();
	MarkTextureAsMissing(hotTxd.Dict->Find(textureName), hotTxd);
}

void rageam::asset::HotDrawable::MarkTextureAsMissing(ConstString textureName, const HotTxd& hotTxd) const
{
	rage::grcTexture* missingDummy = CreateMissingTexture(textureName);
	IterateDrawableTextures([&](rage::grcInstanceVar* var)
		{
			var->SetTexture(missingDummy);
		}, textureName);
	hotTxd.Dict->Insert(textureName, missingDummy);
}

void rageam::asset::HotDrawable::HandleChange_Texture(const file::DirectoryChange& change)
{
	// Some unrelated file was added, usually - temp files (for e.g. Paint.NET creates one)
	bool addedOrRemoved = change.Action == file::ChangeAction_Added || change.Action == file::ChangeAction_Removed;
	bool incompatible = !TxdAsset::IsSupportedTextureExtension(change.Path);
	if (addedOrRemoved && incompatible)
		return;

	HotTxd* hotTxd = m_TXDs.TryGetAt(Hash(TxdAsset::GetTxdAssetPathFromTexture(change.Path)));
	if (!hotTxd) // Shouldn't happen
	{
		AM_ERRF("HotDrawable::HandleChange() -> Texture was changed in TXD that is not in the list!");
		return;
	}

	if (!hotTxd->Dict)
	{
		// Dict failed to compile before, try to compile it again
		AM_DEBUGF("HotDrawable::HandleChange() -> Texture was changed in dictionary that wasn't compiled, trying to compile...");

		// Failed to compile texture - mark it as missing to it can be added later
		if (!hotTxd->TryCompile())
		{
			// TODO: We should resolve missing textures?
			return; // Failed to compile again! Stop using corrupted DDS from CW.
		}

		if (hotTxd->IsEmbed)
		{
			m_Drawable->GetShaderGroup()->SetEmbedTextureDict(hotTxd->Dict.Get());
		}
	}

	// Find changed texture in asset
	TextureTune* textureTune = hotTxd->Asset->TryFindTextureTuneFromPath(change.Path);
	if (!textureTune)
	{
		// Case 0: Texture was first renamed to incompatible name and now renamed back!
		//	We use 'old' (which is ... new) name to look up tune (because it was marked as missing before)
		if (change.Action == file::ChangeAction_Renamed)
		{
			textureTune = hotTxd->Asset->TryFindTextureTuneFromPath(change.NewPath);
		}
		// Case 1: New texture was just added
		if (change.Action == file::ChangeAction_Added)
		{
			textureTune = hotTxd->Asset->CreateTuneForPath(change.Path);
		}

		if (!textureTune) // Shouldn't happen
		{
			AM_ERRF("HotDrawable::HandleChange() -> Modified texture has no tune!");
			return;
		}
	}

	// For renaming we simply have to reinsert texture with new name
	// IMPORTANT:
	// Paint.NET renames image file to temp name while saving!
	// In order to properly handle this and case when user renamed texture to incompatible
	// format we simply mark texture as missing
	if (change.Action == file::ChangeAction_Renamed)
	{
		// Case described above - texture was renamed to incompatible format
		if (!TxdAsset::IsSupportedTextureExtension(change.NewPath))
		{
			//// TODO: Same code as in remove&rename and we should check if texture is missing already!
			//rage::grcTexture* missingDummy = CreateMissingTexture(textureTune->Name);
			//WaitForApplyChangesSignal();
			//{
			//	IterateDrawableTextures([&](rage::grcInstanceVar* var)
			//	{
			//		var->SetTexture(missingDummy);
			//	}, textureTune->Name);

			//	hotTxd->Dict->Insert(textureTune->Name, missingDummy);
			//}
			//SignalApplyChannels();
			WaitForApplyChangesSignal();
			MarkTextureAsMissing(textureTune->Name, *hotTxd);
			SignalApplyChannels();

			return;
		}

		// Format is fine, just rename
		WaitForApplyChangesSignal();
		{
			// Get texture before renaming
			rage::grcTexture* gameTexture = hotTxd->Dict->Move(textureTune->Name);
			if (!gameTexture) // This shouldn't happen!
			{
				AM_ERRF("HotDrawable::HandleChange() -> Renamed texture was NULL.");
				SignalApplyChannels();
				return;
			}

			// Update name in tune
			if (!hotTxd->Asset->RenameTextureTune(*textureTune, change.NewPath.GetFileName()))
			{
				// New name is invalid...
				MarkTextureAsMissing(textureTune->Name, *hotTxd);
				return;
			}
			// Old tune is not valid anymore after changing name, we have to find it again
			textureTune = hotTxd->Asset->TryFindTextureTuneFromPath(change.NewPath);

			// Check if texture was marked as missed before and if so - resolve it
			if (IsMissingTexture(gameTexture))
			{
				rage::grcTexture* newGameTexture = hotTxd->Asset->CompileTexture(textureTune);
				// Check if compiled successfully and replace missing texture with new one
				if (newGameTexture)
				{
					ResolveMissingTextures(newGameTexture);

					delete gameTexture;
					gameTexture = newGameTexture;
				}
			}
			else
			{
				// Texture was compiled already, just rename it
				gameTexture->SetName(textureTune->Name);
			}

			// Reinsert texture with new name
			hotTxd->Dict->Insert(textureTune->Name, gameTexture);
		}
		SignalApplyChannels();
		AM_DEBUGF("HotDrawable::HandleChange() -> Renamed texture to '%s'", textureTune->Name);
		return;
	}

	// Texture was modified - recompile it and update references
	if (change.Action == file::ChangeAction_Modified)
	{
		// Compile texture
		rage::grcTexture* gameTexture = hotTxd->Asset->CompileTexture(textureTune);
		if (!gameTexture)
		{
			WaitForApplyChangesSignal();
			MarkTextureAsMissing(textureTune->Name, *hotTxd);
			SignalApplyChannels();
			AM_ERRF(L"HotDrawable::HandleChange() -> Failed to compile texture '%ls'!", change.Path.GetCStr());
			return;
		}

		// Sync with game thread and replace previous texture
		WaitForApplyChangesSignal();
		{
			// Replace old texture with new one in drawable
			IterateDrawableTextures([&](rage::grcInstanceVar* var)
				{
					var->SetTexture(gameTexture);
				}, gameTexture->GetName());

			// TODO: Can we merge it in one pass with replacing old textures?
			ResolveMissingTextures(gameTexture);

			// Update texture in TXD, old one will be destructed
			hotTxd->Dict->Insert(gameTexture->GetName(), gameTexture);
		}
		SignalApplyChannels();
		AM_DEBUGF("HotDrawable::HandleChange() -> Recompiled texture '%s'", textureTune->Name);
		return;
	}

	// On removing we have to replace every texture reference to missing
	if (change.Action == file::ChangeAction_Removed)
	{
		WaitForApplyChangesSignal();
		MarkTextureAsMissing(textureTune->Name, *hotTxd);
		SignalApplyChannels();

		//rage::grcTexture* missingDummy = CreateMissingTexture(textureTune->Name);
		/*WaitForApplyChangesSignal();
			{
				IterateDrawableTextures([&](rage::grcInstanceVar* var)
					{
						var->SetTexture(missingDummy);
					}, textureTune->Name);

				hotTxd->Dict->Insert(textureTune->Name, missingDummy);
			}
			SignalApplyChannels();*/
		AM_DEBUGF("HotDrawable::HandleChange() -> Removed texture '%s'", textureTune->Name);
		return;
	}

	// Just Compile & Insert
	if (change.Action == file::ChangeAction_Added)
	{
		// Compile texture
		rage::grcTexture* gameTexture = hotTxd->Asset->CompileTexture(textureTune);
		if (!gameTexture)
		{
			WaitForApplyChangesSignal();
			MarkTextureAsMissing(textureTune->Name, *hotTxd);
			SignalApplyChannels();
			AM_ERRF(L"HotDrawable::HandleChange() -> Failed to compile texture '%ls'!", change.Path.GetCStr());
			return;
		}

		WaitForApplyChangesSignal();
		{
			ResolveMissingTextures(gameTexture);
			hotTxd->Dict->Insert(gameTexture->GetName(), gameTexture);
		}
		SignalApplyChannels();
		AM_DEBUGF("HotDrawable::HandleChange() -> Added texture '%s'", textureTune->Name);
		return;
	}

	return;
}

void rageam::asset::HotDrawable::HandleChange_Txd(const file::DirectoryChange& change)
{
	HotTxd* hotTxd = nullptr;

	auto addTxd = [&](ConstWString path)
		{
			bool isEmbed = m_Asset->GetEmbedDictionaryPath() == path;
			if (!isEmbed && !Workspace::IsInWorkspace(path))
			{
				AM_ERRF("HotDrawable::HandleChange() -> Added TXD is not in workspace,"
					" make sure it's not under other asset directory.");
				return;
			}

			TxdAssetPtr newTxdAsset = AssetFactory::LoadFromPath<TxdAsset>(path);
			if (!newTxdAsset)
			{
				AM_ERRF("HotDrawable::HandleChange() -> Failed to load added TXD config.");
				return;
			}

			HotTxd newHotTxd(newTxdAsset);
			newHotTxd.TryCompile();
			newHotTxd.IsEmbed = isEmbed;

			WaitForApplyChangesSignal();
			{
				// Try to resolve missing textures if compiled successfully
				if (newHotTxd.Dict)
				{
					for (u16 i = 0; i < newHotTxd.Dict->GetSize(); i++)
					{
						rage::grcTexture* addedTex = newHotTxd.Dict->GetValueAt(i);
						ResolveMissingTextures(addedTex);
					}
				}

				if (newHotTxd.IsEmbed)
				{
					m_Drawable->GetShaderGroup()->SetEmbedTextureDict(newHotTxd.Dict.Get());
				}

				m_TXDs.EmplaceAt(Hash(newHotTxd.Asset->GetDirectoryPath()), std::move(newHotTxd));
			}
			SignalApplyChannels();

			AM_TRACEF(L"HotDrawable::HandleChange() -> Added new TXD '%ls'", path);
		};

	// Retrieve TXD from storage if it exists, in case if TXD was added it won't yet exist in cache
	if (change.Action == file::ChangeAction_Removed || change.Action == file::ChangeAction_Renamed)
	{
		hotTxd = m_TXDs.TryGetAt(Hash(change.Path));

		// Non .itd was renamed in .itd
		if (change.Action == file::ChangeAction_Renamed && AssetFactory::GetAssetType(change.NewPath) == AssetType_Txd)
		{
			addTxd(change.NewPath);
			return;
		}

		if (!hotTxd)
		{
			AM_ERRF("HotDrawable::HandleChange() -> TXD was changed that is not in the list!");
			return;
		}
	}

	// TXD was renamed
	if (change.Action == file::ChangeAction_Renamed)
	{
		WaitForApplyChangesSignal();
		{
			hotTxd->Asset->SetNewPath(change.NewPath);
			// Reinsert TXD at new hash
			hotTxd = &m_TXDs.EmplaceAt(Hash(change.NewPath), std::move(*hotTxd));

			if (hotTxd->IsEmbed)
			{
				// Embed dictionary was renamed to wrong name
				if (m_Asset->GetEmbedDictionaryPath() != change.NewPath)
				{
					AM_ERRF(L"HotDrawable::HandleChange() -> Embed TXD must be named '%ls.%ls', name '%ls' is not valid.",
						EMBED_DICT_NAME, ASSET_ITD_EXT, change.NewPath.GetFileName().GetCStr());
					m_Drawable->GetShaderGroup()->SetEmbedTextureDict(nullptr);
				}
				else
				{
					m_Drawable->GetShaderGroup()->SetEmbedTextureDict(hotTxd->Dict.Get());
				}
			}
		}
		SignalApplyChannels();
		return;
	}

	// TXD was removed
	if (change.Action == file::ChangeAction_Removed)
	{
		WaitForApplyChangesSignal();
		{
			// Embed dict was removed
			if (hotTxd->IsEmbed)
			{
				m_Drawable->GetShaderGroup()->SetEmbedTextureDict(nullptr);
			}

			// Reset all variables that use textures from removed TXD
			for (u16 i = 0; i < hotTxd->Dict->GetSize(); i++)
			{
				rage::grcTexture* removedTex = hotTxd->Dict->GetValueAt(i);
				rage::grcTexture* missingTex = CreateMissingTexture(removedTex->GetName());
				IterateDrawableTextures([&](rage::grcInstanceVar* var)
					{
						var->SetTexture(missingTex);
					}, removedTex->GetName());
				m_OrphanMissingTextures.Insert(removedTex->GetName(), missingTex);
			}
			m_TXDs.Remove(*hotTxd);
		}
		SignalApplyChannels();
		return;
	}

	// TXD was added
	if (change.Action == file::ChangeAction_Added)
	{
		addTxd(change.Path);
		return;
	}

	return;
}

void rageam::asset::HotDrawable::HandleChange_Scene(const file::DirectoryChange& change)
{
	// File was modified, fully reload scene
	if (change.Action == file::ChangeAction_Modified)
	{
		AM_DEBUGF("HotDrawable::HandleChange() -> Scene file was modified, reloading...");
		auto drawable = std::make_shared<gtaDrawable>();
		//if(!m_Asset->SaveConfig())
		//{
		//	AM_ERRF("HotDrawable::HandleChange() -> Failed to save config, canceling reload...");
		//	return;
		//}

		//m_Asset->Refresh();

		if (!m_Asset->CompileToGame(drawable.get()))
		{
			AM_ERRF("HotDrawable::HandleChange() -> Failed to compile new scene...");
			return;
		}
		WaitForApplyChangesSignal();
		{
			// TODO:
			// We currently doing map & scene copy but it's not really best option,
			// mainly that's required because when we're compiling drawable in background thread,
			// it cannot be accessed in main one
			m_AssetMap = DrawableAssetMap(*m_Asset->CompiledDrawableMap);
			m_DrawableScene = m_Asset->GetScene();
			m_Drawable = std::move(drawable);
			m_HotChanges |= AssetHotFlags_DrawableCompiled;
		}
		SignalApplyChannels();
		return;
	}

	// For renaming we just have to update file name
	if (change.Action == file::ChangeAction_Renamed)
	{
		AM_DEBUGF(L"HotDrawable::HandleChange() -> Scene file was renamed to '%ls'", change.NewPath.GetCStr());
		m_Asset->SetScenePath(change.NewPath);
		return;
	}

	// Scene file doesn't exist anymore
	if (change.Action == file::ChangeAction_Removed)
	{
		// TODO: How do we handle this?
		return;
	}
}

void rageam::asset::HotDrawable::HandleChange_Asset(const rageam::file::DirectoryChange& change)
{
	// Simply update new path
	if (change.Action == file::ChangeAction_Renamed)
	{
		m_AssetPath = change.NewPath;
		m_Asset->SetNewPath(change.NewPath);
		return;
	}

	if (change.Action == file::ChangeAction_Removed)
	{
		// TODO: How do we handle this?
		return;
	}

	return;
}

void rageam::asset::HotDrawable::HandleChange(const file::DirectoryChange& change)
{
	AM_DEBUGF(L"HotDrawable::HandleChange() -> '%hs' in '%ls'", Enum::GetName(change.Action), change.Path.GetCStr());

	// Loaded asset directory (the .idr one) was changed
	if (change.Path == m_AssetPath)
	{
		HandleChange_Asset(change);
		return;
	}

	// Scene model file (gltf/fbx) was changed
	if (change.Path == m_Asset->GetScenePath())
	{
		HandleChange_Scene(change);
		return;
	}

	// A change was done to TXD directory itself
	if (AssetFactory::GetAssetType(change.Path) == AssetType_Txd || AssetFactory::GetAssetType(change.NewPath) == AssetType_Txd)
	{
		HandleChange_Txd(change);
		m_HotChanges |= AssetHotFlags_TxdModified;
		return;
	}

	// Texture was added/changed/deleted in existing TXD
	// To properly update texture we have to keep track of the:
	// - Drawable shader group variables (they directly reference grcTexture)
	// - Texture in the dictionary itself
	// - Texture tune in TXD asset
	file::WPath textureTxdPath;
	if (TxdAsset::GetTxdAssetPathFromTexture(change.Path, textureTxdPath))
	{
		HandleChange_Texture(change);
		m_HotChanges |= AssetHotFlags_TxdModified;
		return;
	}
}

void rageam::asset::HotDrawable::WaitForApplyChangesSignal()
{
	std::unique_lock lock(m_ApplyChangesMutex);
	m_ApplyChangesWaiting = true;
	m_ApplyChangesCondVar.wait(lock);
	m_ApplyChangesWaiting = false;
}

void rageam::asset::HotDrawable::SignalApplyChannels()
{
	m_ApplyChangesCondVar.notify_one();
}

bool rageam::asset::HotDrawable::IsWaitingForApplyChanges()
{
	std::unique_lock lock(m_ApplyChangesMutex);
	return m_ApplyChangesWaiting;
}

void rageam::asset::HotDrawable::StopWatcherAndWorkerThread()
{
	{
		std::unique_lock lock(m_RequestMutex);

		// Check if watcher exists at all, it may not if scene is not loaded
		if (!m_Watcher)
			return;

		// We have to destroy watcher before worker thread because watcher depends on it
		m_Watcher->RequestStop();
	}

	// Wait until watcher stops and thread exits
	m_WorkerThread->RequestExitAndWait();
	m_WorkerThread = nullptr;

	// Now watcher exited and we can safely remove it
	m_Watcher = nullptr;
}

u32 rageam::asset::HotDrawable::WorkerThreadEntryPoint(const ThreadContext* ctx)
{
	HotDrawable* hot = static_cast<HotDrawable*>(ctx->Param);

	while (!ctx->Thread->ExitRequested())
	{
		bool isLoaded;
		{
			std::unique_lock lock(hot->m_RequestMutex);
			isLoaded = hot->m_Drawable != nullptr;
		}

		if (!isLoaded)
		{
			// Asset is not loaded, await request
			{
				std::unique_lock lock(hot->m_RequestMutex);
				hot->m_RequestCondVar.wait(lock, [&] { return hot->m_LoadRequested; });
				hot->m_LoadRequested = false;
				hot->m_IsLoading = true;
			}

			bool loaded = hot->LoadAndCompileAsset();
			{
				std::unique_lock lock(hot->m_RequestMutex);
				hot->m_IsLoading = false;
			}

			if (!loaded)
				break; // Continue waiting for load request...
		}

		// Asset is loaded, poll file changes
		while (true)
		{
			if (ctx->Thread->ExitRequested())
				return 0;

			// Check if watcher was destroyed from Reload
			{
				std::unique_lock lock(hot->m_RequestMutex);
				if (!hot->m_Watcher)
					break;
			}

			// Thread will be paused until change occurs or exit requested from Reload
			file::DirectoryChange change;
			if (hot->m_Watcher->GetNextChange(change))
			{
				hot->HandleChange(change);
			}
		}
	}

	return 0;
}

rageam::asset::HotDrawable::HotDrawable(ConstWString assetPath)
{
	m_AssetPath = assetPath;
}

rageam::asset::HotDrawable::~HotDrawable()
{
	StopWatcherAndWorkerThread();
}

void rageam::asset::HotDrawable::LoadAndCompile(bool keepAsset)
{
	StopWatcherAndWorkerThread();

	m_Drawable = nullptr;
	m_LoadRequested = true;
	if (!keepAsset)
		m_Asset = nullptr;

	ConstString threadName = String::FormatTemp("Hot Drawable %ls", file::GetFileName(m_AssetPath.GetCStr()));
	m_WorkerThread = std::make_unique<Thread>(threadName, WorkerThreadEntryPoint, this);

	m_RequestCondVar.notify_one();
}

AssetHotFlags rageam::asset::HotDrawable::ApplyChanges()
{
	// Pass job to worker thread and wait completion signal after applying changes
	if (IsWaitingForApplyChanges())
	{
		SignalApplyChannels();
		WaitForApplyChangesSignal();
	}

	AssetHotFlags changes = m_HotChanges;
	m_HotChanges = AssetHotFlags_None;
	return changes;
}

rageam::asset::HotDrawableInfo rageam::asset::HotDrawable::GetInfo()
{
	std::unique_lock lock(m_RequestMutex);

	HotDrawableInfo info;
	info.IsLoading = m_IsLoading;
	info.Drawable = m_Drawable;
	info.DrawableAsset = m_Asset;
	info.DrawableAssetMap = &m_AssetMap;
	info.DrawableScene = m_DrawableScene;
	info.TXDs = &m_TXDs;

	return info;
}
