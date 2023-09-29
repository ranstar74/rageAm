//
// File: txd.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/asset/gameasset.h"
#include "am/graphics/texture/compressor.h"
#include "rage/grcore/texture/texturedict.h"
#include "rage/atl/set.h"

namespace rageam::asset
{
	// Version History:
	// 0: Initial

	struct TextureOptions : IXml
	{
		XML_DEFINE(TextureOptions);

		texture::CompressorQuality	Quality = texture::COMPRESSOR_QUALITY_NORMAL;
		texture::ResizeFilter		ResizeFilter = texture::RESIZE_FILTER_KRAISER;
		texture::MipFilter			MipFilter = texture::MIP_FILTER_KRAISER;
		DXGI_FORMAT					Format = texture::TEXTURE_FORMAT_AUTO;
		u32							MaxSize = texture::MAX_RESOLUTION;
		bool						GenerateMips = true;

		void GetCompressionOptions(texture::CompressionOptions& outOptions) const;

		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;
	};

	struct Texture : AssetSource
	{

		static constexpr int MAX_NAME = 128;

		// This is actual name that will be used in game after export, contains no extension and unicode characters
		char Name[MAX_NAME];
		TextureOptions Options;

		bool IsPreCompressed; // For .dds we just use pixel data as-is without re-compressing

		Texture(AssetBase* parent, ConstWString fileName) : AssetSource(parent, fileName)
		{
			// TxdAsset::Refresh() will ensure that name has no unicode characters and conversion is valid
			file::GetFileNameWithoutExtension(Name, MAX_NAME, String::ToAnsiTemp(fileName));

			IsPreCompressed = String::Equals(file::GetExtension(fileName), L"dds", true);
		}

		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;
	};
	struct TextureHashFn { u32 operator()(const Texture& texture) const { return texture.GetHashKey(); } };
	using Textures = rage::atSet<Texture, TextureHashFn>;

	/**
	 * \brief Texture Dictionary
	 * \remarks Resource Info: Extension: "YTD", Version: "13"
	 */
	class TxdAsset : public GameRscAsset<rage::grcTextureDictionary>
	{
		Textures m_Textures;

	public:
		TxdAsset(const file::WPath& path) : GameRscAsset(path) {}

		bool CompileToGame(rage::pgDictionary<rage::grcTextureDX11>* ppOutGameFormat) override;
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

		Textures& GetTextures() { return m_Textures; }
		u32 GetTextureCount() const { return m_Textures.GetNumUsedSlots(); }
	};
	using TxdAssetPtr = amPtr<TxdAsset>;
}
