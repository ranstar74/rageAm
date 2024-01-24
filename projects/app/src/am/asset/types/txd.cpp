#include "txd.h"

#include "am/file/iterator.h"
#include "am/string/string.h"
#include "am/system/worker.h"
#include "am/xml/iterator.h"
#include "rage/grcore/texturepc.h"
#include "helpers/format.h"
#include "texpresets.h"

#include <semaphore>

void rageam::asset::TextureOptions::SerializeChanged(const XmlHandle& node, const TextureOptions& options) const
{
#define TEX_SET_IF_CHANGED(field) \
	if (options.CompressorOptions.field != CompressorOptions.field) XML_SET_CHILD_VALUE(node, CompressorOptions.field);

	TEX_SET_IF_CHANGED(Format);
	TEX_SET_IF_CHANGED(MipFilter);
	TEX_SET_IF_CHANGED(Quality);
	TEX_SET_IF_CHANGED(MaxResolution);
	TEX_SET_IF_CHANGED(GenerateMipMaps);
	TEX_SET_IF_CHANGED(CutoutAlpha);
	TEX_SET_IF_CHANGED(CutoutAlphaThreshold);
	TEX_SET_IF_CHANGED(AlphaTestCoverage);
	TEX_SET_IF_CHANGED(AlphaTestThreshold);
	TEX_SET_IF_CHANGED(AllowRecompress);

#undef TEX_SET_IF_CHANGED
}

void rageam::asset::TextureOptions::Serialize(XmlHandle& node) const
{
	XML_SET_CHILD_VALUE(node, CompressorOptions.Format);
	XML_SET_CHILD_VALUE(node, CompressorOptions.MipFilter);
	XML_SET_CHILD_VALUE(node, CompressorOptions.Quality);
	XML_SET_CHILD_VALUE(node, CompressorOptions.MaxResolution);
	XML_SET_CHILD_VALUE(node, CompressorOptions.GenerateMipMaps);
	XML_SET_CHILD_VALUE(node, CompressorOptions.CutoutAlpha);
	XML_SET_CHILD_VALUE(node, CompressorOptions.CutoutAlphaThreshold);
	XML_SET_CHILD_VALUE(node, CompressorOptions.AlphaTestCoverage);
	XML_SET_CHILD_VALUE(node, CompressorOptions.AlphaTestThreshold);
	XML_SET_CHILD_VALUE(node, CompressorOptions.AllowRecompress);
}

void rageam::asset::TextureOptions::Deserialize(const XmlHandle& node)
{
	XML_GET_CHILD_VALUE(node, CompressorOptions.Format);
	XML_GET_CHILD_VALUE(node, CompressorOptions.MipFilter);
	XML_GET_CHILD_VALUE(node, CompressorOptions.Quality);
	XML_GET_CHILD_VALUE(node, CompressorOptions.MaxResolution);
	XML_GET_CHILD_VALUE(node, CompressorOptions.GenerateMipMaps);
	XML_GET_CHILD_VALUE(node, CompressorOptions.CutoutAlpha);
	XML_GET_CHILD_VALUE(node, CompressorOptions.CutoutAlphaThreshold);
	XML_GET_CHILD_VALUE(node, CompressorOptions.AlphaTestCoverage);
	XML_GET_CHILD_VALUE(node, CompressorOptions.AlphaTestThreshold);
	XML_GET_CHILD_VALUE(node, CompressorOptions.AllowRecompress);
}

rageam::asset::TextureTune::TextureTune(AssetBase* parent, ConstWString fileName) : AssetSource(parent, fileName)
{

}

rageam::asset::TxdAsset* rageam::asset::TextureTune::GetTXD() const
{
	return reinterpret_cast<TxdAsset*>(GetParent());
}

rageam::asset::TextureOptions& rageam::asset::TextureTune::GetCustomOptionsOrFromPreset(amPtr<TexturePreset>* outPreset)
{
	// Take options either from tune (if it has them) or find matching texture preset
	if (Options.HasValue())
	{
		return Options.GetValue();
	}

	TexturePresetPtr texPreset = MatchPreset();
	if (outPreset) *outPreset = texPreset;
	return texPreset->Options;
}

rageam::asset::TexturePresetPtr rageam::asset::TextureTune::MatchPreset() const
{
	TexturePresetStore* presetStore = TexturePresetStore::GetInstance();
	return presetStore->MatchPreset(*this, OverridePreset);
}

bool rageam::asset::TextureTune::GetValidatedTextureName(file::Path& name, bool showWarningMessage) const
{
	return GetTXD()->GetValidatedTextureName(GetFilePath(), name, showWarningMessage);
}

void rageam::asset::TextureTune::Serialize(XmlHandle& node) const
{
	node.SetAttribute("OverridePreset", OverridePreset);
	if (Options.HasValue())
	{
		node.SetAttribute("HasCompressorOptions", true);
		Options.GetValue().Serialize(node);
	}
}

void rageam::asset::TextureTune::Deserialize(const XmlHandle& node)
{
	bool hasOptions;
	if (node.GetAttribute("HasCompressorOptions", hasOptions, true) && hasOptions)
	{
		TextureOptions options;
		options.Deserialize(node);
		Options = options;
	}
}

rageam::asset::TxdAsset::TxdAsset(const file::WPath& path) : GameRscAsset(path)
{

}

bool rageam::asset::TxdAsset::CompileToGame(rage::grcTextureDictionary* object)
{
	ReportProgress(L"Compressing textures", 0);

	rage::grcTextureDictionary& txd = *object;

	std::counting_semaphore<ASSET_TXD_BACKGROUND_THREADS_MAX> sema(ASSET_TXD_BACKGROUND_THREADS_MAX);
	std::mutex mutex;

	u32 textureCount = m_TextureTunes.GetSize();
	u32 texturesDoneCount = 0; // For progress reporting

	Tasks tasks;
	tasks.Reserve(textureCount);

	for (u32 i = 0; i < m_TextureTunes.GetSize(); i++)
	{
		tasks.Emplace(BackgroundWorker::Run([&, i]
			{
				TextureTune& tune = m_TextureTunes[i];

				sema.acquire();
				TexturePresetPtr usedPreset;
				rage::grcTexture* gameTexture = CompileSingleTexture(tune, true, &usedPreset);
				sema.release();

				if (!gameTexture)
					return false;

				mutex.lock();
				// For sanity check, we must ensure that there are no multiple textures with the same name
				if (txd.Contains(gameTexture->GetName()))
				{
					AM_ERRF("TxdAsset::CompileToGame() -> Found 2 textures with the same name ('%s'), this cannot continue.",
						gameTexture->GetName());
					return false;
				}

				// Insert baked texture into dictionary
				txd.Insert(gameTexture->GetName(), gameTexture);

				// Progress report
				if (CompileCallback)
				{
					ConstString presetName = usedPreset ? usedPreset->Name.GetCStr() : "-";
					texturesDoneCount++;
					double progress = static_cast<double>(texturesDoneCount) / textureCount;
					ConstWString message = String::FormatTemp(L"%i/%u %hs (size: %hs, preset: %hs)",
						texturesDoneCount,
						textureCount,
						gameTexture->GetName(),
						FormatSize(gameTexture->GetPhysicalSize()),
						presetName);
					CompileCallback(message, progress);
				}
				mutex.unlock();

				return true;
			}));
	}

	return BackgroundWorker::WaitFor(tasks);
}

void rageam::asset::TxdAsset::ParseFromGame(rage::grcTextureDictionary* object)
{
	m_TextureTunes.Clear();

	for (u16 i = 0; i < object->GetSize(); i++)
	{
		rage::grcTextureDX11* tex = (rage::grcTextureDX11*)(object->GetValueAt(i));

		pVoid pixelData = tex->GetBackingStore();
		AM_ASSERT(pixelData, "TxdAsset::ParseFromGame() -> No pixel data in the ram");

		// Format path to .dds file in asset directory
		file::WPath texPath = GetDirectoryPath() / String::ToWideTemp(tex->GetName()) + L".dds";

		AddTune(texPath);
	}
}

void rageam::asset::TxdAsset::Refresh()
{
	HashSet<u32> scannedTuneHashes;

	file::FindData entry;
	file::Iterator it(GetDirectoryPath() / L"*.*");
	while (it.Next())
	{
		it.GetCurrent(entry);

		// Ignore tune.xml and other files...
		if (!IsSupportedImageFile(entry.Path))
			continue;

		// Find existing tune or crate new one
		TextureTune* tune = TryFindTuneFromPath(entry.Path);
		if (!tune)
			tune = &m_TextureTunes.Construct(this, entry.Path);;

		scannedTuneHashes.Insert(tune->GetHashKey());
	}

	// Find textures that were removed and get rid of their tunes
	List<u32> tuneIndicesToRemove;
	int tunesToRemoveCount = m_TextureTunes.GetSize() - scannedTuneHashes.GetNumUsedSlots();

	// No texture was removed/renamed
	if (tunesToRemoveCount == 0)
		return;

	// Find indices of removed texture tunes
	for (u32 i = 0; i < m_TextureTunes.GetSize(); i++)
	{
		// Texture that tune refers to wasn't scanned, meaning it was removed/renamed
		if (!scannedTuneHashes.Contains(m_TextureTunes[i].GetHashKey()))
		{
			// Insert in beginning to remove from end to beginning (in reversed order)
			// so indices don't invalidate because of element shifting after removal
			tuneIndicesToRemove.Insert(0, i);
		}

		// We found all indices we have to remove
		if (tunesToRemoveCount == tuneIndicesToRemove.GetSize())
			break;
	}

	// Remove 'em all
	for (u32 i : tuneIndicesToRemove)
		m_TextureTunes.RemoveAt(i);
}

void rageam::asset::TxdAsset::Serialize(XmlHandle& node) const
{
	for (const TextureTune& texture : m_TextureTunes)
	{
		file::Path fileName = PATH_TO_UTF8(file::GetFileName(texture.GetFilePath()));

		XmlHandle xTexture = node.AddChild("Texture");
		xTexture.SetAttribute("File", fileName);
		texture.Serialize(xTexture);
	}
}

void rageam::asset::TxdAsset::Deserialize(const XmlHandle& node)
{
	for (const XmlHandle& xTexture : XmlIterator(node, "Texture"))
	{
		ConstString fileName;
		xTexture.GetAttribute("File", fileName);

		file::WPath filePath = GetDirectoryPath() / PATH_TO_WIDE(fileName);

		TextureTune& tune = m_TextureTunes.Construct(this, filePath);
		tune.Deserialize(xTexture);
	}
}

rageam::asset::TextureTune& rageam::asset::TxdAsset::AddTune(ConstWString filePath)
{
	return m_TextureTunes.Construct(this, filePath);
}

bool rageam::asset::TxdAsset::GetValidatedTextureName(const file::WPath& texturePath, file::Path& outName, bool showWarningMessage) const
{
	outName = "";

	file::WPath fileName = texturePath.GetFileNameWithoutExtension();
	if (!ValidateTextureName(fileName, showWarningMessage))
		return false;

	outName = String::ToAnsiTemp(fileName);
	return true;
}

bool rageam::asset::TxdAsset::ContainsTextureWithName(ConstString name) const
{
	file::WPath lhsName = file::PathConverter::Utf8ToWide(name);
	for (TextureTune& tune : m_TextureTunes)
	{
		file::WPath rhsName = tune.GetFilePath();
		rhsName = rhsName.GetFileNameWithoutExtension();

		if (String::Equals(lhsName, rhsName))
			return true;
	}
	return false;
}

rageam::asset::TextureTune* rageam::asset::TxdAsset::TryFindTuneFromPath(ConstWString path) const
{
	u32 hashKey = TextureTune::ComputeHashKey(path);
	for (TextureTune& tune : m_TextureTunes)
	{
		if (tune.GetHashKey() == hashKey)
			return &tune;
	}
	return nullptr;
}

rage::grcTexture* rageam::asset::TxdAsset::CompileSingleTexture(TextureTune& tune, bool storeData, amPtr<TexturePreset>* outPreset) const
{
	// Ensure that texture name is valid before compression
	ConstWString filePath = tune.GetFilePath();
	file::Path validatedName;
	if (!GetValidatedTextureName(filePath, validatedName))
		return nullptr;

	TextureOptions& texOptions = tune.GetCustomOptionsOrFromPreset(outPreset);

	graphics::CompressedImageInfo encodedInfo;
	graphics::ImagePtr compressedImage = graphics::ImageFactory::LoadFromPathAndCompress(
		filePath, texOptions.CompressorOptions, &encodedInfo);
	if (!compressedImage)
		return nullptr;

	graphics::ImageInfo& imageInfo = encodedInfo.ImageInfo;
	rage::grcTextureDX11* gameTexture = new rage::grcTextureDX11(
		imageInfo.Width,
		imageInfo.Height,
		imageInfo.MipCount,
		ImagePixelFormatToDXGI(imageInfo.PixelFormat),
		compressedImage->GetPixelDataBytes(),
		storeData);
	gameTexture->SetName(validatedName);

	return gameTexture;
}

bool rageam::asset::TxdAsset::ValidateTextureName(ConstWString fileName, bool showWarningMessage)
{
	while (*fileName)
	{
		wchar_t c = *fileName;
		fileName++;

		if (static_cast<u16>(c) <= UINT8_MAX)
			continue;

		if (showWarningMessage)
		{
			AM_WARNINGF(
				L"TxdAsset::ValidateTextureName() -> Found non ASCII symbol in %ls (%lc)! Name cannot be converted safely!",
				fileName, c);
		}

		return false;
	}
	return true;
}

bool rageam::asset::TxdAsset::IsSupportedImageFile(ConstWString texturePath)
{
	return graphics::ImageFactory::IsSupportedImageFormat(texturePath);
}

rageam::file::WPath rageam::asset::TxdAsset::GetTxdAssetPathFromTexture(const file::WPath& texturePath)
{
	return texturePath.GetParentDirectory();
}

bool rageam::asset::TxdAsset::GetTxdAssetPathFromTexture(const file::WPath& texturePath, file::WPath& path)
{
	path = GetTxdAssetPathFromTexture(texturePath);
	return ImmutableWString(path).EndsWith(ASSET_ITD_EXT);
}

rage::grcTexture* rageam::asset::TxdAsset::CreateNoneTexture()
{
	if(!sm_NoneTexture)
	{
		sm_NoneTexture = new rage::grcTextureDX11((rage::grcTextureDX11&)*CreateMissingTexture(""));
		sm_NoneTexture->SetName("(None)##NONE");
	}
	return sm_NoneTexture;
}

bool rageam::asset::TxdAsset::IsNoneTexture(const rage::grcTexture* texture)
{
	AM_ASSERTS(texture);
	return String::Equals(texture->GetName(), "(None)##NONE");
}

rage::grcTexture* rageam::asset::TxdAsset::CreateMissingTexture(ConstString textureName)
{
	if(!sm_MissingTexture)
	{
		graphics::ImagePtr checker = graphics::ImageFactory::CreateChecker_NotFound();
		// This is used on game models, we need mip maps
		checker = checker->GenerateMipMaps();

		sm_MissingTexture = new rage::grcTextureDX11(
			checker->GetWidth(), checker->GetHeight(), checker->GetMipCount(), 
			ImagePixelFormatToDXGI(checker->GetPixelFormat()), checker->GetPixelDataBytes());
	}

	char nameBuffer[256];
	sprintf_s(nameBuffer, sizeof nameBuffer, "%s (Missing)##$MT_%s", textureName, textureName);

	rage::grcTextureDX11* texture = new rage::grcTextureDX11((rage::grcTextureDX11&)*sm_MissingTexture);
	texture->SetName(nameBuffer);
	return texture;
}

bool rageam::asset::TxdAsset::IsMissingTexture(const rage::grcTexture* texture)
{
	ImmutableString texName = texture->GetName();
	return texName.Contains("(Missing)##$MT_");
}

ConstString rageam::asset::TxdAsset::UndecorateMissingTextureName(const rage::grcTexture* texture)
{
	if (!texture)
		return nullptr;
	ImmutableString texName = texture->GetName();
	int tokenIndex = texName.IndexOf("##$MT_");
	if (tokenIndex == -1)
		return texture->GetName();
	return texName.Substring(tokenIndex + 6); // Length of ##$MT_
}
