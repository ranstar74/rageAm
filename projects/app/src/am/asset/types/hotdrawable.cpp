#include "hotdrawable.h"

#include "am/system/worker.h"

void rageam::asset::HotDrawable::IterateDrawableTextures(const std::function<void(rage::grcInstanceVar*)>& delegate, ConstString textureName) const
{
	// Apply to textures in drawable
	rage::grmShaderGroup* shaderGroup = m_CompiledDrawable->GetShaderGroup();
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

void rageam::asset::HotDrawable::CompileTxdAsync(DrawableTxd* txd)
{
	BackgroundTaskPtr task = BackgroundWorker::Run([txd]
	{
		return txd->TryCompile();
	});

	// This will be executed in UpdateBackgroundJobs
	task->UserDelegate = [this, txd]
	{
		// Try to resolve missing textures if compiled successfully
		if (txd->Dict)
		{
			for (u16 i = 0; i < txd->Dict->GetSize(); i++)
			{
				rage::grcTexture* addedTex = txd->Dict->GetValueAt(i);
				ResolveMissingTexture(addedTex);
			}
		}

		// If compiled TXD was embed, set it to drawable
		if (m_CompiledDrawable && txd->IsEmbed)
		{
			m_CompiledDrawable->GetShaderGroup()->SetEmbedTextureDict(txd->Dict);
		}

		AM_TRACEF(L"HotDrawable::AddNewTxdAsync() -> Added new TXD '%ls'",
		          txd->Asset->GetDirectoryPath().GetCStr());

		m_HotFlags |= AssetHotFlags_TxdModified;
	};

	m_Tasks.Emplace(std::move(task));
}

void rageam::asset::HotDrawable::AddNewTxdAsync(ConstWString path)
{
	bool isEmbed = m_Asset->GetEmbedDictionaryPath() == path;
	if (!isEmbed && !Workspace::IsInWorkspace(path))
	{
		AM_ERRF(
			"HotDrawable::AddNewTxdAsync() -> Added TXD is not in workspace,"
			" make sure it's not under other asset directory.");
		return;
	}

	TxdAssetPtr newTxdAsset = AssetFactory::LoadFromPath<TxdAsset>(path);
	if (!newTxdAsset)
	{
		AM_ERRF("HotDrawable::AddNewTxdAsync() -> Failed to load added TXD tune.");
		return;
	}

	DrawableTxd txd(newTxdAsset);
	txd.IsEmbed = isEmbed;
	CompileTxdAsync(&m_TXDs.Emplace(std::move(txd)));
}

void rageam::asset::HotDrawable::CompileTexAsync(DrawableTxd* txd, TextureTune* tune)
{
	BackgroundTaskPtr task = BackgroundWorker::Run([tune, txd]
		{
			auto gameTexture = amPtr<rage::grcTexture>(txd->Asset->CompileSingleTexture(*tune));
			bool success = gameTexture != nullptr; // Check here because SetCurrentResult will move pointer
			BackgroundWorker::SetCurrentResult(gameTexture);
			if(success)
				AM_DEBUGF("HotDrawable::CompileTexAsync() -> Recompiled texture '%ls'", tune->GetName());
			else
				AM_DEBUGF("HotDrawable::CompileTexAsync() -> Failed to compile texture '%ls'", tune->GetName());
			return success;
		});

	task->UserDelegate = [this, task, txd]
		{
			amPtr<rage::grcTexture> texture = std::move(task->GetResult<amPtr<rage::grcTexture>>());
			if (texture) // There's chance that it failed to compile...
			{
				// We have call both functions because ResolveMissingTexture replaces ONLY texture that are missing,
				// but we don't know if this texture was missing before or not...
				ReplaceTexture(texture.get());
				ResolveMissingTexture(texture.get());
			}
			else
			{
				MarkTextureAsMissing(texture.get(), *txd);
			}

			m_HotFlags |= AssetHotFlags_TxdModified;
		};

	m_Tasks.Emplace(std::move(task));
}

void rageam::asset::HotDrawable::ReplaceTexture(rage::grcTexture* newTexture) const
{
	IterateDrawableTextures([&](rage::grcInstanceVar* var)
		{
			var->SetTexture(newTexture);
		}, newTexture->GetName());
}

void rageam::asset::HotDrawable::ResolveMissingTexture(rage::grcTexture* newTexture)
{
	ConstString textureName = newTexture->GetName();
	IterateDrawableTextures([&](rage::grcInstanceVar* var)
		{
			ConstString missingTexName = TxdAsset::GetMissingTextureName(var->GetTexture());
			if (missingTexName && String::Equals(missingTexName, textureName, true))
				var->SetTexture(newTexture);
		});

	// Orphan missing texture is not referenced anywhere anymore, remove it
	if (m_OrphanMissingTextures.Contains(textureName))
		m_OrphanMissingTextures.Remove(textureName);
}

void rageam::asset::HotDrawable::MarkTextureAsMissing(const rage::grcTexture* texture, const DrawableTxd& hotTxd) const
{
	if (!texture)
	{
		AM_WARNINGF("HotDrawable::MarkTextureAsMissing() -> Texture was NULL");
		return;
	}

	ConstString textureName = texture->GetName();
	MarkTextureAsMissing(hotTxd.Dict->Find(textureName), hotTxd);
}

void rageam::asset::HotDrawable::MarkTextureAsMissing(const TextureTune& tune, const DrawableTxd& hotTxd) const
{
	file::Path textureName;
	hotTxd.Asset->GetValidatedTextureName(tune.GetFilePath(), textureName);

	rage::grcTexture* missingDummy = TxdAsset::CreateMissingTexture(textureName);
	IterateDrawableTextures([&](rage::grcInstanceVar* var)
		{
			var->SetTexture(missingDummy);
		}, textureName);
	hotTxd.Dict->Insert(textureName, missingDummy);
}

void rageam::asset::HotDrawable::HandleChange_Texture(const file::DirectoryChange& change)
{
	bool addedOrRemoved = 
		change.Action == file::ChangeAction_Added || 
		change.Action == file::ChangeAction_Removed;
	bool incompatible = !TxdAsset::IsSupportedImageFile(change.Path);
	// Some unrelated file was added/removed, usually - temp files (for e.g. Paint.NET creates one)
	if (addedOrRemoved && incompatible)
		return;

	DrawableTxd* txd = m_TXDs.TryGetAt(Hash(TxdAsset::GetTxdAssetPathFromTexture(change.Path)));
	if (!txd) // Shouldn't happen
	{
		AM_ERRF("HotDrawable::HandleChange() -> Texture was changed in TXD that is not in the list!");
		return;
	}

	// Dict failed to compile before, try to compile it again
	if (!txd->Dict)
	{
		AM_DEBUGF("HotDrawable::HandleChange() -> Texture was changed in dictionary that wasn't compiled, trying to compile...");
		CompileTxdAsync(txd);
		return;
	}

	// Find the changed texture in asset
	TextureTune* textureTune = txd->Asset->TryFindTuneFromPath(change.Path);
	if (!textureTune)
	{
		if (change.Action == file::ChangeAction_Added)
		{
			textureTune = &txd->Asset->AddTune(change.Path);
			AM_DEBUGF(L"HotDrawable::HandleChange() -> Added texture '%ls' in TXD '%ls'", 
				textureTune->GetName(), txd->Asset->GetDirectoryPath().GetCStr());
		}

		if (!textureTune) // Shouldn't happen...
		{
			AM_ERRF("HotDrawable::HandleChange() -> Modified texture has no tune!");
			return;
		}
	}

	// Texture was modified/added - recompile it and update references
	if (change.Action == file::ChangeAction_Modified ||
		change.Action == file::ChangeAction_Added)
	{
		CompileTexAsync(txd, textureTune);
		return;
	}

	// The best way to handle texture renaming is ... ignore it
	// If user picked texture A, it must remain texture A.
	// Imagine if texture A gets renamed to B, and then C gets renamed to A,
	// this would completely mess up the things (and this is what paint.NET does)
	if (change.Action == file::ChangeAction_Renamed)
	{
		MarkTextureAsMissing(*textureTune, *txd);
		AM_DEBUGF("HotDrawable::HandleChange() -> Texture '%ls' was renamed, marking as missing...", 
			textureTune->GetName());
		return;
	}

	// On removing we have to replace every texture reference to missing
	if (change.Action == file::ChangeAction_Removed)
	{
		MarkTextureAsMissing(*textureTune, *txd);
		AM_DEBUGF("HotDrawable::HandleChange() -> Removed texture '%ls'", textureTune->GetName());
		return;
	}
}

void rageam::asset::HotDrawable::HandleChange_Txd(const file::DirectoryChange& change)
{
	// TXD was renamed
	if (change.Action == file::ChangeAction_Renamed)
	{
		HashValue pathHashValue = Hash(change.Path);

		// Retrieve TXD from storage if it exists, in case if TXD was added it won't yet exist in cache
		DrawableTxd* txd = m_TXDs.TryGetAt(pathHashValue);

		// Non .itd was renamed in .itd
		if (!txd && AssetFactory::GetAssetType(change.NewPath) == AssetType_Txd)
		{
			AddNewTxdAsync(change.NewPath);
		}
		// .itd directory name was changed, reinsert it with new path
		else
		{
			DrawableTxd renamedTxd = *txd;
			renamedTxd.Asset->SetNewPath(change.NewPath);
			m_TXDs.RemoveAt(pathHashValue);
			m_TXDs.Emplace(std::move(renamedTxd));
		}
		return;
	}

	// TXD was removed
	if (change.Action == file::ChangeAction_Removed)
	{
		HashValue pathHashValue = Hash(change.Path);

		DrawableTxd* removedTxd = m_TXDs.TryGetAt(pathHashValue);
		if (!removedTxd) // Shouldn't happen
		{
			AM_ERRF("HotDrawable::HandleChange() -> TXD was removed that is not in the list!");
			return;
		}

		// Embed dict was removed, unlink it from drawable
		if (removedTxd->IsEmbed)
		{
			m_CompiledDrawable->GetShaderGroup()->SetEmbedTextureDict(nullptr);
		}

		// Reset all variables that use textures from removed TXD
		for (u16 i = 0; i < removedTxd->Dict->GetSize(); i++)
		{
			rage::grcTexture* removedTex = removedTxd->Dict->GetValueAt(i);
			rage::grcTexture* missingTex = TxdAsset::CreateMissingTexture(removedTex->GetName());
			IterateDrawableTextures([&](rage::grcInstanceVar* var)
				{
					var->SetTexture(missingTex);
				}, removedTex->GetName());
			m_OrphanMissingTextures.Insert(removedTex->GetName(), missingTex);
		}

		AM_DEBUGF(L"HotDrawable::HandleChange() -> Removed TXD '%ls'", 
			removedTxd->Asset->GetDirectoryPath().GetCStr());

		m_TXDs.RemoveAt(pathHashValue);
		return;
	}

	// TXD was added
	if (change.Action == file::ChangeAction_Added)
	{
		AddNewTxdAsync(change.Path);
		return;
	}
}

void rageam::asset::HotDrawable::HandleChange_Scene(const file::DirectoryChange& change)
{
	// File was modified, fully reload scene
	if (change.Action == file::ChangeAction_Modified)
	{
		AM_DEBUGF("HotDrawable::HandleChange() -> Scene file was modified, reloading...");
		LoadAndCompileAsync();
		return;
	}
}

void rageam::asset::HotDrawable::HandleChange(const file::DirectoryChange& change)
{
	AM_DEBUGF(L"HotDrawable::HandleChange() -> '%hs' in '%ls'", Enum::GetName(change.Action), change.Path.GetCStr());

	// Loaded asset directory (the .idr one) was changed
	if (change.Path == m_AssetPath)
	{
		return;
	}

	// Scene model file (gltf/fbx) was changed
	if (change.Path == m_Asset->GetScenePath())
	{
		HandleChange_Scene(change);
		return;
	}

	// A change was done to TXD directory itself
	if (AssetFactory::GetAssetType(change.Path) == AssetType_Txd || 
		AssetFactory::GetAssetType(change.NewPath) == AssetType_Txd)
	{
		HandleChange_Txd(change);
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
		return;
	}
}

void rageam::asset::HotDrawable::UpdateBackgroundJobs()
{
	// Pull new changes in file system (either workspace or asset directory)
	file::DirectoryChange change;
	if (m_Watcher->GetNextChange(change))
	{
		// This will only create async task
		HandleChange(change);
	}

	// See if any background task has finished
	List<u32> indicesToRemove; // Finished tasks to remove
	for(u32 i = 0; i < m_Tasks.GetSize(); i++)
	{
		const BackgroundTaskPtr& task = m_Tasks[i];

		// Task is not finished? We'll check again next update...
		if (task->IsFinished())
			continue;

		// Finish up the task, apply changes...
		task->UserDelegate();

		// In reversed order, to prevent element shifting on deletion (last -> first)
		indicesToRemove.Insert(0, i);
	}

	// Clean up finished tasks
	for (u32 index : indicesToRemove)
		m_Tasks.RemoveAt(index);
}

void rageam::asset::HotDrawable::CheckIfDrawableHasCompiled()
{
	// Check if drawable is done compiling...
	if (!m_LoadingTask || !m_LoadingTask->IsFinished())
		return;

	// Will be destroyed on return
	BackgroundTaskPtr loadingTask = std::move(m_LoadingTask);
	if (!loadingTask->IsSuccess())
		return;

	m_HotFlags |= AssetHotFlags_DrawableCompiled;

	amPtr<CompileDrawableResult>& compileResult = loadingTask->GetResult<amPtr<CompileDrawableResult>>();
	m_CompiledDrawable = compileResult->Drawable;
	// Update asset from copy
	m_Asset->SetScene(compileResult->Scene);
	m_Asset->CompiledDrawableMap = std::move(compileResult->Map);

	// Add embed dictionary to TXDs
	if (m_Asset->HasEmbedTXD())
	{
		const amPtr<TxdAsset>&               embedTxdAsset = m_Asset->GetEmbedDictionary();
		const rage::grcTextureDictionaryPtr& embedTxd =
				m_CompiledDrawable->GetShaderGroup()->GetEmbedTextureDictionary();

		m_TXDs.Emplace(DrawableTxd(embedTxdAsset, embedTxd, true));
	}

	// Add workspace dictionaries to TXDs
	// There might be not workspace... if drawable is not in workspace
	if (m_Asset->WorkspaceTXD)
	{
		AM_DEBUGF("HotDrawable::UpdateAndApplyChanges() -> %u TXDs in workspace",
		          m_Asset->WorkspaceTXD->GetTexDictCount());

		// Compile all workspace TXDs
		for (u16 i = 0; i < m_Asset->WorkspaceTXD->GetTexDictCount(); i++)
		{
			const amPtr<TxdAsset>& txdAsset = m_Asset->WorkspaceTXD->GetTexDict(i);
			CompileTxdAsync(&m_TXDs.Emplace(DrawableTxd(txdAsset)));
		}
	}
}

rageam::asset::HotDrawable::HotDrawable(ConstWString assetPath)
{
	m_AssetPath = assetPath;
}

void rageam::asset::HotDrawable::LoadAndCompileAsync(bool keepAsset)
{
	// Wait for existing background stuff to finish...
	// We may want cancellation token here
	BackgroundWorker::WaitFor(m_Tasks);
	if (m_LoadingTask) m_LoadingTask->Wait();

	m_LoadingTask = nullptr;
	m_CompiledDrawable = nullptr;
	m_TXDs.Destroy();
	m_Tasks.Destroy();

	if (!m_Asset || !keepAsset)
	{
		m_Asset = AssetFactory::LoadFromPath<DrawableAsset>(m_AssetPath);
		m_Asset->CompileCallback = CompileCallback;

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
		m_Watcher = std::make_unique<file::Watcher>(watchPath, WATCHER_NOTIFY_FLAGS, file::WatcherFlags_Recurse, 0);
	}

	// NOTE: We're copying asset here because fields like CompiledDrawableMap cannot be shared safely
	// both by update and worker thread at the same time
	amPtr assetCopy = std::make_shared<DrawableAsset>(*m_Asset);

	m_LoadingTask = BackgroundWorker::Run([assetCopy]
		{
			amPtr<gtaDrawable> drawable = std::make_shared<gtaDrawable>();
			if (!assetCopy->CompileToGame(drawable.get()))
			{
				AM_ERRF("HotDrawable::LoadAndCompileAsync() -> Failed to compile new scene...");
				return false;
			}

			amPtr result = std::make_shared<CompileDrawableResult>(
				drawable, assetCopy->GetScene(), std::move(assetCopy->CompiledDrawableMap));

			BackgroundWorker::SetCurrentResult(result);
			return true;
		});
}

rageam::asset::HotDrawableInfo rageam::asset::HotDrawable::UpdateAndApplyChanges()
{
	m_HotFlags = AssetHotFlags_None;

	CheckIfDrawableHasCompiled();
	UpdateBackgroundJobs();

	HotDrawableInfo info;
	info.IsLoading = m_LoadingTask != nullptr;
	info.Drawable = m_CompiledDrawable;
	info.DrawableAsset = m_Asset;
	info.HotFlags = m_HotFlags;
	info.TXDs = &m_TXDs;
	return info;
}
