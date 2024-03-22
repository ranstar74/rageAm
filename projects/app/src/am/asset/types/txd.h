//
// File: txd.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/asset/gameasset.h"
#include "am/graphics/image/bc.h"
#include "rage/grcore/txd.h"

namespace rageam::asset
{
	// Maximum num of texture compressing in background in parallel
	static constexpr int ASSET_TXD_BACKGROUND_THREADS_MAX = 12;

	// Version History:
	// 0: Initial

	struct TexturePreset;
	class TxdAsset;

	struct TextureOptions : IXml
	{
		XML_DEFINE(TextureOptions);

		graphics::ImageCompressorOptions CompressorOptions;

		TextureOptions() = default;
		TextureOptions(const TextureOptions&) = default;

		void SerializeChanged(const XmlHandle& node, const TextureOptions& options) const;
		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;

		bool operator==(const TextureOptions&) const = default;
		TextureOptions& operator=(const TextureOptions&) = default;
	};

	struct TextureTune : AssetSource
	{
		// By default, options are parsed from template and not saved to tune file
		Nullable<TextureOptions>	Options;
		// Empty string for unspecified
		string						OverridePreset;

		TextureTune(AssetBase* parent, ConstWString fileName);
		TextureTune(const TextureTune&) = default;

		TxdAsset*            GetTXD() const;
		TextureOptions&      GetCustomOptionsOrFromPreset(amPtr<TexturePreset>* outPreset = nullptr);
		amPtr<TexturePreset> MatchPreset() const;
		bool				 GetValidatedTextureName(file::Path& name, bool showWarningMessage = true) const;

		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;

		TextureTune& operator=(const TextureTune&) = default;
	};
	using Textures = List<TextureTune>;

	/**
	 * \brief Texture Dictionary
	 * \remarks Resource Info: Extension: "YTD", Version: "13"
	 */
	class TxdAsset : public GameRscAsset<rage::grcTextureDictionary>
	{
		// Pink-black checker
		static inline rage::grcTexture* sm_MissingTexture = nullptr;
		// Reference to sm_MissingTexture
		static inline rage::grcTexture* sm_NoneTexture = nullptr;

		Textures m_TextureTunes;

	public:
		TxdAsset(const file::WPath& path);

		bool CompileToGame(rage::grcTextureDictionary* object) override;
		void ParseFromGame(rage::grcTextureDictionary* object) override;
		void Refresh() override;

		ConstWString GetXmlName()			const override { return L"TextureDictionary"; }
		ConstWString GetCompileExtension()	const override { return L"ytd"; }
		u32 GetFormatVersion()				const override { return 0; }
		u32 GetResourceVersion()			const override { return 13; }

		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;

		eAssetType GetType() const override { return AssetType_Txd; }

		ASSET_IMPLEMENT_ALLOCATE(TxdAsset);

		// ---------- Asset Related ----------

		TextureTune& AddTune(ConstWString filePath);
		Textures& GetTextureTunes() { return m_TextureTunes; }
		u32 GetTextureTuneCount() const { return m_TextureTunes.GetSize(); }

		// Gets error-checked texture name from absolute/relative texture file path
		static bool GetValidatedTextureName(const file::WPath& texturePath, file::Path& outName, bool showWarningMessage = true);

		bool ContainsTextureWithName(ConstString name) const;

		TextureTune* TryFindTuneFromPath(ConstWString path) const;

		// storeData is passed in grcTexture constructor, copies pixel data to local RAM storage
		// not needed in all cases except for compiling in resource binary
		// NOTE: Texture must be either deleted manually via operator delete or wrapped in pgPtr!
		// outPreset will be set to used preset, if any
		rage::grcTexture* CompileSingleTexture(
			TextureTune& tune, bool storeData = false, amPtr<TexturePreset>* outPreset = nullptr) const;

		// Verifies that texture name has no non-ascii symbols because
		// they can't be converted into const char* and user will have issues later
		static bool ValidateTextureName(ConstWString fileName, bool showWarningMessage = true);
		static bool IsSupportedImageFile(ConstWString texturePath);

		static void ShutdownClass()
		{
			delete sm_NoneTexture;
			delete sm_MissingTexture;
			sm_MissingTexture = nullptr;
			sm_NoneTexture = nullptr;
		}

		static file::WPath GetTxdAssetPathFromTexture(const file::WPath& texturePath);
		static bool GetTxdAssetPathFromTexture(const file::WPath& texturePath, file::WPath& path);
		static bool IsTextureInTxdAssetDirectory(const file::WPath& texturePath);

		// (None)###NONE
		static rage::grcTexture* GetNoneTexture();
		static bool              IsNoneTexture(const rage::grcTexture* texture);
		// Missing texture name is composed in format:
		// TEX_NAME (Missing)###$MT_TEXNAME
		// ## is ImGui special char sequence, everything past ## is ignored visually but used as ID
		// Actual texture name is stored after '$MT_' prefix
		static rage::grcTexture* CreateMissingTexture(ConstString textureName);
		// Gets whether given texture was created using CreateMissingTexture function
		static bool IsMissingTexture(const rage::grcTexture* texture);
		// Gets actual name from missing texture name (which is in format 'TEX_NAME (Missing)###$MT_TEXNAME')
		// If texture is not missing, returns just texture name
		static ConstString UndecorateMissingTextureName(const rage::grcTexture* texture);
		static ConstString UndecorateMissingTextureName(ConstString textureName);
		static void        SetMissingTextureName(rage::grcTexture* texture, ConstString textureName);

		// When texture fails to compress, instead of failing, uses missing texture instead
		static inline thread_local bool UseMissingTexturesInsteadOfFailing = false;

		static constexpr ConstString MISSING_TEXTURE_NAME = "(None)##NONE";
	};
	using TxdAssetPtr = amPtr<TxdAsset>;

	struct TxdAssetHashFn
	{
		u32 operator()(const TxdAssetPtr& txd) const { return txd->GetHashKey(); }
	};
}
