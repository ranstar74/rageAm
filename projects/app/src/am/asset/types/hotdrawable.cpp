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

void rageam::asset::HotDrawable::RemoveFromMissingOrphans(ConstString textureName) const
{
	// For safety reasons the move function throws exception if index is invalid, so we have to check first...
	int index = m_OrphanMissingTextures->IndexOf(textureName);
	if (index == -1) // Already removed...
		return;

	rage::grcTexture* texture = m_OrphanMissingTextures->MoveFrom(index);
	AddTextureToDeleteList(texture);
}

void rageam::asset::HotDrawable::AddTextureToDeleteList(rage::grcTexture* texture) const
{
	if (!texture)
		return;

	std::unique_lock lock(sm_TexturesToRemoveMutex);
	TextureToRemove textureToRemove(amUPtr<rage::grcTexture>(texture), ImGui::GetFrameCount());
	sm_TexturesToRemove.Emplace(std::move(textureToRemove));
}

void rageam::asset::HotDrawable::AddTxdTexturesToDeleteList(const rage::grcTextureDictionaryPtr& dict) const
{
	if (!dict)
		return;

	std::unique_lock lock(sm_TexturesToRemoveMutex);
	u16 dictSize = dict->GetSize();
	for (u16 i = 0; i < dictSize; i++)
	{
		// NOTE: We're using index 0 here because Move function removes the texture and shifts all elements,
		// we can't use i for indexing here
		rage::grcTexture* texture = dict->MoveFrom(0);
		TextureToRemove   textureToRemove(amUPtr<rage::grcTexture>(texture), ImGui::GetFrameCount());
		sm_TexturesToRemove.Emplace(std::move(textureToRemove));
	}
}

void rageam::asset::HotDrawable::AddNewTxdAsync(const file::WPath& path)
{
	bool isEmbed = m_Asset->GetEmbedDictionaryPath() == path;
	bool isPlacedInDrawable = path.GetParentDirectory() == m_Asset->GetDirectoryPath();
	if (!isEmbed && isPlacedInDrawable)
	{
		AM_ERRF(
			"HotDrawable::AddNewTxdAsync() -> Embed TXD in drawable director must be called '%ls%ls'",
			EMBED_DICT_NAME, ASSET_ITD_EXT);
		return;
	}

	// External TXD must be placed in workspace
	if (!Workspace::IsInWorkspace(path) && !isEmbed)
	{
		AM_ERRF(
			"HotDrawable::AddNewTxdAsync() -> Added TXD is not placed in workspace directory");
		return;
	}

	TxdAssetPtr asset = AssetFactory::LoadFromPath<TxdAsset>(path, true /* Support hot reload */);
	if (!asset)
	{
		AM_ERRF("HotDrawable::AddNewTxdAsync() -> Failed to load TXD asset.");
		return;
	}

	// We're using raw Hash function here to emplace because asset is not set yet,
	// this must be synced with DrawableTxdHashFn...
	DrawableTxd txd(asset);
	txd.IsEmbed = isEmbed;
	CompileTxdAsync(&m_TXDs.Emplace(std::move(txd)));

	m_HotFlags |= AssetHotFlags_TxdModified;
}

void rageam::asset::HotDrawable::CompileTxdAsync(DrawableTxd* txd)
{
	AM_ASSERTS(txd);

	// We must preserve old dictionary because it still might be referenced
	// We can destroy it right now but this would cause micro-flickering
	//  during the frames when textures are compiling
	rage::grcTextureDictionaryPtr oldDict = std::move(txd->Dict);

	AM_DEBUGF("HotDrawable::CompileTxdAsync() -> Old dict ptr: %p", oldDict.Get());

	// Reload the asset, we use temporary config (if it exists) to sync changes in real-time
	// with the TXD editor, so user can preview them easily
	file::WPath assetPath = txd->Asset->GetDirectoryPath();
	txd->Asset = AssetFactory::LoadFromPath<TxdAsset>(assetPath, true /* Support hot reload */);
	if (!txd->Asset)
	{
		AM_ERRF("HotDrawable::CompileTxdAsync() -> Failed to load TXD asset.");
		DeleteTxdAndRemoveFromTheList(*txd);
		return;
	}

	BackgroundTaskPtr task = BackgroundWorker::Run([txd]
	{
		return txd->TryCompile();
	});

	// This will be executed in UpdateBackgroundJobs
	task->UserData = TaskID_CompileTxd;
	task->UserDelegate = [this, txd, oldDict]
	{
		// TXD failed to compile... get rid of the old one completely
		if (!txd->Dict)
		{
			// In UnlinkTxdFromDrawable we need Dict to be set to the old one
			txd->Dict = std::move(oldDict);
			DeleteTxdAndRemoveFromTheList(*txd);
			return;
		}

		AM_DEBUGF(L"HotDrawable::AddNewTxdAsync() -> Compiled TXD (IsEmbed: %i, Ptr: %p) '%ls'",
		          txd->IsEmbed, txd->Dict.Get(), txd->Asset->GetDirectoryPath().GetCStr());

		// Try to resolve missing textures if compiled successfully
		for (u16 i = 0; i < txd->Dict->GetSize(); i++)
		{
			rage::grcTexture* addedTexture = txd->Dict->GetValueAt(i);
			ConstString textureName = addedTexture->GetName();

			ReplaceDrawableTexture(textureName, addedTexture);
			RemoveFromMissingOrphans(textureName);
		}

		// We must remove all textures that are not in new dictionary
		if (oldDict)
		{
			for (u16 i = 0; i < oldDict->GetSize(); i++)
			{
				auto pair = oldDict->GetAt(i);
				if (!txd->Dict->Contains(pair.Key)) // New txd does not contain one of old textures?
				{
					// Texture is not in new dictionary, we must create missing texture...
					MarkTextureAsMissingOrphan(pair.Value->GetName());
				}
			}

			AddTxdTexturesToDeleteList(oldDict);
		}

		// If compiled TXD was embed, set it to drawable
		if (txd->IsEmbed)
		{
			rage::grmShaderGroup* shaderGroup = m_CompiledDrawable->GetShaderGroup();

			// Sanity check: embed dictionary must be equal to the old one
			AM_DEBUGF("HotDrawable::AddNewTxdAsync() -> Drawable dict ptr: %p",
			          shaderGroup->GetEmbedTextureDictionary().Get());
			AM_ASSERTS(shaderGroup->GetEmbedTextureDictionary() == oldDict);

			shaderGroup->SetEmbedTextureDict(txd->Dict);
		}

		m_HotFlags |= AssetHotFlags_TxdModified;
	};

	m_AsyncTasks.Emplace(std::move(task));
}

void rageam::asset::HotDrawable::CompileTexAsync(DrawableTxd* txd, TextureTune* tune)
{
	file::Path textureName;
	tune->GetValidatedTextureName(textureName);

	// We're compiling texture in TXD, it can't be orphan
	if (IsMissingOrphanTexture(textureName))
		RemoveFromMissingOrphans(textureName);

	// We must add dummy texture first before compiling, so it instantly appears in the list
	// Also if we don't do that and texture fails to compile, MarkTextureIsMissing will
	// fail too because texture was not added yet
	if (!txd->Dict->Contains(textureName))
		// This might be true if texture is already marked as missing (failed to compile, but used)
		txd->Dict->Insert(textureName, TxdAsset::CreateMissingTexture(textureName));

	BackgroundTaskPtr task = BackgroundWorker::Run([tune, txd]
	{
		auto gameTexture = rage::pgUPtr((rage::grcTextureDX11*)txd->Asset->CompileSingleTexture(*tune));
		bool success = gameTexture != nullptr; // Check here because SetCurrentResult will move pointer
		BackgroundWorker::SetCurrentResult(gameTexture);
		if (success)
			AM_DEBUGF("HotDrawable::CompileTexAsync() -> Recompiled texture '%ls'", tune->GetName());
		else
			AM_DEBUGF("HotDrawable::CompileTexAsync() -> Failed to compile texture '%ls'", tune->GetName());
		return success;
	});

	task->UserData = TaskID_CompileTex;
	task->UserDelegate = [this, task, txd, tune]
	{
		if (task->IsSuccess())
		{
			auto& texture = task->GetResult<rage::pgUPtr<rage::grcTextureDX11>>();

			ReplaceDrawableTexture(texture->GetName(), texture.Get());

			// Make sure texture is not marked as missing anymore
			RemoveFromMissingOrphans(texture->GetName());

			// Remove existing texture in TXD
			s32 existingIndex = txd->Dict->IndexOf(texture->GetName());
			if (existingIndex != -1)
				AddTextureToDeleteList(txd->Dict->MoveFrom(existingIndex));

			// Move ownership to TXD...
			txd->Dict->Insert(texture->GetName(), texture.Get());
			texture.SuppressDelete();
		}
		else
		{
			file::Path textureName;
			// Totally ignore texture if name is so fucked up that it was the reason why compilation failed
			if (tune->GetValidatedTextureName(textureName))
			{
				MarkTextureAsMissing(textureName, *txd);
			}
		}

		m_HotFlags |= AssetHotFlags_TxdModified;
	};

	m_AsyncTasks.Emplace(std::move(task));
}

void rageam::asset::HotDrawable::MarkTextureAsMissingOrphan(ConstString textureName) const
{
	if (IsMissingOrphanTexture(textureName))
		return;

	if (!IsTextureUsedInDrawable(textureName))
		return;

	rage::grcTexture* missingTexture = TxdAsset::CreateMissingTexture(textureName);
	m_OrphanMissingTextures->Insert(textureName, missingTexture);

	ReplaceDrawableTexture(textureName, missingTexture);
}

void rageam::asset::HotDrawable::DeleteTxdAndRemoveFromTheList(const DrawableTxd& txd)
{
	// Make sure that drawable does not reference TXD textures anymore by
	// replacing them to missing ones, if they were used
	if (txd.Dict)
	{
		for (u16 i = 0; i < txd.Dict->GetSize(); i++)
		{
			rage::grcTexture* texture = txd.Dict->GetValueAt(i);

			// Texture might be missing in TXD, undecorated the name
			ConstString textureName = TxdAsset::UndecorateMissingTextureName(texture);
			MarkTextureAsMissingOrphan(textureName);

			// MarkTextureAsMissingOrphan(texture->GetName());
		}

		AddTxdTexturesToDeleteList(txd.Dict);
	}

	// Embed dict was removed, unlink it from drawable
	if (txd.IsEmbed)
	{
		rage::grmShaderGroup* shaderGroup = m_CompiledDrawable->GetShaderGroup();

		// Sanity check: there can be only one embed dictionary and it must match
		AM_DEBUGF("HotDrawable::DeleteTxdAndRemoveFromTheList() -> Drawable dict ptr: %p",
		          shaderGroup->GetEmbedTextureDictionary().Get());
		AM_ASSERTS(txd.Dict == shaderGroup->GetEmbedTextureDictionary());

		shaderGroup->SetEmbedTextureDict(nullptr);
	}
	m_TXDs.Remove(txd);
}

void rageam::asset::HotDrawable::DeleteTxdAndRemoveFromTheList(ConstWString path)
{
	DrawableTxd* txd = m_TXDs.TryGetAt(Hash(path));
	if (!txd) // Txd already was removed...
	{
		AM_DEBUGF("HotDrawable::DeleteTxdAndRemoveFromTheList() -> TXD was already removed '%ls'", path);
		return;
	}
	DeleteTxdAndRemoveFromTheList(*txd);
}

void rageam::asset::HotDrawable::MarkTextureAsMissing(ConstString textureName, const DrawableTxd& txd) const
{
	rage::grcTexture* missingTexture = TxdAsset::CreateMissingTexture(textureName);
	ReplaceDrawableTexture(textureName, missingTexture);
	AddTextureToDeleteList(txd.Dict->Move(textureName));
	txd.Dict->Insert(textureName, missingTexture);
}

void rageam::asset::HotDrawable::MarkTextureAsMissing(const TextureTune& textureTune, const DrawableTxd& txd) const
{
	file::Path textureName;
	if (textureTune.GetValidatedTextureName(textureName))
		MarkTextureAsMissing(textureName, txd);
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

void rageam::asset::HotDrawable::HandleChange_Texture(const file::DirectoryChange& change)
{
	m_HotFlags |= AssetHotFlags_TxdModified;

	file::WPath txdPath = TxdAsset::GetTxdAssetPathFromTexture(change.Path);
	DrawableTxd* txd = m_TXDs.TryGetAt(Hash(txdPath));

	// Dictionary wasn't registered yet... this can happen only with embed one
	if (!txd)
	{
		AM_DEBUGF(
			"HotDrawable::HandleChange() -> Texture was changed in not registered yet dictionary, adding new txd...");
		AddNewTxdAsync(txdPath);
		return;
	}

	// Dict failed to compile before, try to compile it again
	if (!txd->Dict)
	{
		AM_DEBUGF(
			"HotDrawable::HandleChange() -> Texture was changed in dictionary that wasn't compiled, trying to compile...");
		CompileTxdAsync(txd);
		return;
	}

	// Weird-named file was renamed into valid texture...
	if (!TxdAsset::IsSupportedImageFile(change.Path) /* New path is valid ... */)
	{
		TextureTune* tune = &txd->Asset->AddTune(change.NewPath);
		CompileTexAsync(txd, tune);
		return;
	}

	// Find the changed texture in asset
	TextureTune* textureTune = txd->Asset->TryFindTuneFromPath(change.Path);
	if (!textureTune)
	{
		textureTune = &txd->Asset->AddTune(change.Path);
		AM_DEBUGF(L"HotDrawable::HandleChange() -> Added texture tune '%ls' in TXD '%ls'",
		          textureTune->GetName(), txd->Asset->GetDirectoryPath().GetCStr());

		if (change.Action == file::ChangeAction_Removed)
		{
			// We must keep tune but since it was just added, it is already 'removed'.
			// (only tune exists, but not compiled texture in dictionary)
			return;
		}
	}

	file::Path textureName;
	if (!textureTune->GetValidatedTextureName(textureName))
		return;

	if (change.Action == file::ChangeAction_Added || 
		change.Action == file::ChangeAction_Modified)
	{
		CompileTexAsync(txd, textureTune);
		return;
	}

	if (change.Action == file::ChangeAction_Removed)
	{
		MarkTextureAsMissingOrphan(textureName);
		AddTextureToDeleteList(txd->Dict->Move(textureName));
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

		MarkTextureAsMissingOrphan(textureName);

		// Renamed to invalid extension, delete the texture
		if (!TxdAsset::IsSupportedImageFile(change.NewPath))
		{
			AddTextureToDeleteList(txd->Dict->Move(textureName));
			return;
		}

		// Rename tune and get new texture name
		textureTune->SetFilePath(change.NewPath);
		file::Path newTextureName;
		if (!textureTune->GetValidatedTextureName(newTextureName))
			return;

		// Rename texture 
		rage::grcTexture* texture = txd->Dict->Move(textureName);
		texture->SetName(newTextureName);
		txd->Dict->Insert(newTextureName, texture);

		return;
	}
}

void rageam::asset::HotDrawable::HandleChange_Txd(const file::DirectoryChange& change)
{
	m_HotFlags |= AssetHotFlags_TxdModified;

	// TXD was added
	if (change.Action == file::ChangeAction_Added)
	{
		AddNewTxdAsync(change.Path);
		return;
	}

	// TXD was renamed
	if (change.Action == file::ChangeAction_Renamed)
	{
		HashValue pathHashValue = Hash(change.Path);

		// Embed dictionary was renamed to wrong name, it must be called EMBED_DICT_NAME
		bool isEmbed = m_Asset->GetEmbedDictionaryPath() == change.Path;
		bool isNewEmbed = m_Asset->GetEmbedDictionaryPath() == change.NewPath;
		if (isEmbed && !isNewEmbed)
		{
			DeleteTxdAndRemoveFromTheList(change.Path);
			AM_ERRF(
				"HotDrawable::HandleChange() -> Embed TXD in drawable director must be called '%ls%ls'",
				EMBED_DICT_NAME, ASSET_ITD_EXT);
			return;
		}

		// Retrieve TXD from storage if it exists, in case if TXD was added it won't yet exist in cache
		DrawableTxd* txd = m_TXDs.TryGetAt(pathHashValue);

		// Non .itd was renamed in .itd
		if (!txd && AssetFactory::GetAssetType(change.NewPath) == AssetType_Txd)
		{
			AM_TRACEF("HotDrawable::HandleChange() -> TXD doesn't exist, adding new...");
			AddNewTxdAsync(change.NewPath);
		}
		// .itd name was changed, reinsert it with new path
		else
		{
			AM_TRACEF("HotDrawable::HandleChange() -> Renaming existing TXD...");
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
		AM_DEBUGF(L"HotDrawable::HandleChange() -> Removed TXD '%ls'", change.Path.GetCStr());
		DeleteTxdAndRemoveFromTheList(change.Path);
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

	// TXD config was changed
	ImmutableWString pathStr = change.Path.GetCStr();
	bool isConfig = pathStr.EndsWith(ASSET_CONFIG_NAME);
	bool isTempConfig = !isConfig && pathStr.EndsWith(ASSET_CONFIG_NAME_TEMP);
	if (isConfig || isTempConfig)
	{
		file::WPath txdPath = change.Path.GetParentDirectory();
		if (AssetFactory::GetAssetType(txdPath) == AssetType_Txd)
		{
			DrawableTxd* txd = m_TXDs.TryGetAt(Hash(txdPath));
			if (txd)
				CompileTxdAsync(txd);
			else
				AddNewTxdAsync(txdPath);
			AM_DEBUGF(L"HotDrawable::HandleChange() -> Config changed in '%ls'", txdPath.GetCStr());
		}
		return;
	}

	// Texture was added/changed/deleted in existing TXD
	// To properly update texture we have to keep track of the:
	// - Drawable shader group variables (they directly reference grcTexture)
	// - Texture in the dictionary itself
	// - Texture tune in TXD asset
	file::WPath textureTxdPath;
	if (TxdAsset::GetTxdAssetPathFromTexture(change.Path, textureTxdPath) &&
		(TxdAsset::IsSupportedImageFile(change.Path) || TxdAsset::IsSupportedImageFile(change.NewPath)))
	{
		HandleChange_Texture(change);
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

void rageam::asset::HotDrawable::CheckIfDrawableHasCompiled()
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
	m_Asset->CompiledDrawableMap = std::move(compileResult->Map);

	// Add embed dictionary to TXDs
	if (m_Asset->HasEmbedTXD())
	{
		const amPtr<TxdAsset>& embedTxdAsset = m_Asset->GetEmbedDictionary();
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

	m_HotFlags |= AssetHotFlags_DrawableCompiled;
}

rageam::asset::HotDrawable::HotDrawable(ConstWString assetPath)
{
	m_AssetPath = assetPath;
	m_OrphanMissingTextures = rage::pgPtr(new rage::grcTextureDictionary());
}

void rageam::asset::HotDrawable::LoadAndCompileAsync(bool keepAsset)
{
	// Wait for existing background stuff to finish...
	// We may want cancellation token here
	BackgroundWorker::WaitFor(m_AsyncTasks);
	if (m_LoadingTask) m_LoadingTask->Wait();

	m_LoadingTask = nullptr;
	m_CompiledDrawable = nullptr;
	m_TXDs.Destroy();
	m_AsyncTasks.Destroy();

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
		m_Watcher = std::make_unique<file::Watcher>(watchPath, WATCHER_NOTIFY_FLAGS, file::WatcherFlags_Recurse);
	}

	// NOTE: We're copying asset here because fields like CompiledDrawableMap cannot be shared safely
	// both by update and worker thread at the same time
	amPtr assetCopy = std::make_shared<DrawableAsset>(*m_Asset);

	m_LoadingTask = BackgroundWorker::Run([assetCopy]
	{
		gtaDrawablePtr drawable = rage::pgCountedPtr(new gtaDrawable());
		if (!assetCopy->CompileToGame(drawable.Get()))
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

void rageam::asset::HotDrawable::NotifyTextureWasUnselected(const rage::grcTexture* texture) const
{
	if (!texture)
		return;

	// Texture is not used anymore in drawable... we don't need to keep it as missing anymore
	ConstString textureName = TxdAsset::UndecorateMissingTextureName(texture);
	if (!IsTextureUsedInDrawable(textureName))
		RemoveFromMissingOrphans(textureName);
}

rageam::asset::HotDrawableInfo rageam::asset::HotDrawable::UpdateAndApplyChanges()
{
	m_HotFlags = AssetHotFlags_None;

	UpdateBackgroundJobs();
	CheckIfDrawableHasCompiled();

	HotDrawableInfo info;
	info.IsLoading = m_LoadingTask != nullptr;
	info.Drawable = m_CompiledDrawable;
	info.DrawableAsset = m_Asset;
	info.HotFlags = m_HotFlags;
	info.TXDs = &m_TXDs;
	return info;
}

rage::grcTexture* rageam::asset::HotDrawable::LookupTexture(ConstString name) const
{
	if (String::IsNullOrEmpty(name))
		return nullptr;

	rage::atHashValue nameHash = rage::atStringHash(name);

	for (DrawableTxd& txd : m_TXDs)
	{
		if (!txd.Dict)
			continue;

		s32 index = txd.Dict->IndexOf(nameHash);
		if (index != -1)
			return txd.Dict->GetValueAt(index);
	}

	s32 orphanIndex = m_OrphanMissingTextures->IndexOf(nameHash);
	if (orphanIndex != -1)
		return m_OrphanMissingTextures->GetValueAt(orphanIndex);

	return nullptr;
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
