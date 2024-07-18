#include "texpresets.h"

#include "am/system/datetime.h"
#include "am/xml/doc.h"
#include "am/xml/iterator.h"

void rageam::asset::TexturePreset::Serialize(XmlHandle& node) const
{
	XML_SET_CHILD_VALUE_ATTR(node, Name);
	XML_SET_CHILD_VALUE_ATTR(node, Rule);
	XML_SET_CHILD_VALUE_ATTR(node, RuleArgument);
	XmlHandle xOptions = node.AddChild("Options");
	Options.Serialize(xOptions);
}

void rageam::asset::TexturePreset::Deserialize(const XmlHandle& node)
{
	XML_GET_CHILD_VALUE_ATTR(node, Name);
	XML_GET_CHILD_VALUE_ATTR(node, Rule);
	if (Rule != TexturePresetRule::Default)
		XML_GET_CHILD_VALUE_ATTR(node, RuleArgument);
	XmlHandle xOptions = node.GetChild("Options");
	Options.Deserialize(xOptions);
}

bool rageam::asset::TexturePreset::Match(ConstWString textureName, bool canCompressTexture)
{
	// Preset uses block compressed format, not compatible
	if (!canCompressTexture && Options.CompressorOptions.Format != graphics::BlockFormat_None)
		return false;

	ImmutableWString textureNameView(textureName);
	switch (Rule)
	{
	case TexturePresetRule::Default:		return true;
	case TexturePresetRule::NameContains:	return textureNameView.Contains(RuleArgument, true);
	case TexturePresetRule::NameBegins:		return textureNameView.StartsWith(RuleArgument, true);
	case TexturePresetRule::NameEnds:		return textureNameView.EndsWith(RuleArgument, true);
	default:
		AM_UNREACHABLE("TexturePreset::MatchPreset() -> Rule '%s' is not implemented.", Enum::GetName(Rule));
	}
}

bool rageam::asset::TexturePresetCollection::LoadFrom(ConstWString path)
{
	Items.Clear();

	try
	{
		XmlDoc xDoc;
		xDoc.LoadFromFile(path);
		XmlHandle xRoot = xDoc.Root();
		for (const XmlHandle& xPreset : XmlIterator(xRoot, "Preset"))
		{
			TexturePresetPtr preset = std::make_shared<TexturePreset>();
			preset->Deserialize(xPreset);
			Items.Emplace(std::move(preset));
		}
		Seal();
	}
	catch (const XmlException& ex)
	{
		ex.Print();
		return false;
	}
	return true;
}

void rageam::asset::TexturePresetCollection::SaveTo(ConstWString path) const
{
	try
	{
		XmlDoc xDoc("TexturePresetCollection");
		XmlHandle xRoot = xDoc.Root();
		xRoot.SetAttribute("Version", 0);

		char timeBuffer[36];
		DateTime::Now().Format(timeBuffer, sizeof timeBuffer, "s");
		xRoot.SetAttribute("Timestamp", timeBuffer);

		for (const TexturePresetPtr& preset : Items)
		{
			XmlHandle xPreset = xRoot.AddChild("Preset");
			preset->Serialize(xPreset);
		}

		xDoc.SaveToFile(path);
	}
	catch (const XmlException& ex)
	{
		ex.Print();
	}
}

void rageam::asset::TexturePresetCollection::Seal()
{
	// TexturePresetRule::Default rules must be placed last! Do grouping
	List<TexturePresetPtr> sortedList;
	sortedList.Reserve(Items.GetSize());
	// First insert all non-default
	for (TexturePresetPtr& preset : Items)
		if (preset->Rule != TexturePresetRule::Default)
			sortedList.Emplace(std::move(preset));
	// Then all default
	for (TexturePresetPtr& preset : Items)
		if (preset && preset->Rule == TexturePresetRule::Default) // Extra null check because of std::move in previous loop
			sortedList.Emplace(std::move(preset));
	Items = std::move(sortedList);

	// Build hashmap for fast lookup
	NameToIndex.InitAndAllocate(Items.GetSize(), false);
	for (u32 i = 0; i < Items.GetSize(); i++)
	{
		TexturePresetPtr& preset = Items[i];
		NameToIndex.InsertAt(Hash(preset->Name), i);
	}
}

rageam::asset::TexturePresetPtr rageam::asset::TexturePresetCollection::Lookup(ConstString presetName, bool canCompressTexture) const
{
	if (String::IsNullOrEmpty(presetName))
		return nullptr;

	u32* pIndex = NameToIndex.TryGetAt(Hash(presetName));
	if (!pIndex)
	{
		AM_ERRF("TexturePresetCollection::Lookup() -> No preset with name '%s'", presetName);
		return nullptr;
	}

	const TexturePresetPtr& preset = Items[*pIndex];
	if(!canCompressTexture && preset->Options.CompressorOptions.Format != graphics::BlockFormat_None)
	{
		AM_ERRF("TexturePresetCollection::Lookup() -> Texture cannot be compressed but preset '%s' has compressed format", presetName);
		return nullptr;
	}

	return preset;
}

rageam::asset::TexturePresetPtr rageam::asset::TexturePresetCollection::Match(ConstWString textureName, bool canCompressTexture) const
{
	for (TexturePresetPtr& preset : Items)
	{
		if (preset->Match(textureName, canCompressTexture))
			return preset;
	}
	return nullptr;
}

rageam::asset::TexturePresetStore::TexturePresetStore()
{
	m_DefaultCompressed = std::make_shared<TexturePreset>();
	m_DefaultCompressed->Name = "Default Compressed";
	m_DefaultCompressed->Rule = TexturePresetRule::Default;
	m_DefaultCompressed->Options.CompressorOptions.Format = graphics::BlockFormat_BC7;
	m_DefaultCompressed->Options.CompressorOptions.Quality = 0.35f;

	m_DefaultRaw = std::make_shared<TexturePreset>();
	m_DefaultRaw->Name = "Default Raw";
	m_DefaultRaw->Rule = TexturePresetRule::Default;
	m_DefaultRaw->Options.CompressorOptions.Format = graphics::BlockFormat_None;

	m_ModifiedCallbacks.InitAndAllocate(36);
}

rageam::file::WPath rageam::asset::TexturePresetStore::GetPresetsPathFromWorkspace(ConstWString workspacePath) const
{
	return file::WPath(workspacePath) / WS_PRESETS_FILE_NAME;
}

void rageam::asset::TexturePresetStore::MarkPresetsMatchFile(ConstWString workspacePath) const
{
	std::unique_lock lock(m_Mutex);

	file::WPath presetsPath = GetPresetsPathFromWorkspace(workspacePath);
	HashValue hash = Hash(presetsPath);
	WorkspacePresets* presets = m_WorkspacePresets.TryGetAt(hash);
	if (presets)
	{
		presets->FileModifyTime = GetFileModifyTime(presetsPath);
	}
}

rageam::asset::TexturePresetCollection& rageam::asset::TexturePresetStore::GetPresets(ConstWString workspacePath)
{
	std::unique_lock lock(m_Mutex);

	static TexturePresetCollection s_EmptyCollection;

	file::WPath presetsPatch = GetPresetsPathFromWorkspace(workspacePath);
	if (!IsFileExists(presetsPatch))
		return s_EmptyCollection;

	u64 presetsModifyTime = GetFileModifyTime(presetsPatch);

	// Try to retrieve cached presets
	HashValue presetsHash = Hash(presetsPatch);
	WorkspacePresets* pWorkspacePresets = m_WorkspacePresets.TryGetAt(presetsHash);
	if (pWorkspacePresets && pWorkspacePresets->FileModifyTime == presetsModifyTime)
		return pWorkspacePresets->Presets;

	// Workspace presets either not loaded or not valid anymore (file was modified...)
	WorkspacePresets newPresets;
	newPresets.FileModifyTime = presetsModifyTime;
	if (!newPresets.Presets.LoadFrom(presetsPatch))
		return s_EmptyCollection;

	return m_WorkspacePresets.EmplaceAt(presetsHash, std::move(newPresets)).Presets;
}

rageam::asset::TexturePresetPtr rageam::asset::TexturePresetStore::MatchPreset(const TextureTune& texture, ConstString overrideHint)
{
	std::unique_lock lock(m_Mutex);

	ConstWString texturePath = texture.GetFilePath();
	file::WPath textureName = file::WPath(texturePath).GetFileNameWithoutExtension();
	bool canBeCompressed = graphics::ImageFactory::CanBlockCompressImage(texturePath);

	ConstWString workspacePath = texture.GetTXD()->GetWorkspacePath();
	TexturePresetCollection& workspacePresets = GetPresets(workspacePath);

	TexturePresetPtr overridePreset = workspacePresets.Lookup(overrideHint, canBeCompressed);
	if (overridePreset)
		return overridePreset;

	TexturePresetPtr matchedPreset= workspacePresets.Match(textureName, canBeCompressed);
	if (matchedPreset)
		return matchedPreset;

	return canBeCompressed ? m_DefaultCompressed : m_DefaultRaw;
}

void rageam::asset::TexturePresetStore::NotifyPresetsWereModified(ConstWString workspacePath) const
{
	std::unique_lock lock(m_CallbackMutex);

	HashValue workspacePathHash = Hash(workspacePath);
	for (u32 i = 0; i < m_ModifiedCallbacks.GetSize(); i++)
	{
		TexturePresetCallback* callback = m_ModifiedCallbacks.GetSlot(i);
		if (callback)
			(*callback)(workspacePath, workspacePathHash);
	}
}

rageam::PoolIndex rageam::asset::TexturePresetStore::AddPresetsModifiedCallback(const TexturePresetCallback& callback)
{
	std::unique_lock lock(m_CallbackMutex);

	TexturePresetCallback* poolItem = m_ModifiedCallbacks.New();
	*poolItem = callback;

	return m_ModifiedCallbacks.GetJustIndex(poolItem);
}

void rageam::asset::TexturePresetStore::RemovePresetsModifiedCallback(PoolIndex index)
{
	std::unique_lock lock(m_CallbackMutex);

	TexturePresetCallback* poolItem =
		static_cast<TexturePresetCallback*>(m_ModifiedCallbacks.GetSlotFromJustIndex(index));
	m_ModifiedCallbacks.Delete(poolItem);
}
