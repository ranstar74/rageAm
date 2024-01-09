//
// File: texpresets.h
//
// Copyright (C) 2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "txd.h"
#include "am/types.h"
#include "am/system/singleton.h"
#include "am/xml/serialize.h"

namespace rageam::asset
{
	// Texture presets come handy when we deal with really a lot of textures,
	// ideally user should configure all project textures using only presets,
	// it is very scalable and easy to configure
	// One of problem this system solves is compiling separate TXDs for different
	// hardware configurations (low resolution, no BC7 for DX10)
	// NOTE: This is very important - presets are only defined at workspace level,
	// defining them globally would cause problem of affecting all user projects at once,
	// and no whatsoever support for legacy projects
	// keeping them at workspace level makes a project fully self-contained
	// The only two global presets we have are default system ones,
	// and ideally they should never be changed, there's no reason to

	enum class TexturePresetRule
	{
		Default,		// For default preset, we should have only 2 of them - raw and compressed
		NameContains,
		NameBegins,
		NameEnds,
	};

	struct TexturePreset : IXml
	{
		string				Name;
		TexturePresetRule	Rule;
		wstring				RuleArgument;
		TextureOptions		Options;

		TexturePreset() = default;

		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;

		bool Match(ConstWString textureName, bool canCompressTexture);
	};
	using TexturePresetPtr = amPtr<TexturePreset>;

	struct TexturePresetCollection
	{
		List<TexturePresetPtr>	Items;
		HashSet<u32>			NameToIndex; // Maps preset name to index in Items

		bool LoadFrom(ConstWString path);
		void SaveTo(ConstWString path) const;

		// Must be called once collection is changed
		void Seal();

		TexturePresetPtr Lookup(ConstString presetName, bool canCompressTexture) const;
		TexturePresetPtr Match(ConstWString textureName, bool canCompressTexture) const;
	};

	using TexturePresetCallback = std::function<void(ConstWString workspacePath, HashValue workspacePathHash)>;

	class TexturePresetStore : public Singleton<TexturePresetStore>
	{
		static constexpr ConstWString WS_PRESETS_FILE_NAME = L"texture_presets.xml";

		struct WorkspacePresets
		{
			TexturePresetCollection		Presets;
			u64							FileModifyTime;
		};

		TexturePresetPtr				m_DefaultCompressed;
		TexturePresetPtr				m_DefaultRaw;
		HashSet<WorkspacePresets>		m_WorkspacePresets;
		mutable std::recursive_mutex	m_Mutex;
		mutable std::mutex				m_CallbackMutex;
		// We use pool because we need a way to unsubscribe callbacks
		Pool<TexturePresetCallback>		m_ModifiedCallbacks;

	public:
		TexturePresetStore();

		// Locks MatchPreset function until Unlock is called, to edit presets safely
		// (store is mostly used from background threads to compress textures)
		void Lock() const { m_Mutex.lock(); }
		void Unlock() const { m_Mutex.unlock(); }

		file::WPath GetPresetsPathFromWorkspace(ConstWString workspacePath) const;

		// After editing presets (for example in UI preset wizard),
		// we must notify cache system that in memory copy of presets match file version
		// (otherwise file will be reloaded, wasted resources...)
		void MarkPresetsMatchFile(ConstWString workspacePath) const;
		// Returns empty collection in case if workspace has no presets,
		// loaded presets are internally cached and reloaded automatically if file was modified
		TexturePresetCollection& GetPresets(ConstWString workspacePath);
		// Looks up matching template for a texture from workspace (if it's even located in a workspace)
		TexturePresetPtr MatchPreset(const TextureTune& texture, ConstString overrideHint = nullptr);

		void NotifyPresetsWereModified(ConstWString workspacePath) const;
		// We want to recompress our textures in case if presets were modified
		PoolIndex AddPresetsModifiedCallback(const TexturePresetCallback& callback);
		void RemovePresetsModifiedCallback(PoolIndex);
	};
}
